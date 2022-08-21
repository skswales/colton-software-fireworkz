/* ed_rec.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

_Check_return_
static STATUS
text_in_card(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data_out,
    P_OBJECT_DATA p_object_data_in);

/******************************************************************************

 Set up a text message block

******************************************************************************/

extern void
rec_text_message_block_init(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_MESSAGE_BLOCK p_text_message_block,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    P_OBJECT_DATA p_object_data)
{
    SLR slr;

    if(status_fail(rec_slr_from_data_ref(p_docu, &p_object_data->data_ref, &slr)))
    {
        myassert1(TEXT("Can't make SLR from this data_ref ") S32_TFMT, p_object_data->data_ref.data_space);
        slr.row = 0;
        slr.col = 0;
    }

    p_text_message_block->p_data = p_data;

    p_text_message_block->inline_object.object_data = *p_object_data;

/* *** Ensure that the object position caries enought guff with it *** */
    p_text_message_block->inline_object.object_data.object_position_start.object_id = OBJECT_ID_REC;
    set_rec_object_position_from_data_ref(p_docu, &p_text_message_block->inline_object.object_data.object_position_start, &p_object_data->data_ref);

/* ***                                                             *** */

    if(P_DATA_NONE != p_text_message_block->inline_object.object_data.u.p_object)
        p_text_message_block->inline_object.inline_len = ustr_inline_strlen(p_text_message_block->inline_object.object_data.u.ustr_inline);
    else
        p_text_message_block->inline_object.inline_len = 0;

    /* Set up some style */
    rec_style_for_data_ref(p_docu, &p_object_data->data_ref, &p_text_message_block->text_format_info.style_text_global, &style_selector_para_text);

    p_text_message_block->text_format_info.h_style_list                     = p_docu->h_style_docu_area;
    p_text_message_block->text_format_info.paginate                         = 0;

    p_text_message_block->text_format_info.skel_rect_object                 = *p_skel_rect;

    p_text_message_block->text_format_info.text_area_border_y               = 2 * p_docu->page_def.grid_size;

    p_text_message_block->text_format_info.skel_rect_work.tl.page_num.x     = 0;
    p_text_message_block->text_format_info.skel_rect_work.tl.page_num.y     = 0;
    p_text_message_block->text_format_info.skel_rect_work.br.page_num.x     = 0;
    p_text_message_block->text_format_info.skel_rect_work.br.page_num.y     = 0;

    p_text_message_block->text_format_info.skel_rect_work.tl.pixit_point.x  = 0;
    p_text_message_block->text_format_info.skel_rect_work.tl.pixit_point.y  = 0;
    p_text_message_block->text_format_info.skel_rect_work.br.pixit_point.x  = p_docu->page_def.cells_usable_x + 2 * p_docu->page_def.grid_size;
    p_text_message_block->text_format_info.skel_rect_work.br.pixit_point.y  = page_height_from_row(p_docu, slr.row) + p_text_message_block->text_format_info.text_area_border_y;

    p_text_message_block->text_format_info.object_visible = 1;
}

static void
text_inline_redisplay_init(
    P_TEXT_INLINE_REDISPLAY p_text_inline_redisplay,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     BOOL do_redraw)
{
    p_text_inline_redisplay->redraw_tag = UPDATE_PANE_CELLS_AREA;
    p_text_inline_redisplay->p_quick_ublock = p_quick_ublock;
    p_text_inline_redisplay->do_redraw = do_redraw;
    p_text_inline_redisplay->redraw_done = FALSE;
    p_text_inline_redisplay->size_changed = TRUE;
    p_text_inline_redisplay->do_position_update = TRUE;
    p_text_inline_redisplay->skel_rect_para_after  = *p_skel_rect;
    p_text_inline_redisplay->skel_rect_para_before = *p_skel_rect;
    p_text_inline_redisplay->skel_rect_text_before = *p_skel_rect;
}

/******************************************************************************
*
* text insert with redisplay
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
card_insert_sub_redisplay(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_POSITION p_object_position_after,
    P_OBJECT_DATA p_object_data,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     BOOL redisplay)
{
    P_REC_PROJECTOR p_rec_projector;
    STATUS status = STATUS_OK;
    TEXT_MESSAGE_BLOCK text_message_block;
    TEXT_INLINE_REDISPLAY text_inline_redisplay;
    SKEL_RECT skel_rect_object;
    S32 ins_size = quick_ublock_bytes(p_quick_ublock);

    if(!ins_size)
        return(STATUS_OK);

    p_rec_projector = p_rec_projector_from_data_ref(p_docu, &p_object_data->data_ref);

    status_return(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_object, &p_object_data->data_ref));

    rec_text_message_block_init(p_docu, &text_message_block, &text_inline_redisplay, &skel_rect_object, p_object_data);

    text_inline_redisplay_init(&text_inline_redisplay, p_quick_ublock, &text_message_block.text_format_info.skel_rect_object, TRUE);

    {
    ARRAY_HANDLE h_text = 0;
    S32 modified;

    rec_cache_enquire(p_docu, &text_message_block.inline_object.object_data.data_ref, &h_text, &modified);

    if(NULL != al_array_extend_by_U8(&h_text, ins_size + (text_message_block.inline_object.inline_len ? 0 : 1), &array_init_block_u8, &status))
    {
        /* add the data to the cache, or update if already there! */
        status = rec_cache_update(p_docu, &text_message_block.inline_object.object_data.data_ref, &h_text, 1);

        /* make sure altered pointer is fed back */
        text_message_block.inline_object.object_data.u.ustr_inline = ustr_inline_from_h_ustr(&h_text);
    }
    } /*block*/

    if(status_ok(status))
    {
        text_message_block.inline_object.object_data.object_id = OBJECT_ID_REC;

        if(!redisplay)
            text_message_block.text_format_info.object_visible = 0;

        status = object_call_id(OBJECT_ID_STORY, p_docu, T5_MSG_INSERT_INLINE_REDISPLAY, &text_message_block);
    }

    if(status_ok(status))
        *p_object_position_after = text_message_block.inline_object.object_data.object_position_end;

    return(status);
}

