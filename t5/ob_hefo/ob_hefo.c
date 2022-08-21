/* ob_hefo.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header/footer object module for Fireworkz */

/* MRJC September 1992 */

#include "common/gflags.h"

#include "ob_hefo/ob_hefo.h"

#include "ob_hefo/ui_hefin.h"

#include "ob_toolb/xp_toolb.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#define HEADER_DEFAULT_MARGIN ((PIXIT) (FP_PIXITS_PER_CM * 1.0))
#define HEADER_DEFAULT_OFFSET ((PIXIT) (68)) /* same as in Letter template */

#define FOOTER_DEFAULT_MARGIN HEADER_DEFAULT_MARGIN
#define FOOTER_DEFAULT_OFFSET ((PIXIT) (FP_PIXITS_PER_CM * 4.0))

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_HEFO)
extern PC_U8 rb_hefo_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_HEFO &rb_hefo_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_HEFO LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_HEFO DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_HEFO DONT_LOAD_RESOURCES

/*
internal routines
*/

_Check_return_ _Success_(status_done(return))
static STATUS
hefo_block_from_page_num(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_HEFO_BLOCK p_hefo_block,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_PAGE_NUM p_page_num,
    _InVal_     OBJECT_ID focus_owner);

static void
hefo_caret_position_set_show(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _InVal_     BOOL scroll);

_Check_return_
static STATUS
hefo_reflect_focus_change(
    _DocuRef_   P_DOCU p_docu);

static void
hefo_markers_clear(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
static STATUS
hefo_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_SAVE p_msg_save);

_Check_return_
static STATUS
page_hefo_break_list_create(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_array_handle,
    /*out*/ UI_SOURCE * p_ui_source);

_Check_return_
static STATUS
page_hefo_break_values_change(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

OBJECT_PROTO(static, object_footer);

OBJECT_PROTO(static, object_header);

_Check_return_
static STATUS
save_hefo_docu_area(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_STYLE_DOCU_AREA p_style_docu_area,
    _InVal_     ROW row,
    _In_        DATA_SPACE data_space);

_Check_return_
static STATUS
save_page_hefo_break_values(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_PAGE_HEFO_BREAK p_page_hefo_break);

_Check_return_
static inline STATUS
object_call_HEFO_with_hb(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block)
{
    return(object_call_id(OBJECT_ID_HEFO, p_docu, t5_message, p_hefo_block));
}

/*
construct table
*/

static const ARG_TYPE
args_cmd_of_hefo_data[] =
{
#define ARG_HEFO_DATA_ROW           0
    ARG_TYPE_ROW | ARG_MANDATORY,

#define ARG_HEFO_DATA_DATA_SPACE    1
    ARG_TYPE_S32 | ARG_MANDATORY,

#define ARG_HEFO_DATA_TEXT          2
    ARG_TYPE_USTR_INLINES,

#define ARG_HEFO_DATA_N_ARGS        3
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_of_hefo_docu_area[] =
{
#define ARG_HEFO_DOCU_AREA_ROW          0
    ARG_TYPE_ROW | ARG_MANDATORY,

#define ARG_HEFO_DOCU_AREA_DATA_SPACE   1
    ARG_TYPE_S32 | ARG_MANDATORY,

#define ARG_HEFO_DOCU_AREA_TL_DATA      2
    ARG_TYPE_S32,

#define ARG_HEFO_DOCU_AREA_TL_OBJECT_ID 3
    ARG_TYPE_S32 | ARG_MANDATORY,

#define ARG_HEFO_DOCU_AREA_BR_DATA      4
    ARG_TYPE_S32,

#define ARG_HEFO_DOCU_AREA_BR_OBJECT_ID 5
    ARG_TYPE_S32 | ARG_MANDATORY,

#define ARG_HEFO_DOCU_AREA_DEFN         6
    ARG_TYPE_USTR,

#define ARG_HEFO_DOCU_AREA_N_ARGS       7
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_page_hefo_break_values[] =
{
    ARG_TYPE_ROW | ARG_MANDATORY,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_S32,
    ARG_TYPE_BOOL,
    ARG_TYPE_F64 /*margin*/,
    ARG_TYPE_F64 /*offset*/,
    ARG_TYPE_BOOL,
    ARG_TYPE_F64 /*margin*/,
    ARG_TYPE_F64 /*offset*/,
    ARG_TYPE_BOOL,
    ARG_TYPE_F64 /*margin*/,
    ARG_TYPE_F64 /*offset*/,
    ARG_TYPE_BOOL,
    ARG_TYPE_F64 /*margin*/,
    ARG_TYPE_F64 /*offset*/,
    ARG_TYPE_BOOL,
    ARG_TYPE_F64 /*margin*/,
    ARG_TYPE_F64 /*offset*/,
    ARG_TYPE_BOOL,
    ARG_TYPE_F64 /*margin*/,
    ARG_TYPE_F64 /*offset*/,

    ARG_TYPE_NONE
};

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "HD",                     args_cmd_of_hefo_data,              T5_CMD_OF_HEFO_DATA,                { 0, 1 } },
    { "HR",                     args_cmd_of_hefo_docu_area,         T5_CMD_OF_HEFO_REGION,              { 0, 1 } },
    { "HBR",                    args_cmd_of_hefo_docu_area,         T5_CMD_OF_HEFO_BASE_REGION,         { 0, 1 } },

    { "PageHefoBreakIntro",     NULL,                               T5_CMD_PAGE_HEFO_BREAK_INTRO,       { 1, 1, 0, 1, 1, 0, 0, 1, 0 } },
    { "PageHefoBreakValues",    args_cmd_page_hefo_break_values,    T5_CMD_PAGE_HEFO_BREAK_VALUES,      { 0, 1, 1, 0, 0, 0, 1, 1, 0 } },

    { NULL,                     NULL,                               T5_EVENT_NONE } /* end of table */
};

static const HEADFOOT_SIZES
headfoot_sizes_default_header = { HEADER_DEFAULT_MARGIN, HEADER_DEFAULT_OFFSET };

static const HEADFOOT_SIZES
headfoot_sizes_default_footer = { FOOTER_DEFAULT_MARGIN, FOOTER_DEFAULT_OFFSET };

/*
* set up a text message block
*/

static void
text_message_block_init(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_MESSAGE_BLOCK p_text_message_block,
    /*_Inout_*/ P_ANY p_data,
    P_HEFO_BLOCK p_hefo_block,
    P_OBJECT_DATA p_object_data)
{
    POSITION position;

    p_text_message_block->p_data = p_data;

    p_text_message_block->inline_object.object_data = *p_object_data;

    if((P_DATA_NONE != p_text_message_block->inline_object.object_data.u.p_object) && (OBJECT_ID_NONE != p_text_message_block->inline_object.object_data.object_id))
        p_text_message_block->inline_object.inline_len = ustr_inline_strlen(p_text_message_block->inline_object.object_data.u.ustr_inline);
    else
        p_text_message_block->inline_object.inline_len = 0;

    style_init(&p_text_message_block->text_format_info.style_text_global);

    position_init_hefo(&position, 0);
    style_from_position(p_docu,
                        &p_text_message_block->text_format_info.style_text_global,
                        &style_selector_para_text,
                        &position,
                        p_hefo_block->p_h_style_list,
                        position_slr_in_docu_area,
                        FALSE);

    p_text_message_block->text_format_info.skel_rect_object = p_hefo_block->skel_rect_object;
    p_text_message_block->text_format_info.skel_rect_work = p_hefo_block->skel_rect_work;
    p_text_message_block->text_format_info.h_style_list = *p_hefo_block->p_h_style_list;
    p_text_message_block->text_format_info.text_area_border_y = 2 * p_docu->page_def.grid_size;
    p_text_message_block->text_format_info.object_visible = 1;
    p_text_message_block->text_format_info.paginate = 0;
}

/*
initialise redisplay block
*/

static void
text_inline_redisplay_init(
    P_TEXT_INLINE_REDISPLAY p_text_inline_redisplay,
    _InRef_maybenone_ PC_QUICK_UBLOCK p_quick_ublock,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     BOOL do_redraw,
    _InVal_     REDRAW_TAG redraw_tag)
{
    p_text_inline_redisplay->redraw_tag = redraw_tag;
    p_text_inline_redisplay->p_quick_ublock = p_quick_ublock;
    p_text_inline_redisplay->do_redraw = do_redraw;
    p_text_inline_redisplay->redraw_done = FALSE;
    p_text_inline_redisplay->size_changed = TRUE;
    p_text_inline_redisplay->do_position_update = TRUE;
    p_text_inline_redisplay->skel_rect_para_after =
    p_text_inline_redisplay->skel_rect_para_before =
    p_text_inline_redisplay->skel_rect_text_before = *p_skel_rect;
}

/*
main events
*/

static void
hefo_msg_reformat(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat)
{
    BOOL header_redraw = FALSE;
    BOOL footer_redraw = FALSE;

    switch(p_docu_reformat->action)
    {
    case REFORMAT_Y:
    case REFORMAT_XY:
        switch(p_docu_reformat->data_space)
        {
        case DATA_NONE:
            header_redraw = footer_redraw = TRUE;
            break;

        case DATA_HEADER_ODD:
        case DATA_HEADER_EVEN:
        case DATA_HEADER_FIRST:
            header_redraw = TRUE;
            break;

        case DATA_FOOTER_ODD:
        case DATA_FOOTER_EVEN:
        case DATA_FOOTER_FIRST:
            footer_redraw = TRUE;
            break;
        }
        break;

    case REFORMAT_HEFO_Y:
        break;
    }

    if(header_redraw)
        view_update_all(p_docu, UPDATE_PANE_MARGIN_HEADER);

    if(footer_redraw)
        view_update_all(p_docu, UPDATE_PANE_MARGIN_FOOTER);

    if((OBJECT_ID_HEADER == p_docu->focus_owner) || (OBJECT_ID_FOOTER == p_docu->focus_owner))
        caret_show_claim(p_docu, p_docu->focus_owner, FALSE);
}

static void
hefo_msg_selection_clear(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->mark_info_hefo.h_markers)
    {
        HEFO_BLOCK hefo_block;
        OBJECT_ID focus_owner;

        switch(p_docu->mark_focus_hefo.data_space)
        {
        default: default_unhandled();
#if CHECKING
        case DATA_HEADER_ODD:
        case DATA_HEADER_EVEN:
        case DATA_HEADER_FIRST:
#endif
            focus_owner = OBJECT_ID_HEADER;
            break;

        case DATA_FOOTER_ODD:
        case DATA_FOOTER_EVEN:
        case DATA_FOOTER_FIRST:
            focus_owner = OBJECT_ID_FOOTER;
            break;
        }

        /* set current position to start of selection */
        p_docu->hefo_position.object_position = array_ptrc(&p_docu->mark_info_hefo.h_markers, MARKERS, 0)->docu_area.tl.object_position;

        hefo_markers_clear(p_docu);

        if(status_done(hefo_block_from_page_num(p_docu,
                                                &hefo_block,
                                                NULL,
                                                &p_docu->hefo_position.page_num,
                                                focus_owner)))
        {
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_SELECTION_SHOW, &hefo_block));
        }
    }
}

static void
hefo_msg_style_changed(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_STYLE_CHANGED p_style_changed)
{
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    ROW row = -1;
    DOCU_REFORMAT docu_reformat;

    docu_reformat.action = REFORMAT_Y;
    docu_reformat.data_type = REFORMAT_ROW;

    while(NULL != (p_page_hefo_break = page_hefo_break_enum(p_docu, &row)))
    {
        S32 style_found = 0;

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_ODD))
            if(style_handle_find_in_docu_area_list(&docu_reformat.data.row,
                                                   &p_page_hefo_break->header_odd.headfoot.h_style_list,
                                                   p_style_changed->style_handle))
            {
                style_found = 1;
                docu_reformat.data_space = DATA_HEADER_ODD;
                status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            }

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_EVEN))
            if(style_handle_find_in_docu_area_list(&docu_reformat.data.row,
                                                   &p_page_hefo_break->header_even.headfoot.h_style_list,
                                                   p_style_changed->style_handle))
            {
                style_found = 1;
                docu_reformat.data_space = DATA_HEADER_EVEN;
                status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            }

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_FIRST))
            if(style_handle_find_in_docu_area_list(&docu_reformat.data.row,
                                                   &p_page_hefo_break->header_first.headfoot.h_style_list,
                                                   p_style_changed->style_handle))
            {
                style_found = 1;
                docu_reformat.data_space = DATA_HEADER_FIRST;
                status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            }

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_ODD))
            if(style_handle_find_in_docu_area_list(&docu_reformat.data.row,
                                                   &p_page_hefo_break->footer_odd.headfoot.h_style_list,
                                                   p_style_changed->style_handle))
            {
                style_found = 1;
                docu_reformat.data_space = DATA_FOOTER_ODD;
                status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            }

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_EVEN))
            if(style_handle_find_in_docu_area_list(&docu_reformat.data.row,
                                                   &p_page_hefo_break->footer_even.headfoot.h_style_list,
                                                   p_style_changed->style_handle))
            {
                style_found = 1;
                docu_reformat.data_space = DATA_FOOTER_EVEN;
                status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            }

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_FIRST))
            if(style_handle_find_in_docu_area_list(&docu_reformat.data.row,
                                                   &p_page_hefo_break->footer_first.headfoot.h_style_list,
                                                   p_style_changed->style_handle))
            {
                style_found = 1;
                docu_reformat.data_space = DATA_FOOTER_FIRST;
                status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            }

        if(style_found)
            break;
    }
}

_Check_return_
static STATUS
hefo_msg_style_use_query(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_STYLE_USE_QUERY p_style_use_query)
{
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    ROW row = -1;

    while(NULL != (p_page_hefo_break = page_hefo_break_enum(p_docu, &row)))
    {
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_ODD))
            style_use_query(p_docu, &p_page_hefo_break->header_odd.headfoot.h_style_list, p_style_use_query);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_EVEN))
            style_use_query(p_docu, &p_page_hefo_break->header_even.headfoot.h_style_list, p_style_use_query);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_FIRST))
            style_use_query(p_docu, &p_page_hefo_break->header_first.headfoot.h_style_list, p_style_use_query);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_ODD))
            style_use_query(p_docu, &p_page_hefo_break->footer_odd.headfoot.h_style_list, p_style_use_query);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_EVEN))
            style_use_query(p_docu, &p_page_hefo_break->footer_even.headfoot.h_style_list, p_style_use_query);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_FIRST))
            style_use_query(p_docu, &p_page_hefo_break->footer_first.headfoot.h_style_list, p_style_use_query);
    }

    return(STATUS_OK);
}

static void
hefo_msg_style_use_remove(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_STYLE_USE_REMOVE p_style_use_remove)
{
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    ROW row = -1;

    while(NULL != (p_page_hefo_break = page_hefo_break_enum(p_docu, &row)))
    {
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_ODD))
            style_use_remove(p_docu, &p_page_hefo_break->header_odd.headfoot.h_style_list, p_style_use_remove->style_handle);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_EVEN))
            style_use_remove(p_docu, &p_page_hefo_break->header_even.headfoot.h_style_list, p_style_use_remove->style_handle);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_FIRST))
            style_use_remove(p_docu, &p_page_hefo_break->header_first.headfoot.h_style_list, p_style_use_remove->style_handle);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_ODD))
            style_use_remove(p_docu, &p_page_hefo_break->footer_odd.headfoot.h_style_list, p_style_use_remove->style_handle);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_EVEN))
            style_use_remove(p_docu, &p_page_hefo_break->footer_even.headfoot.h_style_list, p_style_use_remove->style_handle);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_FIRST))
            style_use_remove(p_docu, &p_page_hefo_break->footer_first.headfoot.h_style_list, p_style_use_remove->style_handle);

        if(row >= 0)
            p_style_use_remove->row = MIN(row, p_style_use_remove->row);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_ob_hefo)
{
    const STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_SELECTION_CLEAR:
        hefo_msg_selection_clear(p_docu);
        break;

    case T5_MSG_FOCUS_CHANGED:
        return(hefo_reflect_focus_change(p_docu));

    case T5_MSG_REFORMAT:
        hefo_msg_reformat(p_docu, (PC_DOCU_REFORMAT) p_data);
        break;

    case T5_MSG_STYLE_CHANGED:
        hefo_msg_style_changed(p_docu, (P_STYLE_CHANGED) p_data);
        break;

    case T5_MSG_STYLE_USE_REMOVE:
        hefo_msg_style_use_remove(p_docu, (P_STYLE_USE_REMOVE) p_data);
        break;

    case T5_MSG_STYLE_USE_QUERY:
        return(hefo_msg_style_use_query(p_docu, (P_STYLE_USE_QUERY) p_data));

    case T5_MSG_SAVE:
        return(hefo_msg_save(p_docu, (PC_MSG_SAVE) p_data));

    default:
        break;
    }

    return(status);
}

