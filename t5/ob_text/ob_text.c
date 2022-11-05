/* ob_text.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Text object module for Fireworkz */

/* MRJC December 1991 */

#include "common/gflags.h"

#include "ob_story/ob_story.h"

#include "ob_hefo/xp_hefo.h"

#include "ob_text/ob_text.h"

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_TEXT)
extern PC_U8 rb_text_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_TEXT &rb_text_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_TEXT DONT_LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_TEXT DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_TEXT DONT_LOAD_RESOURCES

/*
internal routines
*/

/*
construct table
*/

static const ARG_TYPE
text_args_s32[] =
{
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
text_args_ustr[] =
{
    ARG_TYPE_USTR,
    ARG_TYPE_NONE
};

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "ITT",                    NULL,                       (T5_MESSAGE) IL_TAB,                        { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITP",                    text_args_ustr,             (T5_MESSAGE) IL_PAGE_Y,                     { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITPX",                   text_args_ustr,             (T5_MESSAGE) IL_PAGE_X,                     { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITD",                    text_args_ustr,             (T5_MESSAGE) IL_DATE,                       { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITFD",                   text_args_ustr,             (T5_MESSAGE) IL_FILE_DATE,                  { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITWN",                   NULL,                       (T5_MESSAGE) IL_WHOLENAME,                  { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITLN",                   NULL,                       (T5_MESSAGE) IL_LEAFNAME,                   { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITSH",                   NULL,                       (T5_MESSAGE) IL_SOFT_HYPHEN,                { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITR",                    NULL,                       (T5_MESSAGE) IL_RETURN,                     { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITF",                    text_args_s32,              (T5_MESSAGE) IL_MS_FIELD,                   { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ITSSN",                  text_args_ustr,             (T5_MESSAGE) IL_SS_NAME,                    { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};


/******************************************************************************
*
* ss_name dependency handling
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_SS_NAME_RECORD
p_ss_name_record_from_client_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     CLIENT_HANDLE client_handle)
{
    P_SS_NAME_RECORD p_ss_name_record_out = NULL;
    P_TEXT_INSTANCE_DATA p_text_instance = p_object_instance_data_TEXT(p_docu);
    const ARRAY_INDEX n_records = array_elements(&p_text_instance->h_ss_name_record);
    ARRAY_INDEX i;

    for(i = 0; i < n_records; i++)
    {
        const P_SS_NAME_RECORD p_ss_name_record = array_ptr(&p_text_instance->h_ss_name_record, SS_NAME_RECORD, i);

        if(p_ss_name_record->is_deleted)
            continue;

        if(client_handle == p_ss_name_record->client_handle)
        {
            p_ss_name_record_out = p_ss_name_record;
            break;
        }
    }

    return(p_ss_name_record_out);
}

PROC_ELEMENT_IS_DELETED_PROTO(static, ss_name_record_is_deleted)
{
    const P_SS_NAME_RECORD p_ss_name_record = (P_SS_NAME_RECORD) p_any;

    return(p_ss_name_record->is_deleted);
}

static void
ss_name_record_delete(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SS_NAME_RECORD p_ss_name_record)
{
    uref_del_dependency(docno_from_p_docu(p_docu), p_ss_name_record->uref_handle);

    p_ss_name_record->is_deleted = 1;
}

/******************************************************************************
*
* handle uref events for ss_names
*
******************************************************************************/

PROC_UREF_EVENT_PROTO(static, text_ss_uref_event_dep_delete)
{
    /* dependency must be deleted */
    /* free a region */
    const P_SS_NAME_RECORD p_ss_name_record = p_ss_name_record_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

    UNREFERENCED_PARAMETER_InVal_(uref_message);

    PTR_ASSERT(p_ss_name_record);
    if(NULL != p_ss_name_record)
        ss_name_record_delete(p_docu, p_ss_name_record);

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, text_ss_uref_event_dep_update)
{
    /* dependency region must be updated */
    /* find our entry */
    const P_SS_NAME_RECORD p_ss_name_record = p_ss_name_record_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

    PTR_ASSERT(p_ss_name_record);
    if(NULL != p_ss_name_record)
        if(Uref_Dep_Delete == uref_match_slr(&p_ss_name_record->slr, uref_message, p_uref_event_block))
            ss_name_record_delete(p_docu, p_ss_name_record);

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, text_ss_uref_event)
{
    switch(UBF_UNPACK(UREF_COMMS, p_uref_event_block->reason.code))
    {
    case Uref_Dep_Delete: /* dependency must be deleted */
        return(text_ss_uref_event_dep_delete(p_docu, uref_message, p_uref_event_block));

    case Uref_Dep_Update: /* dependency region must be updated */
    case Uref_Dep_Inform:
        return(text_ss_uref_event_dep_update(p_docu, uref_message, p_uref_event_block));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

static S32 next_ss_client_handle = 1;

_Check_return_
_Ret_maybenull_
static P_SS_NAME_RECORD
ss_name_record_find(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     EV_HANDLE ev_handle)
{
    P_SS_NAME_RECORD p_ss_name_record_out = NULL;
    P_TEXT_INSTANCE_DATA p_text_instance = p_object_instance_data_TEXT(p_docu);
    const ARRAY_INDEX n_records = array_elements(&p_text_instance->h_ss_name_record);
    ARRAY_INDEX i;

    for(i = 0; i < n_records; i++)
    {
        const P_SS_NAME_RECORD p_ss_name_record = array_ptr(&p_text_instance->h_ss_name_record, SS_NAME_RECORD, i);

        if(p_ss_name_record->is_deleted)
            continue;

        if(p_ss_name_record->ev_handle == ev_handle)
        {
            p_ss_name_record_out = p_ss_name_record;
            break;
        }
    }

    return(p_ss_name_record_out);
}

_Check_return_
static STATUS
ss_name_record_add(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     EV_HANDLE ev_handle,
    _InRef_     PC_SLR p_slr)
{
    STATUS status = STATUS_OK;
    P_TEXT_INSTANCE_DATA p_text_instance = p_object_instance_data_TEXT(p_docu);
    P_SS_NAME_RECORD p_ss_name_record;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(SS_NAME_RECORD), TRUE);

    if(NULL != (p_ss_name_record = al_array_extend_by(&p_text_instance->h_ss_name_record, SS_NAME_RECORD, 1, &array_init_block, &status)))
    {
        REGION region;

        region_from_two_slrs(&region, p_slr, p_slr, TRUE /* add_one_to_br */);

        if(status_ok(status = uref_add_dependency(p_docu, &region, text_ss_uref_event, next_ss_client_handle, &p_ss_name_record->uref_handle, FALSE)))
        {
            p_ss_name_record->ev_handle = ev_handle;
            p_ss_name_record->slr = *p_slr;
            p_ss_name_record->client_handle = next_ss_client_handle++;
        }
    }

    return(status);
}

/******************************************************************************
*
* scan the inline string and add ss_name dependencies
*
******************************************************************************/

static void
text_ss_name_scan_delete(
    _DocuRef_   P_DOCU p_docu,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n)
{
    U32 offset = 0;

    while(offset < uchars_n)
    {
        const U32 remain = uchars_n - offset;
        PC_UCHARS_INLINE next_uchars_inline = next_inline_off(PC_UCHARS_INLINE, uchars_inline, offset, remain);

        if(NULL == next_uchars_inline)
            break;

        offset = PtrDiffBytesS32(next_uchars_inline, uchars_inline);

        assert(is_inline_off(uchars_inline, offset));
        assert(inline_bytecount_off(uchars_inline, offset));

        if(IL_SS_NAME == inline_code_off(uchars_inline, offset))
        {
            EV_HANDLE ev_handle_name = (EV_HANDLE) data_from_inline_s32(uchars_inline_AddBytes(uchars_inline, offset));
            const P_SS_NAME_RECORD p_ss_name_record = ss_name_record_find(p_docu, ev_handle_name);
            if(NULL != p_ss_name_record)
                ss_name_record_delete(p_docu, p_ss_name_record);
        }

        offset += inline_bytecount_off(uchars_inline, offset);
    }

    /* NO assert(offset == uchars_n); */
}

/******************************************************************************
*
* scan the inline string and add ss_name dependencies
*
******************************************************************************/

_Check_return_
static STATUS
text_ss_name_scan_add(
    _DocuRef_   P_DOCU p_docu,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n,
    _InRef_     PC_SLR p_slr)
{
    U32 offset = 0;

    while(offset < uchars_n)
    {
        const U32 remain = uchars_n - offset;
        PC_UCHARS_INLINE next_uchars_inline = next_inline_off(PC_UCHARS_INLINE, uchars_inline, offset, remain);

        if(NULL == next_uchars_inline)
            break;

        offset = PtrDiffBytesS32(next_uchars_inline, uchars_inline);

        assert(is_inline_off(uchars_inline, offset));
        assert(inline_bytecount_off(uchars_inline, offset));

        if(IL_SS_NAME == inline_code_off(uchars_inline, offset))
        {
            EV_HANDLE ev_handle_name = (EV_HANDLE) data_from_inline_s32(uchars_inline_AddBytes(uchars_inline, offset));
            status_return(ss_name_record_add(p_docu, ev_handle_name, p_slr));
        }

        offset += inline_bytecount_off(uchars_inline, offset);
    }

    /* NO assert(offset == n_bytes); */

    return(STATUS_OK);
}

/* --------------------------------------------------------------------------------------------------- */

/*
* set up a text message block
*/

static void
text_message_block_init(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_MESSAGE_BLOCK p_text_message_block,
    /*_Inout_*/ P_ANY p_data,
    _InRef_opt_ PC_SKEL_RECT p_skel_rect,
    P_OBJECT_DATA p_object_data)
{
    p_text_message_block->p_data = p_data;

    p_text_message_block->inline_object.object_data = *p_object_data;

    if((P_DATA_NONE != p_object_data->u.p_object) && (OBJECT_ID_TEXT == p_object_data->object_id))
        p_text_message_block->inline_object.inline_len = ustr_inline_strlen(p_object_data->u.ustr_inline);
    else
        p_text_message_block->inline_object.inline_len = 0;

    style_init(&p_text_message_block->text_format_info.style_text_global);
    style_from_slr(p_docu,
                   &p_text_message_block->text_format_info.style_text_global,
                   &style_selector_para_text,
                   &p_object_data->data_ref.arg.slr);

    p_text_message_block->text_format_info.h_style_list = p_docu->h_style_docu_area;
    p_text_message_block->text_format_info.paginate = 1;

    p_text_message_block->text_format_info.text_area_border_y = 2 * p_docu->page_def.grid_size;

    p_text_message_block->text_format_info.skel_rect_work.tl.page_num.x =
    p_text_message_block->text_format_info.skel_rect_work.tl.page_num.y = 0;
    p_text_message_block->text_format_info.skel_rect_work.br.page_num = p_text_message_block->text_format_info.skel_rect_work.tl.page_num;

    p_text_message_block->text_format_info.skel_rect_work.tl.pixit_point.x =
    p_text_message_block->text_format_info.skel_rect_work.tl.pixit_point.y = 0;
    p_text_message_block->text_format_info.skel_rect_work.br.pixit_point.x = p_docu->page_def.cells_usable_x
                                                          + 2 * p_docu->page_def.grid_size;
    p_text_message_block->text_format_info.skel_rect_work.br.pixit_point.y = page_height_from_row(p_docu, p_object_data->data_ref.arg.slr.row)
                                                          + p_text_message_block->text_format_info.text_area_border_y;

    if(row_is_visible(p_docu, p_object_data->data_ref.arg.slr.row))
        p_text_message_block->text_format_info.object_visible = 1;
    else
        p_text_message_block->text_format_info.object_visible = 0;

    if(NULL != p_skel_rect)
        p_text_message_block->text_format_info.skel_rect_object = *p_skel_rect;
    else if(p_text_message_block->text_format_info.object_visible)
        skel_rect_from_slr(p_docu, &p_text_message_block->text_format_info.skel_rect_object, &p_object_data->data_ref.arg.slr);
    else
        /* at least initialise it */
        zero_struct_fn(p_text_message_block->text_format_info.skel_rect_object);
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
    _InVal_     BOOL do_position_update)
{
    p_text_inline_redisplay->redraw_tag = UPDATE_PANE_CELLS_AREA;
    p_text_inline_redisplay->p_quick_ublock = p_quick_ublock;
    p_text_inline_redisplay->do_redraw = do_redraw;
    p_text_inline_redisplay->redraw_done = FALSE;
    p_text_inline_redisplay->size_changed = TRUE;
    p_text_inline_redisplay->do_position_update = do_position_update;
    p_text_inline_redisplay->skel_rect_para_after = *p_skel_rect;
    p_text_inline_redisplay->skel_rect_para_before = *p_skel_rect;
    p_text_inline_redisplay->skel_rect_text_before = *p_skel_rect;
}

/******************************************************************************
*
* text delete with redisplay
*
******************************************************************************/

_Check_return_
static STATUS
text_delete_sub_redisplay(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_data_del,
    P_OBJECT_DATA p_object_data,
    _InVal_     BOOL save_data)
{
    STATUS status = STATUS_OK;
    TEXT_MESSAGE_BLOCK text_message_block;
    TEXT_INLINE_REDISPLAY text_inline_redisplay;
    S32 start, end;

    text_message_block_init(p_docu,
                            &text_message_block,
                            &text_inline_redisplay,
                            NULL,
                            p_object_data);

    text_inline_redisplay_init(&text_inline_redisplay,
                               P_QUICK_UBLOCK_NONE,
                               &text_message_block.text_format_info.skel_rect_object,
                               TRUE,
                               TRUE);

    offsets_from_object_data(&start, &end, &text_message_block.inline_object.object_data, text_message_block.inline_object.inline_len);

    if(save_data)
        *p_h_data_del = 0;

    {
    S32 del_size = end - start;

    if(del_size)
    {
        OF_TEMPLATE of_template = { 0 };
        DOCU_AREA docu_area;
        P_UCHARS_INLINE p_del_s;

        PTR_ASSERT(text_message_block.inline_object.object_data.u.p_object);
        p_del_s = uchars_inline_AddBytes_wr(text_message_block.inline_object.object_data.u.ustr_inline, start);

        docu_area_from_object_data(&docu_area, p_object_data);
        text_ss_name_scan_delete(p_docu, p_del_s, del_size);

        of_template.data_class = DATA_SAVE_CHARACTER;

        if(save_data)
            status = save_ownform_to_array_from_docu_area(p_docu, p_h_data_del, &of_template, &docu_area);

        if(status_ok(status))
            status = object_call_STORY_with_tmb(p_docu, T5_MSG_DELETE_INLINE_REDISPLAY, &text_message_block);

        if(status_ok(status))
        {
            status = object_realloc(p_docu,
                                    &text_message_block.inline_object.object_data.u.p_object,
                                    &text_message_block.inline_object.object_data.data_ref.arg.slr,
                                    OBJECT_ID_TEXT,
                                    /* don't create object with just terminator byte */
                                    text_message_block.inline_object.inline_len
                                         ? text_message_block.inline_object.inline_len + 1
                                         : 0);

            /* send back new object pointer */
            p_object_data->u.p_object = text_message_block.inline_object.object_data.u.p_object;
        }

        if(status_ok(status))
            status = format_object_size_set(p_docu,
                                            &text_inline_redisplay.skel_rect_para_after,
                                            &text_inline_redisplay.skel_rect_para_before,
                                            &text_message_block.inline_object.object_data.data_ref.arg.slr,
                                            text_inline_redisplay.redraw_done);

    }
    } /*block*/

    return(status);
}

/******************************************************************************
*
* text insert with redisplay
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
text_insert_sub_redisplay(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_POSITION p_object_position_after,
    P_OBJECT_DATA p_object_data,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     BOOL please_redraw,
    _InVal_     BOOL do_position_update)
{
    STATUS status = STATUS_OK;

    { /* do actual insert operation */
    S32 ins_size = quick_ublock_bytes(p_quick_ublock);

    if(ins_size)
    {
        TEXT_MESSAGE_BLOCK text_message_block;
        TEXT_INLINE_REDISPLAY text_inline_redisplay;

        text_message_block_init(p_docu,
                                &text_message_block,
                                &text_inline_redisplay,
                                NULL,
                                p_object_data);

        text_inline_redisplay_init(&text_inline_redisplay,
                                   p_quick_ublock,
                                   &text_message_block.text_format_info.skel_rect_object,
                                   please_redraw,
                                   do_position_update);

        if(status_ok(status))
            status = object_realloc(p_docu,
                                    &text_message_block.inline_object.object_data.u.p_object,
                                    &text_message_block.inline_object.object_data.data_ref.arg.slr,
                                    OBJECT_ID_TEXT,
                                    text_message_block.inline_object.inline_len + ins_size + 1);

        if(status_ok(status))
        {
            text_message_block.inline_object.object_data.object_id = OBJECT_ID_TEXT;

            /* reload style info */
            style_init(&text_message_block.text_format_info.style_text_global);
            style_from_slr(p_docu,
                           &text_message_block.text_format_info.style_text_global,
                           &style_selector_para_text,
                           &text_message_block.inline_object.object_data.data_ref.arg.slr);

            status = object_call_STORY_with_tmb(p_docu, T5_MSG_INSERT_INLINE_REDISPLAY, &text_message_block);
        }

        if(status_ok(status))
            status = text_ss_name_scan_add(p_docu, quick_ublock_ustr_inline(p_quick_ublock), ins_size, &text_message_block.inline_object.object_data.data_ref.arg.slr);

        if(status_ok(status))
            status = format_object_size_set(p_docu,
                                            &text_inline_redisplay.skel_rect_para_after,
                                            &text_inline_redisplay.skel_rect_para_before,
                                            &text_message_block.inline_object.object_data.data_ref.arg.slr,
                                            text_inline_redisplay.redraw_done);

        if(status_ok(status))
            *p_object_position_after = text_message_block.inline_object.object_data.object_position_end;
    }
    else
        *p_object_position_after = p_object_data->object_position_start; /* SKS 13jun95 after 1.22 */
        /* ^^^ fixes problem whereby replacing a string by nothing missed subsequent occurences in the same cell */
        /* (this was due to *p_object_position_after being left random) */
    } /*block*/

    return(status);
}

/* process these messages using our helper object via TEXT_MESSAGE_BLOCK */

T5_MSG_PROTO(static, text_event_redraw, P_OBJECT_REDRAW p_object_redraw)
{
    if(P_DATA_NONE == p_object_redraw->object_data.u.p_object)
    {
        /* NULL object - nothing to paint, but invert if necessary */
        if(p_object_redraw->flags.show_selection)
        {
            BOOL do_invert;

            if(p_object_redraw->flags.show_content)
                do_invert = p_object_redraw->flags.marked_now;
            else
                do_invert = (p_object_redraw->flags.marked_now != p_object_redraw->flags.marked_screen);

            if(do_invert)
                host_invert_rectangle_filled(&p_object_redraw->redraw_context,
                                             &p_object_redraw->pixit_rect_object,
                                             &p_object_redraw->rgb_fore,
                                             &p_object_redraw->rgb_back);
        }

        return(STATUS_OK);
    }

    {
    TEXT_MESSAGE_BLOCK text_message_block;

    text_message_block_init(p_docu,
                            &text_message_block,
                            p_object_redraw,
                            &p_object_redraw->skel_rect_object,
                            &p_object_redraw->object_data);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
    } /*block*/
}

T5_MSG_PROTO(static, text_msg_object_how_big, _InoutRef_ P_OBJECT_HOW_BIG p_object_how_big)
{
    TEXT_MESSAGE_BLOCK text_message_block;

    text_message_block_init(p_docu,
                            &text_message_block,
                            p_object_how_big,
                            &p_object_how_big->skel_rect,
                            &p_object_how_big->object_data);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
}

T5_MSG_PROTO(static, text_msg_object_how_wide, _InoutRef_ P_OBJECT_HOW_WIDE p_object_how_wide)
{
    TEXT_MESSAGE_BLOCK text_message_block;
    SKEL_RECT skel_rect;

    zero_struct_fn(skel_rect);
    skel_rect.br.pixit_point.x = p_docu->page_def.cells_usable_x;
    skel_rect.br.pixit_point.y = p_docu->page_def.cells_usable_y;

    text_message_block_init(p_docu,
                            &text_message_block,
                            p_object_how_wide,
                            &skel_rect,
                            &p_object_how_wide->object_data);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
}

T5_MSG_PROTO(static, text_msg_object_position_find, _InoutRef_ P_OBJECT_POSITION_FIND p_object_position_find)
{
    TEXT_MESSAGE_BLOCK text_message_block;

    text_message_block_init(p_docu,
                            &text_message_block,
                            p_object_position_find,
                            &p_object_position_find->skel_rect,
                            &p_object_position_find->object_data);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
}

T5_MSG_PROTO(static, text_msg_object_logical_move, _InoutRef_ P_OBJECT_LOGICAL_MOVE p_object_logical_move)
{
    TEXT_MESSAGE_BLOCK text_message_block;

    text_message_block_init(p_docu,
                            &text_message_block,
                            p_object_logical_move,
                            NULL,
                            &p_object_logical_move->object_data);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
}

T5_MSG_PROTO(static, text_cmd_setc, P_OBJECT_SET_CASE p_object_set_case)
{
    STATUS status;
    TEXT_MESSAGE_BLOCK text_message_block;
    TEXT_INLINE_REDISPLAY text_inline_redisplay;

    text_message_block_init(p_docu,
                            &text_message_block,
                            &text_inline_redisplay,
                            NULL,
                            &p_object_set_case->object_data);

    text_inline_redisplay_init(&text_inline_redisplay,
                               P_QUICK_UBLOCK_NONE,
                               &text_message_block.text_format_info.skel_rect_object,
                               p_object_set_case->do_redraw,
                               TRUE);

    status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block);

    if(status_ok(status))
        status = format_object_size_set(p_docu,
                                        &text_inline_redisplay.skel_rect_para_after,
                                        &text_inline_redisplay.skel_rect_para_before,
                                        &p_object_set_case->object_data.data_ref.arg.slr,
                                        text_inline_redisplay.redraw_done);

    return(status);
}

T5_MSG_PROTO(static, text_msg_read_text_draft, P_OBJECT_READ_TEXT_DRAFT p_object_read_text_draft)
{
    TEXT_MESSAGE_BLOCK text_message_block;

    text_message_block_init(p_docu,
                            &text_message_block,
                            p_object_read_text_draft,
                            &p_object_read_text_draft->skel_rect_object,
                            &p_object_read_text_draft->object_data);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
}

T5_MSG_PROTO(static, text_msg_tab_wanted, P_TAB_WANTED p_tab_wanted)
{
    TEXT_MESSAGE_BLOCK text_message_block;

    text_message_block_init(p_docu,
                            &text_message_block,
                            p_tab_wanted,
                            NULL,
                            &p_tab_wanted->object_data);

    return(object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block));
}

/*
main events
*/

_Check_return_
static STATUS
ob_text_msg_docu_colrow(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_old)
{
    /* generate overwrite message for altered text cells */
    if(p_docu->modified_text != p_docu->modified)
    {
        OBJECT_DATA object_data;

        style_docu_area_uref_hold(p_docu, &p_docu->h_style_docu_area, p_slr_old);

        if((STATUS_DONE != object_data_from_slr(p_docu, &object_data, p_slr_old))
           ||
           (OBJECT_ID_TEXT == object_data.object_id))
        {
            UREF_PARMS uref_parms;
            region_from_two_slrs(&uref_parms.source.region, p_slr_old, p_slr_old, TRUE);
            uref_event(p_docu, Uref_Msg_Overwrite, &uref_parms);
        }

        p_docu->modified_text = p_docu->modified;

        style_docu_area_uref_release(p_docu, &p_docu->h_style_docu_area, p_slr_old, OBJECT_ID_TEXT);
    }

    return(STATUS_OK);
}

MAEVE_EVENT_PROTO(static, maeve_event_ob_text)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_DOCU_COLROW:
        return(ob_text_msg_docu_colrow(p_docu, (PC_SLR) p_data));

    default:
        return(STATUS_OK);
    }
}

/*
object services hook
*/

static void
ob_text_maeve_services_ss_name_change(
    P_SS_NAME_CHANGE p_ss_name_change)
{
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno_valid(docno);
        const P_SS_NAME_RECORD p_ss_name_record = ss_name_record_find(p_docu, p_ss_name_change->ev_handle);

        if(NULL != p_ss_name_record)
        {
            if(p_ss_name_change->name_changed)
            {
                /* do reformat when name changes */
                DOCU_REFORMAT docu_reformat;
                docu_reformat.action = REFORMAT_Y;
                docu_reformat.data_type = REFORMAT_SLR;
                docu_reformat.data_space = DATA_SLOT;
                docu_reformat.data.slr = p_ss_name_record->slr;
                status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
            }

            p_ss_name_change->found_use = 1;
        }
    }
}

