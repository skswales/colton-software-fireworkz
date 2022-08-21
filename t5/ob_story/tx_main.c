/* tx_main.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SLR independent text routines */

/* MRJC September 1992 / August 1994 made part of ob_story */

#include "common/gflags.h"

#include "ob_story/ob_story.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

/*
internal routines
*/

_Check_return_
static S32 /* updated offset */
text_backward(
    _In_reads_(offset) PC_USTR_INLINE ustr_inline_in,
    _In_        S32 offset,
    _In_        U8 truth_table[][CHAR_N]);

_Check_return_
static S32
text_classify_char(
    /*_In_*/    PC_UCHARS_INLINE uchars_inline);

_Check_return_
static S32
text_forward(
    _In_reads_(inline_bytes) PC_USTR_INLINE ustr_inline_in,
    _In_        S32 offset,
    _In_        U8 truth_table[][CHAR_N],
    _InVal_     S32 inline_bytes);

static void
text_part_invert(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    P_FORMATTED_TEXT p_formatted_text,
    P_USTR_INLINE ustr_inline,
    _InVal_     S32 start,
    _InVal_     S32 end);

static void
text_segment_paint(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    P_FORMATTED_TEXT p_formatted_text,
    P_SEGMENT p_segment,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_SKEL_RECT p_skel_rect_seg,
    _InRef_     PC_RGB p_rgb_back,
    _In_        U8 rubout,
    _InRef_     PC_STYLE p_style_text_global);

_Check_return_
static STATUS
uchars_inline_result_convert(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n);

_Check_return_
static STATUS /* 0 == word didn't check, 1 == word did check */
word_spell_check(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data,
    _InoutRef_  P_S32 p_len,
    _InVal_     BOOL mistake_query);

/* truth table for start word */

static U8 tab_word_start[CHAR_N][CHAR_N] =
             /* is: PUNCT,  TEXT, SPACE, */
{
    /* was:  PUNCT */   1,     1,     1,
    /*        TEXT */   0,     1,     0,
    /*       SPACE */   1,     1,     1
};

/* truth table for end word */

static U8 tab_word_end[CHAR_N][CHAR_N] =
             /* is: PUNCT,  TEXT, SPACE, */
{
    /* was:  PUNCT */   1,     1,     0,
    /*        TEXT */   0,     1,     0,
    /*       SPACE */   0,     1,     1
};

/* truth table for next/previous word */

static U8 tab_word_next_prev[CHAR_N][CHAR_N] =
             /* is: PUNCT,  TEXT, SPACE, */
{
    /* was:  PUNCT */   0,     1,     1,
    /*        TEXT */   1,     0,     1,
    /*       SPACE */   0,     0,     1
};

/* truth table for start word */

static U8 tab_word_start_next[CHAR_N][CHAR_N] =
             /* is: PUNCT,  TEXT, SPACE, */
{
    /* was:  PUNCT */   1,     0,     1,
    /*        TEXT */   1,     1,     1,
    /*       SPACE */   1,     0,     1
};

/* handle the messages that go via TEXT_MESSAGE_BLOCK */

T5_MSG_PROTO(static, text_main_redraw, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_OBJECT_REDRAW p_object_redraw = (P_OBJECT_REDRAW) p_text_message_block->p_data;
    P_TEXT_CACHE p_text_cache;
    S32 slot_mark_start_now, slot_mark_end_now, slot_mark_start_screen, slot_mark_end_screen;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /******* set marking flags and start and end indexes *******/
    slot_mark_start_now = slot_mark_end_now = slot_mark_start_screen = slot_mark_end_screen = 0;

    if(p_object_redraw->flags.marked_now && p_object_redraw->flags.show_selection)
    {
        slot_mark_start_now =
            (OBJECT_ID_NONE != p_object_redraw->start_now.object_id)
                ? p_object_redraw->start_now.data
                : 0;
        slot_mark_end_now =
            (OBJECT_ID_NONE != p_object_redraw->end_now.object_id)
                ? p_object_redraw->end_now.data
                : S32_MAX;
    }

    if(p_object_redraw->flags.marked_screen && p_object_redraw->flags.show_selection)
    {
        slot_mark_start_screen =
            (OBJECT_ID_NONE != p_object_redraw->start_screen.object_id)
                ? p_object_redraw->start_screen.data
                : 0;
        slot_mark_end_screen =
            (OBJECT_ID_NONE != p_object_redraw->end_screen.object_id)
                ? p_object_redraw->end_screen.data
                : S32_MAX;
    }

    /* get details of formatted text */
    p_text_cache = p_text_cache_from_data_ref(p_docu, &p_text_message_block->text_format_info, &p_text_message_block->inline_object);
    PTR_ASSERT(p_text_cache);
    formatted_text_justify_vertical(&p_text_cache->formatted_text,
                                    &p_text_message_block->text_format_info,
                                    &p_object_redraw->skel_rect_object.tl.page_num);

    { /***** loop over segments to paint text *****/
    const ARRAY_INDEX n_segments = array_elements(&p_text_cache->formatted_text.h_segments);
    ARRAY_INDEX seg_ix;
    P_SEGMENT p_segment = array_range(&p_text_cache->formatted_text.h_segments, SEGMENT, 0, n_segments);

    for(seg_ix = 0; seg_ix < n_segments; ++seg_ix, ++p_segment)
    {
        SKEL_RECT skel_rect_segment;

        /* check that segment is in redraw rect and not completely below object rectangle */
        if(skel_rect_intersect(NULL,
                               skel_rect_from_segment(&skel_rect_segment,
                                                      p_segment,
                                                      &p_object_redraw->skel_rect_object.tl.page_num),
                               &p_object_redraw->skel_rect_clip)
           &&
           skel_point_compare(&skel_rect_segment.tl, &p_object_redraw->skel_rect_object.br, y) < 0)
        {
            /******** paint sections as required *******/
            if(p_object_redraw->flags.show_content)
                text_segment_paint(p_docu,
                                   &p_object_redraw->redraw_context,
                                   &p_text_cache->formatted_text,
                                   p_segment,
                                   p_text_message_block->inline_object.object_data.u.ustr_inline,
                                   &skel_rect_segment,
                                   &p_object_redraw->rgb_back,
                                   FALSE,
                                   &p_text_message_block->text_format_info.style_text_global);

            /******* invert marked text sections *******/
            if(slot_mark_end_now    > slot_mark_start_now ||
               slot_mark_end_screen > slot_mark_start_screen)
            {
                PC_CHUNK p_chunk;
                S32 seg_start, seg_end, seg_mark_start_now, seg_mark_end_now, seg_mark_start_screen, seg_mark_end_screen,
                    seg_marked_now, seg_marked_screen;
                S32 s1, s2, e1, e2;

                /* calculate segment start and end */
                p_chunk = array_ptrc(&p_text_cache->formatted_text.h_chunks, CHUNK, p_segment->start_chunk);
                seg_start = p_chunk->input_ix;

                p_chunk = array_ptrc(&p_text_cache->formatted_text.h_chunks, CHUNK, p_segment->end_chunk - 1);
                seg_end = p_chunk->input_ix + p_chunk->input_len;

                /* set segment start and end indexes for bits that are marked now
                 * and bits that are already marked on the screen
                 */
                seg_mark_start_now    = MAX(seg_start, slot_mark_start_now);
                seg_mark_start_now    = MIN(seg_end,   seg_mark_start_now);
                seg_mark_end_now      = MIN(seg_end,   slot_mark_end_now);
                seg_mark_end_now      = MAX(seg_start, seg_mark_end_now);
                seg_mark_start_screen = MAX(seg_start, slot_mark_start_screen);
                seg_mark_start_screen = MIN(seg_end,   seg_mark_start_screen);
                seg_mark_end_screen   = MIN(seg_end,   slot_mark_end_screen);
                seg_mark_end_screen   = MAX(seg_start, seg_mark_end_screen);

                seg_marked_now = (seg_mark_end_now > seg_mark_start_now);
                seg_marked_screen = (seg_mark_end_screen > seg_mark_start_screen);

                /* update the screen to reflect the 'now' state
                 */
                s1 = s2 = e1 = e2 = -1;
                if(p_object_redraw->flags.show_content)
                {
                    /* we are painting - invert the bit that is marked now */
                    s1 = seg_mark_start_now;
                    e1 = seg_mark_end_now;
                }
                else if(seg_marked_now && seg_marked_screen)
                {
                    s1 = MIN(seg_mark_start_now, seg_mark_start_screen);
                    e1 = MAX(seg_mark_start_now, seg_mark_start_screen);
                    s2 = MIN(seg_mark_end_now, seg_mark_end_screen);
                    e2 = MAX(seg_mark_end_now, seg_mark_end_screen);
                }
                else if(seg_marked_now)
                {
                    s1 = seg_mark_start_now;
                    e1 = seg_mark_end_now;
                }
                else if(seg_marked_screen)
                {
                    s1 = seg_mark_start_screen;
                    e1 = seg_mark_end_screen;
                }

                if(s1 >= 0)
                    text_part_invert(p_docu, p_object_redraw, &p_text_cache->formatted_text,
                                     p_text_message_block->inline_object.object_data.u.ustr_inline, s1, e1);
                if(s2 >= 0)
                    text_part_invert(p_docu, p_object_redraw, &p_text_cache->formatted_text,
                                     p_text_message_block->inline_object.object_data.u.ustr_inline, s2, e2);
            }
        }
    }
    } /*block*/

    return(STATUS_OK);
}

T5_MSG_PROTO(static, text_main_msg_tab_wanted, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_TAB_WANTED p_tab_wanted = (P_TAB_WANTED) p_text_message_block->p_data;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(array_elements(&p_text_message_block->text_format_info.style_text_global.para_style.h_tab_list))
        p_tab_wanted->want_inline_insert = 1;

    return(STATUS_OK);
}

/* SKS 26jul95 separated skel_point/object_position processing cases out for debugging */