/******************************************************************************
*
* text delete with redisplay
*
******************************************************************************/

_Check_return_
static STATUS
card_delete_sub_redisplay(
    P_REC_PROJECTOR p_rec_projector,
    P_OBJECT_DELETE_SUB p_object_delete_sub,
    P_OBJECT_DATA p_object_data)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    TEXT_MESSAGE_BLOCK text_message_block;
    TEXT_INLINE_REDISPLAY text_inline_redisplay;
    S32 start, end, del_size;
    SKEL_RECT skel_rect_object;
    STATUS status = STATUS_OK;

    status_return(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_object, &p_object_data->data_ref));

    rec_text_message_block_init(p_docu, &text_message_block, &text_inline_redisplay, &skel_rect_object, p_object_data);

    text_inline_redisplay_init(&text_inline_redisplay, NULL, &text_message_block.text_format_info.skel_rect_object, TRUE);

    offsets_from_object_data(&start, &end, &text_message_block.inline_object.object_data, text_message_block.inline_object.inline_len);

    del_size = end - start;

    if(del_size && status_ok(status = object_call_id(OBJECT_ID_STORY, p_docu, T5_MSG_DELETE_INLINE_REDISPLAY, &text_message_block)))
    {
        ARRAY_HANDLE h_text;
        S32 modified;

        rec_cache_enquire(p_docu, &text_message_block.inline_object.object_data.data_ref, &h_text, &modified);

        al_array_shrink_by(&h_text, text_message_block.inline_object.inline_len ? -(del_size + 0) : -(del_size + 1));

        rec_cache_update(p_docu, &text_message_block.inline_object.object_data.data_ref, &h_text, 1);

        /* send back new object pointer */
        p_object_delete_sub->object_data.u.ustr_inline = ustr_inline_from_h_ustr(&h_text);
    }

    return(status);
}

_Check_return_
static STATUS
rec_edit_object_delete_sub(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DELETE_SUB p_object_delete_sub)
{
    STATUS status;
    DATA_REF data_ref;
    OBJECT_DATA object_data;
    OBJECT_DATA object_data_rec = p_object_delete_sub->object_data;

    if(DATA_SLOT == object_data_rec.data_ref.data_space)
    {
        if(!rec_data_ref_from_slr(p_docu, &object_data_rec.data_ref.arg.slr, &data_ref))
        {
            assert0();
            return(STATUS_FAIL);
        }

#if CHECKING
        myassert1x(OBJECT_ID_REC == object_data_rec.object_id, TEXT("Object position data NOT OBJECT_ID_REC") S32_TFMT, object_data_rec.object_id);
        { S32 fieldnumber = get_rec_object_position_field_number(&object_data_rec.object_position_start);
        myassert1x(fieldnumber != (-1), TEXT("field number invalid ") S32_TFMT, fieldnumber); } /*block*/
#endif

        set_rec_data_ref_from_object_position(p_docu, &data_ref, &object_data_rec.object_position_start);
        object_data_rec.data_ref = data_ref;
    }
    else
        data_ref = object_data_rec.data_ref;

    assert((data_ref.data_space == DATA_DB_FIELD) || (data_ref.data_space == DATA_DB_TITLE));

    p_object_delete_sub->h_data_del = 0;

    if(status_done(status = text_in_card(p_docu, &object_data, &object_data_rec)))
    {
        P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_data_ref(p_docu, &object_data.data_ref);
        status = card_delete_sub_redisplay(p_rec_projector, p_object_delete_sub, &object_data);
        set_rec_object_position_from_data_ref(p_docu, &p_object_delete_sub->object_data.object_position_end,   &data_ref);
        set_rec_object_position_from_data_ref(p_docu, &p_object_delete_sub->object_data.object_position_start, &data_ref);
    }

    status_assert(status);
    return(STATUS_DONE);
}

/* Fill the text cache from the card

   This fetches the text from a card
   unless the cache holds the required text (modified or original)
   as indicated by the associated data_ref.

   The actual fetch was performed via the object_call_id() with the OBJECT_READ_TEXT message. - Hence it cannot work!
   This was a good mechanism for the ce_edit stuff because it dealt with a number of clients (including ob_rec).

   However:

   ob_rec is a ce_edit client if_and_only_if for sheet-views where OBEJCT_READ_TEXT returns the text of a field-in-a-cell.
   This message gets rejected in the card-views so that ce_edit concludes that it cannot cope.

   Fortunately:

   We can choose to assume an ob_rec(card-view) client.
*/