MAEVE_SERVICES_EVENT_PROTO(static, maeve_services_event_ob_text)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_SS_NAME_CHANGE:
        ob_text_maeve_services_ss_name_change((P_SS_NAME_CHANGE) p_data);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* text object event handler
*
******************************************************************************/

_Check_return_
static STATUS
ob_text_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    status_return(object_instance_data_alloc(p_docu, OBJECT_ID_TEXT, sizeof32(TEXT_INSTANCE_DATA)));

    return(maeve_event_handler_add(p_docu, maeve_event_ob_text, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
ob_text_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_ob_text, (CLIENT_HANDLE) 0);

    {
    const P_TEXT_INSTANCE_DATA p_text_instance = p_object_instance_data_TEXT(p_docu);

    /* garbage collect the ss_name list */
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.remove_deleted = 1;
    al_garbage_flags.shrink = 1;
    al_garbage_flags.may_dispose = 1; /* hopefully does so */
    consume(S32, al_array_garbage_collect(&p_text_instance->h_ss_name_record, 0, ss_name_record_is_deleted, al_garbage_flags));

    assert(0 == p_text_instance->h_ss_name_record);
    } /*block*/

    return(STATUS_OK);
}

T5_MSG_PROTO(static, text_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_TEXT, P_BOUND_MESSAGES_OBJECT_ID_TEXT, P_BOUND_RESOURCES_OBJECT_ID_TEXT));

        status_return(maeve_services_event_handler_add(maeve_services_event_ob_text));

        return(register_object_construct_table(OBJECT_ID_TEXT, object_construct_table, TRUE));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_TEXT));

    case T5_MSG_IC__INIT1:
        return(ob_text_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(ob_text_msg_close1(p_docu));

    case T5_MSG_IC__CLOSE2:
        object_instance_data_dispose(p_docu, OBJECT_ID_TEXT);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

/* turn lots of keys pressed message into insert_sub message */

T5_MSG_PROTO(static, text_msg_object_keys, _InoutRef_ P_OBJECT_KEYS p_object_keys)
{
    STATUS status =
        text_insert_sub_redisplay(p_docu,
                                  &p_object_keys->object_data.object_position_end,
                                  &p_object_keys->object_data,
                                  p_object_keys->p_skelevent_keys->p_quick_ublock,
                                  TRUE, TRUE);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_object_keys->p_skelevent_keys->processed = 1;

    return(status);
}

T5_MSG_PROTO(static, text_msg_object_delete_sub, P_OBJECT_DELETE_SUB p_object_delete_sub)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(
        text_delete_sub_redisplay(p_docu,
                                  &p_object_delete_sub->h_data_del,
                                  &p_object_delete_sub->object_data,
                                  p_object_delete_sub->save_data));
}