/******************************************************************************
*
* insert some text into a text object
*
******************************************************************************/

_Check_return_
static STATUS /* size inserted */
hefo_insert_sub_redisplay(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _InoutRef_  P_OBJECT_POSITION p_object_position,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    STATUS status = STATUS_OK;
    S32 ins_size = quick_ublock_bytes(p_quick_ublock);

    /* do actual insert operation */

    if(ins_size)
    {
        TEXT_MESSAGE_BLOCK text_message_block;
        TEXT_INLINE_REDISPLAY text_inline_redisplay;
        S32 start, end;

        text_message_block_init(p_docu, &text_message_block, &text_inline_redisplay, p_hefo_block, &p_hefo_block->object_data);
        text_message_block.inline_object.object_data.object_position_start = *p_object_position;
        offsets_from_object_data(&start, &end, &text_message_block.inline_object.object_data, text_message_block.inline_object.inline_len);
        text_inline_redisplay_init(&text_inline_redisplay,
                                   p_quick_ublock,
                                   &text_message_block.text_format_info.skel_rect_object,
                                   TRUE,
                                   p_hefo_block->redraw_tag);

        if(status_ok(status))
        {
            if(NULL != al_array_extend_by_U8(p_hefo_block->p_h_data, ins_size + (text_message_block.inline_object.inline_len ? 0 : 1), &array_init_block_u8, &status))
                text_message_block.inline_object.object_data.u.ustr_inline = ustr_inline_from_h_ustr(p_hefo_block->p_h_data);
        }

        if(status_ok(status))
        {
            text_message_block.inline_object.object_data.object_id = OBJECT_ID_HEFO;
            status = object_call_STORY_with_tmb(p_docu, T5_MSG_INSERT_INLINE_REDISPLAY, &text_message_block);
        }

        if(status_ok(status))
        {
            *p_object_position = text_message_block.inline_object.object_data.object_position_end;
            if(text_inline_redisplay.size_changed || !text_inline_redisplay.redraw_done)
                view_update_all(p_docu, p_hefo_block->redraw_tag);
        }
    }

    return(status_ok(status) ? ins_size : status);
}

T5_CMD_PROTO(static, hefo_load_data)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_HEFO_DATA_N_ARGS);
    ROW               row;
    DATA_SPACE        data_space;
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    BIT_NUMBER        bit_number;
    P_HEADFOOT_DEF    p_headfoot_def;
    STATUS            status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    row        = p_args[ARG_HEFO_DATA_ROW].val.row;
    data_space = p_args[ARG_HEFO_DATA_DATA_SPACE].val.u8n;

    /* only ever loads into already defined structure */
    if(NULL == (p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, row)))
    {
        assert0();
        return(STATUS_OK);
    }

    switch(data_space)
    {
    default: default_unhandled();
#if CHECKING
    case DATA_HEADER_ODD:
#endif
        bit_number     =           PAGE_HEFO_HEADER_ODD;
        p_headfoot_def = &p_page_hefo_break->header_odd;
        break;

    case DATA_HEADER_EVEN:
        bit_number     =           PAGE_HEFO_HEADER_EVEN;
        p_headfoot_def = &p_page_hefo_break->header_even;
        break;

    case DATA_HEADER_FIRST:
        bit_number     =           PAGE_HEFO_HEADER_FIRST;
        p_headfoot_def = &p_page_hefo_break->header_first;
        break;

    case DATA_FOOTER_ODD:
        bit_number     =           PAGE_HEFO_FOOTER_ODD;
        p_headfoot_def = &p_page_hefo_break->footer_odd;
        break;

    case DATA_FOOTER_EVEN:
        bit_number     =           PAGE_HEFO_FOOTER_EVEN;
        p_headfoot_def = &p_page_hefo_break->footer_even;
        break;

    case DATA_FOOTER_FIRST:
        bit_number     =           PAGE_HEFO_FOOTER_FIRST;
        p_headfoot_def = &p_page_hefo_break->footer_first;
        break;
    }

    if(!page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number))
    {
        assert0();
        return(STATUS_OK);
    }

    /* remove current data and style list */
    headfoot_dispose(p_docu, p_headfoot_def, FALSE);

    /* add new data */
    if(arg_is_present(p_args, ARG_HEFO_DATA_TEXT))
    {
        { /* SKS fixed 08nov94 */
        PC_USTR_INLINE ustr_inline = p_args[ARG_HEFO_DATA_TEXT].val.ustr_inline;
        const U32 n_bytes = ustr_inline_strlen32p1(ustr_inline); /*CH_NULL*/
        status = al_array_add(&p_headfoot_def->headfoot.h_data, UCHARZ, n_bytes, &array_init_block_uchars, ustr_inline);
        } /*block*/

        {
        DOCU_REFORMAT docu_reformat;
        docu_reformat.action = REFORMAT_HEFO_Y;
        docu_reformat.data_type = REFORMAT_ROW;
        docu_reformat.data_space = DATA_NONE;
        docu_reformat.data.row = row;
        status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
        } /*block*/
    }

    return(status);
}

_Check_return_
static STATUS
hefo_load_region(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, ARG_HEFO_DOCU_AREA_N_ARGS);
    STATUS            status = STATUS_OK;
    DOCU_AREA         docu_area, docu_area_clipped;
    ROW               row;
    DATA_SPACE        data_space;
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    BIT_NUMBER        bit_number;
    P_HEADFOOT_DEF    p_headfoot_def;

    row        = p_args[ARG_HEFO_DOCU_AREA_ROW].val.row;
    data_space = p_args[ARG_HEFO_DOCU_AREA_DATA_SPACE].val.u8n;

    /* only ever loads into already defined structure */
    if(NULL == (p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, row)))
    {
        assert0();
        return(STATUS_OK);
    }

    switch(data_space)
    {
    default: default_unhandled();
#if CHECKING
    case DATA_HEADER_ODD:
#endif
        bit_number     =           PAGE_HEFO_HEADER_ODD;
        p_headfoot_def = &p_page_hefo_break->header_odd;
        break;

    case DATA_HEADER_EVEN:
        bit_number     =           PAGE_HEFO_HEADER_EVEN;
        p_headfoot_def = &p_page_hefo_break->header_even;
        break;

    case DATA_HEADER_FIRST:
        bit_number     =           PAGE_HEFO_HEADER_FIRST;
        p_headfoot_def = &p_page_hefo_break->header_first;
        break;

    case DATA_FOOTER_ODD:
        bit_number     =           PAGE_HEFO_FOOTER_ODD;
        p_headfoot_def = &p_page_hefo_break->footer_odd;
        break;

    case DATA_FOOTER_EVEN:
        bit_number     =           PAGE_HEFO_FOOTER_EVEN;
        p_headfoot_def = &p_page_hefo_break->footer_even;
        break;

    case DATA_FOOTER_FIRST:
        bit_number     =           PAGE_HEFO_FOOTER_FIRST;
        p_headfoot_def = &p_page_hefo_break->footer_first;
        break;
    }

    if(!page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number))
    {
        assert0();
        return(STATUS_OK);
    }

    docu_area_init_hefo(&docu_area);
    docu_area.tl.object_position.object_id = p_args[ARG_HEFO_DOCU_AREA_TL_OBJECT_ID].val.object_id;
    if((OBJECT_ID_NONE != docu_area.tl.object_position.object_id) && arg_is_present(p_args, ARG_HEFO_DOCU_AREA_TL_DATA))
        docu_area.tl.object_position.data  = p_args[ARG_HEFO_DOCU_AREA_TL_DATA].val.s32;

    docu_area.br.object_position.object_id = p_args[ARG_HEFO_DOCU_AREA_BR_OBJECT_ID].val.object_id;
    if((OBJECT_ID_NONE != docu_area.br.object_position.object_id) && arg_is_present(p_args, ARG_HEFO_DOCU_AREA_BR_DATA))
        docu_area.br.object_position.data  = p_args[ARG_HEFO_DOCU_AREA_BR_DATA].val.s32;

    docu_area_clean_up(&docu_area);

    docu_area_clipped = docu_area;

    /* List of inlines - if empty there is no sense in adding it! */
    if(arg_is_present(p_args, ARG_HEFO_DOCU_AREA_DEFN))
    {
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

        STYLE_DOCU_AREA_ADD_INLINE(&style_docu_area_add_parm, p_args[ARG_HEFO_DOCU_AREA_DEFN].val.ustr_inline);

        if(t5_message == T5_CMD_OF_HEFO_BASE_REGION)
        {
            style_docu_area_add_parm.base = 1;
            style_docu_area_add_parm.region_class = REGION_BASE;
        }

        style_docu_area_add_parm.p_array_handle = &p_headfoot_def->headfoot.h_style_list;
        style_docu_area_add_parm.data_space     = p_headfoot_def->data_space;

        status_assert(status = style_docu_area_add(p_docu, &docu_area_clipped, &style_docu_area_add_parm));
    }
    else
        assert0();

    return(status);
}

/******************************************************************************
*
* dispose of hefo markers
*
******************************************************************************/

static void
hefo_markers_clear(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->mark_info_hefo.h_markers)
    {
        al_array_dispose(&p_docu->mark_info_hefo.h_markers);
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));
    }
}

/******************************************************************************
*
* save all page breaks/headers/footers that are defined in the region being saved
*
******************************************************************************/

_Check_return_
static STATUS
save_hefo_data(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_PAGE_HEFO_BREAK p_page_hefo_break,
    _In_        DATA_SPACE data_space)
{
    OBJECT_ID         object_id = OBJECT_ID_HEFO;
    T5_MESSAGE        t5_message = T5_CMD_OF_HEFO_DATA;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE    arglist_handle;
    BIT_NUMBER        bit_number;
    P_HEADFOOT_DEF    p_headfoot_def;
    STATUS            status;

    switch(data_space)
    {
    default: default_unhandled();
#if CHECKING
    case DATA_HEADER_ODD:
#endif
        bit_number     =           PAGE_HEFO_HEADER_ODD;
        p_headfoot_def = &p_page_hefo_break->header_odd;
        break;

    case DATA_HEADER_EVEN:
        bit_number     =           PAGE_HEFO_HEADER_EVEN;
        p_headfoot_def = &p_page_hefo_break->header_even;
        break;

    case DATA_HEADER_FIRST:
        bit_number     =           PAGE_HEFO_HEADER_FIRST;
        p_headfoot_def = &p_page_hefo_break->header_first;
        break;

    case DATA_FOOTER_ODD:
        bit_number     =           PAGE_HEFO_FOOTER_ODD;
        p_headfoot_def = &p_page_hefo_break->footer_odd;
        break;

    case DATA_FOOTER_EVEN:
        bit_number     =           PAGE_HEFO_FOOTER_EVEN;
        p_headfoot_def = &p_page_hefo_break->footer_even;
        break;

    case DATA_FOOTER_FIRST:
        bit_number     =           PAGE_HEFO_FOOTER_FIRST;
        p_headfoot_def = &p_page_hefo_break->footer_first;
        break;
    }

    if(!page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number))
        return(STATUS_OK);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, ARG_HEFO_DATA_N_ARGS);

        p_args[ARG_HEFO_DATA_ROW].val.row = p_page_hefo_break->region.tl.row;
        p_args[ARG_HEFO_DATA_DATA_SPACE].val.s32 = data_space;
        p_args[ARG_HEFO_DATA_TEXT].val.ustr = array_ustr(&p_headfoot_def->headfoot.h_data);

        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

        arglist_dispose(&arglist_handle);
    }

    status_return(status);

    { /* look through region list forwards */
    P_ARRAY_HANDLE p_h_style_list = &p_headfoot_def->headfoot.h_style_list;
    ARRAY_INDEX style_docu_area_idx;

    for(style_docu_area_idx = 0; style_docu_area_idx < array_elements(p_h_style_list); ++style_docu_area_idx)
    {
        const P_STYLE_DOCU_AREA p_style_docu_area = array_ptr(p_h_style_list, STYLE_DOCU_AREA, style_docu_area_idx);

        if(p_style_docu_area->is_deleted)
            continue;

        if(!p_style_docu_area->internal)
            /* docu area test done by caller */
            status_break(status = save_hefo_docu_area(p_of_op_format, p_style_docu_area, p_page_hefo_break->region.tl.row, data_space));
    }
    } /*block*/

    return(status);
}

_Check_return_
static STATUS
save_hefo_docu_area(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_STYLE_DOCU_AREA p_style_docu_area,
    _InVal_     ROW row,
    _In_        DATA_SPACE data_space)
{
    OBJECT_ID         object_id = OBJECT_ID_HEFO;
    T5_MESSAGE        t5_message = p_style_docu_area->base ? T5_CMD_OF_HEFO_BASE_REGION : T5_CMD_OF_HEFO_REGION;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE    arglist_handle;
    P_ARGLIST_ARG     p_args;
    STATUS            status;
    OBJECT_ID         tl_object_id, br_object_id;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    status_return(arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table));

    p_args = p_arglist_args(&arglist_handle, ARG_HEFO_DOCU_AREA_N_ARGS);

    p_args[ARG_HEFO_DOCU_AREA_ROW].val.row = row;
    p_args[ARG_HEFO_DOCU_AREA_DATA_SPACE].val.s32 = data_space;

    tl_object_id = (p_style_docu_area->docu_area.whole_row || p_style_docu_area->docu_area.whole_col)
                 ? OBJECT_ID_NONE
                 : p_style_docu_area->docu_area.tl.object_position.object_id;
    p_args[ARG_HEFO_DOCU_AREA_TL_OBJECT_ID].val.s32 = tl_object_id;

    if(OBJECT_ID_NONE == tl_object_id)
        arg_dispose(&arglist_handle, ARG_HEFO_DOCU_AREA_TL_DATA);
    else
        p_args[ARG_HEFO_DOCU_AREA_TL_DATA].val.s32 = p_style_docu_area->docu_area.tl.object_position.data;

    br_object_id = (p_style_docu_area->docu_area.whole_row || p_style_docu_area->docu_area.whole_col)
                 ? OBJECT_ID_NONE
                 : p_style_docu_area->docu_area.br.object_position.object_id;
    p_args[ARG_HEFO_DOCU_AREA_BR_OBJECT_ID].val.s32 = br_object_id;

    if(OBJECT_ID_NONE == br_object_id)
        arg_dispose(&arglist_handle, ARG_HEFO_DOCU_AREA_BR_DATA);
    else
        p_args[ARG_HEFO_DOCU_AREA_BR_DATA].val.s32 = p_style_docu_area->docu_area.br.object_position.data;

    {
    STYLE style;
    style_from_docu_area_no_indirection(p_docu_from_docno(p_of_op_format->docno), &style, p_style_docu_area);
    status = style_ustr_inline_from_struct(p_docu_from_docno(p_of_op_format->docno), &quick_ublock, &style);
    } /*block*/

    p_args[ARG_HEFO_DOCU_AREA_DEFN].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);

    if(status_ok(status))
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

    arglist_dispose(&arglist_handle);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