_Check_return_
extern STATUS
rec_text_from_card(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data_out,
    P_OBJECT_DATA p_object_data_in)
{
    STATUS status = STATUS_OK;
    ARRAY_HANDLE_USTR h_text_ustr = 0;
    S32 modified;

    assert( (p_object_data_in->data_ref.data_space == DATA_DB_FIELD) || (p_object_data_in->data_ref.data_space == DATA_DB_TITLE) );

    *p_object_data_out = *p_object_data_in;

    /* Examine the buffer state, see if there is buffered data for this data ref */
    status = rec_cache_enquire(p_docu, &p_object_data_in->data_ref, &h_text_ustr, &modified);

    /* At this point STATUS_DONE means already got it in the cache, STATUS_OK means go and fetch it please */

    if(status == STATUS_OK)
    {
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
        quick_ublock_with_buffer_setup(quick_ublock);

        /* Go fetch it */
        if(status_ok(status = rec_object_read_card(p_docu, &p_object_data_in->data_ref, &quick_ublock)))
        {
            if(0 != quick_ublock_bytes(&quick_ublock))
            {
                OBJECT_POSITION object_position_after;

                quick_ublock_nullch_add(&quick_ublock);

                p_object_data_out->object_id = OBJECT_ID_NONE;
                p_object_data_out->u.p_object = P_DATA_NONE;

                if(status_ok(status = card_insert_sub_redisplay(p_docu,
                                                                &object_position_after,
                                                                p_object_data_out,
                                                                &quick_ublock,
                                                                FALSE)))
                {
                    S32 modified;

                    if(status_done(status = rec_cache_enquire(p_docu, &p_object_data_out->data_ref, &h_text_ustr, &modified)))
                        rec_cache_modified(p_docu, &p_object_data_out->data_ref, 0);
                    else
                        h_text_ustr = 0;
                }
            }
        }

        quick_ublock_dispose(&quick_ublock);
    } /* endif need to fetch */

    if(status_done(status))
    {
        p_object_data_out->object_id = OBJECT_ID_REC;
        p_object_data_out->u.ustr_inline = ustr_inline_from_h_ustr(&h_text_ustr);
    }
    else
        p_object_data_out->u.p_object = P_DATA_NONE;

    return(status);
}

/******************************************************************************
*
* is the text for this card in the cache
*
******************************************************************************/

_Check_return_
static STATUS
text_in_card(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data_out,
    P_OBJECT_DATA p_object_data_in)
{
    STATUS status = STATUS_OK;
    ARRAY_HANDLE h_text = 0;
    S32 modified;

    /* Is it in the cache */
    if(status_done(status = rec_cache_enquire(p_docu, &p_object_data_in->data_ref, &h_text, &modified) ))
    {
        *p_object_data_out = *p_object_data_in;
        p_object_data_out->object_id = OBJECT_ID_REC;
        p_object_data_out->u.ustr_inline = ustr_inline_from_h_ustr(&h_text);
    }

  return(status);
}

_Check_return_
static STATUS
rec_new_object_from_text(
    _DocuRef_   P_DOCU p_docu,
    P_NEW_OBJECT_FROM_TEXT p_new_object_from_text)
{
    DATA_REF data_ref = p_new_object_from_text->data_ref;

    if(DATA_SLOT == p_new_object_from_text->data_ref.data_space)
        if(!rec_data_ref_from_slr(p_docu, &p_new_object_from_text->data_ref.arg.slr, &data_ref))
            return(STATUS_FAIL);

    switch(data_ref.data_space)
    {
    case DATA_DB_TITLE:
    case DATA_DB_FIELD:
        return(rec_object_write_text(p_docu, &data_ref, p_new_object_from_text->p_quick_ublock, FILETYPE_TEXT));

    default:
        myassert1x((data_ref.data_space == DATA_DB_FIELD) || (data_ref.data_space == DATA_DB_TITLE), TEXT("Bad DATA_REF ") S32_TFMT, data_ref.data_space);
        return(STATUS_FAIL);
    }
}

/******************************************************************************
*
* rec_object_read_card
*
* similar to rec_object_read_text
* used for card-editing
*
******************************************************************************/

_Check_return_
extern STATUS
rec_object_read_card(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)
{
    P_REC_PROJECTOR p_rec_projector;
    STATUS status;

    if(NULL == (p_rec_projector = p_rec_projector_from_data_ref(p_docu, p_data_ref)))
        return(STATUS_FAIL);

    if(DATA_DB_TITLE == p_data_ref->data_space)
    {
        P_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&p_rec_projector->h_rec_frames, p_data_ref->arg.db_title.field_id);
        return(quick_ublock_ustr_add(p_quick_ublock, array_ustr(&p_rec_frame->h_title_text_ustr)));
    }

    {
    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_data_ref->arg.db_field.field_id);

    if((P_DATA_NONE == p_fielddef) || (p_fielddef->hidden))
        return(STATUS_FAIL);

    switch(p_fielddef->type)
    {
    case FIELD_FILE:
    case FIELD_PICTURE:
        return(STATUS_FAIL);

    case FIELD_TEXT:
        {
        REC_RESOURCE rec_resource;

        if(status_ok(status = rec_read_resource(&rec_resource, p_rec_projector, p_data_ref, FALSE)))
            if(status_ok(status = quick_ublock_uchars_add(p_quick_ublock, rec_resource.ev_data.arg.string.uchars, rec_resource.ev_data.arg.string.size)))
                status = STATUS_DONE;

        ss_data_free_resources(&rec_resource.ev_data);
        break;
        }

    default: /* do it with numforming */
        {
        STYLE style;
        rec_style_for_data_ref(p_docu, p_data_ref, &style, &style_selector_numform);
        status = rec_text_from_data_ref(p_quick_ublock, &style, p_rec_projector, p_data_ref);
        break;
        }
    }
    } /*block*/

    if(status_ok(status)) /* never return CH_NULL-terminated string */
        quick_ublock_nullch_strip(p_quick_ublock);

    return(status);
}