T5_MSG_PROTO(static, text_main_msg_object_position_from_skel_point, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_OBJECT_POSITION_FIND p_object_position_find = (P_OBJECT_POSITION_FIND) p_text_message_block->p_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* must cope with NULL objects */
    if( (P_DATA_NONE != p_text_message_block->inline_object.object_data.u.p_object)
    &&  p_text_message_block->inline_object.inline_len)
    {
        P_TEXT_CACHE p_text_cache;
        TEXT_LOCATION text_location;

        p_text_cache = p_text_cache_from_data_ref(p_docu, &p_text_message_block->text_format_info, &p_text_message_block->inline_object);
        PTR_ASSERT(p_text_cache);

        text_location_from_skel_point(p_docu,
                                      &text_location,
                                      p_text_message_block->inline_object.object_data.u.ustr_inline,
                                      &p_text_cache->formatted_text,
                                      &p_object_position_find->pos,
                                      &p_object_position_find->skel_rect.tl.page_num);

        p_object_position_find->pos = text_location.skel_point;
        p_object_position_find->object_data.object_position_start.data = text_location.string_ix;
        p_object_position_find->object_data.object_position_start.object_id = p_text_message_block->inline_object.object_data.object_id;
        p_object_position_find->caret_height = text_location.caret_height;
    }
    else
    {
        p_object_position_find->pos = p_object_position_find->skel_rect.tl;
        p_object_position_find->caret_height = 0;
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, text_main_msg_skel_point_from_object_position, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_OBJECT_POSITION_FIND p_object_position_find = (P_OBJECT_POSITION_FIND) p_text_message_block->p_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* must cope with NULL objects */
    if( (P_DATA_NONE != p_text_message_block->inline_object.object_data.u.p_object)
    &&  p_text_message_block->inline_object.inline_len)
    {
        P_TEXT_CACHE p_text_cache;
        TEXT_LOCATION text_location;

        p_text_cache = p_text_cache_from_data_ref(p_docu, &p_text_message_block->text_format_info, &p_text_message_block->inline_object);
        PTR_ASSERT(p_text_cache);
        formatted_text_justify_vertical(&p_text_cache->formatted_text,
                                        &p_text_message_block->text_format_info,
                                        &p_text_message_block->text_format_info.skel_rect_object.tl.page_num);

        /* set caret position */
        text_location.string_ix = p_object_position_find->object_data.object_position_start.data;

        text_skel_point_from_location(p_docu,
                                      &text_location,
                                      p_text_message_block->inline_object.object_data.u.ustr_inline,
                                      &p_text_cache->formatted_text,
                                      FALSE,
                                      &p_object_position_find->skel_rect.tl.page_num);

        p_object_position_find->pos = text_location.skel_point;

        {
        SKEL_POINT skel_point_cut_off = text_location.skel_point;

        if(skel_point_compare(&skel_point_cut_off, &p_object_position_find->skel_rect.br, y) > 0)
            p_object_position_find->caret_height = 0;
        else
            p_object_position_find->caret_height = text_location.caret_height;
        } /*block*/

    }
    else
    {
        PIXIT_POINT caret_offset;
        PIXIT width = p_text_message_block->text_format_info.skel_rect_object.br.pixit_point.x
                    - p_text_message_block->text_format_info.skel_rect_object.tl.pixit_point.x;

        /* set caret height from current style leading */
        p_object_position_find->caret_height =
            style_leading_from_style(&p_text_message_block->text_format_info.style_text_global,
                                     &p_text_message_block->text_format_info.style_text_global.font_spec,
                                     p_docu->flags.draft_mode);

        caret_offset.x = p_text_message_block->text_format_info.style_text_global.para_style.margin_left +
                         p_text_message_block->text_format_info.style_text_global.para_style.margin_para;

        switch(p_text_message_block->text_format_info.style_text_global.para_style.justify)
        {
        default:
            break;

        case SF_JUSTIFY_CENTRE:
            caret_offset.x = p_text_message_block->text_format_info.style_text_global.para_style.margin_left
                           +
                           ((width - p_text_message_block->text_format_info.style_text_global.para_style.margin_right)
                                   - p_text_message_block->text_format_info.style_text_global.para_style.margin_left) / 2;
            break;

        case SF_JUSTIFY_RIGHT:
            caret_offset.x = width - p_text_message_block->text_format_info.style_text_global.para_style.margin_right;
            break;
        }

        caret_offset.y = p_text_message_block->text_format_info.style_text_global.para_style.para_start;

        if(p_object_position_find->skel_rect.tl.page_num.y == p_object_position_find->skel_rect.br.page_num.y)
            switch(p_text_message_block->text_format_info.style_text_global.para_style.justify_v)
            {
            default:
                break;

            case SF_JUSTIFY_V_CENTRE:
            case SF_JUSTIFY_V_BOTTOM:
                {
#if 1 /* SKS 26jul95 for empty database fields */
                PIXIT vertical_space = p_text_message_block->text_format_info.skel_rect_object.br.pixit_point.y
                                     - p_text_message_block->text_format_info.skel_rect_object.tl.pixit_point.y
#else
                PIXIT vertical_space = p_object_position_find->skel_rect.br.pixit_point.y
                                     - p_object_position_find->skel_rect.tl.pixit_point.y
#endif
                                     - p_text_message_block->text_format_info.style_text_global.para_style.para_start
                                     - p_text_message_block->text_format_info.style_text_global.para_style.para_end
                                     - p_object_position_find->caret_height;

                if(SF_JUSTIFY_RIGHT == p_text_message_block->text_format_info.style_text_global.para_style.justify_v)
                    vertical_space /= 2;

                if(vertical_space > 0)
                    caret_offset.y += vertical_space;

                break;
                }
            }

        p_object_position_find->pos = p_text_message_block->text_format_info.skel_rect_object.tl;
        p_object_position_find->pos.pixit_point.x += caret_offset.x;
        p_object_position_find->pos.pixit_point.y += caret_offset.y;
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, text_main_msg_object_logical_move, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_OBJECT_LOGICAL_MOVE p_object_logical_move = (P_OBJECT_LOGICAL_MOVE) p_text_message_block->p_data;
    P_TEXT_CACHE p_text_cache;
    TEXT_LOCATION text_location;
    P_SEGMENT p_segment;
    SKEL_RECT skel_rect;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE == p_object_logical_move->object_data.u.p_object)
        return(STATUS_FAIL);

    p_text_cache = p_text_cache_from_data_ref(p_docu, &p_text_message_block->text_format_info, &p_text_message_block->inline_object);
    PTR_ASSERT(p_text_cache);

    /* find current position in text object */
    text_location.string_ix = p_object_logical_move->object_data.object_position_start.data;
    text_skel_point_from_location(p_docu,
                                  &text_location,
                                  p_object_logical_move->object_data.u.ustr_inline,
                                  &p_text_cache->formatted_text,
                                  FALSE,
                                  &p_text_message_block->text_format_info.skel_rect_object.tl.page_num);

    switch(p_object_logical_move->action)
    {
    case OBJECT_LOGICAL_MOVE_UP:
        { /* on the top segment ? */
        if(!text_location.seg_ix)
            return(STATUS_FAIL);

        /* find previous segment */
        p_segment = array_ptr(&p_text_cache->formatted_text.h_segments, SEGMENT, text_location.seg_ix - 1);
        skel_rect_from_segment(&skel_rect, p_segment, &p_text_message_block->text_format_info.skel_rect_object.tl.page_num);
        p_object_logical_move->skel_point_out = skel_rect.tl;
        break;
        }

    case OBJECT_LOGICAL_MOVE_DOWN:
        { /* on the bottom segment ? */
        if(text_location.seg_ix + 1 >= array_elements(&p_text_cache->formatted_text.h_segments))
            return(STATUS_FAIL);

        /* find next segment */
        p_segment = array_ptr(&p_text_cache->formatted_text.h_segments, SEGMENT, text_location.seg_ix + 1);
        skel_rect_from_segment(&skel_rect, p_segment, &p_text_message_block->text_format_info.skel_rect_object.tl.page_num);
        p_object_logical_move->skel_point_out = skel_rect.tl;
        break;
        }

    case OBJECT_LOGICAL_MOVE_LEFT:
        {
        p_segment = array_ptr(&p_text_cache->formatted_text.h_segments, SEGMENT, text_location.seg_ix);
        skel_rect_from_segment(&skel_rect, p_segment, &p_text_message_block->text_format_info.skel_rect_object.tl.page_num);
        p_object_logical_move->skel_point_out = skel_rect.tl;
        break;
        }

    default: default_unhandled();
#if CHECKING
    case OBJECT_LOGICAL_MOVE_RIGHT:
#endif
        {
        p_segment = array_ptr(&p_text_cache->formatted_text.h_segments, SEGMENT, text_location.seg_ix);
        skel_rect_from_segment(&skel_rect, p_segment, &p_text_message_block->text_format_info.skel_rect_object.tl.page_num);
        p_object_logical_move->skel_point_out = skel_rect.tl;
        p_object_logical_move->skel_point_out.pixit_point.x += p_segment->width;
        break;
        }
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, text_main_msg_object_read_text_draft, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_OBJECT_READ_TEXT_DRAFT p_object_read_text_draft = (P_OBJECT_READ_TEXT_DRAFT) p_text_message_block->p_data;
    const P_TEXT_CACHE p_text_cache = p_text_cache_from_data_ref(p_docu, &p_text_message_block->text_format_info, &p_text_message_block->inline_object);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_text_cache)
        return(STATUS_FAIL);

    return(
        plain_text_from_formatted_text(p_docu,
                                       &p_object_read_text_draft->h_plain_text,
                                       &p_text_cache->formatted_text,
                                       p_object_read_text_draft->object_data.u.ustr_inline,
                                       &p_text_message_block->text_format_info.skel_rect_object.tl.page_num,
                                       &p_text_message_block->text_format_info.style_text_global));
}

/******************************************************************************
*
* handle object position set calls
*
******************************************************************************/

T5_MSG_PROTO(static, text_main_msg_object_position_set, _InoutRef_ P_OBJECT_POSITION_SET p_object_position_set) /* STATUS_OK == not done, STATUS_DONE == done */
{
    const P_OBJECT_DATA p_object_data = &p_object_position_set->object_data;
    const S32 action = p_object_position_set->action;
    STATUS status = STATUS_DONE;
    S32 start, end;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    offsets_from_object_data(&start, &end, p_object_data, PTR_NOT_NONE(p_object_data->u.ustr_inline) ? ustr_inline_strlen(p_object_data->u.ustr_inline) : 0);

    switch(action)
    {
    case OBJECT_POSITION_SET_START:
        p_object_data->object_position_start.data = 0;
        p_object_data->object_position_start.object_id = p_object_data->object_id;
        break;

    case OBJECT_POSITION_SET_END:
        p_object_data->object_position_start.data = end;
        p_object_data->object_position_start.object_id = p_object_data->object_id;
        break;

    case OBJECT_POSITION_SET_START_CLEAR:
        if(!start)
            p_object_data->object_position_start.object_id = OBJECT_ID_NONE;
        break;

    case OBJECT_POSITION_SET_END_CLEAR:
        if(start >= end)
        {
            p_object_data->object_position_start.object_id = OBJECT_ID_NONE;
            p_object_data->object_position_start.data = -1;
        }
        break;

    case OBJECT_POSITION_SET_BACK:
        if(0 == start)
            status = STATUS_OK;
        else
        {
            S32 charcount, ix;
            PTR_ASSERT(p_object_data->u.ustr_inline);
            charcount = inline_b_bytecount_off(p_object_data->u.ustr_inline, start);
            ix = start - charcount;
            p_object_data->object_position_start.data = ix;
        }
        break;

    case OBJECT_POSITION_SET_FORWARD:
        if(start >= end)
            status = STATUS_OK;
        else
        {
            S32 charcount, ix;
            PTR_ASSERT(p_object_data->u.ustr_inline);
            charcount = inline_bytecount_off(p_object_data->u.ustr_inline, start);
            ix = start + charcount;
            p_object_data->object_position_start.data = ix;
        }
        break;

    case OBJECT_POSITION_SET_START_WORD:
        PTR_ASSERT(p_object_data->u.ustr_inline);
        p_object_data->object_position_start.data = text_backward(p_object_data->u.ustr_inline, start, tab_word_start);
        break;

    case OBJECT_POSITION_SET_END_WORD:
        PTR_ASSERT(p_object_data->u.ustr_inline);
        p_object_data->object_position_start.data = text_forward(p_object_data->u.ustr_inline, start, tab_word_end, end);
        break;

    case OBJECT_POSITION_SET_PREV_WORD:
        if(!start)
            status = STATUS_OK;
        else
        {
            PTR_ASSERT(p_object_data->u.ustr_inline);
            start = text_backward(p_object_data->u.ustr_inline, start, tab_word_next_prev);
            p_object_data->object_position_start.data = text_backward(p_object_data->u.ustr_inline, start, tab_word_start);
        }
        break;

    case OBJECT_POSITION_SET_NEXT_WORD:
        if(start >= end)
            status = STATUS_OK;
        else
        {
            PTR_ASSERT(p_object_data->u.ustr_inline);
            start = text_forward(p_object_data->u.ustr_inline, start, tab_word_next_prev, end);
            p_object_data->object_position_start.data = text_forward(p_object_data->u.ustr_inline, start, tab_word_start, end);
        }
        break;

    case OBJECT_POSITION_SET_AT_CHECK:
        if(start >= end)
            status = STATUS_OK;
        break;

    default: default_unhandled();
        break;
    }

    return(status);
}

/* request to object to check a word */

T5_MSG_PROTO(static, text_main_msg_object_word_check, P_OBJECT_DATA p_object_data)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE == p_object_data->u.p_object)
        return(STATUS_OK);

    {
    S32 len = ustr_inline_strlen(p_object_data->u.ustr_inline);
    return(word_spell_check(p_docu, p_object_data, &len, FALSE));
    } /*block*/
}