_Check_return_
static STATUS
hefo_msg_data_save_2(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.data_class >= DATA_SAVE_MANY)
    {
        P_PAGE_HEFO_BREAK p_page_hefo_break;
        ROW row = -1;

        while(NULL != (p_page_hefo_break = page_hefo_break_enum(p_docu, &row)))
        {
            DOCU_AREA docu_area;

            docu_area_from_region(&docu_area, &p_page_hefo_break->region);

            if(docu_area_in_docu_area(&p_of_op_format->save_docu_area, &docu_area))
            {
                /* save defining values */
                status_break(status = save_page_hefo_break_values(p_of_op_format, p_page_hefo_break));

                /* save hefo data and regions */
                status_break(status = save_hefo_data(p_of_op_format, p_page_hefo_break, DATA_HEADER_ODD  ));
                status_break(status = save_hefo_data(p_of_op_format, p_page_hefo_break, DATA_HEADER_EVEN ));
                status_break(status = save_hefo_data(p_of_op_format, p_page_hefo_break, DATA_HEADER_FIRST));
                status_break(status = save_hefo_data(p_of_op_format, p_page_hefo_break, DATA_FOOTER_ODD  ));
                status_break(status = save_hefo_data(p_of_op_format, p_page_hefo_break, DATA_FOOTER_EVEN ));
                status_break(status = save_hefo_data(p_of_op_format, p_page_hefo_break, DATA_FOOTER_FIRST));
            }
        }
    }

    return(status);
}

