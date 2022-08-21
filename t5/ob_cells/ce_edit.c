/* ce_edit.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* edit routines for ob_cells */

/* MRJC August 1994 */

#include "common/gflags.h"

#ifndef    __cells_ob_cells_h
#include "ob_cells/ob_cells.h"
#endif

#include "ob_story/ob_story.h"

OBJECT_PROTO(extern, object_cells_edit);

_Check_return_
static OBJECT_ID
cell_new_type(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

/*
* set up a text message block
*/

static void
text_message_block_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_TEXT_MESSAGE_BLOCK p_text_message_block,
    /*_Inout_*/ P_ANY p_data,
    _InRef_opt_ PC_SKEL_RECT p_skel_rect,
    _InRef_     PC_OBJECT_DATA p_object_data)
{
    p_text_message_block->p_data = p_data;

    p_text_message_block->inline_object.object_data = *p_object_data;

    if(P_DATA_NONE != p_text_message_block->inline_object.object_data.u.p_object)
        p_text_message_block->inline_object.inline_len = ustr_inline_strlen(p_text_message_block->inline_object.object_data.u.ustr_inline);
    else
        p_text_message_block->inline_object.inline_len = 0;

    trace_2(TRACE_APP_CLICK,
            TEXT("text_message_block_init len: ") S32_TFMT TEXT(", text: %s"),
            p_text_message_block->inline_object.inline_len,
            report_ustr_inline(p_text_message_block->inline_object.object_data.u.ustr_inline));

    assert_EQ(p_object_data->data_ref.data_space, DATA_SLOT); /* PMF */
    style_init(&p_text_message_block->text_format_info.style_text_global);
    style_from_slr(p_docu, &p_text_message_block->text_format_info.style_text_global, &style_selector_para_text, &p_object_data->data_ref.arg.slr);

    p_text_message_block->text_format_info.h_style_list = p_docu->h_style_docu_area;
    p_text_message_block->text_format_info.paginate = 0;

    if(NULL == p_skel_rect)
        skel_rect_from_slr(p_docu, &p_text_message_block->text_format_info.skel_rect_object, &p_object_data->data_ref.arg.slr);
    else
        p_text_message_block->text_format_info.skel_rect_object = *p_skel_rect;

    p_text_message_block->text_format_info.text_area_border_y = 2 * p_docu->page_def.grid_size;

    p_text_message_block->text_format_info.skel_rect_work.tl.page_num.x =
    p_text_message_block->text_format_info.skel_rect_work.tl.page_num.y = 0;
    p_text_message_block->text_format_info.skel_rect_work.br.page_num = p_text_message_block->text_format_info.skel_rect_work.tl.page_num;

    p_text_message_block->text_format_info.skel_rect_work.tl.pixit_point.x =
    p_text_message_block->text_format_info.skel_rect_work.tl.pixit_point.y = 0;
    p_text_message_block->text_format_info.skel_rect_work.br.pixit_point.x = p_docu->page_def.cells_usable_x
                                                          + 2 * p_docu->page_def.grid_size;

    assert(p_object_data->data_ref.data_space == DATA_SLOT); /* PMF */
    p_text_message_block->text_format_info.skel_rect_work.br.pixit_point.y = page_height_from_row(p_docu, p_object_data->data_ref.arg.slr.row)
                                                          + p_text_message_block->text_format_info.text_area_border_y;
    p_text_message_block->text_format_info.object_visible = 1;
}

/*
initialise redisplay block
*/