T5_MSG_PROTO(static, text_msg_object_string_replace, P_OBJECT_STRING_REPLACE p_object_string_replace)
{
    STATUS status = STATUS_OK;
    P_USTR_INLINE ustr_inline = p_object_string_replace->object_data.u.ustr_inline;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != ustr_inline)
    {
        S32 start, end, size;
        S32 case_1, case_2;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 128);
        quick_ublock_with_buffer_setup(quick_ublock);

        size = ustr_inline_strlen(ustr_inline);

        offsets_from_object_data(&start, &end, &p_object_string_replace->object_data, size);

        /* read case of existing string */
        case_1 = case_2 = -1;

        if(p_object_string_replace->copy_capitals)
        {
            U32 bytes_of_char_start = 0;

            if((end - start) != 0)
            {
                UCS4 ucs4 = ustr_char_decode_off((PC_USTR) ustr_inline, start, /*ref*/bytes_of_char_start);
                case_1 = t5_ucs4_is_uppercase(ucs4);
            }

            if((end - start) > (S32) bytes_of_char_start)
            {
                UCS4 ucs4 = ustr_char_decode_off_NULL((PC_USTR) ustr_inline, start + bytes_of_char_start);
                case_2 = t5_ucs4_is_uppercase(ucs4);
            }
        }

        status_assert(text_delete_sub_redisplay(p_docu,
                                                NULL,
                                                &p_object_string_replace->object_data,
                                                FALSE));

        /* convert replace data */
        status_assert(ustr_inline_replace_convert(&quick_ublock,
                                                  p_object_string_replace->p_quick_ublock,
                                                  case_1,
                                                  case_2,
                                                  p_object_string_replace->copy_capitals));

        /* insert replace data */
        status = text_insert_sub_redisplay(p_docu,
                                           &p_object_string_replace->object_position_after,
                                           &p_object_string_replace->object_data,
                                           &quick_ublock,
                                           TRUE, TRUE);

        quick_ublock_dispose(&quick_ublock);
    }

    return(status);
}