_Check_return_
static STATUS
hefo_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_SAVE p_msg_save)
{
    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__DATA_SAVE_2:
        return(hefo_msg_data_save_2(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
hefo_reflect_focus_change(
    _DocuRef_   P_DOCU p_docu)
{
    T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;

    t5_toolbar_tool_enable.enabled = ((OBJECT_ID_HEADER == p_docu->focus_owner) || (OBJECT_ID_FOOTER == p_docu->focus_owner));
    t5_toolbar_tool_enable.enable_id = TOOL_ENABLE_HEFO_FOCUS;

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("INSERT_DATE"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("BOX"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("STYLE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("EFFECTS"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("BOLD"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("ITALIC"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("UNDERLINE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("SUPERSCRIPT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("SUBSCRIPT"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_LEFT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_CENTRE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_RIGHT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_FULL"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_LEFT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_CENTRE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_RIGHT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_DECIMAL"));

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_NONE;
    if(OBJECT_ID_HEADER == p_docu->focus_owner)
    {
        ui_text.type = UI_TEXT_TYPE_RESID;
        ui_text.text.resource_id = MSG_STATUS_HEFO_HEADER;
    }
    else if(OBJECT_ID_FOOTER == p_docu->focus_owner)
    {
        ui_text.type = UI_TEXT_TYPE_RESID;
        ui_text.text.resource_id = MSG_STATUS_HEFO_FOOTER;
    }
    status_line_set(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS(OBJECT_ID_HEFO), &ui_text);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
page_hefo_break_value_load(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARGLIST_HANDLE p_arglist_handle,
    P_PAGE_HEFO_BREAK p_page_hefo_break,
    _In_        DATA_SPACE data_space)
{
    S32            argidx_define, argidx_margin, argidx_offset;
    BIT_NUMBER     bit_number;
    P_HEADFOOT_DEF p_headfoot_def;
    PC_HEADFOOT_SIZES p_headfoot_sizes;
    PC_ARGLIST_ARG p_arg_define;

    switch(data_space)
    {
    default: default_unhandled();
#if CHECKING
    case DATA_HEADER_ODD:
#endif
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_HEADER_ODD_DEFINE;
        bit_number     =                  PAGE_HEFO_HEADER_ODD;
        p_headfoot_def =        &p_page_hefo_break->header_odd;
        p_headfoot_sizes =  &headfoot_sizes_default_header;
        break;

    case DATA_HEADER_EVEN:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_HEADER_EVEN_DEFINE;
        bit_number     =                  PAGE_HEFO_HEADER_EVEN;
        p_headfoot_def =        &p_page_hefo_break->header_even;
        p_headfoot_sizes =  &headfoot_sizes_default_header;
        break;

    case DATA_HEADER_FIRST:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_HEADER_FIRST_DEFINE;
        bit_number     =                  PAGE_HEFO_HEADER_FIRST;
        p_headfoot_def =        &p_page_hefo_break->header_first;
        p_headfoot_sizes =  &headfoot_sizes_default_header;
        break;

    case DATA_FOOTER_ODD:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_FOOTER_ODD_DEFINE;
        bit_number     =                  PAGE_HEFO_FOOTER_ODD;
        p_headfoot_def =        &p_page_hefo_break->footer_odd;
        p_headfoot_sizes =  &headfoot_sizes_default_footer;
        break;

    case DATA_FOOTER_EVEN:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_FOOTER_EVEN_DEFINE;
        bit_number     =                  PAGE_HEFO_FOOTER_EVEN;
        p_headfoot_def =        &p_page_hefo_break->footer_even;
        p_headfoot_sizes =  &headfoot_sizes_default_footer;
        break;

    case DATA_FOOTER_FIRST:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_FOOTER_FIRST_DEFINE;
        bit_number     =                  PAGE_HEFO_FOOTER_FIRST;
        p_headfoot_def =        &p_page_hefo_break->footer_first;
        p_headfoot_sizes =  &headfoot_sizes_default_footer;
        break;
    }

    argidx_margin = argidx_define + 1;
    argidx_offset = argidx_margin + 1;

    if(arg_present(p_arglist_handle, argidx_define, &p_arg_define))
    {
        BOOL new_on = p_arg_define->val.fBool;
        BOOL old_on = (page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number) != 0);

        if(old_on && !new_on)
        {
            headfoot_dispose(p_docu, p_headfoot_def, TRUE);

            page_hefo_selector_bit_clear(&p_page_hefo_break->selector, bit_number);
        }
        else if(!old_on && new_on)
        {
            status_return(headfoot_init(p_docu, p_headfoot_def, data_space));

            page_hefo_selector_bit_set(&p_page_hefo_break->selector, bit_number);

            p_headfoot_def->headfoot_sizes = *p_headfoot_sizes;
        }

        if(new_on)
        {
            PC_ARGLIST_ARG p_arg_margin = pc_arglist_arg(p_arglist_handle, argidx_margin);
            PC_ARGLIST_ARG p_arg_offset = pc_arglist_arg(p_arglist_handle, argidx_offset);

            if(p_arg_margin->type != ARG_TYPE_NONE)
                p_headfoot_def->headfoot_sizes.margin = (PIXIT) p_arg_margin->val.fp_pixit;

            if(p_arg_offset->type != ARG_TYPE_NONE)
                p_headfoot_def->headfoot_sizes.offset = (PIXIT) p_arg_offset->val.fp_pixit;
        }
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_page_hefo_break_values)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_PAGE_HEFO_BREAK_VALUES_N_ARGS);
    ROW row;
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    STATUS status;

    if(p_t5_cmd->interactive)
        return(t5_cmd_page_hefo_break_values_interactive(p_docu, t5_message, p_t5_cmd));

    row = p_args[ARG_PAGE_HEFO_BREAK_VALUES_ROW].val.row;

    /* ensure thing at this exact row */
    if(NULL == (p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, row)))
        if(NULL == (p_page_hefo_break = p_page_hefo_break_new_for_row(p_docu, row, &status)))
            return(status);

    if(arg_is_present(p_args, ARG_PAGE_HEFO_BREAK_VALUES_PAGE_BREAK))
    {
        if(p_args[ARG_PAGE_HEFO_BREAK_VALUES_PAGE_BREAK].val.fBool)
            page_hefo_selector_bit_set(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_BREAK);
        else
            page_hefo_selector_bit_clear(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_BREAK);
    }

    if(arg_is_present(p_args, ARG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER_DEFINE))
    {
        if(p_args[ARG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER_DEFINE].val.fBool)
            page_hefo_selector_bit_set(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_NUM);
        else
            page_hefo_selector_bit_clear(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_NUM);
    }

    if(arg_is_present(p_args, ARG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER))
        p_page_hefo_break->page_y = p_args[ARG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER].val.s32 - 1;

    if(status_ok(status = page_hefo_break_value_load(p_docu, &p_t5_cmd->arglist_handle, p_page_hefo_break, DATA_HEADER_ODD  )))
    if(status_ok(status = page_hefo_break_value_load(p_docu, &p_t5_cmd->arglist_handle, p_page_hefo_break, DATA_HEADER_EVEN )))
    if(status_ok(status = page_hefo_break_value_load(p_docu, &p_t5_cmd->arglist_handle, p_page_hefo_break, DATA_HEADER_FIRST)))
    if(status_ok(status = page_hefo_break_value_load(p_docu, &p_t5_cmd->arglist_handle, p_page_hefo_break, DATA_FOOTER_ODD  )))
    if(status_ok(status = page_hefo_break_value_load(p_docu, &p_t5_cmd->arglist_handle, p_page_hefo_break, DATA_FOOTER_EVEN )))
    if(status_ok(status = page_hefo_break_value_load(p_docu, &p_t5_cmd->arglist_handle, p_page_hefo_break, DATA_FOOTER_FIRST)))
    { /*EMPTY*/ }

    {
    DOCU_REFORMAT docu_reformat;
    docu_reformat.action = REFORMAT_HEFO_Y;
    docu_reformat.data_type = REFORMAT_ROW;
    docu_reformat.data_space = DATA_NONE;
    docu_reformat.data.row = row;
    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
    } /*block*/

    return(status);
}

/*
listed info
*/

typedef struct PAGE_HEFO_BREAK_LIST_INFO
{
    UCHARZ ustr_name[36]; /* textual representation of thing */
    ROW row; /* row this belongs to */
}
PAGE_HEFO_BREAK_LIST_INFO, * P_PAGE_HEFO_BREAK_LIST_INFO;

/* ------------------------------------------------------------------------- */

enum PAGE_HEFO_BREAK_INTRO_IDS
{
    PAGE_HEFO_BREAK_INTRO_ID_CHANGE = IDOK,

    PAGE_HEFO_BREAK_INTRO_ID_LIST = 635,
    PAGE_HEFO_BREAK_INTRO_ID_DELETE,
    PAGE_HEFO_BREAK_INTRO_ID_NEW,
#define PAGE_HEFO_BREAK_INTRO_BUTTONS_H (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Wechseln") + (2 * DIALOG_DEFPUSHEXTRA_H))

    PAGE_HEFO_BREAK_INTRO_ID_MAX
};

#define PAGE_HEFO_BREAK_INTRO_LIST_H (DIALOG_SYSCHARS_H("Page 12345") + DIALOG_STDLISTOVH_H)

static const DIALOG_CONTROL
page_hefo_break_intro_list =
{
    PAGE_HEFO_BREAK_INTRO_ID_LIST, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_SELF, IDCANCEL },
    { 0, 0, PAGE_HEFO_BREAK_INTRO_LIST_H },
    { DRT(LTLB, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
page_hefo_break_intro_change =
{
    PAGE_HEFO_BREAK_INTRO_ID_CHANGE, DIALOG_CONTROL_WINDOW,
    { PAGE_HEFO_BREAK_INTRO_ID_LIST, PAGE_HEFO_BREAK_INTRO_ID_LIST },
    { DIALOG_STDSPACING_H, 0, PAGE_HEFO_BREAK_INTRO_BUTTONS_H, DIALOG_STDPUSHBUTTON_V },
    { DRT(RTLT, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
page_hefo_break_intro_change_data = { { PAGE_HEFO_BREAK_INTRO_ID_CHANGE }, UI_TEXT_INIT_RESID(MSG_CHANGE) };

static const DIALOG_CONTROL
page_hefo_break_intro_delete =
{
    PAGE_HEFO_BREAK_INTRO_ID_DELETE, DIALOG_CONTROL_WINDOW,
    { PAGE_HEFO_BREAK_INTRO_ID_CHANGE, PAGE_HEFO_BREAK_INTRO_ID_CHANGE, PAGE_HEFO_BREAK_INTRO_ID_CHANGE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
page_hefo_break_intro_delete_data = { { PAGE_HEFO_BREAK_INTRO_ID_DELETE }, UI_TEXT_INIT_RESID(MSG_DELETE) };

static const DIALOG_CONTROL
page_hefo_break_intro_new =
{
    PAGE_HEFO_BREAK_INTRO_ID_NEW, DIALOG_CONTROL_WINDOW,
    { PAGE_HEFO_BREAK_INTRO_ID_DELETE, PAGE_HEFO_BREAK_INTRO_ID_DELETE, PAGE_HEFO_BREAK_INTRO_ID_DELETE },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
page_hefo_break_intro_new_data = { { PAGE_HEFO_BREAK_INTRO_ID_NEW }, UI_TEXT_INIT_RESID(MSG_NEW) };

static const DIALOG_CONTROL
page_hefo_break_intro_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { PAGE_HEFO_BREAK_INTRO_ID_NEW, PAGE_HEFO_BREAK_INTRO_ID_NEW, PAGE_HEFO_BREAK_INTRO_ID_NEW },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDPUSHBUTTON_V },
    { DRT(LBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
page_hefo_break_intro_ctl_create[] =
{
    { &dialog_main_group },

    { &page_hefo_break_intro_list,   &stdlisttext_data },

    { &page_hefo_break_intro_change, &page_hefo_break_intro_change_data },
    { &page_hefo_break_intro_delete, &page_hefo_break_intro_delete_data },
    { &page_hefo_break_intro_new,    &page_hefo_break_intro_new_data },
    { &page_hefo_break_intro_cancel, &stdbutton_cancel_data }
};

static UI_SOURCE
page_hefo_break_list_source;

_Check_return_
static STATUS
dialog_page_hefo_break_intro_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case PAGE_HEFO_BREAK_INTRO_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &page_hefo_break_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_page_hefo_break_intro_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const PC_S32 p_selected_item = (PC_S32) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case PAGE_HEFO_BREAK_INTRO_ID_LIST:
        p_dialog_msg_ctl_create_state->state_set.bits = DIALOG_STATE_SET_ALTERNATE;
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = *p_selected_item;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_page_hefo_break_intro_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_S32 p_selected_item = (P_S32) p_dialog_msg_ctl_state_change->client_handle;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case PAGE_HEFO_BREAK_INTRO_ID_LIST:
        /* root header is not deletable */
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, PAGE_HEFO_BREAK_INTRO_ID_DELETE, (p_dialog_msg_ctl_state_change->new_state.list_text.itemno > 0));
        *p_selected_item = p_dialog_msg_ctl_state_change->new_state.list_text.itemno;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_page_hefo_break_intro)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_page_hefo_break_intro_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_page_hefo_break_intro_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_page_hefo_break_intro_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
t5_cmd_page_hefo_break_intro(
    _DocuRef_   P_DOCU p_docu)
{
    /* always interactive - put up a dialog with the list of things */
    STATUS status;
    S32 completion_code;
    S32 selected_item;
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    ROW row;
    ARRAY_HANDLE page_hefo_break_list_handle = 0;

    page_hefo_break_list_source.type = UI_SOURCE_TYPE_NONE;

    status_return(status = page_hefo_break_list_create(p_docu, &page_hefo_break_list_handle, &page_hefo_break_list_source));

    if(status_fail(status))
    {
        ui_lists_dispose(&page_hefo_break_list_handle, &page_hefo_break_list_source);
        return(status);
    }

    /* lookup 'current' state and suggest that if it is vaguely consistent */
    selected_item = DIALOG_CTL_STATE_LIST_ITEM_NONE;

    switch(p_docu->focus_owner)
    {
    case OBJECT_ID_HEADER:
    case OBJECT_ID_FOOTER:
        /* whichever it is doesn't really matter */
        row = row_from_page_y(p_docu, p_docu->hefo_position.page_num.y);
        p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, row);
        PTR_ASSERT(p_page_hefo_break);
        break;

    default:
        {
        ROW_ENTRY row_entry;
        PAGE_NUM page;

        row = p_docu->cur.slr.row;
        row_entry_from_row(p_docu, &row_entry, row);
        page.y = row_entry.rowtab.edge_top.page;

        p_page_hefo_break = p_page_hefo_break_from_row_below(p_docu, row_from_page_y(p_docu, page.y));
        break;
        }
    }

    if(NULL != p_page_hefo_break)
    {
        ARRAY_INDEX i;

        for(i = 0; i < array_elements(&page_hefo_break_list_handle); ++i)
        {
            P_PAGE_HEFO_BREAK_LIST_INFO p_page_hefo_break_list_info = array_ptr(&page_hefo_break_list_handle, PAGE_HEFO_BREAK_LIST_INFO, i);

            if(p_page_hefo_break_list_info->row == p_page_hefo_break->region.tl.row)
            {
                selected_item = i;
                break;
            }
        }
    }

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, page_hefo_break_intro_ctl_create, elemof32(page_hefo_break_intro_ctl_create), MSG_DIALOG_PAGE_HEFO_BREAK_INTRO_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_PAGE_HEFO_BREAK_INTRO_CAPTION;
    dialog_cmd_process_dbox.bits.note_position = 1;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_page_hefo_break_intro;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &selected_item;
    completion_code = status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    switch(completion_code)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_COMPLETION_CANCEL:
#endif
        break;

    case PAGE_HEFO_BREAK_INTRO_ID_NEW:
        /* it's a new thing, create iff successful at row */
        status_assert(status = page_hefo_break_values_change(p_docu, row));
        break;

    case PAGE_HEFO_BREAK_INTRO_ID_CHANGE:
        {
        if(selected_item < 0)
            status = STATUS_OK;
        else
        {
            P_PAGE_HEFO_BREAK_LIST_INFO p_page_hefo_break_list_info = array_ptr(&page_hefo_break_list_handle, PAGE_HEFO_BREAK_LIST_INFO, selected_item);

            status_assert(status = page_hefo_break_values_change(p_docu, p_page_hefo_break_list_info->row));
        }

        break;
        }

    case PAGE_HEFO_BREAK_INTRO_ID_DELETE:
        {
        if(selected_item <= 0)
            status = STATUS_OK;
        else
        {
            P_PAGE_HEFO_BREAK_LIST_INFO p_page_hefo_break_list_info = array_ptr(&page_hefo_break_list_handle, PAGE_HEFO_BREAK_LIST_INFO, selected_item);

            /* ensure focus is not in any hefo */
            caret_show_claim(p_docu, p_docu->object_id_flow, FALSE);

            page_hefo_break_delete_from_row(p_docu, p_page_hefo_break_list_info->row);

            {
            DOCU_REFORMAT docu_reformat;
            docu_reformat.action = REFORMAT_HEFO_Y;
            docu_reformat.data_type = REFORMAT_ROW;
            docu_reformat.data_space = DATA_NONE;
            docu_reformat.data.row = p_page_hefo_break_list_info->row;
            status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            } /*block*/
        }

        break;
        }
    }

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_NOTE_POSITION_TRASH, P_DATA_NONE));

    ui_lists_dispose(&page_hefo_break_list_handle, &page_hefo_break_list_source);

    return(status);
}

static void
page_hefo_break_textual_representation(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer)
{
    PAGE_NUM page;
    STATUS is_page  = 0;
    STATUS msg;
    S32 arg;
    ROW_ENTRY row_entry;

    row_entry_from_row(p_docu, &row_entry, row);
    page.y = row_entry.rowtab.edge_top.page;

    if(row == row_from_page_y(p_docu, page.y))
        /* direct hit at top of page */
        is_page = 1;
#if 0
    else
    { /* try with following page */
        P_PAGE_HEFO_BREAK p_page_hefo_break;

        page.y++;

        p_page_hefo_break = p_page_hefo_break_from_page(p_docu, page.y);

        if(p_page_hefo_break && (row == p_page_hefo_break->region.tl.row))
            is_page = 1;
    }
#endif

    msg = is_page ? MSG_DIALOG_HEFO_REPR_PAGE : MSG_DIALOG_HEFO_REPR_ROW;
    arg = is_page ? ((S32) page.y + 1)        : ((S32) row  + 1);

    consume_int(ustr_xsnprintf(ustr_buf, elemof_buffer, resource_lookup_ustr(msg), arg));
}

_Check_return_
static STATUS
page_hefo_break_list_create(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_array_handle,
    /*out*/ P_UI_SOURCE p_ui_source)
{
    P_PAGE_HEFO_BREAK_LIST_INFO p_page_hefo_break_list_info;
    SC_ARRAY_INIT_BLOCK aib = aib_init(4, sizeof32(*p_page_hefo_break_list_info), TRUE);
    STATUS status = STATUS_OK;

    {
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    ROW row = -1;

    while(NULL != (p_page_hefo_break = page_hefo_break_enum(p_docu, &row)))
    {
        UNREFERENCED_PARAMETER(p_page_hefo_break);

        if(NULL != (p_page_hefo_break_list_info = al_array_extend_by(p_array_handle, PAGE_HEFO_BREAK_LIST_INFO, 1, &aib, &status)))
        {
            p_page_hefo_break_list_info->row = row;
            page_hefo_break_textual_representation(p_docu, p_page_hefo_break_list_info->row, ustr_bptr(p_page_hefo_break_list_info->ustr_name), sizeof32(p_page_hefo_break_list_info->ustr_name));
        }

        status_break(status);
    }
    } /*block*/

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
    {
        assert_EQ(offsetof32(PAGE_HEFO_BREAK_LIST_INFO, ustr_name), 0);
        status = ui_source_create_ub(p_array_handle, p_ui_source, UI_TEXT_TYPE_USTR_PERM, -1);
    }

    if(status_fail(status))
        ui_lists_dispose(p_array_handle, p_ui_source);

    return(status);
}

static void
save_page_hefo_break_value_fill(
    P_ARGLIST_HANDLE p_arglist_handle,
    P_PAGE_HEFO_BREAK p_page_hefo_break,
    _In_        DATA_SPACE data_space)
{
    const P_ARGLIST_ARG p_args = p_arglist_args(p_arglist_handle, ARG_PAGE_HEFO_BREAK_VALUES_N_ARGS);
    S32            argidx_define, argidx_margin, argidx_offset;
    BIT_NUMBER     bit_number;
    P_HEADFOOT_DEF p_headfoot_def;

    switch(data_space)
    {
    default: default_unhandled();
#if CHECKING
    case DATA_HEADER_ODD:
#endif
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_HEADER_ODD_DEFINE;
        bit_number     =                  PAGE_HEFO_HEADER_ODD;
        p_headfoot_def =        &p_page_hefo_break->header_odd;
        break;

    case DATA_HEADER_EVEN:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_HEADER_EVEN_DEFINE;
        bit_number     =                  PAGE_HEFO_HEADER_EVEN;
        p_headfoot_def =        &p_page_hefo_break->header_even;
        break;

    case DATA_HEADER_FIRST:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_HEADER_FIRST_DEFINE;
        bit_number     =                  PAGE_HEFO_HEADER_FIRST;
        p_headfoot_def =        &p_page_hefo_break->header_first;
        break;

    case DATA_FOOTER_ODD:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_FOOTER_ODD_DEFINE;
        bit_number     =                  PAGE_HEFO_FOOTER_ODD;
        p_headfoot_def =        &p_page_hefo_break->footer_odd;
        break;

    case DATA_FOOTER_EVEN:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_FOOTER_EVEN_DEFINE;
        bit_number     =                  PAGE_HEFO_FOOTER_EVEN;
        p_headfoot_def =        &p_page_hefo_break->footer_even;
        break;

    case DATA_FOOTER_FIRST:
        argidx_define  = ARG_PAGE_HEFO_BREAK_VALUES_FOOTER_FIRST_DEFINE;
        bit_number     =                  PAGE_HEFO_FOOTER_FIRST;
        p_headfoot_def =        &p_page_hefo_break->footer_first;
        break;
    }

    argidx_margin = argidx_define + 1;
    argidx_offset = argidx_margin + 1;

    p_args[argidx_define].val.fBool = (page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number) != 0);

    if(p_args[argidx_define].val.fBool)
    {
        p_args[argidx_margin].val.fp_pixit = (FP_PIXIT) p_headfoot_def->headfoot_sizes.margin;
        p_args[argidx_offset].val.fp_pixit = (FP_PIXIT) p_headfoot_def->headfoot_sizes.offset;
    }
    else
    {
        arg_dispose(p_arglist_handle, argidx_margin);
        arg_dispose(p_arglist_handle, argidx_offset);
    }
}

_Check_return_
static STATUS
save_page_hefo_break_values(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_PAGE_HEFO_BREAK p_page_hefo_break)
{
    ROW row = p_page_hefo_break->region.whole_col ? 0 : p_page_hefo_break->region.tl.row;
    const OBJECT_ID object_id = OBJECT_ID_HEFO;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_PAGE_HEFO_BREAK_VALUES, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, ARG_PAGE_HEFO_BREAK_VALUES_N_ARGS);

        p_args[ARG_PAGE_HEFO_BREAK_VALUES_ROW].val.row = row;

        p_args[ARG_PAGE_HEFO_BREAK_VALUES_PAGE_BREAK].val.fBool         = (page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_BREAK) != 0);
        p_args[ARG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER_DEFINE].val.fBool = (page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_NUM) != 0);
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_NUM))
            p_args[ARG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER].val.s32      = p_page_hefo_break->page_y + 1;
        else
            arg_dispose(&arglist_handle, ARG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER);

        save_page_hefo_break_value_fill(&arglist_handle, p_page_hefo_break, DATA_HEADER_ODD  );
        save_page_hefo_break_value_fill(&arglist_handle, p_page_hefo_break, DATA_HEADER_EVEN );
        save_page_hefo_break_value_fill(&arglist_handle, p_page_hefo_break, DATA_HEADER_FIRST);
        save_page_hefo_break_value_fill(&arglist_handle, p_page_hefo_break, DATA_FOOTER_ODD  );
        save_page_hefo_break_value_fill(&arglist_handle, p_page_hefo_break, DATA_FOOTER_EVEN );
        save_page_hefo_break_value_fill(&arglist_handle, p_page_hefo_break, DATA_FOOTER_FIRST);

        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
page_hefo_break_values_change(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    const OBJECT_ID object_id = OBJECT_ID_HEFO;
    const T5_MESSAGE t5_message = T5_CMD_PAGE_HEFO_BREAK_VALUES;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, ARG_PAGE_HEFO_BREAK_VALUES_ROW+1);

        p_args[ARG_PAGE_HEFO_BREAK_VALUES_ROW].val.row = row;

        /* none of the rest are present */
        arglist_dispose_after(&arglist_handle, ARG_PAGE_HEFO_BREAK_VALUES_ROW);

        command_set_interactive();

        status = execute_command(p_docu, t5_message, &arglist_handle, object_id);

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

/******************************************************************************
*
* hefo event handler
*
******************************************************************************/

_Check_return_
static STATUS
hefo_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->mark_info_hefo.h_markers = 0;
    p_docu->mark_info_hefo.h_markers_screen = 0;
    p_docu->mark_focus_hefo.data_space = DATA_NONE;

    return(maeve_event_handler_add(p_docu, maeve_event_ob_hefo, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
hefo_msg_init2(
    _DocuRef_   P_DOCU p_docu)
{
    view_install_pane_window_hefo_handlers(p_docu, object_header, object_footer);

    return(STATUS_OK);
}

_Check_return_
static STATUS
hefo_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_ob_hefo, (CLIENT_HANDLE) 0);

    al_array_dispose(&p_docu->mark_info_hefo.h_markers);
    al_array_dispose(&p_docu->mark_info_hefo.h_markers_screen);

    return(STATUS_OK);
}

_Check_return_
static STATUS
hefo_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        object_install(OBJECT_ID_HEADER, object_header); /* these sub-objects must NOT themselves need init <<< */
        object_install(OBJECT_ID_FOOTER, object_footer);

        status_return(resource_init(OBJECT_ID_HEFO, P_BOUND_MESSAGES_OBJECT_ID_HEFO, P_BOUND_RESOURCES_OBJECT_ID_HEFO));

        return(register_object_construct_table(OBJECT_ID_HEFO, object_construct_table, FALSE /* no inlines in table */));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_HEFO));

    /* initialise object in new document */
    case T5_MSG_IC__INIT1:
        return(hefo_msg_init1(p_docu));

    case T5_MSG_IC__INIT2:
        return(hefo_msg_init2(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(hefo_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_hefo);
OBJECT_PROTO(extern, object_hefo)
{
    STATUS status = STATUS_OK;

    switch(t5_message)
    {
    /* transmogrify messages into inlines */
    case T5_CMD_INSERT_FIELD_DATE:
    case T5_CMD_INSERT_FIELD_FILE_DATE:
    case T5_CMD_INSERT_FIELD_PAGE_X:
    case T5_CMD_INSERT_FIELD_PAGE_Y:
    case T5_CMD_INSERT_FIELD_SS_NAME:
    case T5_CMD_INSERT_FIELD_MS_FIELD:
    case T5_CMD_INSERT_FIELD_WHOLENAME:
    case T5_CMD_INSERT_FIELD_LEAFNAME:
    case T5_CMD_INSERT_FIELD_RETURN:
    case T5_CMD_INSERT_FIELD_SOFT_HYPHEN:
    case T5_CMD_INSERT_FIELD_TAB:

    case T5_CMD_WORD_COUNT:
        /* send these messages to our helper object */
        { /* SKS after 1.07 02dec93 - we can't just dereference p_data willy-nilly! */
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        return(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, !IS_PTR_NONE(p_hefo_block) ? p_hefo_block->p_data : P_DATA_NONE));
        }

    case T5_CMD_TAB_RIGHT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        TAB_WANTED tab_wanted;
        TEXT_MESSAGE_BLOCK text_message_block;

        zero_struct(tab_wanted);
        tab_wanted.object_data = p_hefo_block->object_data;
        tab_wanted.t5_message = t5_message;
        text_message_block_init(p_docu, &text_message_block, &tab_wanted, p_hefo_block, &tab_wanted.object_data);
        status = object_call_STORY_with_tmb(p_docu, T5_MSG_TAB_WANTED, &text_message_block);
        if(tab_wanted.want_inline_insert)
            status_consume(object_call_HEFO_with_hb(p_docu, T5_CMD_INSERT_FIELD_TAB, P_DATA_NONE));
        break;
        }

    /* get the text object to handle these messages */
    case T5_MSG_SAVE_CONSTRUCT_OWNFORM:
        return(object_call_id(OBJECT_ID_TEXT, p_docu, t5_message, p_data));

    case T5_MSG_INITCLOSE:
        return(hefo_msg_initclose(p_docu, (PC_MSG_INITCLOSE) p_data));

    case T5_EVENT_REDRAW:
        {
        const P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        const P_OBJECT_REDRAW p_object_redraw = P_DATA_FROM_HEFO_BLOCK(OBJECT_REDRAW, p_hefo_block);

        p_object_redraw->pixit_rect_object.tl = p_object_redraw->skel_rect_object.tl.pixit_point;
        p_object_redraw->pixit_rect_object.br = p_object_redraw->skel_rect_object.br.pixit_point;
        p_object_redraw->flags.marked_now = 0;
        p_object_redraw->flags.marked_screen = 0;

        if(data_refs_equal(&p_docu->mark_focus_hefo, &p_hefo_block->object_data.data_ref))
        {
            if(p_docu->mark_info_hefo.h_markers)
            {
                P_MARKERS p_markers = array_base(&p_docu->mark_info_hefo.h_markers, MARKERS);
                p_object_redraw->flags.marked_now = 1;
                p_object_redraw->start_now = p_markers->docu_area.tl.object_position;
                p_object_redraw->end_now = p_markers->docu_area.br.object_position;
                trace_2(TRACE_APP_SKEL,
                        TEXT("OB_HEFO redraw marked_now, start: ") S32_TFMT TEXT(", end: ") S32_TFMT,
                        p_markers->docu_area.tl.object_position.data,
                        p_markers->docu_area.br.object_position.data);
            }

            if(p_docu->mark_info_hefo.h_markers_screen)
            {
                P_MARKERS p_markers = array_base(&p_docu->mark_info_hefo.h_markers_screen, MARKERS);
                p_object_redraw->flags.marked_screen = 1;
                p_object_redraw->start_screen = p_markers->docu_area.tl.object_position;
                p_object_redraw->end_screen = p_markers->docu_area.br.object_position;
            }
        }

        {
        TEXT_MESSAGE_BLOCK text_message_block;
        text_message_block_init(p_docu, &text_message_block, p_object_redraw, p_hefo_block, &p_object_redraw->object_data);
        status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block);
        fonty_cache_trash(&p_object_redraw->redraw_context);
        } /*block*/

        break;
        }

    case T5_MSG_OBJECT_HOW_BIG:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_OBJECT_HOW_BIG p_object_how_big = P_DATA_FROM_HEFO_BLOCK(OBJECT_HOW_BIG, p_hefo_block);
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu, &text_message_block, p_object_how_big, p_hefo_block, &p_object_how_big->object_data);
        return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
        }

    case T5_MSG_OBJECT_KEYS:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_HEFO_KEYS p_hefo_keys = P_DATA_FROM_HEFO_BLOCK(HEFO_KEYS, p_hefo_block);

        status = hefo_insert_sub_redisplay(p_docu,
                                           p_hefo_block,
                                           &p_hefo_keys->object_position,
                                           p_hefo_keys->p_skelevent_keys->p_quick_ublock);

        p_hefo_keys->p_skelevent_keys->processed = 1;
        break;
        }

    case T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION:
    case T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_OBJECT_POSITION_FIND p_object_position_find = P_DATA_FROM_HEFO_BLOCK(OBJECT_POSITION_FIND, p_hefo_block);
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu, &text_message_block, p_object_position_find, p_hefo_block, &p_object_position_find->object_data);
        return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
        }

    case T5_MSG_OBJECT_POSITION_SET:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        return(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, P_DATA_FROM_HEFO_BLOCK(OBJECT_POSITION_SET, p_hefo_block)));
        }

    case T5_MSG_OBJECT_LOGICAL_MOVE:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_OBJECT_LOGICAL_MOVE p_object_logical_move = P_DATA_FROM_HEFO_BLOCK(OBJECT_LOGICAL_MOVE, p_hefo_block);
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu, &text_message_block, p_object_logical_move, p_hefo_block, &p_object_logical_move->object_data);
        return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
        }

    case T5_MSG_CARET_SHOW_CLAIM:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_CARET_SHOW_CLAIM p_caret_show_claim = P_DATA_FROM_HEFO_BLOCK(CARET_SHOW_CLAIM, p_hefo_block);
        OBJECT_POSITION_FIND object_position_find;

        if(array_elements(p_hefo_block->p_h_data))
        {
            /* try to set caret sub-object position */
            if(OBJECT_ID_NONE == p_docu->hefo_position.object_position.object_id)
            {
                p_docu->hefo_position.object_position.object_id = OBJECT_ID_HEFO;
                p_docu->hefo_position.object_position.data = 0;
            }
        }
        else
            p_docu->hefo_position.object_position.object_id = OBJECT_ID_NONE;

        object_position_find.object_data = p_hefo_block->object_data;
        object_position_find.object_data.object_position_start = p_docu->hefo_position.object_position;
        object_position_find.skel_rect = p_hefo_block->skel_rect_object;
        p_hefo_block->p_data = &object_position_find;
        status_assert(object_hefo(p_docu, T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION, p_hefo_block));

        if((p_docu->caret_height = object_position_find.caret_height) != 0)
            p_docu->caret = object_position_find.pos;
        else
            /* ensure we have a point inside the object area */
            p_docu->caret = p_hefo_block->skel_rect_object.tl;

        edge_set_from_skel_point(&p_docu->caret_x, &p_docu->caret, x);

        status_assert(maeve_event(p_docu, T5_MSG_DOCU_CARETMOVE, P_DATA_NONE));

        view_show_caret(p_docu,
                        p_hefo_block->redraw_tag,
                        &p_docu->caret,
                        p_docu->mark_info_hefo.h_markers ? 0 : p_docu->caret_height);

        if(p_caret_show_claim->scroll)
            view_scroll_caret(p_docu,
                              p_hefo_block->redraw_tag,
                              &p_docu->caret,
                              p_hefo_block->skel_rect_object.tl.pixit_point.x - p_docu->caret.pixit_point.x,
                              p_hefo_block->skel_rect_object.br.pixit_point.x - p_docu->caret.pixit_point.x,
                              0,
                              p_docu->caret_height);