static void
text_inline_redisplay_init(
    _OutRef_    P_TEXT_INLINE_REDISPLAY p_text_inline_redisplay,
    _InRef_maybenone_ PC_QUICK_UBLOCK p_quick_ublock,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     BOOL do_redraw)
{
    p_text_inline_redisplay->p_quick_ublock = p_quick_ublock;
    p_text_inline_redisplay->skel_rect_para_after = *p_skel_rect;
    p_text_inline_redisplay->redraw_tag = UPDATE_PANE_CELLS_AREA;
    p_text_inline_redisplay->do_redraw = do_redraw;
    p_text_inline_redisplay->redraw_done = FALSE;
    p_text_inline_redisplay->size_changed = TRUE;
    p_text_inline_redisplay->do_position_update = TRUE;
    /*skel_rect_text_before*/
    /*skel_rect_para_before*/
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
    /*_Out_*/   P_OBJECT_POSITION p_object_position_after, /* NB not set if zero elements */
    P_OBJECT_DATA p_object_data,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    STATUS status = STATUS_OK;

    { /* do actual insert operation */
    S32 ins_size = quick_ublock_bytes(p_quick_ublock);

    if(ins_size)
    {
        const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);
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
                                   TRUE);

        if(status_ok(status))
        {
            if(NULL != al_array_extend_by_U8(&p_cells_instance->h_text, ins_size + (text_message_block.inline_object.inline_len ? 0 : 1), &array_init_block_u8, &status))
            {
                /* make sure altered pointer is fed back */
                text_message_block.inline_object.object_data.u.ustr_inline = ustr_inline_from_h_ustr(&p_cells_instance->h_text);
                p_cells_instance->data_ref_text = text_message_block.inline_object.object_data.data_ref;
                p_cells_instance->text_modified = 1;
            }
        }

        if(status_ok(status))
        {
            text_message_block.inline_object.object_data.object_id = OBJECT_ID_CELLS_EDIT;
            status = object_call_STORY_with_tmb(p_docu, T5_MSG_INSERT_INLINE_REDISPLAY, &text_message_block);
        }

        if(status_ok(status))
        {
            assert(text_message_block.inline_object.object_data.data_ref.data_space == DATA_SLOT); /* PMF */
            status = format_object_size_set(p_docu,
                                            &text_inline_redisplay.skel_rect_para_after,
                                            &text_inline_redisplay.skel_rect_para_before,
                                            &text_message_block.inline_object.object_data.data_ref.arg.slr,
                                            text_inline_redisplay.redraw_done);
        }

        if(status_ok(status))
            *p_object_position_after = text_message_block.inline_object.object_data.object_position_end;
    }
    } /*block*/

    return(status);
}

/******************************************************************************
*
* replace edited text in a cell
*
******************************************************************************/

_Check_return_
static STATUS
text_to_cell(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);

    if(p_cells_instance->h_text)
    {
        ARRAY_HANDLE h_text = p_cells_instance->h_text;
        p_cells_instance->h_text = 0;

        style_docu_area_uref_hold(p_docu, &p_docu->h_style_docu_area, &p_cells_instance->data_ref_text.arg.slr);

        if(p_cells_instance->text_modified)
        {
#if FALSE
            if(!array_elements(&h_text))
            {
                assert(p_cells_instance->data_ref_text.data_space == DATA_SLOT); /* PMF */
                status = cells_blank_make(p_docu, &p_cells_instance->data_ref_text.arg.slr);
            }
            else
#endif
            {
                NEW_OBJECT_FROM_TEXT new_object_from_text;
                QUICK_UBLOCK quick_ublock;
                quick_ublock_setup_using_array(&quick_ublock, h_text);

                new_object_from_text.data_ref = p_cells_instance->data_ref_text;
                new_object_from_text.p_quick_ublock = &quick_ublock;
                new_object_from_text.please_redraw = TRUE;
                new_object_from_text.please_uref_overwrite = TRUE;
                new_object_from_text.try_autoformat = TRUE;

                status = object_call_id(p_cells_instance->object_id, p_docu, T5_MSG_NEW_OBJECT_FROM_TEXT, &new_object_from_text);
                /* NB no quick_ublock_dispose */
            }

            p_cells_instance->text_modified = 0;
        }
        else
            /* 16.12.94 even tho not modified, cells may change size when you move off them */
            status = format_object_size_set(p_docu, NULL, NULL, &p_cells_instance->data_ref_text.arg.slr, FALSE);

        {
        OBJECT_DATA object_data;
        status_consume(object_data_from_slr(p_docu, &object_data, &p_cells_instance->data_ref_text.arg.slr));
        style_docu_area_uref_release(p_docu,
                                     &p_docu->h_style_docu_area,
                                     &p_cells_instance->data_ref_text.arg.slr,
                                     (OBJECT_ID_TEXT == object_data.object_id) ? OBJECT_ID_TEXT : OBJECT_ID_NONE);
        } /*block*/

        al_array_dispose(&h_text);
    }

    return(status);
}

/******************************************************************************
*
* load spreadsheet constant data into buffer
* as text for editing by story object
*
******************************************************************************/