_Check_return_
static STATUS
rec_object_read_text(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_READ_TEXT p_object_read_text)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    STATUS status;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_read_text->object_data.data_ref.arg.slr, &data_ref)))
        return(STATUS_FAIL);

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
        return(STATUS_FAIL);

    {
    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, field_id_from_rec_data_ref(&data_ref));
    REC_FRAME rec_frame;

    if(P_DATA_NONE == p_fielddef)
        return(STATUS_FAIL);

    if(p_fielddef->hidden)
        return(STATUS_OK); /* return blank */

    status_return(get_frame_by_field_id(p_rec_projector, p_fielddef->id, &rec_frame));

    switch(p_fielddef->type)
    {
    case FIELD_FILE:
    case FIELD_PICTURE:
        if(OBJECT_READ_TEXT_RESULT != p_object_read_text->type)
        {
            /* you can't textually edit these */
            status = STATUS_FAIL;
            break;
        }

        return(STATUS_OK); /* return blank */

    case FIELD_TEXT:
        {
        BOOL plain = (OBJECT_READ_TEXT_RESULT == p_object_read_text->type); /* SKS 15apr96 stop inlines getting back out (see ob_story/tx_main which plains the output) */
        REC_RESOURCE rec_resource;

        if(status_ok(status = rec_read_resource(&rec_resource, p_rec_projector, &data_ref, plain)))
            if(status_ok(status = quick_ublock_uchars_add(p_object_read_text->p_quick_ublock, rec_resource.ev_data.arg.string.uchars, rec_resource.ev_data.arg.string.size)))
                status = STATUS_DONE;

        ss_data_free_resources(&rec_resource.ev_data);
        break;
        }

    case FIELD_FORMULA:
        if(OBJECT_READ_TEXT_RESULT != p_object_read_text->type)
        {
            /* you can't textually edit these */
            status = STATUS_FAIL;
            break;
        }

        /*FALLTHRU*/

    default: /* do it with numforming */
        {
        STYLE style;
        rec_style_for_data_ref(p_docu, &p_object_read_text->object_data.data_ref, &style, &style_selector_numform);
        status = rec_text_from_data_ref(p_object_read_text->p_quick_ublock, &style, p_rec_projector, &data_ref);
        break;
        }
    }
    } /*block*/

    if(status_ok(status)) /* never return CH_NULL-terminated string */
        quick_ublock_nullch_strip(p_object_read_text->p_quick_ublock);

    return(status);
}

_Check_return_
static STATUS
rec_object_set_case(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_OBJECT_SET_CASE p_object_set_case)
{
    OBJECT_DATA object_data_rec = p_object_set_case->object_data;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    SKEL_RECT skel_rect_object;
    STATUS status;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &object_data_rec.data_ref.arg.slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
        return(STATUS_FAIL);

    set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_object_set_case->object_data.object_position_start);

    if(status_ok(status = rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_object, &data_ref)))
    {
        OBJECT_SET_CASE object_set_case_mine = *p_object_set_case;
        object_data_rec.data_ref = data_ref;
        if(status_done(status = text_in_card(p_docu, &object_set_case_mine.object_data, &object_data_rec)))
        {
            TEXT_MESSAGE_BLOCK text_message_block;
            TEXT_INLINE_REDISPLAY text_inline_redisplay;
            rec_text_message_block_init(p_docu, &text_message_block, &text_inline_redisplay, &skel_rect_object, &object_set_case_mine.object_data);
            text_inline_redisplay_init(&text_inline_redisplay, NULL, &text_message_block.text_format_info.skel_rect_object, TRUE);
            status = object_call_id(OBJECT_ID_STORY, p_docu, t5_message, &text_message_block);
            rec_cache_modified(p_docu, &data_ref, 1);
        }
    }

    return(status);
}

_Check_return_
static STATUS
rec_edit_object_keys(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_KEYS p_object_keys)
{
    STATUS status = STATUS_OK;
    DATA_REF data_ref;
    OBJECT_DATA object_data;
    OBJECT_DATA object_data_rec = p_object_keys->object_data;

    assert(DATA_SLOT == p_object_keys->object_data.data_ref.data_space);

    if(rec_data_ref_from_slr(p_docu, &p_object_keys->object_data.data_ref.arg.slr, &data_ref))
    {
#if CHECKING
        myassert1x(OBJECT_ID_REC == p_object_keys->object_data.object_id, TEXT("Object position data NOT OBJECT_ID_REC") S32_TFMT, p_object_keys->object_data.object_id);
        { S32 fieldnumber = get_rec_object_position_field_number(&p_object_keys->object_data.object_position_start);
        myassert1x(fieldnumber != (-1), TEXT("field number invalid ") S32_TFMT, fieldnumber); } /*block*/
#endif

        set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_object_keys->object_data.object_position_start);

        object_data_rec.data_ref = data_ref;

        if(status_ok(status = rec_text_from_card(p_docu, &object_data, &object_data_rec)))
        {
            p_object_keys->p_skelevent_keys->processed = 1;

            status = card_insert_sub_redisplay(p_docu,
                                               &p_object_keys->object_data.object_position_end,
                                               &object_data,
                                               p_object_keys->p_skelevent_keys->p_quick_ublock,
                                               TRUE);

            set_rec_object_position_from_data_ref(p_docu, &p_object_keys->object_data.object_position_end,   &data_ref);
            set_rec_object_position_from_data_ref(p_docu, &p_object_keys->object_data.object_position_start, &data_ref);
        }

        if(status_ok(status))
            return(STATUS_DONE);
    }

    return(STATUS_OK); /* don't fail further message processing if we couldn't do it */
}