T5_MSG_PROTO(static, text_msg_object_copy, P_OBJECT_COPY p_object_copy)
{
    STATUS status = STATUS_OK;
    PC_USTR_INLINE p_from = (PC_USTR_INLINE) p_object_from_slr(p_docu, &p_object_copy->slr_from, OBJECT_ID_TEXT);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_from) /* SKS after 1.20/50 14mar95 this is now a valid case since caller no longer checks. Stops Taiwanese fill down */
    {
        const U32 text_size = ustr_inline_strlen32p1(p_from); /*CH_NULL*/
        P_ANY p_to;

        if(status_ok(status = object_realloc(p_docu, &p_to, &p_object_copy->slr_to, OBJECT_ID_TEXT, text_size)))
        {
            p_from = (PC_USTR_INLINE) p_object_from_slr(p_docu, &p_object_copy->slr_from, OBJECT_ID_TEXT); /* reload */
            PTR_ASSERT(p_from);
            memcpy32(p_to, p_from, text_size);
        }
    }

    return(status);
}

T5_MSG_PROTO(static, text_msg_object_compare, _InoutRef_ P_OBJECT_COMPARE p_object_compare)
{
    const PC_USTR_INLINE ustr_inline_1 = (PC_USTR_INLINE) p_object_compare->p_object_1;
    const PC_USTR_INLINE ustr_inline_2 = (PC_USTR_INLINE) p_object_compare->p_object_2;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(ustr_inline_1 && ustr_inline_2)
    {
        p_object_compare->res =
            uchars_inline_compare_n2((PC_UCHARS_INLINE) ustr_inline_1, ustr_inline_strlen32(ustr_inline_1),
                                     (PC_UCHARS_INLINE) ustr_inline_2, ustr_inline_strlen32(ustr_inline_2));
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, text_msg_object_data_read, _InoutRef_ P_OBJECT_DATA_READ p_object_data_read)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE == p_object_data_read->object_data.u.p_object)
    {
        ss_data_set_blank(&p_object_data_read->ss_data);
        p_object_data_read->constant = 1;
    }
    else
    {
        OBJECT_READ_TEXT object_read_text;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);
        object_read_text.p_quick_ublock = &quick_ublock;
        object_read_text.object_data = p_object_data_read->object_data;
        object_read_text.type = OBJECT_READ_TEXT_PLAIN;
        status_return(object_call_id(OBJECT_ID_STORY, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text));
        /* NB it comes unterminated */

        if(status_ok(status = ss_string_make_uchars(&p_object_data_read->ss_data, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock))))
            p_object_data_read->constant = 1;

        quick_ublock_dispose(&quick_ublock);
    }

    return(status);
}