#if TRACE_ALLOWED
        {
        static S32 count = 0;
        trace_1(TRACE_APP_SKEL, TEXT("hefo_caret_claim, count: ") S32_TFMT, count++);
        } /*block*/
#endif

        break;
        }

    case T5_MSG_ANCHOR_NEW:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(MARKERS), TRUE);
        P_MARKERS p_markers;

        p_docu->mark_focus_hefo = p_hefo_block->object_data.data_ref;

        if(NULL != (p_markers = al_array_extend_by(&p_docu->mark_info_hefo.h_markers, MARKERS, 1, &array_init_block, &status)))
        {
            UNREFERENCED_PARAMETER(p_markers);

            al_array_auto_compact_set(&p_docu->mark_info_hefo.h_markers);

            status_assert(object_hefo(p_docu, T5_MSG_ANCHOR_UPDATE, P_DATA_NONE));
        }

        break;
        }

    case T5_MSG_ANCHOR_UPDATE:
        {
        P_MARKERS p_markers;

        p_markers = array_base(&p_docu->mark_info_hefo.h_markers, MARKERS);
        *p_markers = p_docu->anchor_mark;

        if((OBJECT_ID_NONE != p_markers->docu_area.tl.object_position.object_id) && (OBJECT_ID_NONE != p_markers->docu_area.br.object_position.object_id))
        {
            /* we don't need generalised object position handling here - since at the
             * moment the only objects allowed in headers/footers are HEFO/ob_story
             */
            if(object_position_compare(&p_markers->docu_area.tl.object_position,
                                       &p_markers->docu_area.br.object_position,
                                       OBJECT_POSITION_COMPARE_SE) > 0)
            {
                OBJECT_POSITION temp = p_markers->docu_area.tl.object_position;
                p_markers->docu_area.tl.object_position = p_markers->docu_area.br.object_position;
                p_markers->docu_area.br.object_position = temp;
            }
        }

        /* clear object position if at start */
        if((OBJECT_ID_NONE != p_markers->docu_area.tl.object_position.object_id) && (0 == p_markers->docu_area.tl.object_position.data))
            p_markers->docu_area.tl.object_position.object_id = OBJECT_ID_NONE;

        break;
        }

    case T5_MSG_ANCHOR_FINISHED:
        {
        status_assert(object_hefo(p_docu, T5_MSG_ANCHOR_UPDATE, P_DATA_NONE));

        /* check for empty markers and throw em away */
        if(p_docu->mark_info_hefo.h_markers)
        {
            P_MARKERS p_markers = array_base(&p_docu->mark_info_hefo.h_markers, MARKERS);

            if(docu_area_is_empty(&p_markers->docu_area))
                hefo_markers_clear(p_docu);
            else
                status_assert(maeve_event(p_docu, T5_MSG_SELECTION_NEW, P_DATA_NONE));
        }
        break;
        }

    case T5_MSG_SELECTION_SHOW:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;

        if(p_docu->mark_info_hefo.h_markers)
        {
            /* if markers are about to be shown, this will hide caret first
             * Windows needs its caret out of the way during inverts otherwise you get ghosts
             */
            hefo_caret_position_set_show(p_docu, p_hefo_block, FALSE);
        }

        if(p_docu->mark_info_hefo.h_markers
           ||
           p_docu->mark_info_hefo.h_markers_screen)
        {
            SKEL_RECT skel_rect;
            RECT_FLAGS rect_flags;
            REDRAW_FLAGS redraw_flags;

            skel_rect.tl.pixit_point.x =
            skel_rect.tl.pixit_point.y = 0;
            skel_rect.tl.page_num.x =
            skel_rect.tl.page_num.y = 0;
            skel_rect.br = skel_rect.tl;

            RECT_FLAGS_CLEAR(rect_flags);
            rect_flags.extend_right_window = rect_flags.extend_down_window = 1;

            REDRAW_FLAGS_CLEAR(redraw_flags);
            redraw_flags.show_selection = TRUE;

            view_update_now(p_docu, p_hefo_block->redraw_tag, &skel_rect, rect_flags, redraw_flags, LAYER_CELLS);
        }

        /* dispose of markers that were */
        al_array_dispose(&p_docu->mark_info_hefo.h_markers_screen);

        /* update our record of markers on screen */
        status_assert(al_array_duplicate(&p_docu->mark_info_hefo.h_markers_screen, &p_docu->mark_info_hefo.h_markers));

        if(!p_docu->mark_info_cells.h_markers)
        {
            /* if markers have been cleared, this will show caret after:
             * Windows needs its caret out of the way during inverts otherwise you get ghosts
             */
            hefo_caret_position_set_show(p_docu, p_hefo_block, FALSE);
        }

        break;
        }

    case T5_MSG_SELECTION_HIDE:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        ARRAY_HANDLE h_markers_temp;

        h_markers_temp = p_docu->mark_info_hefo.h_markers;
        p_docu->mark_info_hefo.h_markers = 0;
        status_assert(object_hefo(p_docu, T5_MSG_SELECTION_SHOW, p_hefo_block));
        p_docu->mark_info_hefo.h_markers = h_markers_temp;
        break;
        }

    case T5_MSG_MARK_INFO_READ:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_MARK_INFO p_mark_info = P_DATA_FROM_HEFO_BLOCK(MARK_INFO, p_hefo_block);

        if(data_refs_equal(&p_hefo_block->object_data.data_ref, &p_docu->mark_focus_hefo))
            *p_mark_info = p_docu->mark_info_hefo;
        else
        {
            p_mark_info->h_markers =
            p_mark_info->h_markers_screen = 0;
        }
        break;
        }

    case T5_MSG_OBJECT_DELETE_SUB:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_OBJECT_DELETE_SUB p_object_delete_sub = P_DATA_FROM_HEFO_BLOCK(OBJECT_DELETE_SUB, p_hefo_block);
        TEXT_MESSAGE_BLOCK text_message_block;
        TEXT_INLINE_REDISPLAY text_inline_redisplay;

        p_object_delete_sub->h_data_del = 0;
        text_message_block_init(p_docu, &text_message_block, &text_inline_redisplay, p_hefo_block, &p_object_delete_sub->object_data);
        text_message_block.inline_object.object_data.object_position_start = p_object_delete_sub->object_data.object_position_start;
        text_message_block.inline_object.object_data.object_position_end = p_object_delete_sub->object_data.object_position_end;
        text_inline_redisplay_init(&text_inline_redisplay,
                                   P_QUICK_UBLOCK_NONE,
                                   &text_message_block.text_format_info.skel_rect_object,
                                   TRUE,
                                   p_hefo_block->redraw_tag);

        {
        S32 start, end, del_size;

        offsets_from_object_data(&start, &end, &text_message_block.inline_object.object_data, text_message_block.inline_object.inline_len);
        del_size = end - start;

        if(del_size)
        {
            /* save needs a call to save part of an object, given two
             * object_positions and a list of regions; a special hefo_save/_load
             * perhaps; see equivalent routine in ob_text
             */
            if(status_ok(status = object_call_STORY_with_tmb(p_docu, T5_MSG_DELETE_INLINE_REDISPLAY, &text_message_block)))
            {
                /* adjust object size -- don't create object with just terminator byte */
                al_array_shrink_by(p_hefo_block->p_h_data,
                                             (text_message_block.inline_object.inline_len == 0)
                                                 ? -(del_size + 1)
                                                 : -(del_size + 0));

                if(text_inline_redisplay.size_changed || !text_inline_redisplay.redraw_done)
                    view_update_all(p_docu, p_hefo_block->redraw_tag);
            }

        }
        } /*block*/

        break;
        }

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_OBJECT_SET_CASE p_object_set_case = P_DATA_FROM_HEFO_BLOCK(OBJECT_SET_CASE, p_hefo_block);
        TEXT_MESSAGE_BLOCK text_message_block;
        TEXT_INLINE_REDISPLAY text_inline_redisplay;

        text_message_block_init(p_docu, &text_message_block, &text_inline_redisplay, p_hefo_block, &p_object_set_case->object_data);
        text_inline_redisplay_init(&text_inline_redisplay,
                                   P_QUICK_UBLOCK_NONE,
                                   &text_message_block.text_format_info.skel_rect_object,
                                   p_object_set_case->do_redraw,
                                   p_hefo_block->redraw_tag);
        return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
        break;
        }

    case T5_CMD_OF_HEFO_DATA:
        return(hefo_load_data(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_OF_HEFO_REGION:
    case T5_CMD_OF_HEFO_BASE_REGION:
        return(hefo_load_region(p_docu, t5_message, (P_CONSTRUCT_CONVERT) p_data));

    case T5_CMD_PAGE_HEFO_BREAK_INTRO:
        return(t5_cmd_page_hefo_break_intro(p_docu));

    case T5_CMD_PAGE_HEFO_BREAK_VALUES:
        return(t5_cmd_page_hefo_break_values(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }

    return(status);
}

/*
re-integrated from ob_hefo2.c
*/

#include "ob_cells/sk_draw.h"

static void
hefo_caret_to_point(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InVal_     BOOL caret_x_save);

static void
hefo_cursor_left(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message /* cursor_ or word_left */,
    P_HEFO_BLOCK p_hefo_block);

static void
hefo_cursor_right(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message /* cursor_ or word_right */,
    P_HEFO_BLOCK p_hefo_block);

_Check_return_ _Success_(status_ok(return))
static STATUS
hefo_logical_move(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block,
    _OutRef_    P_SKEL_POINT p_skel_point,
    _InVal_     enum OBJECT_LOGICAL_MOVE_ACTION action);

_Check_return_
static STATUS /* >0 if no op; no error */
hefo_selection_delete_auto(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block);

_Check_return_
static STATUS
position_set_from_skel_point_hefo(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _InRef_     PC_SKEL_POINT p_skel_point);

_Check_return_
static STATUS
selection_style_hefo(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _OutRef_    P_STYLE_SELECTOR p_style_selector_fuzzy,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_in /* effects to look at */,
    P_HEFO_BLOCK p_hefo_block);

PROC_EVENT_PROTO(static, proc_event_hefo_common);

/******************************************************************************
*
* paint hefo gridlines
*
******************************************************************************/

static void
hefo_grid(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_STYLE p_style,
    _InVal_     S32 focus)
{
    /* only render gridlines if requested */
    if(!p_object_redraw->flags.show_content)
        return;

    if(grid_is_on(p_docu))
    {
        const PC_REDRAW_CONTEXT p_redraw_context = &p_object_redraw->redraw_context;
        PIXIT_RECT pixit_rect;
        PIXIT_LINE pixit_line;
        BORDER_LINE_FLAGS border_line_flags;

        /* adjust to border rectangle */
        pixit_rect = p_object_redraw->pixit_rect_object;

        /* left border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.br.x = pixit_line.tl.x;
        pixit_line.horizontal = 0;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.border_style = p_style->para_style.grid_left;

        host_paint_border_line(p_redraw_context, &pixit_line, &p_style->para_style.rgb_grid_left, border_line_flags);

        /* top border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.br.y = pixit_line.tl.y;
        pixit_line.horizontal = 1;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.border_style = p_style->para_style.grid_top;

        {
        RGB rgb = p_style->para_style.rgb_grid_top;

        if(focus == OBJECT_ID_FOOTER && p_style->para_style.grid_top == SF_BORDER_NONE)
        {
            if(!(p_redraw_context->flags.printer  ||
                 p_redraw_context->flags.metafile ||
                 p_redraw_context->flags.drawfile ) )
            {
                rgb = rgb_stash[COLOUR_OF_HEFO_LINE];
                border_line_flags.border_style = SF_BORDER_THIN;
            }
        }

        host_paint_border_line(p_redraw_context, &pixit_line, &rgb, border_line_flags);
        } /*block*/

        /* right border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.tl.x = pixit_line.br.x;
        pixit_line.horizontal = 0;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.border_style = p_style->para_style.grid_right;
        border_line_flags.add_lw_to_b = 1;

        host_paint_border_line(p_redraw_context, &pixit_line, &p_style->para_style.rgb_grid_right, border_line_flags);

        /* bottom border */
        pixit_line.tl = pixit_rect.tl;
        pixit_line.br = pixit_rect.br;
        pixit_line.tl.y = pixit_line.br.y;
        pixit_line.horizontal = 1;

        CLEAR_BORDER_LINE_FLAGS(border_line_flags);
        border_line_flags.border_style = p_style->para_style.grid_bottom;
        border_line_flags.add_lw_to_r = 1;

        {
        RGB rgb = p_style->para_style.rgb_grid_bottom;

        if(focus == OBJECT_ID_HEADER && p_style->para_style.grid_bottom == SF_BORDER_NONE)
        {
            if(!(p_redraw_context->flags.printer  ||
                 p_redraw_context->flags.metafile ||
                 p_redraw_context->flags.drawfile ) )
            {
                rgb = rgb_stash[COLOUR_OF_HEFO_LINE];
                border_line_flags.border_style = SF_BORDER_THIN;
            }
        }

        host_paint_border_line(p_redraw_context, &pixit_line, &rgb, border_line_flags);
        } /*block*/
    }
}

/******************************************************************************
*
* initialise hefo object data
*
******************************************************************************/

static void
hefo_object_data_init(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _In_        DATA_SPACE data_space)
{
    /* set up hefo object_data structure */
    p_hefo_block->object_data.data_ref.data_space = data_space;

    if(array_elements(p_hefo_block->p_h_data))
    {
        p_hefo_block->object_data.u.p_object = array_base(p_hefo_block->p_h_data, void);
        p_hefo_block->object_data.object_id = OBJECT_ID_HEFO;
        p_hefo_block->object_data.object_position_start = p_docu->hefo_position.object_position;
    }
    else
    {
        object_position_init(&p_hefo_block->object_data.object_position_start);
        p_hefo_block->object_data.object_id = OBJECT_ID_NONE;
        p_hefo_block->object_data.u.p_object = P_DATA_NONE;
    }

    object_position_init(&p_hefo_block->object_data.object_position_end);
}

/******************************************************************************
*
* make up some object data from a position
*
******************************************************************************/

static BOOL
object_data_from_hefo_block_and_object_position(
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     P_HEFO_BLOCK p_hefo_block,
    _InRef_maybenone_ PC_OBJECT_POSITION p_object_position)
{
    *p_object_data = p_hefo_block->object_data;

    if(!IS_P_DATA_NONE(p_object_position))
        p_object_data->object_position_start = *p_object_position;

    return(!IS_P_DATA_NONE(p_hefo_block->object_data.u.p_object));
}

_Check_return_
static STATUS
do_hefo_common_event_click(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block)
{
    return(proc_event_hefo_common(p_docu, t5_message, p_hefo_block));
}

_Check_return_
static STATUS
try_hefo_common_event_click(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     OBJECT_ID object_id)
{
    HEFO_BLOCK hefo_block;

    if(status_done(hefo_block_from_page_num(p_docu,
                                            &hefo_block,
                                            p_skelevent_click,
                                            &p_skelevent_click->skel_point.page_num,
                                            object_id)))
    {
        return(do_hefo_common_event_click(p_docu, t5_message, &hefo_block));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
try_hefo_common_event_click_single(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     OBJECT_ID object_id)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_SINGLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
    {
        if(p_docu->focus_owner != object_id)
            return(STATUS_OK);
    }

    return(try_hefo_common_event_click(p_docu, t5_message_effective, p_skelevent_click, object_id));
}

_Check_return_
static STATUS
try_hefo_common_event_click_drag(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_CLICK p_skelevent_click,
    _InVal_     OBJECT_ID object_id)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
    {
        if(p_docu->focus_owner != object_id)
            return(STATUS_OK);
    }

    return(try_hefo_common_event_click(p_docu, t5_message_effective, p_skelevent_click, object_id));
}

_Check_return_
static STATUS
do_hefo_common_event_keys(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block)
{
    return(proc_event_hefo_common(p_docu, t5_message, p_hefo_block));
}

_Check_return_
static STATUS
try_hefo_common_event_keys(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_KEYS p_skelevent_keys,
    _InVal_     OBJECT_ID object_id)
{
    HEFO_BLOCK hefo_block;

    if(status_done(hefo_block_from_page_num(p_docu,
                                            &hefo_block,
                                            p_skelevent_keys,
                                            &p_docu->hefo_position.page_num,
                                            object_id)))
    {
        return(do_hefo_common_event_keys(p_docu, t5_message, &hefo_block));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
do_hefo_common_event_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block)
{
    return(proc_event_hefo_common(p_docu, t5_message, p_hefo_block));
}

_Check_return_
static STATUS
try_hefo_common_event_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InVal_     OBJECT_ID object_id)
{
    HEFO_BLOCK hefo_block;

    if(status_done(hefo_block_from_page_num(p_docu,
                                            &hefo_block,
                                            p_skelevent_redraw,
                                            &p_skelevent_redraw->work_skel_rect.tl.page_num,
                                            object_id)))
    {
        return(do_hefo_common_event_redraw(p_docu, t5_message, &hefo_block));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
do_hefo_common_msg(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block)
{
    return(proc_event_hefo_common(p_docu, t5_message, p_hefo_block));
}

_Check_return_
static STATUS
try_hefo_common_msg(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_ANY p_data,
    _InVal_     OBJECT_ID object_id)
{
    HEFO_BLOCK hefo_block;

    if(status_done(hefo_block_from_page_num(p_docu,
                                            &hefo_block,
                                            p_data,
                                            &p_docu->hefo_position.page_num,
                                            object_id)))
    {
        return(do_hefo_common_msg(p_docu, t5_message, &hefo_block));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
do_hefo_common_msg_object_read_text_draft(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block)
{
    P_OBJECT_READ_TEXT_DRAFT p_object_read_text_draft = P_DATA_FROM_HEFO_BLOCK(OBJECT_READ_TEXT_DRAFT, p_hefo_block);
    TEXT_MESSAGE_BLOCK text_message_block;

    p_object_read_text_draft->object_data = p_hefo_block->object_data;

    text_message_block_init(p_docu, &text_message_block, p_object_read_text_draft, p_hefo_block, &p_object_read_text_draft->object_data);

    if(0 == text_message_block.inline_object.inline_len)
        return(STATUS_OK);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
}

_Check_return_
static STATUS
try_hefo_common_msg_object_read_text_draft(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_OBJECT_READ_TEXT_DRAFT p_object_read_text_draft,
    _InVal_     OBJECT_ID object_id)
{
    HEFO_BLOCK hefo_block;

    if(status_done(hefo_block_from_page_num(p_docu,
                                            &hefo_block,
                                            p_object_read_text_draft,
                                            &p_object_read_text_draft->skel_rect_object.tl.page_num,
                                            object_id)))
    {
        return(do_hefo_common_msg_object_read_text_draft(p_docu, t5_message, &hefo_block));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* header / footer common event handling
*
******************************************************************************/

_Check_return_
static STATUS
try_hefo_common_event(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /**/        P_ANY p_data,
    _InVal_     OBJECT_ID object_id)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_DRAG_ABORTED:

    case T5_MSG_CARET_SHOW_CLAIM:
    case T5_MSG_MARK_INFO_READ:
    case T5_MSG_RULER_INFO:
    case T5_MSG_SELECTION_STYLE:

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:

    case T5_CMD_BOX:
    case T5_CMD_STYLE_APPLY:
    case T5_CMD_STYLE_APPLY_SOURCE:
    case T5_CMD_STYLE_REGION_EDIT:

    case T5_CMD_INSERT_FIELD_DATE:
    case T5_CMD_INSERT_FIELD_FILE_DATE:
    case T5_CMD_INSERT_FIELD_PAGE_X:
    case T5_CMD_INSERT_FIELD_PAGE_Y:
    case T5_CMD_INSERT_FIELD_SS_NAME:
    case T5_CMD_INSERT_FIELD_MS_FIELD:
    case T5_CMD_INSERT_FIELD_WHOLENAME:
    case T5_CMD_INSERT_FIELD_LEAFNAME:
    case T5_CMD_INSERT_FIELD_RETURN:
    case T5_CMD_INSERT_FIELD_SOFT_HYPHEN:
    case T5_CMD_INSERT_FIELD_TAB:

    case T5_CMD_WORD_COUNT:

    case T5_CMD_DELETE_CHARACTER_RIGHT:
    case T5_CMD_DELETE_CHARACTER_LEFT:

    case T5_CMD_CURSOR_LEFT:
    case T5_CMD_CURSOR_RIGHT:
    case T5_CMD_CURSOR_UP:
    case T5_CMD_CURSOR_DOWN:
    case T5_CMD_WORD_LEFT:
    case T5_CMD_WORD_RIGHT:
    case T5_CMD_LINE_START:
    case T5_CMD_LINE_END:

    case T5_CMD_TAB_RIGHT:

    case T5_CMD_SELECT_WORD:
    case T5_CMD_SELECT_CELL:
    case T5_CMD_SELECT_DOCUMENT:
    case T5_CMD_TOGGLE_MARKS:
    case T5_CMD_SELECTION_CLEAR:
        return(try_hefo_common_msg(p_docu, t5_message, p_data, object_id));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
    case T5_EVENT_CLICK_DRAG_FINISHED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
    case T5_EVENT_FILEINSERT_DOINSERT:
        return(try_hefo_common_event_click(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data, object_id));

    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
        return(try_hefo_common_event_click_single(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data, object_id));

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(try_hefo_common_event_click_drag(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data, object_id));

    case T5_EVENT_REDRAW:
        return(try_hefo_common_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data, object_id));

    case T5_EVENT_KEYS:
        return(try_hefo_common_event_keys(p_docu, t5_message, (P_SKELEVENT_KEYS) p_data, object_id));

    case T5_MSG_OBJECT_READ_TEXT_DRAFT:
        return(try_hefo_common_msg_object_read_text_draft(p_docu, t5_message, (P_OBJECT_READ_TEXT_DRAFT) p_data, object_id));

    default:
        return(STATUS_OK);
    }
}
/******************************************************************************
*
* header event handler
*
******************************************************************************/

OBJECT_PROTO(static, object_header)
{
    return(try_hefo_common_event(p_docu, t5_message, p_data, OBJECT_ID_HEADER));
}

/******************************************************************************
*
* footer event handler
*
******************************************************************************/

OBJECT_PROTO(static, object_footer)
{
    return(try_hefo_common_event(p_docu, t5_message, p_data, OBJECT_ID_FOOTER));
}

/******************************************************************************
*
* common header/footer code
*
******************************************************************************/

PROC_EVENT_PROTO(static, proc_event_hefo_common)
{
    STATUS status = STATUS_OK;

    switch(t5_message)
    {
    default:
        return(object_call_HEFO_with_hb(p_docu, t5_message, p_data));

    case T5_EVENT_CLICK_LEFT_SINGLE:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_SKELEVENT_CLICK p_skelevent_click = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_CLICK, p_hefo_block);

        p_skelevent_click->processed = 1;

        hefo_caret_to_point(p_docu, p_hefo_block, &p_skelevent_click->skel_point, FALSE);
        break;
        }

    case T5_EVENT_CLICK_LEFT_DRAG:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_SKELEVENT_CLICK p_skelevent_click = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_CLICK, p_hefo_block);
        OBJECT_POSITION_FIND object_position_find;

        p_skelevent_click->processed = 1;

        CODE_ANALYSIS_ONLY(zero_struct(object_position_find));
        p_hefo_block->p_data = &object_position_find;
        status_assert(position_set_from_skel_point_hefo(p_docu, p_hefo_block, &p_skelevent_click->skel_point));

        docu_area_init_hefo(&p_docu->anchor_mark.docu_area);
        p_docu->anchor_mark.docu_area.tl.object_position = object_position_find.object_data.object_position_start;

        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_NEW, p_hefo_block));

        host_drag_start(NULL);
        break;
        }

    case T5_EVENT_CLICK_DRAG_FINISHED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_SKELEVENT_CLICK p_skelevent_click = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_CLICK, p_hefo_block);
        MARK_INFO mark_info;

        /* read current markers */
        CODE_ANALYSIS_ONLY(zero_struct(mark_info));
        p_hefo_block->p_data = &mark_info;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

        if(mark_info.h_markers)
        {
            OBJECT_POSITION_FIND object_position_find;

            CODE_ANALYSIS_ONLY(zero_struct(object_position_find));
            p_hefo_block->p_data = &object_position_find;
            status_assert(position_set_from_skel_point_hefo(p_docu, p_hefo_block, &p_skelevent_click->skel_point));

            p_docu->anchor_mark.docu_area.br.object_position = object_position_find.object_data.object_position_start;

            status_consume(object_call_HEFO_with_hb(p_docu,
                           (T5_EVENT_CLICK_DRAG_FINISHED == t5_message) ? T5_MSG_ANCHOR_FINISHED : T5_MSG_ANCHOR_UPDATE,
                           p_hefo_block));

            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_SELECTION_SHOW, p_hefo_block));
        }
        break;
        }

    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_CMD_SELECT_WORD:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        OBJECT_POSITION_SET object_position_set;

        if(object_data_from_hefo_block_and_object_position(&object_position_set.object_data, p_hefo_block, P_OBJECT_POSITION_NONE))
        {
            if(T5_EVENT_CLICK_LEFT_DOUBLE == t5_message)
            {
                P_SKELEVENT_CLICK p_skelevent_click = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_CLICK, p_hefo_block);
                p_skelevent_click->processed = 1;
            }

            docu_area_init_hefo(&p_docu->anchor_mark.docu_area);

            object_position_set.action = OBJECT_POSITION_SET_START_WORD;
            p_hefo_block->p_data = &object_position_set;
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
            p_docu->anchor_mark.docu_area.tl.object_position = object_position_set.object_data.object_position_start;

            if(OBJECT_ID_NONE != p_docu->anchor_mark.docu_area.tl.object_position.object_id)
            {
                object_position_set.action = OBJECT_POSITION_SET_END_WORD;
                status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
                p_docu->anchor_mark.docu_area.br.object_position = object_position_set.object_data.object_position_start;
            }

            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_NEW, p_hefo_block));
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_FINISHED, p_hefo_block));
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_SELECTION_SHOW, p_hefo_block));
        }
        break;
        }

    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_CMD_SELECT_CELL:
    case T5_CMD_SELECT_DOCUMENT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;

        if(T5_EVENT_CLICK_LEFT_TRIPLE == t5_message)
        {
            P_SKELEVENT_CLICK p_skelevent_click = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_CLICK, p_hefo_block);
            p_skelevent_click->processed = 1;
        }

        docu_area_init_hefo(&p_docu->anchor_mark.docu_area);

        {
        OBJECT_POSITION_SET object_position_set;
        if(object_data_from_hefo_block_and_object_position(&object_position_set.object_data, p_hefo_block, P_OBJECT_POSITION_NONE))
        {
            object_position_set.action = OBJECT_POSITION_SET_START;
            p_hefo_block->p_data = &object_position_set;
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
            p_docu->anchor_mark.docu_area.tl.object_position = object_position_set.object_data.object_position_start;

            if(OBJECT_ID_NONE != p_docu->anchor_mark.docu_area.tl.object_position.object_id)
            {
                object_position_set.action = OBJECT_POSITION_SET_END;
                status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
                p_docu->anchor_mark.docu_area.br.object_position = object_position_set.object_data.object_position_start;
            }
        }
        } /*block*/

        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_NEW, p_hefo_block));
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_FINISHED, p_hefo_block));
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_SELECTION_SHOW, p_hefo_block));
        break;
        }

    case T5_CMD_SELECTION_CLEAR:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
        caret_show_claim(p_docu, p_hefo_block->event_focus, FALSE);
        break;
        }

    case T5_EVENT_CLICK_RIGHT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_SKELEVENT_CLICK p_skelevent_click = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_CLICK, p_hefo_block);
        OBJECT_POSITION_FIND object_position_find;
        MARK_INFO mark_info;

        p_skelevent_click->processed = 1;

        /* read current markers */
        CODE_ANALYSIS_ONLY(zero_struct(mark_info));
        p_hefo_block->p_data = &mark_info;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

        /* set up anchor start position -
         * use current position if no selection
         */
        if(!mark_info.h_markers)
        {
            docu_area_init_hefo(&p_docu->anchor_mark.docu_area);
            p_docu->anchor_mark.docu_area.tl.object_position = p_docu->hefo_position.object_position;
        }

        CODE_ANALYSIS_ONLY(zero_struct(object_position_find));
        p_hefo_block->p_data = &object_position_find;
        status_assert(position_set_from_skel_point_hefo(p_docu, p_hefo_block, &p_skelevent_click->skel_point));

        p_docu->anchor_mark.docu_area.br.object_position = object_position_find.object_data.object_position_start;

        if(!mark_info.h_markers)
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_NEW, p_hefo_block));

        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_UPDATE, p_hefo_block));

        /* either start drag or finish altogether */
        if(T5_EVENT_CLICK_RIGHT_SINGLE == t5_message)
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_ANCHOR_FINISHED, p_hefo_block));
        else
            host_drag_start(NULL);

        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_SELECTION_SHOW, p_hefo_block));
        break;
        }

    case T5_EVENT_CLICK_DRAG_ABORTED:
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
        break;

    case T5_EVENT_FILEINSERT_DOINSERT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_SKELEVENT_CLICK p_skelevent_click = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_CLICK, p_hefo_block);
        const OBJECT_ID object_id = object_id_from_t5_filetype(p_skelevent_click->data.fileinsert.t5_filetype);
        MSG_INSERT_FILE msg_insert_file;
        zero_struct(msg_insert_file);

        if(OBJECT_ID_NONE == object_id)
            return(create_error(ERR_UNKNOWN_FILETYPE));

        p_skelevent_click->processed = 1;

        /* no loading bodge - we're in the header/footer */

        cur_change_before(p_docu);

        msg_insert_file.filename = p_skelevent_click->data.fileinsert.filename;
        msg_insert_file.t5_filetype = p_skelevent_click->data.fileinsert.t5_filetype;
        msg_insert_file.scrap_file = !p_skelevent_click->data.fileinsert.safesource;
        msg_insert_file.insert = TRUE;
        msg_insert_file.ctrl_pressed = p_skelevent_click->click_context.ctrl_pressed;
        /***msg_insert_file.of_ip_format.flags.insert = 1;*/
        msg_insert_file.position = p_docu->cur;
        msg_insert_file.skel_point = p_skelevent_click->skel_point;
        status = object_call_id_load(p_docu, (OBJECT_ID_SKEL == object_id) ? T5_MSG_INSERT_OWNFORM : T5_MSG_INSERT_FOREIGN, &msg_insert_file, object_id);

        cur_change_after(p_docu);

        break;
        }

    case T5_EVENT_REDRAW:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        const P_SKELEVENT_REDRAW p_skelevent_redraw = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_REDRAW, p_hefo_block);
        OBJECT_REDRAW object_redraw;
        STYLE style;
        STYLE_SELECTOR selector;
        POSITION position;
        PIXIT_RECT pixit_rect;
        RECT_FLAGS rect_flags_clip;
        RECT_FLAGS_CLEAR(rect_flags_clip);

        p_hefo_block->p_data = &object_redraw;

        pixit_rect.tl = p_skelevent_redraw->clip_skel_rect.tl.pixit_point;
        pixit_rect.br = p_skelevent_redraw->clip_skel_rect.br.pixit_point;

        object_redraw.skel_rect_object = p_hefo_block->skel_rect_object;
        object_redraw.skel_rect_clip = p_skelevent_redraw->clip_skel_rect;
        object_redraw.skel_rect_work = p_skelevent_redraw->work_skel_rect;
        object_redraw.redraw_context = p_skelevent_redraw->redraw_context;

        object_redraw.pixit_rect_object.tl = object_redraw.skel_rect_object.tl.pixit_point;
        object_redraw.pixit_rect_object.br = object_redraw.skel_rect_object.br.pixit_point;

        object_redraw.flags.show_content = p_skelevent_redraw->flags.show_content;
        object_redraw.flags.show_selection = p_skelevent_redraw->flags.show_selection;

        void_style_selector_or(&selector, &style_selector_para_border, &style_selector_para_grid);
        style_selector_bit_set(&selector, STYLE_SW_PS_RGB_BACK);
        style_selector_bit_set(&selector, STYLE_SW_FS_COLOUR);

        style_init(&style);
        position_init_hefo(&position, 0);
        style_from_position(p_docu,
                            &style,
                            &selector,
                            &position,
                            p_hefo_block->p_h_style_list,
                            position_slr_in_docu_area,
                            FALSE);

        object_redraw.rgb_fore = style.font_spec.colour;
        object_redraw.rgb_back = style.para_style.rgb_back;
        style_dispose(&style);

        object_redraw.flags.marked_now = 0;
        object_redraw.flags.marked_screen = 0;

        if(data_refs_equal(&p_docu->mark_focus_hefo, &p_hefo_block->object_data.data_ref))
        {
            if(p_docu->mark_info_hefo.h_markers)
                object_redraw.flags.marked_now = 1;

            if(p_docu->mark_info_hefo.h_markers_screen)
                object_redraw.flags.marked_screen = 1;
        }

        object_redraw.object_data = p_hefo_block->object_data;

        object_background(p_docu, &object_redraw, &style);
        hefo_grid(p_docu, &object_redraw, &style, p_hefo_block->event_focus);
        object_border(p_docu, &object_redraw, &style);

        trace_2(TRACE_APP_SKEL_DRAW,
                TEXT("proc_hefo_common data_space: ") U32_TFMT TEXT(", row: ") ROW_TFMT,
                p_hefo_block->object_data.data_ref.data_space,
                p_hefo_block->object_data.data_ref.arg.row);

        if(!object_redraw.redraw_context.flags.printer)
        {
            rect_flags_clip.reduce_left_by_2  = 1;
            rect_flags_clip.reduce_up_by_2    = 1;
            rect_flags_clip.reduce_right_by_1 = 1;
            rect_flags_clip.reduce_down_by_1  = 1;
        }

        if(host_set_clip_rectangle2(&object_redraw.redraw_context,
                                    &pixit_rect,
                                    &object_redraw.pixit_rect_object,
                                    rect_flags_clip))
        {
            if(array_elements(p_hefo_block->p_h_data))
                status = object_call_HEFO_with_hb(p_docu, t5_message, p_hefo_block);
            else
            {   /* NULL object - nothing to paint, but invert if necessary */
                if(object_redraw.flags.show_selection)
                {
                    BOOL do_invert;

                    if(object_redraw.flags.show_content)

                        do_invert = object_redraw.flags.marked_now;
                    else
                        do_invert = (object_redraw.flags.marked_now != object_redraw.flags.marked_screen);

                    if(do_invert)
                        host_invert_rectangle_filled(&object_redraw.redraw_context,
                                                     &object_redraw.pixit_rect_object,
                                                     &object_redraw.rgb_fore,
                                                     &object_redraw.rgb_back);
                }
            }
        }

        break;
        }

    case T5_EVENT_KEYS:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_SKELEVENT_KEYS p_skelevent_keys = P_DATA_FROM_HEFO_BLOCK(SKELEVENT_KEYS, p_hefo_block);
        HEFO_KEYS hefo_keys;

        status_assert(hefo_selection_delete_auto(p_docu, p_hefo_block));

        p_hefo_block->p_data = &hefo_keys;
        hefo_keys.p_skelevent_keys = p_skelevent_keys;
        hefo_keys.object_position = p_docu->hefo_position.object_position;

        if(status_ok(status = object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_KEYS, p_hefo_block)))
        {
            p_docu->hefo_position.object_position = hefo_keys.object_position;
            hefo_object_data_init(p_docu, p_hefo_block, p_hefo_block->object_data.data_ref.data_space);
            hefo_caret_position_set_show(p_docu, p_hefo_block, TRUE);
        }

        break;
        }

    case T5_CMD_WORD_LEFT:
    case T5_CMD_CURSOR_LEFT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        hefo_cursor_left(p_docu, t5_message, p_hefo_block);
        break;
        }

    case T5_CMD_WORD_RIGHT:
    case T5_CMD_CURSOR_RIGHT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        hefo_cursor_right(p_docu, t5_message, p_hefo_block);
        break;
        }

    case T5_CMD_CURSOR_DOWN:
    case T5_CMD_CURSOR_UP:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        SKEL_POINT skel_point;

        if(status_ok(hefo_logical_move(p_docu,
                                       p_hefo_block,
                                       &skel_point,
                                       (t5_message == T5_CMD_CURSOR_DOWN)
                                            ? OBJECT_LOGICAL_MOVE_DOWN
                                            : OBJECT_LOGICAL_MOVE_UP)))
        {
            skel_point_update_from_edge(&skel_point, &p_docu->caret_x, x);
            hefo_caret_to_point(p_docu, p_hefo_block, &skel_point, TRUE);
        }

        break;
        }

    case T5_CMD_LINE_END:
    case T5_CMD_LINE_START:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        SKEL_POINT skel_point;

        if(status_ok(hefo_logical_move(p_docu,
                                       p_hefo_block,
                                       &skel_point,
                                       (t5_message == T5_CMD_LINE_START)
                                             ? OBJECT_LOGICAL_MOVE_LEFT
                                             : OBJECT_LOGICAL_MOVE_RIGHT)))
        {
            skel_point.pixit_point.y = p_docu->caret.pixit_point.y;
            skel_point.page_num.y = p_docu->caret.page_num.y;
            hefo_caret_to_point(p_docu, p_hefo_block, &skel_point, TRUE);
        }

        break;
        }

    case T5_CMD_DELETE_CHARACTER_RIGHT:
    case T5_CMD_DELETE_CHARACTER_LEFT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        OBJECT_DELETE_SUB object_delete_sub;

        if(object_data_from_hefo_block_and_object_position(&object_delete_sub.object_data, p_hefo_block, P_OBJECT_POSITION_NONE))
        {
            MARK_INFO mark_info;

            /* read current markers */
            CODE_ANALYSIS_ONLY(zero_struct(mark_info));
            p_hefo_block->p_data = &mark_info;
            status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

            if(!mark_info.h_markers)
            {
                OBJECT_POSITION_SET object_position_set;

                if(object_data_from_hefo_block_and_object_position(&object_position_set.object_data, p_hefo_block, P_OBJECT_POSITION_NONE))
                {
                    object_position_set.action = t5_message == T5_CMD_DELETE_CHARACTER_LEFT
                                                        ? OBJECT_POSITION_SET_BACK
                                                        : OBJECT_POSITION_SET_FORWARD;
                    p_hefo_block->p_data = &object_position_set;
                    status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
                }

                if(t5_message == T5_CMD_DELETE_CHARACTER_LEFT)
                {
                    object_delete_sub.object_data.object_position_end = p_docu->hefo_position.object_position;
                    object_delete_sub.object_data.object_position_start =
                    p_docu->hefo_position.object_position = object_position_set.object_data.object_position_start;
                }
                else
                {
                    object_delete_sub.object_data.object_position_start = p_docu->hefo_position.object_position;
                    object_delete_sub.object_data.object_position_end = object_position_set.object_data.object_position_start;
                }
            }
            else
            {
                P_DOCU_AREA p_docu_area = &array_ptr(&mark_info.h_markers, MARKERS, 0)->docu_area;
                object_delete_sub.object_data.object_position_start = p_docu_area->tl.object_position;
                object_delete_sub.object_data.object_position_end = p_docu_area->br.object_position;
                status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
            }

            object_delete_sub.save_data = 0;
            p_hefo_block->p_data = &object_delete_sub;
            status = object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_DELETE_SUB, p_hefo_block);
            hefo_caret_position_set_show(p_docu, p_hefo_block, FALSE);
        }
        break;
    }

    /* apply a style to a selection */
    case T5_CMD_STYLE_APPLY:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        const PC_T5_CMD p_t5_cmd = P_DATA_FROM_HEFO_BLOCK(T5_CMD, p_hefo_block);
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        PC_USTR_INLINE ustr_inline = p_args[0].val.ustr_inline;
        MARK_INFO mark_info;
        DOCU_AREA docu_area;
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

        /* read current markers */
        CODE_ANALYSIS_ONLY(zero_struct(mark_info));
        p_hefo_block->p_data = &mark_info;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

        if(mark_info.h_markers)
            docu_area = array_ptr(&mark_info.h_markers, MARKERS, 0)->docu_area;
        else
        {
            docu_area_init_hefo(&docu_area);
            docu_area.tl.object_position = p_docu->hefo_position.object_position;
            docu_area.br.object_position = p_docu->hefo_position.object_position;
        }

        STYLE_DOCU_AREA_ADD_INLINE(&style_docu_area_add_parm, ustr_inline);
        style_docu_area_add_parm.p_array_handle = p_hefo_block->p_h_style_list;
        style_docu_area_add_parm.data_space = p_hefo_block->object_data.data_ref.data_space;
        status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);
        break;
        }

    /* modify source of style if no selection */
    case T5_CMD_STYLE_APPLY_SOURCE:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        const PC_T5_CMD p_t5_cmd = P_DATA_FROM_HEFO_BLOCK(T5_CMD, p_hefo_block);
        MARK_INFO mark_info;

        /* read current markers */
        CODE_ANALYSIS_ONLY(zero_struct(mark_info));
        p_hefo_block->p_data = &mark_info;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

        /* if there's a selection - redirect to normal STYLE_APPLY */
        if(mark_info.h_markers)
        {
            p_hefo_block->p_data = de_const_cast(P_T5_CMD, p_t5_cmd);
            status = proc_event_hefo_common(p_docu, T5_CMD_STYLE_APPLY, p_hefo_block);
        }
        else
        {
            const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
            PC_USTR_INLINE ustr_inline = p_args[0].val.ustr_inline;
            POSITION position;
            position_init_hefo(&position, p_docu->hefo_position.object_position.data);
            style_effect_source_modify(p_docu, p_hefo_block->p_h_style_list, &position, ustr_inline, NULL);
        }

        break;
        }

    case T5_MSG_SELECTION_STYLE:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_SELECTION_STYLE p_selection_style = P_DATA_FROM_HEFO_BLOCK(SELECTION_STYLE, p_hefo_block);

        status = selection_style_hefo(p_docu,
                                      &p_selection_style->style_out,
                                      &p_selection_style->selector_fuzzy_out,
                                      &p_selection_style->selector_in,
                                      p_hefo_block);
        break;
        }

    case T5_CMD_BOX:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        const PC_T5_CMD p_t5_cmd = P_DATA_FROM_HEFO_BLOCK(T5_CMD, p_hefo_block);
        BOX_APPLY box_apply;
        box_apply.p_array_handle = p_hefo_block->p_h_style_list;
        box_apply.data_space = p_hefo_block->object_data.data_ref.data_space;
        docu_area_init_hefo(&box_apply.docu_area);
        box_apply.arglist_handle = p_t5_cmd->arglist_handle;
        status = object_skel(p_docu, T5_MSG_BOX_APPLY, &box_apply);
        break;
        }

    case T5_MSG_RULER_INFO:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        P_RULER_INFO p_ruler_info = P_DATA_FROM_HEFO_BLOCK(RULER_INFO, p_hefo_block);
        STYLE style;
        POSITION position;

        style_init(&style);

        position_init_hefo(&position, 0);
        style_from_position(p_docu,
                            &style,
                            &style_selector_para_ruler,
                            &position,
                            p_hefo_block->p_h_style_list,
                            position_slr_in_docu_area,
                            FALSE);

        p_ruler_info->valid = TRUE;

        p_ruler_info->col_info.col = 0;
        p_ruler_info->col_info.edge_left.page = p_hefo_block->skel_rect_work.tl.page_num.x;
        p_ruler_info->col_info.edge_left.pixit = 0;
        p_ruler_info->col_info.edge_right.page = p_ruler_info->col_info.edge_left.page;
        p_ruler_info->col_info.edge_right.pixit = style.col_style.width;
        p_ruler_info->col_info.margin_right = style.para_style.margin_right;
        p_ruler_info->col_info.margin_left = style.para_style.margin_left;
        p_ruler_info->col_info.margin_para = style.para_style.margin_para;
        p_ruler_info->col_info.h_tab_list = style.para_style.h_tab_list;
        array_init_block_setup(&p_ruler_info->col_info.tab_init_block, 1, sizeof32(TAB_INFO), TRUE);

        style_dispose(&style);
        break;
        }

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        OBJECT_SET_CASE object_set_case;
        MARK_INFO mark_info;

        /* read current markers */
        CODE_ANALYSIS_ONLY(zero_struct(mark_info));
        p_hefo_block->p_data = &mark_info;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

        object_set_case.object_data = p_hefo_block->object_data;
        object_set_case.do_redraw = TRUE;

        if(mark_info.h_markers)
        {
            P_DOCU_AREA p_docu_area = &array_ptr(&mark_info.h_markers, MARKERS, 0)->docu_area;
            object_set_case.object_data.object_position_start = p_docu_area->tl.object_position;
            object_set_case.object_data.object_position_end = p_docu_area->br.object_position;
        }
        else
        {
            OBJECT_POSITION_SET object_position_set;

            object_position_set.object_data = p_hefo_block->object_data;
            if(P_DATA_NONE != p_hefo_block->object_data.u.p_object)
            {
                object_position_set.action = OBJECT_POSITION_SET_FORWARD;
                p_hefo_block->p_data = &object_position_set;
                status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
                p_docu->hefo_position.object_position = object_set_case.object_data.object_position_end
                                                      = object_position_set.object_data.object_position_start;
            }
        }

        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_SELECTION_HIDE, p_hefo_block));
        p_hefo_block->p_data = &object_set_case;
        status_consume(object_call_HEFO_with_hb(p_docu, t5_message, p_hefo_block));
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_SELECTION_SHOW, p_hefo_block));
        hefo_caret_position_set_show(p_docu, p_hefo_block, FALSE);
        break;
        }

    case T5_CMD_WORD_COUNT:
        {
        P_HEFO_BLOCK p_hefo_block = (P_HEFO_BLOCK) p_data;
        MARK_INFO mark_info;

        /* read current markers */
        CODE_ANALYSIS_ONLY(zero_struct(mark_info));
        p_hefo_block->p_data = &mark_info;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

        if(mark_info.h_markers)
        {
            OBJECT_WORD_COUNT object_word_count;
            P_DOCU_AREA p_docu_area = &array_ptr(&mark_info.h_markers, MARKERS, 0)->docu_area;

            (void) object_data_from_hefo_block_and_object_position(&object_word_count.object_data,
                                                                   p_hefo_block,
                                                                   &p_docu_area->tl.object_position);
            object_word_count.object_data.object_position_end = p_docu_area->br.object_position;
            object_word_count.words_counted = 0;
            p_hefo_block->p_data = &object_word_count;
            status_consume(object_call_HEFO_with_hb(p_docu, t5_message, p_hefo_block));

            status_line_setf(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, (object_word_count.words_counted == 1) ? MSG_STATUS_WORD_COUNTED : MSG_STATUS_WORDS_COUNTED, object_word_count.words_counted );
        }
        break;
        }

    case T5_CMD_TOGGLE_MARKS:
        /* fire off appropriate command to focus owner, avoiding macro recording */
        return(docu_focus_owner_object_call(p_docu, (p_docu->mark_info_hefo.h_markers ? T5_CMD_SELECTION_CLEAR : T5_CMD_SELECT_DOCUMENT), p_data));
    }

    return(status);
}