/* request from spelling checker to check object */

T5_MSG_PROTO(static, text_main_msg_object_check, P_OBJECT_CHECK p_object_check)
{
    STATUS status = STATUS_OK;
    S32 len = (P_DATA_NONE != p_object_check->object_data.u.p_object) ? ustr_inline_strlen(p_object_check->object_data.u.ustr_inline) : 0;
    S32 start, end;
    S32 now;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    offsets_from_object_data(&start, &end, &p_object_check->object_data, len);

    p_object_check->status = CHECK_CONTINUE;

    now = start;

    while(now < end)
    {
        p_object_check->object_data.object_position_start.data = now;
        p_object_check->object_data.object_position_start.object_id = p_object_check->object_data.object_id;

        if(status_done(status = word_spell_check(p_docu, &p_object_check->object_data, &len, TRUE)))
        {
            end = MIN(end, len);

            now = text_forward(p_object_check->object_data.u.ustr_inline,
                               p_object_check->object_data.object_position_end.data,
                               tab_word_start_next,
                               len);
        }
        else
        {
            p_object_check->status = CHECK_CANCEL;
            trace_0(TRACE_APP_SKEL, TEXT("OB_TEXT object_check cancelled"));
            break;
        }
    }

    return(status);
}

T5_MSG_PROTO(static, text_main_msg_object_string_search, P_OBJECT_STRING_SEARCH p_object_string_search)
{
    STATUS status = STATUS_OK;
    const PC_USTR_INLINE ustr_inline = p_object_string_search->object_data.u.ustr_inline;
    S32 start, end;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    object_position_init(&p_object_string_search->object_position_found_start);
    object_position_init(&p_object_string_search->object_position_found_end);

    if(P_DATA_NONE == ustr_inline)
        return(STATUS_OK);

    offsets_from_object_data(&start, &end, &p_object_string_search->object_data, ustr_inline_strlen(ustr_inline));

    if(status_done(status =
        uchars_inline_search(ustr_inline,
                             p_object_string_search->ustr_search_for,
                             &start,
                             &end,
                             p_object_string_search->ignore_capitals,
                             p_object_string_search->whole_words)))
    {
        p_object_string_search->object_position_found_start.object_id =
        p_object_string_search->object_position_found_end.object_id = p_object_string_search->object_data.object_id;
        p_object_string_search->object_position_found_start.data = start;
        p_object_string_search->object_position_found_end.data = end;
    }

    return(status);
}

T5_MSG_PROTO(static, text_main_msg_object_read_text, P_OBJECT_READ_TEXT p_object_read_text)
{
    const PC_USTR_INLINE ustr_inline = p_object_read_text->object_data.u.ustr_inline;
    S32 start, end;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE == ustr_inline)
        return(STATUS_OK);

    offsets_from_object_data(&start, &end, &p_object_read_text->object_data, ustr_inline_strlen(ustr_inline));

    switch(p_object_read_text->type)
    {
    default: default_unhandled();
#if CHECKING
    case OBJECT_READ_TEXT_PLAIN:
#endif
        return(uchars_inline_plain_convert(p_object_read_text->p_quick_ublock, uchars_inline_AddBytes(ustr_inline, start), end - start));

    case OBJECT_READ_TEXT_RESULT:
        return(uchars_inline_result_convert(p_docu, p_object_read_text->p_quick_ublock, uchars_inline_AddBytes(ustr_inline, start), end - start));

    case OBJECT_READ_TEXT_SEARCH:
        return(uchars_inline_search_convert(p_object_read_text->p_quick_ublock, uchars_inline_AddBytes(ustr_inline, start), end - start));

    case OBJECT_READ_TEXT_EDIT:
        return(quick_ublock_uchars_add(p_object_read_text->p_quick_ublock, uchars_AddBytes(ustr_inline, start), end - start));
    }
}