T5_MSG_PROTO(static, text_msg_load_construct_ownform, _InoutRef_ P_CONSTRUCT_CONVERT p_construct_convert)
{
    STATUS status = STATUS_OK;
    const IL_CODE il_code = (IL_CODE) p_construct_convert->p_construct->t5_message;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(t5_message_is_inline(p_construct_convert->p_construct->t5_message));

    /* create an array containing an inline */
    switch(il_code)
    {
    default: default_unhandled();
#if CHECKING
    case IL_SOFT_HYPHEN:
    case IL_RETURN:
    case IL_TAB:
    case IL_WHOLENAME:
    case IL_LEAFNAME:
#endif
        status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_NONE, NULL, 0);
        break;

    case IL_DATE:
    case IL_FILE_DATE:
    case IL_PAGE_X:
    case IL_PAGE_Y:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
        PC_USTR ustr = p_args[0].val.ustr;

        if(il_code == IL_DATE)
        {
            static const UCHARZ old_prefix[] = "date ";

            if(0 == memcmp32(ustr, old_prefix, sizeof32(old_prefix)-1))
                ustr_IncBytes(ustr, sizeof32(old_prefix)-1); /* SKS 02apr93 skip old files' numform date prefix */
        }

        status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_USTR, ustr, 0);
        break;
        }

    case IL_SS_NAME:
        { /* get handle for name */
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
        PC_USTR ustr_name = p_args[0].val.ustr;
        SS_NAME_ENSURE ss_name_ensure;

        if(NULL != (ss_name_ensure.ustr_name_id = ustr_name))
        {
            status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_ENSURE, &ss_name_ensure);

            if(status_ok(status))
                status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_S32, &ss_name_ensure.ev_handle, 0);
        }

        break;
        }

    case IL_MS_FIELD:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
        PC_S32 p_s32 = &p_args[0].val.s32;
        status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_S32, p_s32, 0);
        break;
        }
    }

    return(status);
}