/* Process the logical move message
   This contains :
    OBJECT_DATA object_data; (data_ref, object_id, object_position_start/end, p_object)
    S32 action;                          action required
    SKEL_EDGE edge; (pixit, page)        position after move

  Subcontracts in-field movements to ob_story

  Subcontracts the lot for sheets ?

  Only process UP/DOWN messages

  hacked by MRJC 28.1.95 to work!

*/

_Check_return_
static STATUS
rec_edit_object_logical_move(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_OBJECT_LOGICAL_MOVE p_object_logical_move)
{
    STATUS status = STATUS_FAIL;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    S32 fieldnumber, fieldtarget;
    OBJECT_DATA object_data_rec= p_object_logical_move->object_data;
    DATA_SPACE data_space;
    S32 delta = 1 ; /* move DOWN */
    SKEL_RECT skel_rect;

    if(OBJECT_ID_REC != p_object_logical_move->object_data.object_id)
        return(STATUS_FAIL);

    myassert1x(p_object_logical_move->object_data.data_ref.data_space == DATA_SLOT, TEXT("Not DATA_SLOT ") S32_TFMT, p_object_logical_move->object_data.data_ref.data_space);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_logical_move->object_data.data_ref.arg.slr, &data_ref)))
        return(STATUS_FAIL);

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
        return(STATUS_FAIL);

    switch(p_object_logical_move->action)
    {
    case OBJECT_LOGICAL_MOVE_UP:
        delta = -1 ; /* ie move up */

        /*FALLTHRU*/

    case OBJECT_LOGICAL_MOVE_DOWN:
        if(OBJECT_ID_REC != object_data_rec.object_position_start.object_id)
            status = STATUS_FAIL;
        else
        { /* It is possible to arrive here with object_position_start set to object_id == OBJECT_ID_REC but more_data == -1 ie unknown */
            data_space = get_rec_object_position_data_space(&object_data_rec.object_position_start);

            if((data_space == DATA_DB_FIELD) || (data_space == DATA_DB_TITLE))
                set_rec_data_ref_from_object_position(p_docu, &data_ref, &object_data_rec.object_position_start);
            /* else Just assume the first field (which is what we've got anyway ! ) */
            object_data_rec.data_ref = data_ref;

            p_rec_projector = p_rec_projector_from_data_ref(p_docu, &data_ref);

            {
            OBJECT_LOGICAL_MOVE object_logical_move_mine = *p_object_logical_move;
            if(status_ok(status = rec_text_from_card(p_docu, &object_logical_move_mine.object_data, &object_data_rec)))
            {
                TEXT_MESSAGE_BLOCK text_message_block;
                SKEL_RECT skel_rect_object;
                rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_object, &data_ref);

                rec_text_message_block_init(p_docu, &text_message_block, &object_logical_move_mine, &skel_rect_object, &object_logical_move_mine.object_data);

                if(text_message_block.inline_object.inline_len == 0)
                    status = STATUS_FAIL;
                else
                {
                    if(status_ok(status = object_call_id(OBJECT_ID_STORY, p_docu, t5_message, &text_message_block)))
                    { /* Ob_story has done it for us */
                        p_object_logical_move->skel_point_out = object_logical_move_mine.skel_point_out;
                    }
                }
            }

            if(status_fail(status))
            {
                /* I guess we could try to move to the previous or next field
                   find out the field number and the number of fields
                */
                fieldnumber = get_rec_object_position_field_number(&object_data_rec.object_position_start);
                fieldtarget = fieldnumber + delta;
                p_object_logical_move->use_both_xy = TRUE;

                if((fieldtarget <= 0) || (fieldtarget > array_elements(&p_rec_projector->opendb.table.h_fielddefs)))
                    status = STATUS_FAIL;
                else
                {
                    set_rec_object_position_field_number(&object_data_rec.object_position_start, fieldtarget);
                    set_rec_data_ref_from_object_position(p_docu, &data_ref, &object_data_rec.object_position_start);

                    rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect, &data_ref);
                    if(delta < 0)
                    {
                        p_object_logical_move->skel_point_out = skel_rect.tl;
                        p_object_logical_move->skel_point_out.pixit_point.y = skel_rect.br.pixit_point.y - 1;
                    }
                    else
                        p_object_logical_move->skel_point_out = skel_rect.tl;

                    status = STATUS_OK;
                }
            }
            } /*block*/

        }

        break;

    default: default_unhandled();