/******************************************************************************
*
* given a page number, set up a hefo block
*
******************************************************************************/

_Check_return_ _Success_(status_done(return))
static STATUS
hefo_block_from_page_num(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_HEFO_BLOCK p_hefo_block,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_PAGE_NUM p_page_num,
    _InVal_     OBJECT_ID focus_owner)
{
    P_HEADFOOT_DEF p_headfoot_def;

    p_hefo_block->magic = HEFO_BLOCK_MAGIC;

    switch(focus_owner)
    {
    default: default_unhandled();
#if CHECKING
    case OBJECT_ID_HEADER:
#endif
        p_hefo_block->redraw_tag = UPDATE_PANE_MARGIN_HEADER;
        break;

    case OBJECT_ID_FOOTER:
        p_hefo_block->redraw_tag = UPDATE_PANE_MARGIN_FOOTER;
        break;
    }

    if(NULL == (p_headfoot_def = p_headfoot_def_from_page_y(p_docu, &p_hefo_block->object_data.data_ref.arg.row, p_page_num->y, focus_owner)))
    {
        caret_show_claim(p_docu, OBJECT_ID_SKEL, FALSE);
        return(STATUS_OK); /* i.e. NOT DONE */
    }

    p_hefo_block->p_h_data = &p_headfoot_def->headfoot.h_data;
    p_hefo_block->p_h_style_list = &p_headfoot_def->headfoot.h_style_list;
    p_hefo_block->event_focus = focus_owner;

    hefo_object_data_init(p_docu, p_hefo_block, p_headfoot_def->data_space);

    p_hefo_block->skel_rect_work.tl.page_num =
    p_hefo_block->skel_rect_work.br.page_num = *p_page_num;

    p_hefo_block->skel_rect_work.tl.pixit_point.x =
    p_hefo_block->skel_rect_work.tl.pixit_point.y = 0;

    page_limits_from_page(p_docu, &p_hefo_block->skel_rect_work.br.pixit_point, p_hefo_block->redraw_tag, p_page_num);
    p_hefo_block->skel_rect_work.br.pixit_point.x = p_docu->page_def.cells_usable_x;
    p_hefo_block->skel_rect_work.br.pixit_point.y -= p_docu->page_def.grid_size;

    p_hefo_block->skel_rect_object = p_hefo_block->skel_rect_work;
    p_hefo_block->skel_rect_object.tl.pixit_point.y += p_headfoot_def->headfoot_sizes.offset;

    {
    OBJECT_HOW_BIG object_how_big;
    object_how_big.object_data = p_hefo_block->object_data;
    object_how_big.skel_rect = p_hefo_block->skel_rect_object;
    p_hefo_block->p_data = &object_how_big;
    status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_HOW_BIG, p_hefo_block));
    p_hefo_block->skel_rect_object.br.pixit_point.y = MIN(object_how_big.skel_rect.br.pixit_point.y,
                                                          p_hefo_block->skel_rect_work.br.pixit_point.y);
    } /*block*/

    p_hefo_block->p_data = p_data;

    return(STATUS_DONE);
}