_Check_return_
static STATUS /* =1 if text loaded */
text_from_cell(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data_out,
    _InRef_     P_OBJECT_DATA p_object_data_in)
{
    STATUS status = STATUS_OK;
    const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);

    *p_object_data_out = *p_object_data_in;

    if(p_cells_instance->h_text)
    {
        if(data_refs_equal(&p_cells_instance->data_ref_text, &p_object_data_in->data_ref))
            /* nothing to do - we've got the right piece of text */
            status = STATUS_DONE;
        else
            /* send text back into old cell */
            status_assert(text_to_cell(p_docu));
    }

    { /* ask object if it's editable */
    OBJECT_EDITABLE object_editable;
    object_editable.object_data = *p_object_data_in;
    object_editable.editable = TRUE;
    if(status_ok(object_call_id(p_object_data_in->object_id, p_docu, T5_MSG_OBJECT_EDITABLE, &object_editable)))
        if(!object_editable.editable)
            status = ERR_READ_ONLY;
    } /*block*/

    if(status == STATUS_OK)
    {
        OBJECT_READ_TEXT object_read_text;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
        quick_ublock_with_buffer_setup(quick_ublock);

        zero_struct(object_read_text);
        object_read_text.p_quick_ublock = &quick_ublock;
        object_read_text.type = OBJECT_READ_TEXT_EDIT;
        object_read_text.object_data = *p_object_data_in;
        object_position_init(&object_read_text.object_data.object_position_start);
        object_position_init(&object_read_text.object_data.object_position_end);

        if(status_ok(status = object_call_id(p_object_data_in->object_id, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text)))
        {
            if(0 != quick_ublock_bytes(&quick_ublock))
            {
                OBJECT_POSITION object_position_after;

                p_object_data_out->object_id = OBJECT_ID_NONE;
                p_object_data_out->u.p_object = P_DATA_NONE;

                if(status_ok(status = text_insert_sub_redisplay(p_docu,
                                                                &object_position_after,
                                                                p_object_data_out,
                                                                &quick_ublock)))
                {
                    p_cells_instance->data_ref_text = p_object_data_in->data_ref;
                    p_cells_instance->text_modified = 0;
                    p_object_data_out->u.ustr_inline = ustr_inline_from_h_ustr(&p_cells_instance->h_text);
                    status = STATUS_DONE;
                }
            }
        }

        /* override existing object type with configured new type */

        assert(p_object_data_in->data_ref.data_space == DATA_SLOT); /* PMF */
        p_cells_instance->object_id = cell_new_type(p_docu, &p_object_data_in->data_ref.arg.slr);

        quick_ublock_dispose(&quick_ublock);
    }

    if(status_done(status))
    {
        p_object_data_out->object_id = OBJECT_ID_CELLS_EDIT;
        p_object_data_out->u.ustr_inline = ustr_inline_from_h_ustr(&p_cells_instance->h_text);
    }
    else
        p_object_data_out->u.p_object = P_DATA_NONE;

    return(status);
}

/******************************************************************************
*
* is the text for this cell in the buffer?
*
******************************************************************************/

_Check_return_ _Success_(status_done(return))
static STATUS /* status = STATUS_DONE if text is in cell */
text_in_cell(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data_out,
    _InRef_     PC_OBJECT_DATA p_object_data_in)
{
    STATUS status = STATUS_OK;
    const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);

    assert(p_object_data_in->data_ref.data_space == DATA_SLOT);

    if(/*(NULL != p_cells_instance) &&*/ array_elements(&p_cells_instance->h_text))
    {
        assert(p_object_data_in->data_ref.data_space == DATA_SLOT);

        if(slr_equal(&p_cells_instance->data_ref_text.arg.slr, &p_object_data_in->data_ref.arg.slr))
        {
            *p_object_data_out = *p_object_data_in;
            p_object_data_out->object_id = OBJECT_ID_CELLS_EDIT;
            p_object_data_out->u.ustr_inline = ustr_inline_from_h_ustr(&p_cells_instance->h_text);
            status = STATUS_DONE;
        }
    }

    return(status);
}

/******************************************************************************
*
* return the object type for a blank cell
*
******************************************************************************/