#if CHECKING
    case OBJECT_LOGICAL_MOVE_LEFT:
    case OBJECT_LOGICAL_MOVE_RIGHT:
#endif
        status = STATUS_FAIL;

        if(OBJECT_ID_REC == object_data_rec.object_position_start.object_id)
        { /* It is possible to arrive here with object_position_start set to object_id == OBJECT_ID_REC but more_data == -1 ie unknown */
            data_space = get_rec_object_position_data_space(&object_data_rec.object_position_start);
            if(data_space == DATA_DB_FIELD || data_space ==DATA_DB_TITLE)
                set_rec_data_ref_from_object_position(p_docu, &data_ref, &object_data_rec.object_position_start);
            /* else Just assume the first field (which is what we've got anyway ! ) */

            object_data_rec.data_ref = data_ref;

            p_rec_projector = p_rec_projector_from_data_ref(p_docu, &data_ref);

            {
            OBJECT_LOGICAL_MOVE object_logical_move_mine = *p_object_logical_move;
            if(status_ok(status = rec_text_from_card(p_docu, &object_logical_move_mine.object_data, &object_data_rec)))
            {
                TEXT_MESSAGE_BLOCK text_message_block;
                SKEL_RECT skel_rect_object;
                rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_object, &data_ref);
                rec_text_message_block_init(p_docu, &text_message_block, &object_logical_move_mine, &skel_rect_object, &object_logical_move_mine.object_data);
                if(status_ok(status = object_call_id(OBJECT_ID_STORY, p_docu, t5_message, &text_message_block)))
                    /* Ob_story has done it for us */
                    p_object_logical_move->skel_point_out = object_logical_move_mine.skel_point_out;
            }
            } /*block*/
        }

        break;
    }

    return(status);
}

/* STATUS_FAIL means pass the buck to ce_edit */

T5_MSG_PROTO(static, rec_edit_object_string_search, P_OBJECT_STRING_SEARCH p_object_string_search)
{
    STATUS status = STATUS_OK;

    if(OBJECT_ID_REC == p_object_string_search->object_data.object_id)
    {
        OBJECT_DATA object_data_rec = p_object_string_search->object_data;
        OBJECT_DATA object_data;
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;
        myassert1x(p_object_string_search->object_data.data_ref.data_space == DATA_SLOT, TEXT("Not DATA_SLOT ") S32_TFMT, p_object_string_search->object_data.data_ref.data_space);

        if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_string_search->object_data.data_ref.arg.slr, &data_ref)))
            return(STATUS_FAIL);

        if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
            return(STATUS_FAIL);

        if(OBJECT_ID_REC == p_docu->cur.object_position.object_id)
            set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_docu->cur.object_position);

        object_data_rec.data_ref = data_ref; /* Make it one of our data_refs */

        /* Try to read some text */

        if(status_done(status = rec_text_from_card(p_docu, &object_data, &object_data_rec)))
        {
            OBJECT_STRING_SEARCH object_string_search = *p_object_string_search;
            object_string_search.object_data = object_data;

            status = object_call_id(OBJECT_ID_STORY, p_docu, t5_message, &object_string_search);

            if(status_done(status))
            {
                set_rec_object_position_from_data_ref(p_docu, &object_string_search.object_position_found_start, &data_ref);
                set_rec_object_position_from_data_ref(p_docu, &object_string_search.object_position_found_end,   &data_ref);
            }

            p_object_string_search->object_position_found_start = object_string_search.object_position_found_start;
            p_object_string_search->object_position_found_end   = object_string_search.object_position_found_end;

            myassert1x(p_object_string_search->object_data.data_ref.data_space == DATA_SLOT, TEXT("Not DATA_SLOT ") S32_TFMT, p_object_string_search->object_data.data_ref.data_space);
        }
    }
    else
        myassert0(TEXT("ed_rec caught T5_MSG_OBJECT_STRING_SEARCH for bad object data"));

    return(status);
}

T5_MSG_PROTO(static, rec_edit_object_string_replace, P_OBJECT_STRING_REPLACE p_object_string_replace)
{
    STATUS status = STATUS_OK;

    IGNOREPARM_InVal_(t5_message);

    if(OBJECT_ID_REC == p_object_string_replace->object_data.object_id)
    {
        OBJECT_DATA object_data_rec = p_object_string_replace->object_data;
        OBJECT_DATA object_data;
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;

        myassert1x(p_object_string_replace->object_data.data_ref.data_space == DATA_SLOT, TEXT("Not DATA_SLOT ") S32_TFMT, p_object_string_replace->object_data.data_ref.data_space);

        if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_string_replace->object_data.data_ref.arg.slr, &data_ref)))
            return(STATUS_FAIL);

        if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
            return(STATUS_FAIL);

        set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_object_string_replace->object_data.object_position_start);

        object_data_rec.data_ref = data_ref; /* Make it one of our data_refs */

        if(status_done(status = text_in_card(p_docu, &object_data, &object_data_rec)))
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
                    UCS4 ucs4 = uchars_char_decode_off(ustr_inline, start, /*ref*/bytes_of_char_start);
                    case_1 = t5_ucs4_is_uppercase(ucs4);
                }

                if((end - start) > (S32) bytes_of_char_start)
                {
                    UCS4 ucs4 = uchars_char_decode_off_NULL(ustr_inline, start + bytes_of_char_start);
                    case_2 = t5_ucs4_is_uppercase(ucs4);
                }
            }

            {
            OBJECT_DELETE_SUB object_delete_sub;
            object_delete_sub.object_data = object_data;
            object_delete_sub.save_data = 0;
            rec_edit_object_delete_sub(p_docu, &object_delete_sub);
            } /*block*/

            { /* convert & insert replace data */
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
            quick_ublock_with_buffer_setup(quick_ublock);

            status_assert(ustr_inline_replace_convert(&quick_ublock, p_object_string_replace->p_quick_ublock, case_1, case_2, p_object_string_replace->copy_capitals));

            status = card_insert_sub_redisplay(p_docu,
                                               &p_object_string_replace->object_position_after,
                                               &object_data,
                                               &quick_ublock,
                                               TRUE);

            quick_ublock_dispose(&quick_ublock);
            } /*block*/
        }
    }
    else
        myassert0(TEXT("ed_rec caught T5_MSG_OBJECT_STRING_REPLACE for bad object data"));

    return(status);
}