static void
hefo_caret_position_set_show(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _InVal_     BOOL scroll)
{
    CARET_SHOW_CLAIM caret_show_claim;
    caret_show_claim.focus = p_hefo_block->event_focus;
    caret_show_claim.scroll = scroll;
    p_hefo_block->p_data = &caret_show_claim;
    status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_CARET_SHOW_CLAIM, p_hefo_block));
}

/******************************************************************************
*
* move the caret to a given point
*
******************************************************************************/

static void
hefo_caret_to_point(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InVal_     BOOL caret_x_save)
{
    OBJECT_POSITION_FIND object_position_find;
    SKEL_EDGE skel_edge_caret_x;

    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    CODE_ANALYSIS_ONLY(zero_struct(object_position_find));
    p_hefo_block->p_data = &object_position_find;
    status_assert(position_set_from_skel_point_hefo(p_docu, p_hefo_block, p_skel_point));

    skel_edge_caret_x = p_docu->caret_x;

    /* set physical position */
    p_docu->hefo_position.data_ref = object_position_find.object_data.data_ref;
    p_docu->hefo_position.page_num = p_hefo_block->skel_rect_work.tl.page_num;
    p_docu->hefo_position.object_position = object_position_find.object_data.object_position_start;

    /* set caret parameters */
    caret_show_claim(p_docu, p_hefo_block->event_focus, TRUE);

    if(caret_x_save)
        p_docu->caret_x = skel_edge_caret_x;
}