_Check_return_
static OBJECT_ID
cell_new_type(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    STYLE style;
    STYLE_SELECTOR selector;
    OBJECT_ID object_id;

    /* get new object type */
    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_PS_NEW_OBJECT);

    style_init(&style);
    style_from_slr(p_docu, &style, &selector, p_slr);
    object_id = OBJECT_ID_UNPACK(style.para_style.new_object);
    if(!object_present(object_id))
        object_id = OBJECT_ID_TEXT;

    return(object_id);
}

/******************************************************************************
*
* call a cell in the cell layer
*
******************************************************************************/

_Check_return_
extern STATUS
cell_call_id(
    _InVal_     OBJECT_ID object_id,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InVal_     BOOL allow_cells_edit)
{
    STATUS status = object_call_id(object_id, p_docu, t5_message, p_data);

    if(status_ok(status))
        return(status);

    /* status_fail(status) */

    /* certain messages when failed are passed to cells_edit to start in-cell editing
     * these correspond to the messages in which object_cells_edit() will load
     * text from an object using text_from_cell()
     */
    switch(t5_message)
    {
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

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:

    case T5_MSG_OBJECT_STRING_SEARCH:
    case T5_MSG_OBJECT_STRING_REPLACE:

    case T5_MSG_OBJECT_POSITION_SET:
    case T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION:
    case T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT:
        if(allow_cells_edit && (OBJECT_ID_CELLS_EDIT != object_id))
        {
            OBJECT_IN_CELL_ALLOWED object_in_cell_allowed;
            zero_struct(object_in_cell_allowed);
            object_in_cell_allowed.in_cell_allowed = TRUE;
            if(status_ok(object_call_id(object_id, p_docu, T5_MSG_OBJECT_IN_CELL_ALLOWED, &object_in_cell_allowed)))
                if(object_in_cell_allowed.in_cell_allowed)
                    status = object_cells_edit(p_docu, t5_message, p_data);
        }
        break;

    case T5_MSG_LOAD_FRAG_OWNFORM: /* SKS 11apr97 fudge */
        if(OBJECT_ID_CELLS_EDIT != object_id)
        {
            OBJECT_IN_CELL_ALLOWED object_in_cell_allowed;
            zero_struct(object_in_cell_allowed);
            object_in_cell_allowed.in_cell_allowed = TRUE;
            if(status_ok(object_call_id(object_id, p_docu, T5_MSG_OBJECT_IN_CELL_ALLOWED, &object_in_cell_allowed)))
                if(object_in_cell_allowed.in_cell_allowed)
                    status = object_cells_edit(p_docu, t5_message, p_data);
        }
        break;

    default:
        break;
    }

    return(status);
}

/******************************************************************************
*
* make up some object data from a docu_area
*
******************************************************************************/

/*ncr*/
extern BOOL
cell_data_from_docu_area_br(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    POSITION position;

    if(slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr))
        position = p_docu_area->tl;
    else
    {
        position = p_docu_area->br;
        position.slr.col -= 1;
        position.slr.row -= 1;
        object_position_init(&position.object_position);
    }

    return(cell_data_from_position_and_object_position(p_docu, p_object_data, &position, &p_docu_area->br.object_position));
}

/******************************************************************************
*
* make up some object data from a docu_area
*
******************************************************************************/

/*ncr*/
extern BOOL
cell_data_from_docu_area_tl(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    PC_OBJECT_POSITION p_object_position_end;

    if(slr_last_in_docu_area(p_docu_area, &p_docu_area->tl.slr))
        p_object_position_end = &p_docu_area->br.object_position;
    else
        p_object_position_end = P_OBJECT_POSITION_NONE;

    return(cell_data_from_position_and_object_position(p_docu, p_object_data, &p_docu_area->tl, p_object_position_end));
}

/******************************************************************************
*
* make up some object data from a position
*
******************************************************************************/

/*ncr*/
extern BOOL
cell_data_from_position(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_POSITION p_position)
{
    BOOL res = cell_data_from_slr(p_docu, p_object_data, &p_position->slr);

    if(p_object_data->object_id == p_position->object_position.object_id)
        p_object_data->object_position_start = p_position->object_position;

    return(res);
}