T5_MSG_PROTO(static, text_msg_save_construct_ownform, P_SAVE_CONSTRUCT_OWNFORM p_save_inline_ownform)
{
    STATUS status = STATUS_OK;
    const IL_CODE il_code = inline_code(p_save_inline_ownform->ustr_inline);
    PC_CONSTRUCT_TABLE p_construct_table;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    status_return(arglist_prepare_with_construct(&p_save_inline_ownform->arglist_handle,
                                                 OBJECT_ID_TEXT,
                                                 (T5_MESSAGE) il_code,
                                                 &p_construct_table));
    switch(il_code)
    {
    case IL_SS_NAME:
        { /* SS_NAMEs must convert their handle into text */
        SS_NAME_ID_FROM_HANDLE ss_name_id_from_handle;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 128);
        quick_ublock_with_buffer_setup(quick_ublock);

        ss_name_id_from_handle.ev_handle = (EV_HANDLE) data_from_inline_s32(p_save_inline_ownform->ustr_inline);
        ss_name_id_from_handle.docno = docno_from_p_docu(p_docu);
        ss_name_id_from_handle.p_quick_ublock = &quick_ublock;

        if(status_ok(object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_ID_FROM_HANDLE, &ss_name_id_from_handle)))
        {
            if(status_ok(status = arg_alloc_ustr(&p_save_inline_ownform->arglist_handle, 0, quick_ublock_ustr(&quick_ublock))))
            {
                p_save_inline_ownform->p_construct = p_construct_table;
                p_save_inline_ownform->object_id   = OBJECT_ID_TEXT;
            }
        }
        else
            arglist_dispose(&p_save_inline_ownform->arglist_handle);

        quick_ublock_dispose(&quick_ublock);

        break;
        }

    default:
        {
        assert(IL_NONE != il_code);

        switch(inline_data_type(p_save_inline_ownform->ustr_inline))
        {
        case IL_TYPE_USTR:
            status = arg_alloc_ustr(&p_save_inline_ownform->arglist_handle, 0, inline_data_ptr(PC_USTR, p_save_inline_ownform->ustr_inline));
            break;

        case IL_TYPE_S32: /* SKS 03.03.93 */
            p_arglist_arg(&p_save_inline_ownform->arglist_handle, 0)->val.s32 = data_from_inline_s32(p_save_inline_ownform->ustr_inline);
            break;

        default:
            break;
        }

        if(status_fail(status))
            arglist_dispose(&p_save_inline_ownform->arglist_handle);
        else
        {
            p_save_inline_ownform->p_construct = p_construct_table;
            p_save_inline_ownform->object_id   = OBJECT_ID_TEXT;
        }

        break;
        }
    }

    return(status);
}