/******************************************************************************
*
* move cursor left; message tells how much
*
******************************************************************************/

static void
hefo_cursor_left(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message /* cursor_ or word_left */,
    P_HEFO_BLOCK p_hefo_block)
{
    OBJECT_POSITION_SET object_position_set;

    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    if(object_data_from_hefo_block_and_object_position(&object_position_set.object_data, p_hefo_block, P_OBJECT_POSITION_NONE))
    {
        object_position_set.action =
            (OBJECT_ID_NONE != p_docu->hefo_position.object_position.object_id)
                ? (t5_message == T5_CMD_CURSOR_LEFT)
                        ? OBJECT_POSITION_SET_BACK
                        : OBJECT_POSITION_SET_PREV_WORD
                : OBJECT_POSITION_SET_END;

        p_hefo_block->p_data = &object_position_set;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
        p_docu->hefo_position.object_position = object_position_set.object_data.object_position_start;
    }

    hefo_caret_position_set_show(p_docu, p_hefo_block, TRUE);
}

/******************************************************************************
*
* move cursor right; message tells how much
*
******************************************************************************/

static void
hefo_cursor_right(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message /* cursor_ or word_right */,
    P_HEFO_BLOCK p_hefo_block)
{
    OBJECT_POSITION_SET object_position_set;

    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    if(object_data_from_hefo_block_and_object_position(&object_position_set.object_data, p_hefo_block, P_OBJECT_POSITION_NONE))
    {
        object_position_set.action =
            (OBJECT_ID_NONE != p_docu->hefo_position.object_position.object_id)
                 ? (t5_message == T5_CMD_CURSOR_RIGHT)
                        ? OBJECT_POSITION_SET_FORWARD
                        : OBJECT_POSITION_SET_NEXT_WORD
                 : OBJECT_POSITION_SET_START;

        p_hefo_block->p_data = &object_position_set;
        status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_SET, p_hefo_block));
        p_docu->hefo_position.object_position = object_position_set.object_data.object_position_start;
    }

    hefo_caret_position_set_show(p_docu, p_hefo_block, TRUE);
}

/******************************************************************************
*
* do a logical move
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
hefo_logical_move(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_HEFO_BLOCK p_hefo_block,
    _OutRef_    P_SKEL_POINT p_skel_point,
    _InVal_     enum OBJECT_LOGICAL_MOVE_ACTION action)
{
    STATUS status = STATUS_FAIL;

    if(array_elements(p_hefo_block->p_h_data))
    {
        OBJECT_LOGICAL_MOVE object_logical_move;
        zero_struct(object_logical_move);

        object_logical_move.object_data = p_hefo_block->object_data;
        object_logical_move.action = action;

        p_hefo_block->p_data = &object_logical_move;
        if(status_ok(status = object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_LOGICAL_MOVE, p_hefo_block)))
            *p_skel_point = object_logical_move.skel_point_out;
    }

    return(status);
}

/******************************************************************************
*
* delete selected text as a result of a key press operation;
* the routine objects if the selection covers more than one cell
*
******************************************************************************/

_Check_return_
static STATUS /* >0 if no op; no error */
hefo_selection_delete_auto(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block)
{
    MARK_INFO mark_info;

    /* read current markers */
    CODE_ANALYSIS_ONLY(zero_struct(mark_info));
    p_hefo_block->p_data = &mark_info;
    status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

    /* delete selection >>> should this be saved somewhere ?? */
    if(mark_info.h_markers)
    {
        OBJECT_DELETE_SUB object_delete_sub;
        P_DOCU_AREA p_docu_area = &array_ptr(&mark_info.h_markers, MARKERS, 0)->docu_area;

        if(object_data_from_hefo_block_and_object_position(&object_delete_sub.object_data,
                                                           p_hefo_block,
                                                           &p_docu_area->tl.object_position))
        {
            object_delete_sub.object_data.object_position_end = p_docu_area->br.object_position;
            object_delete_sub.save_data = 0;

            status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

            p_hefo_block->p_data = &object_delete_sub;
            return(object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_DELETE_SUB, p_hefo_block));
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* set object_position given skel_point in header/footer
*
******************************************************************************/

_Check_return_
static STATUS
position_set_from_skel_point_hefo(
    _DocuRef_   P_DOCU p_docu,
    P_HEFO_BLOCK p_hefo_block,
    _InRef_     PC_SKEL_POINT p_skel_point)
{
    STATUS status = STATUS_OK;
    P_OBJECT_POSITION_FIND p_object_position_find = P_DATA_FROM_HEFO_BLOCK(OBJECT_POSITION_FIND, p_hefo_block);

    p_object_position_find->object_data = p_hefo_block->object_data;
    p_object_position_find->skel_rect = p_hefo_block->skel_rect_work;

    if(!array_elements(p_hefo_block->p_h_data))
    {
        STYLE style;
        STYLE_SELECTOR selector;
        POSITION position;

        /* get caret height from style leading */
        style_selector_copy(&selector, &style_selector_para_leading);
        style_init(&style);

        position_init_hefo(&position, 0);
        style_from_position(p_docu,
                            &style,
                            &style_selector_para_text,
                            &position,
                            p_hefo_block->p_h_style_list,
                            position_slr_in_docu_area,
                            FALSE);

        p_object_position_find->caret_height = style_leading_from_style(&style, &style.font_spec, p_docu->flags.draft_mode);
        p_object_position_find->pos = p_hefo_block->skel_rect_work.tl;

        style_dispose(&style);
    }
    else
    {
        /* send message to object to get position */
        p_object_position_find->pos = *p_skel_point;

        status = object_call_HEFO_with_hb(p_docu, T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT, p_hefo_block);
    }

    return(status);
}

/******************************************************************************
*
* work out the style of a selection of text
*
******************************************************************************/

_Check_return_
static STATUS
selection_style_hefo(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _OutRef_    P_STYLE_SELECTOR p_style_selector_fuzzy,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_in /* effects to look at */,
    P_HEFO_BLOCK p_hefo_block)
{
    STATUS status = STATUS_OK;
    MARK_INFO mark_info;
    DOCU_AREA docu_area;
    STYLE_SELECTOR selector_char;
    STYLE_SELECTOR selector_para;
    BOOL some_para;
    ARRAY_HANDLE h_sub_changes;
    STYLE style;

    /* read current markers */
    CODE_ANALYSIS_ONLY(zero_struct(mark_info));
    p_hefo_block->p_data = &mark_info;
    status_consume(object_call_HEFO_with_hb(p_docu, T5_MSG_MARK_INFO_READ, p_hefo_block));

    if(mark_info.h_markers)
        docu_area = array_ptr(&mark_info.h_markers, MARKERS, 0)->docu_area;
    else
    {
        docu_area_init_hefo(&docu_area);
        docu_area.tl.object_position = p_docu->hefo_position.object_position;
        docu_area.br.object_position = p_docu->hefo_position.object_position;
    }

    /* split selector into character and paragraph granularity */
    void_style_selector_and(&selector_char, p_style_selector_in, &style_selector_font_spec);
    style_selector_bit_set(&selector_char, STYLE_SW_NAME);
    some_para = style_selector_bic(&selector_para, p_style_selector_in, &selector_char);

    style_init(&style);
    style_from_position(p_docu, &style, &selector_char, &docu_area.tl, p_hefo_block->p_h_style_list, position_in_docu_area, TRUE);
    if(some_para) /* SKS 18feb97 sometimes the above split leaves you with nothing in this one */
    style_from_position(p_docu, &style, &selector_para, &docu_area.tl, p_hefo_block->p_h_style_list, position_slr_in_docu_area, FALSE);

    style_copy_into(p_style, &style, p_style_selector_in);

    /* all things not defined at the start of the region are fuzzy */
    void_style_selector_not(p_style_selector_fuzzy, &p_style->selector);

    /* look only for changes in the defined effects */
    void_style_selector_and(&selector_para, &selector_para, &p_style->selector);
    void_style_selector_and(&selector_char, &selector_char, &p_style->selector);
    style_dispose(&style);

    {
    OBJECT_POSITION object_position;
    object_position_init(&object_position);
    object_position.object_id = OBJECT_ID_HEFO;
    if(style_selector_any(&selector_char)
       &&
       (h_sub_changes = style_sub_changes(p_docu,
                                          &selector_char,
                                          &p_hefo_block->object_data.data_ref,
                                          &object_position,
                                          p_hefo_block->p_h_style_list)) != 0)
    {
        const ARRAY_INDEX n_style_sub_changes = array_elements(&h_sub_changes);
        ARRAY_INDEX i;
        P_STYLE_SUB_CHANGE p_style_sub_change = array_range(&h_sub_changes, STYLE_SUB_CHANGE, 0, n_style_sub_changes);

        for(i = 0; i < n_style_sub_changes; ++i, ++p_style_sub_change)
        {
            if(position_in_docu_area(&docu_area, &p_style_sub_change->position))
            {
                STYLE_SELECTOR style_selector_diff;

                consume_bool(style_compare(&style_selector_diff, p_style, &p_style_sub_change->style));

                /* mask with the bits we're looking for */
                if(style_selector_and(&style_selector_diff, &style_selector_diff, &selector_char))
                {
                    /* the bits that changed are now fuzzy */
                    void_style_selector_or(p_style_selector_fuzzy, p_style_selector_fuzzy, &style_selector_diff);

                    /* not looking for these fuzzy bits any more */
                    void_style_selector_bic(&selector_char, &selector_char, &style_selector_diff);
                }
            }
        }
    }
    } /*block*/

    return(status);
}

/* end of ob_hefo.c */