/*ncr*/
extern BOOL
cell_data_from_position_and_object_position(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_POSITION p_position,
    _InRef_maybenone_ PC_OBJECT_POSITION p_object_position_end)
{
    BOOL res = cell_data_from_slr(p_docu, p_object_data, &p_position->slr);

    if(p_object_data->object_id == p_position->object_position.object_id)
        p_object_data->object_position_start = p_position->object_position;

    if( !IS_P_DATA_NONE(p_object_position_end) &&
        (p_object_data->object_id == p_object_position_end->object_id) )
    {
        p_object_data->object_position_end = *p_object_position_end;
    }

    return(res);
}

/******************************************************************************
*
* make up some object data from an slr
*
******************************************************************************/

/*ncr*/
extern BOOL
cell_data_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_SLR p_slr)
{
    BOOL res;

    data_ref_from_slr(&p_object_data->data_ref, p_slr);

    object_position_init(&p_object_data->object_position_start);
    object_position_init(&p_object_data->object_position_end);

    CODE_ANALYSIS_ONLY(p_object_data->object_id = OBJECT_ID_NONE); /* set by text_in_cell() or here */
    CODE_ANALYSIS_ONLY(p_object_data->u.p_object = P_DATA_NONE); /* ditto */

    if(status_done(text_in_cell(p_docu, p_object_data, p_object_data)))
    {
        res = TRUE;
    }
    else
    {
        const P_CELL p_cell = p_cell_from_slr(p_docu, p_slr);

        if(NULL != p_cell)
        {
            p_object_data->object_id = object_id_from_cell(p_cell);
            p_object_data->u.p_object = &p_cell->object[0];
            res = TRUE;
        }
        else
        {
            p_object_data->object_id = cell_new_type(p_docu, p_slr);
            p_object_data->u.p_object = P_DATA_NONE;
            res = FALSE;
        }
    }

    return(res);
}

_Check_return_
extern OBJECT_ID
cell_owner_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    const P_CELL p_cell = p_cell_from_slr(p_docu, p_slr);

    if(NULL != p_cell)
        return(object_id_from_cell(p_cell));

    return(cell_new_type(p_docu, p_slr));
}

/*
main events
*/