PROC_EVENT_PROTO(extern, proc_event_ed_rec_direct)
{
    STATUS status = STATUS_OK;

    switch(t5_message)
    {
    case T5_MSG_LOAD_CELL_OWNFORM:
    case T5_MSG_LOAD_FRAG_OWNFORM:
        {
        P_LOAD_CELL_OWNFORM p_load_cell_ownform = (P_LOAD_CELL_OWNFORM) p_data;
        OBJECT_DATA object_data_rec = p_load_cell_ownform->object_data;
        OBJECT_DATA object_data;
        P_REC_PROJECTOR p_rec_projector;

        if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_load_cell_ownform->object_data.data_ref.arg.slr, &object_data_rec.data_ref)))
            return(create_error(REC_ERR_NO_DATABASE));

        IGNOREPARM(p_rec_projector);

        set_rec_data_ref_from_object_position(p_docu, &object_data_rec.data_ref, &object_data_rec.object_position_start);

        if(status_done(status = rec_text_from_card(p_docu, &object_data, &object_data_rec)))
        {
            PC_USTR_INLINE ustr_inline =
                p_load_cell_ownform->ustr_inline_contents
                    ? p_load_cell_ownform->ustr_inline_contents
                    : (PC_USTR_INLINE) p_load_cell_ownform->ustr_formula;

            /* check the data type */
            switch(p_load_cell_ownform->data_type)
            {
            default: default_unhandled();
#if CHECKING
            case OWNFORM_DATA_TYPE_TEXT:
            case OWNFORM_DATA_TYPE_DATE:
            case OWNFORM_DATA_TYPE_CONSTANT:
            case OWNFORM_DATA_TYPE_FORMULA:
            case OWNFORM_DATA_TYPE_ARRAY:
#endif
                break;

            case OWNFORM_DATA_TYPE_DRAWFILE:
            case OWNFORM_DATA_TYPE_OWNER:
                return(STATUS_FAIL);
            }

            if(NULL != ustr_inline)
            {
                QUICK_UBLOCK quick_ublock;
                quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr_inline, ustr_inline_strlen32(ustr_inline));
                status = card_insert_sub_redisplay(p_docu,
                                                   &object_data.object_position_end,
                                                   &object_data,
                                                   &quick_ublock,
                                                   TRUE);
                /* NB no quick_ublock_dispose() */
            }

            p_load_cell_ownform->processed = 1;

            if(status_ok(status))
                status = STATUS_DONE;
        }

        break;
        }

    case T5_MSG_SAVE_CELL_OWNFORM:
        {
        P_SAVE_CELL_OWNFORM p_save_cell_ownform = (P_SAVE_CELL_OWNFORM) p_data;
        P_OF_OP_FORMAT p_of_op_format = p_save_cell_ownform->p_of_op_format;
        assert(p_of_op_format);

        /* SKS 20apr95 save cell data out only if we don't save out the DB constructs */
        /* SKS 14apr96 change condition so that CSV saving can abuse this call */
        if(!p_of_op_format->saved_database_constructs) /* sadly needed here */
        /*used to be ... if(p_of_op_format->of_template.data_class < DATA_SAVE_WHOLE_DOC)*/
        {
            OBJECT_DATA object_data_rec = p_save_cell_ownform->object_data;
            OBJECT_DATA object_data;
            P_REC_PROJECTOR p_rec_projector;

            if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_save_cell_ownform->object_data.data_ref.arg.slr, &object_data_rec.data_ref)))
                return(create_error(REC_ERR_NO_DATABASE));

            if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
            {
                set_rec_data_ref_from_object_position(p_docu, &object_data_rec.data_ref, &object_data_rec.object_position_start);

                if(status_done(status = rec_text_from_card(p_docu, &object_data, &object_data_rec)))
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
            }

            if(!status_done(status))
            {
                OBJECT_READ_TEXT object_read_text;
                object_read_text.object_data = p_save_cell_ownform->object_data;
                object_read_text.p_quick_ublock = &p_save_cell_ownform->contents_data_quick_ublock;
                object_read_text.type = OBJECT_READ_TEXT_RESULT;
                status = proc_event_ed_rec_direct(p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text);
            }
        }

        break;
        }

    case T5_MSG_OBJECT_STRING_SEARCH:
        return(rec_edit_object_string_search(p_docu, t5_message, p_data));

    case T5_MSG_OBJECT_STRING_REPLACE:
        return(rec_edit_object_string_replace(p_docu, t5_message, p_data));

    /* Used for in-sheet editing */

    case T5_MSG_NEW_OBJECT_FROM_TEXT: /* This message implies replace the edited text to the database */
        return(rec_new_object_from_text(p_docu, p_data));

    case T5_MSG_OBJECT_READ_TEXT: /* This message implies return the textual contents of the thing refered to */
        return(rec_object_read_text(p_docu, p_data));

    /* end of in-sheet editing */

    /* Used for in-card editing */

    case T5_MSG_OBJECT_KEYS:
        return(rec_edit_object_keys(p_docu, p_data));

    case T5_MSG_OBJECT_LOGICAL_MOVE:
        return(rec_edit_object_logical_move(p_docu, t5_message, p_data));

    case T5_MSG_OBJECT_DELETE_SUB:
        return(rec_edit_object_delete_sub(p_docu, p_data));

   case T5_EVENT_CLICK_LEFT_SINGLE:
        return(rec_card_design_single_click(p_docu, p_data));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
        return(rec_card_design_double_click(p_docu, p_data));

    case T5_EVENT_CLICK_LEFT_DRAG:
        return(rec_card_design_drag(p_docu, p_data));

    case T5_EVENT_CLICK_DRAG_STARTED:
        return(rec_card_design_dragging_started(p_docu, p_data));

    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        return(rec_card_design_dragging_movement(p_docu, p_data));

    case T5_EVENT_CLICK_DRAG_FINISHED:
        return(rec_card_design_dragging_finished(p_docu, p_data));

    case T5_EVENT_CLICK_DRAG_ABORTED:
        return(rec_card_design_dragging_aborted(p_docu, p_data));

    /* >>>MRJC */
    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:
        return(rec_object_set_case(p_docu, t5_message, p_data));

    /* >>>MRJC */
    case T5_MSG_FIELD_NEXT:
    case T5_MSG_FIELD_PREV:
        {
        P_OBJECT_DATA p_object_data = (P_OBJECT_DATA) p_data;
        OBJECT_DATA object_data_rec = *p_object_data;
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;
        S32 fieldnumber;
        S32 fieldtarget;

        assert(OBJECT_ID_REC == p_object_data->object_position_start.object_id);

        if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_data->data_ref.arg.slr, &data_ref)))
            return(create_error(REC_ERR_NO_DATABASE));

        set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_object_data->object_position_start);

        fieldnumber = get_rec_object_position_field_number(&object_data_rec.object_position_start);

        switch(t5_message)
        {
        default: default_unhandled();
#if CHECKING
        case T5_MSG_FIELD_NEXT:
#endif
            fieldtarget = fieldnumber + 1;
            break;

        case T5_MSG_FIELD_PREV:
            fieldtarget = fieldnumber - 1;
            break;
        }

        if((fieldtarget <= 0) || (fieldtarget > array_elements(&p_rec_projector->opendb.table.h_fielddefs)))
            status = STATUS_FAIL;
        else
        {
            cur_change_before(p_docu);

            set_rec_object_position_field_number(&object_data_rec.object_position_start, fieldtarget);

            {
            OBJECT_POSITION_SET object_position_set;
            object_position_set.object_data = object_data_rec;
            object_position_set.action = OBJECT_POSITION_SET_START;
            status_consume(object_call_id(OBJECT_ID_STORY, p_docu, T5_MSG_OBJECT_POSITION_SET, &object_position_set));
            p_docu->cur.object_position = object_position_set.object_data.object_position_start;
            } /*block*/

            cur_change_after(p_docu);
        }

        break;
        }
    }

    return(status);
}