T5_MSG_PROTO(static, text_main_cmd_word_count, P_OBJECT_WORD_COUNT p_object_word_count)
{
    S32 len = (P_DATA_NONE != p_object_word_count->object_data.u.p_object) ? ustr_inline_strlen(p_object_word_count->object_data.u.ustr_inline) : 0;
    S32 start, end, offset;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    offsets_from_object_data(&start, &end, &p_object_word_count->object_data, len);

    p_object_word_count->words_counted = 0;

    offset = start;

    while(offset < end)
    {
        S32 word_start = text_backward(p_object_word_count->object_data.u.ustr_inline, offset, tab_word_start);
        S32 word_end = text_forward(p_object_word_count->object_data.u.ustr_inline, word_start, tab_word_end, len);

        offset = text_forward(p_object_word_count->object_data.u.ustr_inline, word_end, tab_word_start_next, len);

        p_object_word_count->words_counted += 1;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* invert all or part of a segment
*
******************************************************************************/

static void
text_part_invert(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    P_FORMATTED_TEXT p_formatted_text,
    P_USTR_INLINE ustr_inline,
    _InVal_     S32 start,
    _InVal_     S32 end)
{
    if(end > start)
    {
        TEXT_LOCATION text_location;
        PIXIT_RECT pixit_rect;

        trace_2(TRACE_APP_SKEL_DRAW, TEXT("text_part_invert start: ") S32_TFMT TEXT(", end: ") S32_TFMT, start, end);

        /* work out start position */
        text_location.string_ix = start;
        text_skel_point_from_location(p_docu,
                                      &text_location,
                                      ustr_inline,
                                      p_formatted_text,
                                      FALSE,
                                      &p_object_redraw->skel_rect_object.tl.page_num);

        pixit_rect.tl = text_location.skel_point.pixit_point;

        /* work out end position */
        text_location.string_ix = end;
        text_skel_point_from_location(p_docu,
                                      &text_location,
                                      ustr_inline,
                                      p_formatted_text,
                                      TRUE,
                                      &p_object_redraw->skel_rect_object.tl.page_num);

        pixit_rect.br = text_location.skel_point.pixit_point;
        pixit_rect.br.y += text_location.caret_height;

        trace_2(TRACE_APP_SKEL_DRAW, TEXT("text_part_invert: ") S32_TFMT TEXT(", ") S32_TFMT, start, end);

        host_invert_rectangle_filled(&p_object_redraw->redraw_context,
                                     &pixit_rect,
                                     &p_object_redraw->rgb_fore,
                                     &p_object_redraw->rgb_back);
    }
}

/******************************************************************************
*
* paint a segment of text
*
******************************************************************************/

/* own underline code */

static void
text_segment_paint_underlines(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    P_FORMATTED_TEXT p_formatted_text,
    P_SEGMENT p_segment)
{
    PIXIT_POINT pixit_point = p_segment->skel_point.pixit_point;
    ARRAY_INDEX chunk_ix;
    PC_CHUNK p_chunk;

    pixit_point.y += p_segment->base_line;

    for(chunk_ix = p_segment->start_chunk, p_chunk = array_ptrc(&p_formatted_text->h_chunks, CHUNK, p_segment->start_chunk);
        chunk_ix < p_segment->end_chunk;
        chunk_ix++, p_chunk++)
    {
        if(p_chunk->fonty_chunk.underline)
        {
            const FONTY_HANDLE fonty_handle = p_chunk->fonty_chunk.fonty_handle;
            const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
            PIXIT thickness = (p_chunk->fonty_chunk.ascent * 14) / 256;
            PIXIT_LINE pixit_line;

            pixit_line.tl = pixit_point;
            pixit_line.tl.y += (p_chunk->fonty_chunk.ascent * 26) / 256;
            pixit_line.tl.x += p_chunk->lead_space;
            pixit_line.br = pixit_line.tl;

            /* strap together all adjacent underline chunks */
            while((chunk_ix + 1) < p_segment->end_chunk
                  &&
                  p_chunk[1].fonty_chunk.underline)
            {
                pixit_point.x += p_chunk->lead_space + p_chunk->width;
                chunk_ix++;
                p_chunk++;
            }

            pixit_line.br.x = pixit_point.x + (p_chunk->lead_space + p_chunk->width);
            pixit_line.br.x -= p_chunk->fonty_chunk.trail_space;

            host_paint_underline(p_redraw_context, &pixit_line, &p_font_context->font_spec.colour, thickness);
        }

        pixit_point.x += (p_chunk->lead_space + p_chunk->width);
    }
}

#if RISCOS

static void
text_segment_paint_drawfile(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    P_FORMATTED_TEXT p_formatted_text,
    P_SEGMENT p_segment,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_SKEL_RECT p_skel_rect_seg,
    _InRef_     PC_RGB p_rgb_back,
    _InRef_     PC_STYLE p_style_text_global)
{
    BOOL found_some_underlines = FALSE;
    PIXIT_POINT pixit_point = p_segment->skel_point.pixit_point;
    ARRAY_INDEX chunk_ix;
    PC_CHUNK p_chunk;
    PIXIT_RECT pixit_rect_rubout;

    pixit_rect_rubout.tl = p_skel_rect_seg->tl.pixit_point;
    pixit_rect_rubout.br = p_skel_rect_seg->br.pixit_point;

    trace_2(TRACE_APP_SKEL_DRAW,
            TEXT("OB_TEXT pixit_point pixit_point.y: ") S32_TFMT TEXT(", leading: ") S32_TFMT,
            pixit_point.y,
            p_segment->leading);

    for(chunk_ix = p_segment->start_chunk, p_chunk = array_ptrc(&p_formatted_text->h_chunks, CHUNK, chunk_ix);
        chunk_ix < p_segment->end_chunk;
        chunk_ix++, p_chunk++)
    {
        const FONTY_HANDLE fonty_handle = p_chunk->fonty_chunk.fonty_handle;

        pixit_point.x += p_chunk->lead_space;

        if(p_chunk->fonty_chunk.underline)
            found_some_underlines = TRUE;

        switch(p_chunk->type)
        {
        case CHUNK_FREE:
            {
            S32 len_to_use = p_chunk->input_len - p_chunk->trail_spaces;

            /* work out if we need to draw trail spaces so we get underlined spaces
             * we do this if the next chunk is also underlined
             */
            if(p_chunk->fonty_chunk.underline
               &&
               (chunk_ix + 1) < p_segment->end_chunk
               &&
               p_chunk[1].fonty_chunk.underline)
                len_to_use = p_chunk->input_len;

            /* paint the chunk */
            {
                fonty_text_paint_simple_uchars(
                    p_docu, p_redraw_context, &pixit_point,
                    uchars_AddBytes(ustr_inline, p_chunk->input_ix), len_to_use,
                    p_segment->base_line, p_rgb_back, fonty_handle);
            }

            break;
            }

        case CHUNK_TAB:
            break;

#if (TSTR_IS_SBSTR && 0)
        case CHUNK_UTF8:
            {
            STATUS status;
            const PC_UCHARS_INLINE uchars_inline = uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix);
            PC_UTF8 utf8 = inline_data_ptr(PC_UTF8, uchars_inline);
            const U32 il_data_size = inline_data_size(uchars_inline);
            QUICK_WBLOCK_WITH_BUFFER(quick_wblock, 64);
            quick_wblock_with_buffer_setup(quick_wblock);

            status = quick_wblock_utf8_add(&quick_wblock, utf8, il_data_size);

            status_assert(status);

            /* paint the the content of the quick_block */
            {
                fonty_text_paint_simple_wchars(
                    p_docu, p_redraw_context, &pixit_point,
                    quick_wblock_wchars(&quick_wblock), quick_wblock_chars(&quick_wblock),
                    p_segment->base_line, p_rgb_back, fonty_handle);
            }

            quick_wblock_dispose(&quick_wblock);
            break;
            }
#endif /* TSTR_IS_SBSTR */

        default: default_unhandled();
#if CHECKING
        case CHUNK_DATE:
        case CHUNK_FILE_DATE:
        case CHUNK_PAGE_X:
        case CHUNK_PAGE_Y:
        case CHUNK_SS_NAME:
        case CHUNK_MS_FIELD:
        case CHUNK_WHOLENAME:
        case CHUNK_LEAFNAME:
        case CHUNK_HYPHEN:
#if !(TSTR_IS_SBSTR && 0)
        case CHUNK_UTF8:
#endif
        case CHUNK_UTF8_AS_TEXT:
#endif
            {
            STATUS status;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
            quick_ublock_with_buffer_setup(quick_ublock);

            status_assert(status = text_from_field_uchars(p_docu, &quick_ublock,
                                                          uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix) CODE_ANALYSIS_ONLY_ARG(p_chunk->input_len),
                                                          &p_skel_rect_seg->tl.page_num, p_style_text_global));

            /* paint the the content of the quick_block */
           {
                fonty_text_paint_simple_uchars(
                    p_docu, p_redraw_context, &pixit_point,
                    quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock),
                    p_segment->base_line, p_rgb_back, fonty_handle);
            }

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case CHUNK_SOFT_HYPHEN:
        case CHUNK_RETURN:
            break;
        }

        pixit_point.x += p_chunk->width;
    }

    if(found_some_underlines)
        text_segment_paint_underlines(p_redraw_context, p_formatted_text, p_segment);
}