MAEVE_EVENT_PROTO(static, maeve_event_cells_edit)
{
    UNREFERENCED_PARAMETER(p_data);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_CELL_MERGE:
        return(text_to_cell(p_docu));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* cells editor object event routines
*
******************************************************************************/

_Check_return_
static STATUS
cells_edit_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    /* p_cells_instance->h_text already zero */

    return(maeve_event_handler_add(p_docu, maeve_event_cells_edit, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
cells_edit_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);

    maeve_event_handler_del(p_docu, maeve_event_cells_edit, (CLIENT_HANDLE) 0);

    al_array_dispose(&p_cells_instance->h_text);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_edit_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(cells_edit_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(cells_edit_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, cells_edit_cmd_setc_do, P_OBJECT_SET_CASE p_object_set_case)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_set_case->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;
        TEXT_INLINE_REDISPLAY text_inline_redisplay;

        text_message_block_init(p_docu,
                                &text_message_block,
                                &text_inline_redisplay,
                                NULL,
                                &object_data);

        text_inline_redisplay_init(&text_inline_redisplay,
                                   P_QUICK_UBLOCK_NONE,
                                   &text_message_block.text_format_info.skel_rect_object,
                                   p_object_set_case->do_redraw);

        if(status_ok(status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block)))
        {
            const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);

            p_cells_instance->text_modified = 1;

            status = format_object_size_set(p_docu,
                                            &text_inline_redisplay.skel_rect_para_after,
                                            &text_inline_redisplay.skel_rect_para_before,
                                            &text_message_block.inline_object.object_data.data_ref.arg.slr,
                                            text_inline_redisplay.redraw_done);
        }

        if(status_ok(status))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_event_redraw, _InoutRef_ P_OBJECT_REDRAW p_object_redraw)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_redraw->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu,
                                &text_message_block,
                                p_object_redraw,
                                &p_object_redraw->skel_rect_object,
                                &object_data);

        if(status_ok(status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block)))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_delete_sub, _InoutRef_ P_OBJECT_DELETE_SUB p_object_delete_sub)
{
    STATUS status;
    OBJECT_DATA object_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_object_delete_sub->h_data_del = 0;

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_delete_sub->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;
        TEXT_INLINE_REDISPLAY text_inline_redisplay;
        S32 start, end;

        text_message_block_init(p_docu,
                                &text_message_block,
                                &text_inline_redisplay,
                                NULL,
                                &object_data);

        text_inline_redisplay_init(&text_inline_redisplay,
                                   P_QUICK_UBLOCK_NONE,
                                   &text_message_block.text_format_info.skel_rect_object,
                                   TRUE);

        offsets_from_object_data(&start, &end, &text_message_block.inline_object.object_data, text_message_block.inline_object.inline_len);

        {
        S32 del_size = end - start;

        if(del_size)
        {
            const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);

            {
            OF_TEMPLATE of_template = { 0 };
            DOCU_AREA docu_area;
            of_template.data_class = DATA_SAVE_CHARACTER;
            docu_area_from_object_data(&docu_area, &object_data);
            if(p_object_delete_sub->save_data)
                status = save_ownform_to_array_from_docu_area(p_docu, &p_object_delete_sub->h_data_del, &of_template, &docu_area);
            } /*block*/

            if(status_ok(status))
                status = object_call_STORY_with_tmb(p_docu, T5_MSG_DELETE_INLINE_REDISPLAY, &text_message_block);

            if(status_ok(status))
            {
                al_array_shrink_by(&p_cells_instance->h_text,
                                             text_message_block.inline_object.inline_len
                                                 ? -(del_size + 0)
                                                 : -(del_size + 1));
                p_cells_instance->text_modified = 1;

                /* send back new object pointer */
                p_object_delete_sub->object_data.u.ustr_inline = ustr_inline_from_h_ustr(&p_cells_instance->h_text);
            }

            if(status_ok(status))
            {
                assert(text_message_block.inline_object.object_data.data_ref.data_space == DATA_SLOT); /* PMF */
                status = format_object_size_set(p_docu,
                                                &text_inline_redisplay.skel_rect_para_after,
                                                &text_inline_redisplay.skel_rect_para_before,
                                                &text_message_block.inline_object.object_data.data_ref.arg.slr,
                                                text_inline_redisplay.redraw_done);
            }
        }
        } /*block*/

        if(status_ok(status))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_logical_move, P_OBJECT_LOGICAL_MOVE p_object_logical_move)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_logical_move->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu,
                                &text_message_block,
                                p_object_logical_move,
                                NULL,
                                &object_data);

        if(status_ok(status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block)))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_how_big, P_OBJECT_HOW_BIG p_object_how_big)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_how_big->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu,
                                &text_message_block,
                                p_object_how_big,
                                &p_object_how_big->skel_rect,
                                &object_data);

        if(status_ok(status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block)))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_how_wide, P_OBJECT_HOW_WIDE p_object_how_wide)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_how_wide->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;
        SKEL_RECT skel_rect;

        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.pixit_point.y = 0;
        skel_rect.tl.page_num.x = 0;
        skel_rect.tl.page_num.y = 0;

        text_message_block_init(p_docu,
                                &text_message_block,
                                p_object_how_wide,
                                &skel_rect,
                                &object_data);

        if(status_ok(status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block)))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_keys, P_OBJECT_KEYS p_object_keys)
{
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_ok(status = check_protection_simple(p_docu, TRUE)))
    {
        /* turn lots of keys pressed message into insert_sub message; */
        OBJECT_DATA object_data;

        if(status_ok(status = text_from_cell(p_docu, &object_data, &p_object_keys->object_data)))
        {
            status = text_insert_sub_redisplay(p_docu,
                                               &p_object_keys->object_data.object_position_end,
                                               &object_data,
                                               p_object_keys->p_skelevent_keys->p_quick_ublock);

            p_object_keys->p_skelevent_keys->processed = 1;

            if(status_ok(status))
                status = STATUS_DONE;
        }
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_position_find, P_OBJECT_POSITION_FIND p_object_position_find)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_ok(status = text_from_cell(p_docu, &object_data, &p_object_position_find->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu,
                                &text_message_block,
                                p_object_position_find,
                                &p_object_position_find->skel_rect,
                                &object_data);

        if(status_ok(status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block)))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_position_set, P_OBJECT_POSITION_SET p_object_position_set)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_from_cell(p_docu, &object_data, &p_object_position_set->object_data)))
    {
        p_object_position_set->object_data = object_data;

        status = object_call_id(OBJECT_ID_STORY, p_docu, t5_message, p_object_position_set);
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_read_text, P_OBJECT_READ_TEXT p_object_read_text)
{
    STATUS status;
    OBJECT_DATA object_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_read_text->object_data)))
    {
        const P_CELLS_INSTANCE_DATA p_cells_instance = p_object_instance_data_CELLS(p_docu);

        status = quick_ublock_ustr_add(p_object_read_text->p_quick_ublock, array_ustr(&p_cells_instance->h_text));
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_string_replace, P_OBJECT_STRING_REPLACE p_object_string_replace)
{
    STATUS status;
    OBJECT_DATA object_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_object_string_replace->object_data)))
    {
        PC_USTR_INLINE ustr_inline = object_data.u.ustr_inline;
        S32 start, end;
        S32 case_1, case_2;

        offsets_from_object_data(&start, &end, &object_data, ustr_inline_strlen(ustr_inline));

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

        {
        OBJECT_DELETE_SUB object_delete_sub;
        object_delete_sub.object_data = object_data;
        object_delete_sub.save_data = 0;
        status_assert(object_cells_edit(p_docu, T5_MSG_OBJECT_DELETE_SUB, &object_delete_sub));
        } /*block*/

        { /* convert replace data */
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
        quick_ublock_with_buffer_setup(quick_ublock);

        status_assert(ustr_inline_replace_convert(&quick_ublock,
                                                  p_object_string_replace->p_quick_ublock,
                                                  case_1,
                                                  case_2,
                                                  p_object_string_replace->copy_capitals));

        /* insert replace data */
        status = text_insert_sub_redisplay(p_docu,
                                           &p_object_string_replace->object_position_after,
                                           &object_data,
                                           &quick_ublock);

        quick_ublock_dispose(&quick_ublock);
        } /*block*/
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_object_string_search, _InRef_ P_OBJECT_STRING_SEARCH p_object_string_search)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_from_cell(p_docu, &object_data, &p_object_string_search->object_data)))
    {
        OBJECT_STRING_SEARCH object_string_search = *p_object_string_search;
        object_string_search.object_data = object_data;
        status = object_call_id(OBJECT_ID_STORY, p_docu, t5_message, &object_string_search);
        p_object_string_search->object_position_found_start = object_string_search.object_position_found_start;
        p_object_string_search->object_position_found_end = object_string_search.object_position_found_end;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_load_cell_ownform, P_LOAD_CELL_OWNFORM p_load_cell_ownform)
{
    STATUS status;
    OBJECT_DATA object_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_ok(status = text_from_cell(p_docu, &object_data, &p_load_cell_ownform->object_data)))
    {   /* SKS 11apr97 was status_done() but that's silly and I claim it'd never been thru this bit before */
        /* check the data type */
        switch(p_load_cell_ownform->data_type)
        {
        case OWNFORM_DATA_TYPE_TEXT:
        case OWNFORM_DATA_TYPE_DATE:
        case OWNFORM_DATA_TYPE_CONSTANT:
        case OWNFORM_DATA_TYPE_FORMULA:
        case OWNFORM_DATA_TYPE_ARRAY:
            break;

        case OWNFORM_DATA_TYPE_DRAWFILE:
        case OWNFORM_DATA_TYPE_OWNER:
            return(STATUS_FAIL);
        }

        {
        PC_USTR_INLINE ustr_inline =
            p_load_cell_ownform->ustr_inline_contents
            ? p_load_cell_ownform->ustr_inline_contents
            : (PC_USTR_INLINE) p_load_cell_ownform->ustr_formula;

        if(NULL != ustr_inline)
        {
            QUICK_UBLOCK quick_ublock;
            quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr_inline, ustr_inline_strlen32(ustr_inline));
            status = text_insert_sub_redisplay(p_docu,
                                               &object_data.object_position_end,
                                               &object_data,
                                               &quick_ublock);
            /* NB no quick_ublock_dispose() */
        }

        p_load_cell_ownform->processed = 1;
        } /*block*/

        if(status_ok(status))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_save_cell_ownform, P_SAVE_CELL_OWNFORM p_save_cell_ownform)
{
    STATUS status;
    OBJECT_DATA object_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_save_cell_ownform->object_data)))
    {
        PC_USTR_INLINE ustr_inline = object_data.u.ustr_inline;

        if(P_DATA_NONE != ustr_inline)
        {
            S32 save_start, save_end;

            offsets_from_object_data(&save_start, &save_end, &object_data, ustr_inline_strlen(ustr_inline));

            if(status_ok(status = quick_ublock_uchars_add(&p_save_cell_ownform->contents_data_quick_ublock, uchars_AddBytes(ustr_inline, save_start), save_end - save_start)))
                /* Tell caller what type of data we have just saved */
                p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_TEXT;
        }

        if(status_ok(status))
            status = STATUS_DONE;
    }

    return(status);
}