_Check_return_
extern STATUS
rec_object_editable(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_EDITABLE p_object_editable)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;

    assert(p_object_editable->object_data.data_ref.data_space == DATA_SLOT);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_editable->object_data.data_ref.arg.slr, &data_ref)))
    {
        p_object_editable->editable = FALSE;
        return(STATUS_OK);
    }

    if(status_fail(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_EDIT)))
    {
        p_object_editable->editable = FALSE;
        return(STATUS_OK);
    }

    p_object_editable->editable = TRUE; /* Unless it isn't */

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
    {
        PC_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, data_ref.arg.db_field.field_id);
        p_object_editable->editable = (P_DATA_NONE != p_fielddef) ? p_fielddef->writeable : FALSE /* Attempt to edit unknown field */;
        return(STATUS_OK);
    }

    if(OBJECT_ID_REC == p_object_editable->object_data.object_position_start.object_id)
    {
        set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_object_editable->object_data.object_position_start);

        if(data_ref.data_space == DATA_DB_TITLE)
        {
            P_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&p_rec_projector->h_rec_frames, data_ref.arg.db_title.field_id);
            p_object_editable->editable = (P_DATA_NONE != p_rec_frame) ? p_rec_frame->title_show : FALSE /* Attempt to edit unknown title */;
        }
        else
        {
            PC_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, data_ref.arg.db_field.field_id);
            p_object_editable->editable = (P_DATA_NONE != p_fielddef) ? p_fielddef->writeable : FALSE /* Attempt to edit unknown field */;
        }
    }

    return(STATUS_OK);
}

/* end of ed_rec.c */