static void
text_segment_paint(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    P_FORMATTED_TEXT p_formatted_text,
    P_SEGMENT p_segment,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_SKEL_RECT p_skel_rect_seg,
    _InRef_     PC_RGB p_rgb_back,
    _In_        U8 rubout,
    _InRef_     PC_STYLE p_style_text_global)
{
    if(p_redraw_context->flags.drawfile)
    {
        text_segment_paint_drawfile(p_docu, p_redraw_context, p_formatted_text, p_segment, ustr_inline, p_skel_rect_seg, p_rgb_back, p_style_text_global);
        return;
    }

    {
    BOOL found_some_underlines = FALSE;
    PIXIT_POINT pixit_point = p_segment->skel_point.pixit_point;
    ARRAY_INDEX chunk_ix;
    PC_CHUNK p_chunk;
    PIXIT_RECT pixit_rect_rubout;
    BOOL first_chunk = TRUE;
    S32 lead_space_mp = 0;
    struct current_state
    {
        RGB rgb;
        PIXIT shift_y;
        HOST_FONT host_font;
    } current;
    PIXIT move_x = 0;
    PIXIT shift_y;
    BOOL dump_accumulated = FALSE;
    U32 out_len;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
    quick_ublock_with_buffer_setup(quick_ublock);

    zero_struct(current);

    pixit_rect_rubout.tl = p_skel_rect_seg->tl.pixit_point;
    pixit_rect_rubout.br = p_skel_rect_seg->br.pixit_point;

    trace_2(TRACE_APP_SKEL_DRAW,
            TEXT("OB_TEXT pixit_point pixit_point.y: ") S32_TFMT TEXT(", leading: ") S32_TFMT,
            p_segment->skel_point.pixit_point.y,
            p_segment->leading);

    pixit_point.y += p_segment->base_line;

    for(chunk_ix = p_segment->start_chunk, p_chunk = array_ptrc(&p_formatted_text->h_chunks, CHUNK, chunk_ix);
        chunk_ix < p_segment->end_chunk;
        chunk_ix++, p_chunk++)
    {
        const FONTY_HANDLE fonty_handle = p_chunk->fonty_chunk.fonty_handle;
        const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
        HOST_FONT host_font = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, p_docu->flags.draft_mode);
        HOST_FONT host_font_utf8 = HOST_FONT_NONE;

        /*reportf(TEXT("chunk[%d]: type %d @ %d"), chunk_ix, p_chunk->type, p_chunk->input_ix);*/
        if(CHUNK_UTF8 == p_chunk->type)
        {
            host_font_utf8 = fonty_host_font_utf8_from_fonty_handle_redraw(p_redraw_context, fonty_handle, p_docu->flags.draft_mode);
            /*reportf(TEXT("CHUNK_UTF8: got host font handle %d for redraw"), host_font_utf8);*/
        }

        if(p_chunk->fonty_chunk.underline)
            found_some_underlines = TRUE;

        if(first_chunk)
        {
            current.rgb = p_font_context->font_spec.colour;

            dump_accumulated = FALSE;
        }
        else
        {
            /* output part segment on colour change */
            if(rgb_compare_not_equals(&current.rgb, &p_font_context->font_spec.colour))
                dump_accumulated = TRUE;
        }

        if(dump_accumulated)
        {
            out_len = quick_ublock_bytes(&quick_ublock);

            if(0 != out_len)
            {
                status_assert(quick_ublock_nullch_add(&quick_ublock));

                if(rubout)
                {
                    pixit_rect_rubout.tl.x = pixit_point.x;

                    host_fonty_text_paint_uchars_rubout(p_redraw_context, &pixit_point, quick_ublock_uchars(&quick_ublock), out_len, &current.rgb, p_rgb_back, HOST_FONT_NONE, &pixit_rect_rubout);
                }
                else
                {
                    host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_point, quick_ublock_uchars(&quick_ublock), out_len, &current.rgb, p_rgb_back, HOST_FONT_NONE, TA_LEFT);
                }

                quick_ublock_dispose(&quick_ublock);
            }

            current.rgb = p_font_context->font_spec.colour;

            pixit_point.x += move_x;
            move_x = 0;

            lead_space_mp = 0;

            dump_accumulated = FALSE;
        }

        /* output font handle change to fonty string */
        if((HOST_FONT_NONE != host_font_utf8) /*&& (current.host_font != host_font_utf8)*/)
        {
            status_assert(riscos_fonty_paint_font_change(&quick_ublock, host_font_utf8));
            current.host_font = host_font_utf8;
        }
        else
#if 0 /* output font handle willy-nilly - fixes 3.07 font manager kern problem */
        if(first_chunk || (current.host_font != host_font))
#endif
        {
            status_assert(riscos_fonty_paint_font_change(&quick_ublock, host_font));
            current.host_font = host_font;
        }

        lead_space_mp += p_chunk->lead_space * MILLIPOINTS_PER_PIXIT;

        /* output x shift to fonty string */
        if(lead_space_mp)
        {
            status_assert(riscos_fonty_paint_shift_x(&quick_ublock, lead_space_mp, p_redraw_context));
            lead_space_mp = 0;
        }

        /* output y shifts to fonty string */
        shift_y = riscos_fonty_paint_calc_shift_y(p_font_context);

        if(current.shift_y != shift_y)
        {
            status_assert(riscos_fonty_paint_shift_y(&quick_ublock, (shift_y - current.shift_y) * MILLIPOINTS_PER_PIXIT, p_redraw_context));
            current.shift_y = shift_y;
        }

        switch(p_chunk->type)
        {
        case CHUNK_FREE:
            status_assert(quick_ublock_uchars_add(&quick_ublock,
                                                  uchars_AddBytes(ustr_inline, p_chunk->input_ix),
                                                  p_chunk->input_len - p_chunk->trail_spaces));
            lead_space_mp += (p_chunk->width * MILLIPOINTS_PER_PIXIT) - p_chunk->fonty_chunk.width_mp;
            break;

        case CHUNK_TAB:
            lead_space_mp += p_chunk->width * MILLIPOINTS_PER_PIXIT;
            break;

        case CHUNK_UTF8:
            if(HOST_FONT_NONE == host_font_utf8)
            {
                /*reportf(TEXT("CHUNK_UTF8: no host font handle for redraw - odd, as we must have had one for formatting"));*/
                goto text_from_field;
            }

            {
            const PC_UCHARS_INLINE uchars_inline = uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix);
            PC_UTF8 il_data = inline_data_ptr(PC_UTF8, uchars_inline);
            const U32 il_data_size = inline_data_size(uchars_inline);

            status_assert(quick_ublock_uchars_add(&quick_ublock, (PC_UCHARS) il_data, il_data_size));
            lead_space_mp += (p_chunk->width * MILLIPOINTS_PER_PIXIT) - p_chunk->fonty_chunk.width_mp;
            break;
            }

        default: default_unhandled();
#if CHECKING
        case CHUNK_DATE:
        case CHUNK_FILE_DATE:
        case CHUNK_PAGE_X:
        case CHUNK_PAGE_Y:
        case CHUNK_MS_FIELD:
        case CHUNK_SS_NAME:
        case CHUNK_WHOLENAME:
        case CHUNK_LEAFNAME:
        case CHUNK_HYPHEN:
      /*case CHUNK_UTF8:*/
        case CHUNK_UTF8_AS_TEXT:
#endif
        text_from_field:;
            status_assert(text_from_field_uchars(p_docu, &quick_ublock,
                                                 uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix) CODE_ANALYSIS_ONLY_ARG(p_chunk->input_len),
                                                 &p_skel_rect_seg->tl.page_num, p_style_text_global));
            lead_space_mp += (p_chunk->width * MILLIPOINTS_PER_PIXIT) - p_chunk->fonty_chunk.width_mp;
            break;

        case CHUNK_SOFT_HYPHEN:
        case CHUNK_RETURN:
            break;
        }

        move_x += (p_chunk->lead_space + p_chunk->width);

        first_chunk = FALSE;
    }

    out_len = quick_ublock_bytes(&quick_ublock);

    if(0 != out_len)
    {
        status_assert(quick_ublock_nullch_add(&quick_ublock));

        if(rubout)
        {
            pixit_rect_rubout.tl.x = pixit_point.x;

            host_fonty_text_paint_uchars_rubout(p_redraw_context, &pixit_point, quick_ublock_ustr(&quick_ublock), out_len, &current.rgb, p_rgb_back, HOST_FONT_NONE, &pixit_rect_rubout);
        }
        else
        {
            host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_point, quick_ublock_ustr(&quick_ublock), out_len, &current.rgb, p_rgb_back, HOST_FONT_NONE, TA_LEFT);
        }
    }

    quick_ublock_dispose(&quick_ublock);

    if(found_some_underlines)
        text_segment_paint_underlines(p_redraw_context, p_formatted_text, p_segment);
    } /*block*/
}

#elif WINDOWS

static void
text_segment_paint(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    P_FORMATTED_TEXT p_formatted_text,
    P_SEGMENT p_segment,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_SKEL_RECT p_skel_rect_seg,
    _InRef_     PC_RGB p_rgb_back,
    _In_        U8 rubout,
    _InRef_     PC_STYLE p_style_text_global)
{
    BOOL found_some_underlines = FALSE;
    PIXIT_POINT pixit_point = p_segment->skel_point.pixit_point;
    ARRAY_INDEX chunk_ix;
    PC_CHUNK p_chunk;
    PIXIT_RECT pixit_rect_rubout;

    pixit_rect_rubout.tl = p_skel_rect_seg->tl.pixit_point;
    pixit_rect_rubout.br = p_skel_rect_seg->br.pixit_point;

    trace_2(TRACE_APP_SKEL_DRAW,
            TEXT("OB_TEXT pixit_point pixit_point.y: ") S32_TFMT TEXT(", leading: ") S32_TFMT,
            pixit_point.y,
            p_segment->leading);

    for(chunk_ix = p_segment->start_chunk, p_chunk = array_ptrc(&p_formatted_text->h_chunks, CHUNK, chunk_ix);
        chunk_ix < p_segment->end_chunk;
        chunk_ix++, p_chunk++)
    {
        const FONTY_HANDLE fonty_handle = p_chunk->fonty_chunk.fonty_handle;

        pixit_point.x += p_chunk->lead_space;

        if(p_chunk->fonty_chunk.underline)
            found_some_underlines = TRUE;

        switch(p_chunk->type)
        {
        case CHUNK_FREE:
            {
            S32 len_to_use = p_chunk->input_len - p_chunk->trail_spaces;

            /* work out if we need to draw trail spaces so we get underlined spaces
             * we do this if the next chunk is also underlined
             */
            if(p_chunk->fonty_chunk.underline
               &&
               (chunk_ix + 1) < p_segment->end_chunk
               &&
               p_chunk[1].fonty_chunk.underline)
                len_to_use = p_chunk->input_len;

            /* paint the chunk */
            if(rubout)
            {   /* using its rubout box if required */
                pixit_rect_rubout.br.x = p_skel_rect_seg->br.pixit_point.x;
                if(chunk_ix != (p_segment->end_chunk -1))
                    pixit_rect_rubout.br.x = MIN(pixit_rect_rubout.br.x, pixit_point.x + p_chunk->width);

                fonty_text_paint_rubout_uchars(
                    p_docu, p_redraw_context, &pixit_point,
                    uchars_AddBytes(ustr_inline, p_chunk->input_ix), len_to_use,
                    p_segment->base_line, p_rgb_back, fonty_handle, &pixit_rect_rubout);

                pixit_rect_rubout.tl.x = pixit_rect_rubout.br.x;
                if(pixit_rect_rubout.tl.x >= p_skel_rect_seg->br.pixit_point.x)
                    rubout = FALSE;
            }
            else
            {
                fonty_text_paint_simple_uchars(
                    p_docu, p_redraw_context, &pixit_point,
                    uchars_AddBytes(ustr_inline, p_chunk->input_ix), len_to_use,
                    p_segment->base_line, p_rgb_back, fonty_handle);
            }

            break;
            }

        case CHUNK_TAB:
            break;

#if TSTR_IS_SBSTR
        case CHUNK_UTF8:
            {
            STATUS status;
            const PC_UCHARS_INLINE uchars_inline = uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix);
            PC_UTF8 utf8 = inline_data_ptr(PC_UTF8, uchars_inline);
            const U32 il_data_size = inline_data_size(uchars_inline);
            QUICK_WBLOCK_WITH_BUFFER(quick_wblock, 64);
            quick_wblock_with_buffer_setup(quick_wblock);

            status = quick_wblock_utf8_add(&quick_wblock, utf8, il_data_size);

            status_assert(status);

            /* paint the the content of the quick_block */
            if(rubout)
            {   /* using its rubout box if required */
                pixit_rect_rubout.br.x = p_skel_rect_seg->br.pixit_point.x;
                if(chunk_ix != (p_segment->end_chunk -1))
                    pixit_rect_rubout.br.x = MIN(pixit_rect_rubout.br.x, pixit_point.x + p_chunk->width);

                fonty_text_paint_rubout_wchars(
                    p_docu, p_redraw_context, &pixit_point,
                    quick_wblock_wchars(&quick_wblock), quick_wblock_chars(&quick_wblock),
                    p_segment->base_line, p_rgb_back, fonty_handle, &pixit_rect_rubout);

                pixit_rect_rubout.tl.x = pixit_rect_rubout.br.x;
                if(pixit_rect_rubout.tl.x >= p_skel_rect_seg->br.pixit_point.x)
                    rubout = FALSE;
            }
            else
            {
                fonty_text_paint_simple_wchars(
                    p_docu, p_redraw_context, &pixit_point,
                    quick_wblock_wchars(&quick_wblock), quick_wblock_chars(&quick_wblock),
                    p_segment->base_line, p_rgb_back, fonty_handle);
            }

            quick_wblock_dispose(&quick_wblock);
            break;
            }
#endif /* TSTR_IS_SBSTR */

        default: default_unhandled();
#if CHECKING
        case CHUNK_DATE:
        case CHUNK_FILE_DATE:
        case CHUNK_PAGE_X:
        case CHUNK_PAGE_Y:
        case CHUNK_SS_NAME:
        case CHUNK_MS_FIELD:
        case CHUNK_WHOLENAME:
        case CHUNK_LEAFNAME:
        case CHUNK_HYPHEN:
#if !TSTR_IS_SBSTR
        case CHUNK_UTF8:
#endif
        case CHUNK_UTF8_AS_TEXT:
#endif
            {
            STATUS status;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
            quick_ublock_with_buffer_setup(quick_ublock);

            status_assert(status = text_from_field_uchars(p_docu, &quick_ublock,
                                                          uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix) CODE_ANALYSIS_ONLY_ARG(p_chunk->input_len),
                                                          &p_skel_rect_seg->tl.page_num, p_style_text_global));

            /* paint the the content of the quick_block */
            if(rubout)
            {   /* using its rubout box if required */
                pixit_rect_rubout.br.x = p_skel_rect_seg->br.pixit_point.x;
                if(chunk_ix != (p_segment->end_chunk -1))
                    pixit_rect_rubout.br.x = MIN(pixit_rect_rubout.br.x, pixit_point.x + p_chunk->width);

                fonty_text_paint_rubout_uchars(
                    p_docu, p_redraw_context, &pixit_point,
                    quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock),
                    p_segment->base_line, p_rgb_back, fonty_handle, &pixit_rect_rubout);

                pixit_rect_rubout.tl.x = pixit_rect_rubout.br.x;
                if(pixit_rect_rubout.tl.x >= p_skel_rect_seg->br.pixit_point.x)
                    rubout = FALSE;
            }
            else
            {
                fonty_text_paint_simple_uchars(
                    p_docu, p_redraw_context, &pixit_point,
                    quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock),
                    p_segment->base_line, p_rgb_back, fonty_handle);
            }

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case CHUNK_SOFT_HYPHEN:
        case CHUNK_RETURN:
            break;
        }

        pixit_point.x += p_chunk->width;
    }

    if(found_some_underlines)
        text_segment_paint_underlines(p_redraw_context, p_formatted_text, p_segment);
}