T5_MSG_PROTO(static, text_msg_load_cell_ownform, _InoutRef_ P_LOAD_CELL_OWNFORM p_load_cell_ownform)
{
    STATUS status = STATUS_OK;
    PC_USTR_INLINE ustr_inline;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* check the data type */
    switch(p_load_cell_ownform->data_type)
    {
    case OWNFORM_DATA_TYPE_TEXT:
    case OWNFORM_DATA_TYPE_DATE:
    case OWNFORM_DATA_TYPE_CONSTANT:
    case OWNFORM_DATA_TYPE_FORMULA:
    case OWNFORM_DATA_TYPE_ARRAY:
        break;

    default: default_unhandled();
#if CHECKING
    case OWNFORM_DATA_TYPE_DRAWFILE:
    case OWNFORM_DATA_TYPE_OWNER:
#endif
        return(STATUS_FAIL);
    }

    ustr_inline =
        p_load_cell_ownform->ustr_inline_contents
            ? p_load_cell_ownform->ustr_inline_contents
            : (PC_USTR_INLINE) p_load_cell_ownform->ustr_formula;

    if(NULL != ustr_inline)
    {
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr_inline, ustr_inline_strlen32(ustr_inline));
        status = text_insert_sub_redisplay(p_docu,
                                           &p_load_cell_ownform->object_data.object_position_end,
                                           &p_load_cell_ownform->object_data,
                                           &quick_ublock,
                                           FALSE, TRUE);
        /* NB no quick_ublock_dispose() */
    }

    p_load_cell_ownform->processed = 1;

    return(status);
}

T5_MSG_PROTO(static, text_msg_save_cell_ownform, _InoutRef_ P_SAVE_CELL_OWNFORM p_save_cell_ownform)
{
    STATUS status = STATUS_OK;
    const P_USTR_INLINE ustr_inline = p_save_cell_ownform->object_data.u.ustr_inline;
    S32 start, end;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != ustr_inline)
    {
        offsets_from_object_data(&start, &end, &p_save_cell_ownform->object_data, ustr_inline_strlen(ustr_inline));

        if(status_ok(status = quick_ublock_uchars_add(&p_save_cell_ownform->contents_data_quick_ublock, uchars_AddBytes(ustr_inline, start), end - start)))
            /* Tell caller what type of data we have just saved */
            p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_TEXT;
    }

    return(status);
}

T5_MSG_PROTO(static, text_msg_load_cell_foreign, _InoutRef_ P_LOAD_CELL_FOREIGN p_load_cell_foreign)
{
    STATUS status = STATUS_OK;
    PC_USTR_INLINE ustr_inline;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* check the data type */
    switch(p_load_cell_foreign->data_type)
    {
    case OWNFORM_DATA_TYPE_TEXT:
    case OWNFORM_DATA_TYPE_DATE:
    case OWNFORM_DATA_TYPE_CONSTANT:
    case OWNFORM_DATA_TYPE_FORMULA:
    case OWNFORM_DATA_TYPE_ARRAY:
        break;

    default: default_unhandled();
#if CHECKING
    case OWNFORM_DATA_TYPE_DRAWFILE:
    case OWNFORM_DATA_TYPE_OWNER:
#endif
        return(STATUS_FAIL);
    }

    ustr_inline =
        p_load_cell_foreign->ustr_inline_contents
            ? p_load_cell_foreign->ustr_inline_contents
            : (PC_USTR_INLINE) p_load_cell_foreign->ustr_formula;

    if(NULL != ustr_inline)
    {
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr_inline, ustr_inline_strlen32(ustr_inline));
        status = text_insert_sub_redisplay(p_docu,
                                           &p_load_cell_foreign->object_data.object_position_end,
                                           &p_load_cell_foreign->object_data,
                                           &quick_ublock,
                                           FALSE, TRUE);
        /* NB no quick_ublock_dispose() */
    }

    p_load_cell_foreign->processed = 1;

    return(status);
}

T5_MSG_PROTO(static, text_msg_new_object_from_text, _InRef_ P_NEW_OBJECT_FROM_TEXT p_new_object_from_text)
{
    STATUS status = STATUS_OK;
    OBJECT_DATA object_data;
    OBJECT_POSITION object_position_after;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(p_new_object_from_text->data_ref.data_space == DATA_SLOT);

    status_consume(object_data_from_slr(p_docu, &object_data, &p_new_object_from_text->data_ref.arg.slr));

    /* tell dependents about it */
    switch(object_data.object_id)
    {
    case OBJECT_ID_TEXT:
    case OBJECT_ID_NONE:
        break;

    default:
        if(p_new_object_from_text->please_uref_overwrite)
        {
            UREF_PARMS uref_parms;
            region_from_two_slrs(&uref_parms.source.region,
                                 &p_new_object_from_text->data_ref.arg.slr,
                                 &p_new_object_from_text->data_ref.arg.slr,
                                 TRUE);
            uref_event(p_docu, Uref_Msg_Overwrite, &uref_parms);
            status_consume(object_data_from_slr(p_docu, &object_data, &p_new_object_from_text->data_ref.arg.slr));
        }
        break;
    }

    status = text_insert_sub_redisplay(p_docu,
                                       &object_position_after,
                                       &object_data,
                                       p_new_object_from_text->p_quick_ublock,
                                       p_new_object_from_text->please_redraw, FALSE);

    return(status);
}

T5_MSG_PROTO(static, text_msg_spell_auto_check, P_SPELL_AUTO_CHECK p_spell_auto_check)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if( (OBJECT_ID_NONE != p_spell_auto_check->position_now.object_position.object_id) &&
        (OBJECT_ID_NONE != p_spell_auto_check->position_was.object_position.object_id) &&
        (P_DATA_NONE != p_spell_auto_check->object_data.u.p_object) )
    {
        OBJECT_DATA object_data = p_spell_auto_check->object_data;
        P_USTR_INLINE ustr_inline = p_spell_auto_check->object_data.u.ustr_inline;
        S32 len = ustr_inline_strlen(ustr_inline);

        if(p_spell_auto_check->position_now.object_position.data >= p_spell_auto_check->position_was.object_position.data)
        { /* move forward */
            S32 start, end;
            object_data.object_position_start = p_spell_auto_check->position_was.object_position;
            object_data.object_position_start.data = MAX(0, object_data.object_position_start.data - 1);
            object_data.object_position_end = p_spell_auto_check->position_was.object_position;
            offsets_from_object_data(&start, &end, &object_data, len);

            if(  t5_isalpha(PtrGetByteOff(ustr_inline, start)) &&
                !t5_isalpha(PtrGetByteOff(ustr_inline, end))   )
            {
                STATUS status_word_check;
                if(status_fail(status_word_check = object_call_id(OBJECT_ID_STORY, p_docu, T5_MSG_OBJECT_WORD_CHECK, &object_data)))
                    status = status_word_check;
                else if(STATUS_OK == status_word_check) /* expect STATUS_DONE */
                    host_bleep();
            }
        }
        else if(p_spell_auto_check->position_now.object_position.data < p_spell_auto_check->position_was.object_position.data)
        { /* move back */
            S32 start, end;
            object_data.object_position_start = p_spell_auto_check->position_now.object_position;
            object_data.object_position_end = p_spell_auto_check->position_now.object_position;
            object_data.object_position_end.data += 1;
            offsets_from_object_data(&start, &end, &object_data, len);

            if( !t5_isalpha(PtrGetByteOff(ustr_inline, start)) &&
                 t5_isalpha(PtrGetByteOff(ustr_inline, end))   )
            {
                STATUS status_word_check;
                object_data.object_position_start = p_spell_auto_check->position_was.object_position;
                object_data.object_position_end = p_spell_auto_check->position_was.object_position;
                object_data.object_position_end.data += 1;
                offsets_from_object_data(&start, &end, &object_data, len);

                if(status_fail(status_word_check = object_call_id(OBJECT_ID_STORY, p_docu, T5_MSG_OBJECT_WORD_CHECK, &object_data)))
                    status = status_word_check;
                else if(STATUS_OK == status_word_check) /* expect STATUS_DONE */
                    host_bleep();
            }
        }
    }

    return(status);
}