T5_MSG_PROTO(static, cells_edit_msg_tab_wanted, P_TAB_WANTED p_tab_wanted)
{
    STATUS status;
    OBJECT_DATA object_data;

    if(status_done(status = text_in_cell(p_docu, &object_data, &p_tab_wanted->object_data)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;

        text_message_block_init(p_docu,
                                &text_message_block,
                                p_tab_wanted,
                                NULL,
                                &object_data);

        if(status_ok(status = object_call_STORY_with_tmb(p_docu, t5_message, &text_message_block)))
            status = STATUS_DONE;
    }

    return(status);
}

/* STATUS_DONE indicates message processed */

OBJECT_PROTO(extern, object_cells_edit)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(cells_edit_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

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
        return(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, p_data));

    case T5_EVENT_REDRAW:
        return(cells_edit_event_redraw(p_docu, t5_message, (P_OBJECT_REDRAW) p_data));

    case T5_MSG_OBJECT_HOW_BIG:
        return(cells_edit_msg_object_how_big(p_docu, t5_message, (P_OBJECT_HOW_BIG) p_data));

    case T5_MSG_OBJECT_HOW_WIDE:
        return(cells_edit_msg_object_how_wide(p_docu, t5_message, (P_OBJECT_HOW_WIDE) p_data));

    case T5_MSG_OBJECT_KEYS:
        return(cells_edit_msg_object_keys(p_docu, t5_message, (P_OBJECT_KEYS) p_data));

    case T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT:
    case T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION:
        return(cells_edit_msg_object_position_find(p_docu, t5_message, (P_OBJECT_POSITION_FIND) p_data));

    case T5_MSG_OBJECT_POSITION_SET:
        return(cells_edit_msg_object_position_set(p_docu, t5_message, (P_OBJECT_POSITION_SET) p_data));

    case T5_MSG_OBJECT_LOGICAL_MOVE:
        return(cells_edit_msg_object_logical_move(p_docu, t5_message, (P_OBJECT_LOGICAL_MOVE) p_data));

    case T5_MSG_OBJECT_DELETE_SUB:
        return(cells_edit_msg_object_delete_sub(p_docu, t5_message, (P_OBJECT_DELETE_SUB) p_data));

    case T5_MSG_OBJECT_READ_TEXT:
        return(cells_edit_msg_object_read_text(p_docu, t5_message, (P_OBJECT_READ_TEXT) p_data));

    case T5_MSG_TAB_WANTED:
        return(cells_edit_msg_tab_wanted(p_docu, t5_message, (P_TAB_WANTED) p_data));

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:
        return(cells_edit_cmd_setc_do(p_docu, t5_message, (P_OBJECT_SET_CASE) p_data));

    case T5_MSG_LOAD_CELL_OWNFORM:
    case T5_MSG_LOAD_FRAG_OWNFORM:
        return(cells_edit_msg_load_cell_ownform(p_docu, t5_message, (P_LOAD_CELL_OWNFORM) p_data));

    case T5_MSG_SAVE_CELL_OWNFORM:
        return(cells_edit_msg_save_cell_ownform(p_docu, t5_message, (P_SAVE_CELL_OWNFORM) p_data));

    case T5_MSG_OBJECT_STRING_SEARCH:
        return(cells_edit_msg_object_string_search(p_docu, t5_message, (P_OBJECT_STRING_SEARCH) p_data));

    case T5_MSG_OBJECT_STRING_REPLACE:
        return(cells_edit_msg_object_string_replace(p_docu, t5_message, (P_OBJECT_STRING_REPLACE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ce_edit.c */