#endif /* OS */

/******************************************************************************
*
* move backwards through text according to supplied truth table
*
******************************************************************************/

_Check_return_
static S32 /* updated offset */
text_backward(
    _In_reads_(offset) PC_UCHARS_INLINE uchars_inline_in,
    _In_        S32 offset,
    _In_        U8 truth_table[][CHAR_N])
{
    PC_UCHARS_INLINE uchars_inline = uchars_inline_AddBytes(uchars_inline_in, offset);
    S32 this_class = text_classify_char(uchars_inline);

    while(offset)
    {
        S32 charcount = inline_b_bytecount_off(uchars_inline_in, offset);
        PC_UCHARS_INLINE uchars_inline_prev = uchars_inline_AddBytes(uchars_inline_in, offset - charcount);
        S32 class_prev = text_classify_char(uchars_inline_prev);

        if(!truth_table[this_class][class_prev])
            break;

        this_class = class_prev;
        offset -= charcount;
        uchars_inline = uchars_inline_prev;
    }

    return(PtrDiffBytesU32(uchars_inline, uchars_inline_in));
}

/******************************************************************************
*
* classify text character
*
******************************************************************************/

_Check_return_
static S32
text_classify_char(
    /*_In_*/    PC_UCHARS_INLINE uchars_inline)
{
    UCS4 ucs4;

    if(is_inline(uchars_inline))
    {
        switch(inline_code(uchars_inline))
        {
        case IL_TAB:
        case IL_RETURN:
            return(CHAR_SPACE);

        default:
            return(CHAR_TEXT);
        }
    }

    if(CH_SPACE == PtrGetByte(uchars_inline))
        return(CHAR_SPACE);

    ucs4 = uchars_char_decode_NULL((PC_UCHARS) uchars_inline);

    if(t5_ucs4_is_alphabetic(ucs4) || t5_ucs4_is_decimal_digit(ucs4))
        return(CHAR_TEXT);

    return(CHAR_PUNCT);
}

/******************************************************************************
*
* move forwards through text according to supplied truth table
*
******************************************************************************/

_Check_return_
static S32
text_forward(
    _In_reads_(inline_bytes) PC_UCHARS_INLINE uchars_inline_in,
    _In_        S32 offset,
    _In_        U8 truth_table[][CHAR_N],
    _InVal_     S32 inline_bytes)
{
    PC_UCHARS_INLINE uchars_inline = uchars_inline_AddBytes(uchars_inline_in, offset);
    S32 class_prev = offset ? text_classify_char(uchars_inline_AddBytes(uchars_inline, -1)) : CHAR_PUNCT;

    while(offset < inline_bytes)
    {
        S32 this_class = text_classify_char(uchars_inline);
        S32 charcount;

        if(!truth_table[class_prev][this_class])
            break;

        class_prev = this_class;
        charcount = inline_bytecount(uchars_inline);
        offset += charcount;
        ustr_inline_IncBytes(uchars_inline, charcount);
    }

    return(PtrDiffBytesU32(uchars_inline, uchars_inline_in));
}

/******************************************************************************
*
* incremental redisplay for a text object
*
******************************************************************************/