T5_CMD_PROTO(static, object_text_cmd)
{
    assert(T5_CMD__ACTUAL_END <= T5_CMD__END);

    switch(T5_MESSAGE_CMD_OFFSET(t5_message))
    {
    default:
#if CHECKING
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_DATE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_FILE_DATE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_PAGE_X):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_PAGE_Y):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_SS_NAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_MS_FIELD):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_WHOLENAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_LEAFNAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_RETURN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_SOFT_HYPHEN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_TAB):

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_WORD_COUNT):
#endif
        /* send these messages to our helper object */
        return(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, de_const_cast(P_T5_CMD, p_t5_cmd)));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_UPPER):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_LOWER):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_INICAP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_SWAP):
        return(text_cmd_setc(p_docu, t5_message, (P_OBJECT_SET_CASE) de_const_cast(P_T5_CMD, p_t5_cmd)));
    }
}

OBJECT_PROTO(extern, object_text);
OBJECT_PROTO(extern, object_text)
{
    if(T5_MESSAGE_IS_CMD(t5_message))
        return(object_text_cmd(p_docu, t5_message, (PC_T5_CMD) p_data));

    switch(t5_message)
    {
    default:
#if CHECKING
    case T5_MSG_OBJECT_POSITION_SET:
    case T5_MSG_OBJECT_CHECK:
#endif
        /* send these messages directly to our helper object */
        return(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, p_data));

        /* --- go via TEXT_MESSAGE_BLOCK --- */

    case T5_EVENT_REDRAW:
        return(text_event_redraw(p_docu, t5_message, (P_OBJECT_REDRAW) p_data));

    case T5_MSG_INITCLOSE:
        return(text_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_OBJECT_HOW_BIG:
        return(text_msg_object_how_big(p_docu, t5_message, (P_OBJECT_HOW_BIG) p_data));

    case T5_MSG_OBJECT_HOW_WIDE:
        return(text_msg_object_how_wide(p_docu, t5_message, (P_OBJECT_HOW_WIDE) p_data));

    case T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION:
    case T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT:
        return(text_msg_object_position_find(p_docu, t5_message, (P_OBJECT_POSITION_FIND) p_data));

    case T5_MSG_OBJECT_LOGICAL_MOVE:
        return(text_msg_object_logical_move(p_docu, t5_message, (P_OBJECT_LOGICAL_MOVE) p_data));

    case T5_MSG_OBJECT_READ_TEXT_DRAFT:
        return(text_msg_read_text_draft(p_docu, t5_message, (P_OBJECT_READ_TEXT_DRAFT) p_data));

    case T5_MSG_TAB_WANTED:
        return(text_msg_tab_wanted(p_docu, t5_message, (P_TAB_WANTED) p_data));

        /* --- normal --- */

    case T5_MSG_OBJECT_KEYS:
        return(text_msg_object_keys(p_docu, t5_message, (P_OBJECT_KEYS) p_data));

    case T5_MSG_OBJECT_DELETE_SUB:
        return(text_msg_object_delete_sub(p_docu, t5_message, (P_OBJECT_DELETE_SUB) p_data));

    case T5_MSG_OBJECT_STRING_REPLACE:
        return(text_msg_object_string_replace(p_docu, t5_message, (P_OBJECT_STRING_REPLACE) p_data));

    case T5_MSG_OBJECT_COPY:
        return(text_msg_object_copy(p_docu, t5_message, (P_OBJECT_COPY) p_data));

    case T5_MSG_OBJECT_COMPARE:
        return(text_msg_object_compare(p_docu, t5_message, (P_OBJECT_COMPARE) p_data));

    case T5_MSG_OBJECT_DATA_READ:
        return(text_msg_object_data_read(p_docu, t5_message, (P_OBJECT_DATA_READ) p_data));

    case T5_MSG_LOAD_CONSTRUCT_OWNFORM:
        return(text_msg_load_construct_ownform(p_docu, t5_message, (P_CONSTRUCT_CONVERT) p_data));

    case T5_MSG_SAVE_CONSTRUCT_OWNFORM:
        return(text_msg_save_construct_ownform(p_docu, t5_message, (P_SAVE_CONSTRUCT_OWNFORM) p_data));

    case T5_MSG_LOAD_CELL_OWNFORM:
    case T5_MSG_LOAD_FRAG_OWNFORM:
        return(text_msg_load_cell_ownform(p_docu, t5_message, (P_LOAD_CELL_OWNFORM) p_data));

    case T5_MSG_SAVE_CELL_OWNFORM:
        return(text_msg_save_cell_ownform(p_docu, t5_message, (P_SAVE_CELL_OWNFORM) p_data));

    case T5_MSG_LOAD_CELL_FOREIGN:
        return(text_msg_load_cell_foreign(p_docu, t5_message, (P_LOAD_CELL_FOREIGN) p_data));

    case T5_MSG_NEW_OBJECT_FROM_TEXT:
        return(text_msg_new_object_from_text(p_docu, t5_message, (P_NEW_OBJECT_FROM_TEXT) p_data));

    case T5_MSG_SPELL_AUTO_CHECK:
        return(text_msg_spell_auto_check(p_docu, t5_message, (P_SPELL_AUTO_CHECK) p_data));
    }
}

/* end of ob_text.c */