extern void
text_redisplay_fast(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_TEXT_REDISPLAY_INFO p_text_redisplay_info,
    P_TEXT_CACHE p_text_cache,
    _InRef_     PC_SKEL_RECT p_skel_rect_text_old,
    _InRef_     PC_SKEL_RECT p_skel_rect_text_new,
    _InVal_     REDRAW_TAG redraw_tag,
    P_STYLE p_style_text_global)
{
    ARRAY_INDEX seg_ix, seg_ix_start, seg_ix_end;
    P_SEGMENT p_segment_new, p_segment_old;

    if(array_elements(&p_text_redisplay_info->formatted_text.h_segments) !=
       array_elements(&p_text_cache->formatted_text.h_segments))
    {
        seg_ix_start = 0;
        seg_ix_end = array_elements(&p_text_cache->formatted_text.h_segments);
    }
    else
    {
        /* work out which segments must be painted */
        const ARRAY_INDEX n_segments = array_elements(&p_text_cache->formatted_text.h_segments);

        seg_ix_start = S32_MAX;
        seg_ix_end = 0;

        p_segment_new = array_range(&p_text_cache->formatted_text.h_segments, SEGMENT, 0, n_segments),
        p_segment_old = array_range(&p_text_redisplay_info->formatted_text.h_segments, SEGMENT, 0, n_segments);

        for(seg_ix = 0; seg_ix < n_segments; ++seg_ix, ++p_segment_new, ++p_segment_old)
        {
            const PC_CHUNK p_chunk_start_old = array_ptrc(&p_text_redisplay_info->formatted_text.h_chunks, CHUNK, p_segment_old->start_chunk);
            const PC_CHUNK p_chunk_end_old   = array_ptrc(&p_text_redisplay_info->formatted_text.h_chunks, CHUNK, p_segment_old->end_chunk - 1);

            const PC_UCHARS_INLINE p_text_old     = uchars_inline_AddBytes(ustr_inline_from_h_ustr(&p_text_redisplay_info->h_ustr_inline), p_chunk_start_old->input_ix);
            const PC_UCHARS_INLINE p_text_old_end = uchars_inline_AddBytes(ustr_inline_from_h_ustr(&p_text_redisplay_info->h_ustr_inline), p_chunk_end_old->input_ix + p_chunk_end_old->input_len);

            const PC_CHUNK p_chunk_start_new = array_ptrc(&p_text_cache->formatted_text.h_chunks, CHUNK, p_segment_new->start_chunk);
            const PC_CHUNK p_chunk_end_new   = array_ptrc(&p_text_cache->formatted_text.h_chunks, CHUNK, p_segment_new->end_chunk - 1);

            const PC_UCHARS_INLINE p_text_new     = uchars_inline_AddBytes(ustr_inline, p_chunk_start_new->input_ix);
            const PC_UCHARS_INLINE p_text_new_end = uchars_inline_AddBytes(ustr_inline, p_chunk_end_new->input_ix + p_chunk_end_new->input_len);

            const S32 len_old = PtrDiffBytesU32(p_text_old_end, p_text_old);
            const S32 len_new = PtrDiffBytesU32(p_text_new_end, p_text_new);

            if((len_old != len_new) || (0 != short_memcmp32(p_text_new, p_text_old, (U32) len_new)))
            {
                seg_ix_start = MIN(seg_ix,     seg_ix_start);
                seg_ix_end   = MAX(seg_ix + 1, seg_ix_end);
            }
        }
    }

    for(seg_ix = seg_ix_start; seg_ix < seg_ix_end; seg_ix += 1)
    {
        SKEL_RECT skel_rect_seg, skel_rect_clip, skel_rect_work;
        REDRAW_CONTEXT redraw_context;
        RECT_FLAGS rect_flags_clip;

        p_segment_new = array_ptr(&p_text_cache->formatted_text.h_segments, SEGMENT, seg_ix);

        skel_rect_from_segment(&skel_rect_seg, p_segment_new, &p_text_redisplay_info->skel_rect_object.tl.page_num);
        skel_rect_seg.tl.pixit_point.x = p_text_redisplay_info->skel_rect_object.tl.pixit_point.x;
        skel_rect_seg.br.pixit_point.x = p_text_redisplay_info->skel_rect_object.br.pixit_point.x;

        /* we can stop if we get outside the original object rectangle -
         * if the row height is fixed, text outside
         *    the original rectangle won't be displayed;
         * if not fixed and the height is bigger it will have been caught by
         *    format_object_size_set above
        */
        if(skel_point_compare(&skel_rect_seg.tl, &p_text_redisplay_info->skel_rect_object.br, y) >= 0)
            break;

        skel_rect_work = p_text_redisplay_info->skel_rect_work;
        skel_rect_work.tl.page_num = skel_rect_work.br.page_num = skel_rect_seg.tl.page_num;
        skel_rect_intersect(&skel_rect_clip, &p_text_redisplay_info->skel_rect_object, &skel_rect_work);

        RECT_FLAGS_CLEAR(rect_flags_clip);
        rect_flags_clip.reduce_left_by_2  = 1;
        rect_flags_clip.reduce_right_by_1 = 1;
        if(seg_ix == seg_ix_start)
            rect_flags_clip.reduce_up_by_2   = 1;
        if(seg_ix + 1 == seg_ix_end)
            rect_flags_clip.reduce_down_by_1 = 1;

        /* clipping here is done by fast update call */
        if(view_update_fast_start(p_docu, &redraw_context, redraw_tag, &skel_rect_clip, rect_flags_clip, LAYER_CELLS))
        {
            do  {
                text_segment_paint(p_docu,
                                   &redraw_context,
                                   &p_text_cache->formatted_text,
                                   p_segment_new,
                                   ustr_inline,
                                   &skel_rect_seg,
                                   &p_style_text_global->para_style.rgb_back,
                                   TRUE,
                                   p_style_text_global);
            }
            while(view_update_fast_continue(p_docu, &redraw_context));
        }
    }

    trace_4(TRACE_APP_SKEL_DRAW,
            TEXT("old_rect: ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
            p_text_redisplay_info->skel_rect_object.tl.pixit_point.x,
            p_text_redisplay_info->skel_rect_object.tl.pixit_point.y,
            p_text_redisplay_info->skel_rect_object.br.pixit_point.x,
            p_text_redisplay_info->skel_rect_object.br.pixit_point.y);

    trace_4(TRACE_APP_SKEL_DRAW,
            TEXT("new_rect: ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
            p_skel_rect_text_new->tl.pixit_point.x,
            p_skel_rect_text_new->tl.pixit_point.y,
            p_skel_rect_text_new->br.pixit_point.x,
            p_skel_rect_text_new->br.pixit_point.y);

    /* if the top of the new cell rectangle is below the old top,
     * clear the uncovered area of the text object
     */
    if(skel_point_compare(&p_skel_rect_text_new->tl, &p_skel_rect_text_old->tl, y) > 0)
    {
        SKEL_RECT skel_rect_work = p_text_redisplay_info->skel_rect_work;

        for(skel_rect_work.tl.page_num = p_skel_rect_text_old->tl.page_num;
            skel_rect_work.tl.page_num.y <= p_skel_rect_text_new->tl.page_num.y;
            skel_rect_work.tl.page_num.y += 1)
        {
            SKEL_RECT skel_rect_uncovered;
            REDRAW_CONTEXT redraw_context;

            skel_rect_work.br.page_num = skel_rect_work.tl.page_num;
            if(skel_rect_intersect(&skel_rect_uncovered, p_skel_rect_text_old, &skel_rect_work))
            {
                RECT_FLAGS rect_flags;
                PIXIT_RECT pixit_rect;

                RECT_FLAGS_CLEAR(rect_flags);
                rect_flags.reduce_left_by_2  = 1;
                rect_flags.reduce_right_by_1 = 1;

                if(skel_rect_uncovered.tl.page_num.y == p_skel_rect_text_new->tl.page_num.y)
                    skel_rect_uncovered.br.pixit_point.y = p_skel_rect_text_new->tl.pixit_point.y;
                else
                {
                    rect_flags.reduce_up_by_2   = 1;
                    rect_flags.reduce_down_by_1 = 1;
                }

                skel_rect_intersect(&skel_rect_uncovered, &skel_rect_uncovered, &p_text_redisplay_info->skel_rect_object);

                pixit_rect.tl = skel_rect_uncovered.tl.pixit_point;
                pixit_rect.br = skel_rect_uncovered.br.pixit_point;

                if(view_update_fast_start(p_docu, &redraw_context, redraw_tag, &skel_rect_uncovered, rect_flags, LAYER_CELLS))
                {
                    do  {
                        trace_4(TRACE_APP_SKEL_DRAW,
                                TEXT("rect to clear: ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
                                pixit_rect.tl.x, pixit_rect.tl.y, pixit_rect.br.x, pixit_rect.br.y);

                        host_paint_rectangle_filled(&redraw_context, &pixit_rect, &p_style_text_global->para_style.rgb_back);
                    }
                    while(view_update_fast_continue(p_docu, &redraw_context));
                }
            }
        }
    }

    /* if the bottom of the new cell rectangle is above the old bottom,
     * clear the uncovered area of the cell
     */
    if(skel_point_compare(&p_skel_rect_text_new->br, &p_skel_rect_text_old->br, y) < 0)
    {
        SKEL_RECT skel_rect_work = p_text_redisplay_info->skel_rect_work;

        for(skel_rect_work.tl.page_num = p_skel_rect_text_new->br.page_num;
            skel_rect_work.tl.page_num.y <= p_skel_rect_text_old->br.page_num.y;
            skel_rect_work.tl.page_num.y += 1)
        {
            SKEL_RECT skel_rect_uncovered;
            REDRAW_CONTEXT redraw_context;

            skel_rect_work.br.page_num = skel_rect_work.tl.page_num;
            if(skel_rect_intersect(&skel_rect_uncovered, p_skel_rect_text_old, &skel_rect_work))
            {
                RECT_FLAGS rect_flags;
                PIXIT_RECT pixit_rect;

                RECT_FLAGS_CLEAR(rect_flags);
                rect_flags.reduce_left_by_2  = 1;
                rect_flags.reduce_right_by_1 = 1;

                if(skel_rect_uncovered.tl.page_num.y == p_skel_rect_text_new->br.page_num.y)
                    skel_rect_uncovered.tl.pixit_point.y = p_skel_rect_text_new->br.pixit_point.y;
                else
                {
                    rect_flags.reduce_up_by_2   = 1;
                    rect_flags.reduce_down_by_1 = 1;
                }

                skel_rect_intersect(&skel_rect_uncovered, &skel_rect_uncovered, &p_text_redisplay_info->skel_rect_object);

                pixit_rect.tl = skel_rect_uncovered.tl.pixit_point;
                pixit_rect.br = skel_rect_uncovered.br.pixit_point;

                if(view_update_fast_start(p_docu, &redraw_context, redraw_tag, &skel_rect_uncovered, rect_flags, LAYER_CELLS))
                {
                    do  {
                        trace_4(TRACE_APP_SKEL_DRAW,
                                TEXT("rect to clear: ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
                                pixit_rect.tl.x, pixit_rect.tl.y, pixit_rect.br.x, pixit_rect.br.y);

                        host_paint_rectangle_filled(&redraw_context, &pixit_rect, &p_style_text_global->para_style.rgb_back);
                    }
                    while(view_update_fast_continue(p_docu, &redraw_context));
                }
            }
        }
    }

    /* if the cell is now blank, clear the uncovered area */
    if(0 == array_elements(&p_text_cache->formatted_text.h_segments))
    {
        RECT_FLAGS rect_flags;
        PIXIT_RECT pixit_rect;
        REDRAW_CONTEXT redraw_context;

        RECT_FLAGS_CLEAR(rect_flags);
        rect_flags.reduce_left_by_2  = 1;
        rect_flags.reduce_right_by_1 = 1;
        rect_flags.reduce_up_by_2    = 1;
        rect_flags.reduce_down_by_1  = 1;

        pixit_rect.tl = p_text_redisplay_info->skel_rect_object.tl.pixit_point;
        pixit_rect.br = p_text_redisplay_info->skel_rect_object.br.pixit_point;

        if(view_update_fast_start(p_docu, &redraw_context, redraw_tag, &p_text_redisplay_info->skel_rect_object, rect_flags, LAYER_CELLS))
        {
            do  {
                trace_4(TRACE_APP_SKEL_DRAW,
                        TEXT("rect to clear: ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
                        pixit_rect.tl.x, pixit_rect.tl.y, pixit_rect.br.x, pixit_rect.br.y);

                host_paint_rectangle_filled(&redraw_context, &pixit_rect, &p_style_text_global->para_style.rgb_back);
            }
            while(view_update_fast_continue(p_docu, &redraw_context));
        }
    }

    fonty_cache_trash(P_REDRAW_CONTEXT_NONE);
}

/******************************************************************************
*
* dispose of redisplay info
*
******************************************************************************/

extern void
text_redisplay_info_dispose(
    P_TEXT_REDISPLAY_INFO p_text_redisplay_info)
{
    formatted_text_dispose_segments(&p_text_redisplay_info->formatted_text);
    (void) formatted_text_dispose_chunks(&p_text_redisplay_info->formatted_text, 0);
    al_array_dispose(&p_text_redisplay_info->h_ustr_inline);
}

/******************************************************************************
*
* save info required to redisplay text object after mods
*
******************************************************************************/

extern void
text_redisplay_info_save(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_REDISPLAY_INFO p_text_redisplay_info,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object,
    _InVal_     S32 inline_ix)
{
    formatted_text_init(&p_text_redisplay_info->formatted_text);
    p_text_redisplay_info->h_ustr_inline = 0;
    p_text_redisplay_info->object_visible = p_text_format_info->object_visible;
    p_text_redisplay_info->skel_rect_object = p_text_format_info->skel_rect_object;
    p_text_redisplay_info->skel_rect_work = p_text_format_info->skel_rect_work;

    if(p_text_redisplay_info->object_visible)
    {
        /* keep a copy of old text */
        if(p_inline_object->inline_len)
        {
            const P_TEXT_CACHE p_text_cache = p_text_cache_from_data_ref(p_docu, p_text_format_info, p_inline_object);

            if(NULL != p_text_cache)
            {
                ARRAY_INDEX chunk_ix = chunk_ix_from_inline_ix(&p_text_cache->formatted_text, inline_ix);

                status_assert(formatted_text_duplicate(&p_text_redisplay_info->formatted_text, &p_text_cache->formatted_text));
                formatted_text_dispose_segments(&p_text_cache->formatted_text);
                (void) formatted_text_dispose_chunks(&p_text_cache->formatted_text, MAX(0, chunk_ix - 1));
            }

            status_consume(al_array_add(&p_text_redisplay_info->h_ustr_inline, BYTE, p_inline_object->inline_len + 1 /*CH_NULL*/, &array_init_block_u8, p_inline_object->object_data.u.ustr_inline));
        }
    }
}

/******************************************************************************
*
* spellcheck the word in object_data
*
******************************************************************************/

_Check_return_
static STATUS /* 0 == word didn't check, 1 == word did check */
word_spell_check(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data,
    _InoutRef_  P_S32 p_len,
    _InVal_     BOOL mistake_query)
{
    STATUS status = STATUS_OK;
    WORD_CHECK word_check;
    S32 word_end;

    do  {
        PC_USTR_INLINE ustr_inline = p_object_data->u.ustr_inline;
        S32 start, end, word_start, word_len;
        P_UCHARS uchars;
        U32 dst_idx;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);

        offsets_from_object_data(&start, &end, p_object_data, *p_len);

        /* find word limits */
        word_start = text_backward(ustr_inline, start, tab_word_start);
        word_end = text_forward(ustr_inline, word_start, tab_word_end, *p_len);
        word_len = word_end - word_start;

        word_check.status = CHECK_CONTINUE;

        if(NULL == (uchars = quick_ublock_extend_by(&quick_ublock, word_len + 1 /*CH_NULL*/, &status))) /* overallocates in case of inlines, but don't care */
            break;

        dst_idx = uchars_inline_copy_strip(uchars, word_len, uchars_inline_AddBytes(ustr_inline, word_start), word_len);

        PtrPutByteOff(uchars, dst_idx++, CH_NULL);

        if(1 != dst_idx)
        {
            word_check.ustr_word = quick_ublock_ustr(&quick_ublock);
            word_check.object_data = *p_object_data;
            word_check.object_data.object_position_start.object_id =
            word_check.object_data.object_position_end.object_id = word_check.object_data.object_id;
            word_check.object_data.object_position_start.data = word_start;
            word_check.object_data.object_position_end.data = word_end;
            word_check.mistake_query = mistake_query;

            trace_1(TRACE_APP_SKEL,
                    TEXT("OB_TEXT object_check word: %s"),
                    report_ustr(word_check.ustr_word));

            status = object_call_id_load(p_docu, T5_MSG_SPELL_WORD_CHECK, &word_check, OBJECT_ID_SPELL);

            if(word_check.status == CHECK_CONTINUE_RECHECK)
            {
                p_object_data->u.p_object = word_check.object_data.u.p_object;
                *p_len = (P_DATA_NONE != p_object_data->u.ustr_inline) ? ustr_inline_strlen(p_object_data->u.ustr_inline) : 0; /* SKS 11dec94 */
                end = MIN(end, *p_len);
            }
        }

        quick_ublock_dispose(&quick_ublock);

        if(P_DATA_NONE == p_object_data->u.p_object)
            break;

        status_break(status);
    }
    while(CHECK_CONTINUE_RECHECK == word_check.status);

    status_return(status);

    p_object_data->object_position_end.object_id = p_object_data->object_id;
    p_object_data->object_position_end.data = word_end;
    return(word_check.status == CHECK_CONTINUE ? 1 : 0);
}

_Check_return_
static STATUS
skelevent_keys_from_quick_block(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock)
{
    SKELEVENT_KEYS skelevent_keys;
    skelevent_keys.p_quick_ublock = p_quick_ublock;
    skelevent_keys.processed = 0;
    return(object_skel(p_docu, T5_EVENT_KEYS, &skelevent_keys));
}

/******************************************************************************
*
* convert a raw inline string to plain result text
*
******************************************************************************/

_Check_return_
static STATUS
uchars_inline_result_convert(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n)
{
    STATUS status = STATUS_OK;
    U32 offset = 0;

    while((offset < uchars_n) && status_ok(status))
    {
        if(is_inline_off(uchars_inline, offset))
        {
            const U32 bytes_of_inline = inline_bytecount_off(uchars_inline, offset);

            status = text_from_field_uchars(p_docu, p_quick_ublock,
                                            uchars_inline_AddBytes(uchars_inline, offset) CODE_ANALYSIS_ONLY_ARG(bytes_of_inline),
                                            P_PAGE_NUM_NONE, P_STYLE_NONE);

            offset += bytes_of_inline;
        }
        else
        {
            const U32 bytes_of_char = uchars_bytes_of_char_off((PC_UCHARS) uchars_inline, offset);

            status = quick_ublock_uchars_add(p_quick_ublock, uchars_AddBytes(uchars_inline, offset), bytes_of_char);

            offset += bytes_of_char;
        }
    }

    assert(offset == uchars_n);

    /* SKS after 1.07 15jul94 now unterminated */
    return(status);
}

/* transmogrify messages with no arguments into inlines */

T5_CMD_PROTO(static, text_main_cmd_insert_field_none)
{
    STATUS status = STATUS_OK;
    IL_CODE il_code;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, INLINE_OVH + 1);
    quick_ublock_with_buffer_setup(quick_ublock);

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    switch(t5_message)
    {
    case T5_CMD_INSERT_FIELD_WHOLENAME:
        il_code = IL_WHOLENAME;
        break;

    case T5_CMD_INSERT_FIELD_LEAFNAME:
        il_code = IL_LEAFNAME;
        break;

    case T5_CMD_INSERT_FIELD_SOFT_HYPHEN:
        il_code = IL_SOFT_HYPHEN;
        break;

    case T5_CMD_INSERT_FIELD_RETURN:
        il_code = IL_RETURN;
        break;

    default: default_unhandled();
#if CHECKING
    case T5_CMD_INSERT_FIELD_TAB:
#endif
        il_code = IL_TAB;
        break;
    }

    if(status_ok(status = inline_quick_ublock_from_code(&quick_ublock, il_code)))
        status = skelevent_keys_from_quick_block(p_docu, &quick_ublock);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

/* turn page number etc. into inline */

T5_CMD_PROTO(static, text_main_cmd_insert_field_ustr)
{
    STATUS status = STATUS_OK;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    PC_USTR ustr_arg = p_args[0].val.ustr;
    IL_CODE il_code;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, INLINE_OVH + 32);
    quick_ublock_with_buffer_setup(quick_ublock);

    switch(t5_message)
    {
    case T5_CMD_INSERT_FIELD_DATE:
        il_code = IL_DATE;
        break;

    case T5_CMD_INSERT_FIELD_FILE_DATE:
        il_code = IL_FILE_DATE;
        break;

    case T5_CMD_INSERT_FIELD_PAGE_X:
        il_code = IL_PAGE_X;
        break;

    default: default_unhandled();
#if CHECKING
    case T5_CMD_INSERT_FIELD_PAGE_Y:
#endif
        il_code = IL_PAGE_Y;
        break;
    }

    if(status_ok(status = inline_quick_ublock_from_ustr(&quick_ublock, il_code, ustr_arg)))
        status = skelevent_keys_from_quick_block(p_docu, &quick_ublock);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

T5_CMD_PROTO(static, text_main_cmd_insert_field_ss_name)
{
    STATUS status = STATUS_OK;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const S32 s32 = p_args[0].val.s32;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, INLINE_OVH + 32);
    quick_ublock_with_buffer_setup(quick_ublock);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_ok(status = inline_quick_ublock_from_data(&quick_ublock, IL_SS_NAME, IL_TYPE_S32, &s32, 0)))
        status = skelevent_keys_from_quick_block(p_docu, &quick_ublock);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

T5_CMD_PROTO(static, text_main_cmd_insert_field_ms_field)
{
    STATUS status = STATUS_OK;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const S32 field_idx = p_args[0].val.s32 - 1;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, INLINE_OVH + 32);
    quick_ublock_with_buffer_setup(quick_ublock);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_ok(status = inline_quick_ublock_from_data(&quick_ublock, IL_MS_FIELD, IL_TYPE_S32, &field_idx, 0)))
        status = skelevent_keys_from_quick_block(p_docu, &quick_ublock);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

PROC_EVENT_PROTO(extern, proc_event_tx_main)
{
    switch(t5_message)
    {
        /* --- via TEXT_MESSAGE_BLOCK --- */

    case T5_EVENT_REDRAW:
        return(text_main_redraw(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_TAB_WANTED:
        return(text_main_msg_tab_wanted(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT:
        return(text_main_msg_object_position_from_skel_point(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION:
        return(text_main_msg_skel_point_from_object_position(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_OBJECT_LOGICAL_MOVE:
        return(text_main_msg_object_logical_move(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_OBJECT_READ_TEXT_DRAFT:
        return(text_main_msg_object_read_text_draft(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

        /* --- normal --- */

    case T5_MSG_OBJECT_POSITION_SET:
        return(text_main_msg_object_position_set(p_docu, t5_message, (P_OBJECT_POSITION_SET) p_data));

    case T5_MSG_OBJECT_WORD_CHECK:
        return(text_main_msg_object_word_check(p_docu, t5_message, (P_OBJECT_DATA) p_data));

    case T5_MSG_OBJECT_CHECK:
        return(text_main_msg_object_check(p_docu, t5_message, (P_OBJECT_CHECK) p_data));

    case T5_MSG_OBJECT_STRING_SEARCH:
        return(text_main_msg_object_string_search(p_docu, t5_message, (P_OBJECT_STRING_SEARCH) p_data));

    case T5_MSG_OBJECT_READ_TEXT:
        return(text_main_msg_object_read_text(p_docu, t5_message, (P_OBJECT_READ_TEXT) p_data));

    case T5_CMD_WORD_COUNT:
        return(text_main_cmd_word_count(p_docu, t5_message, (P_OBJECT_WORD_COUNT) p_data));

    case T5_CMD_INSERT_FIELD_WHOLENAME:
    case T5_CMD_INSERT_FIELD_LEAFNAME:
    case T5_CMD_INSERT_FIELD_SOFT_HYPHEN:
    case T5_CMD_INSERT_FIELD_RETURN:
    case T5_CMD_INSERT_FIELD_TAB:
        return(text_main_cmd_insert_field_none(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_INSERT_FIELD_DATE:
    case T5_CMD_INSERT_FIELD_FILE_DATE:
    case T5_CMD_INSERT_FIELD_PAGE_X:
    case T5_CMD_INSERT_FIELD_PAGE_Y:
        return(text_main_cmd_insert_field_ustr(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_INSERT_FIELD_SS_NAME:
        return(text_main_cmd_insert_field_ss_name(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_INSERT_FIELD_MS_FIELD:
        return(text_main_cmd_insert_field_ms_field(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of tx_main.c */
