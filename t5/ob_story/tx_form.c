/* tx_form.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Text formatting for Fireworkz */

/* MRJC February 1992 */

#include "common/gflags.h"

#include "ob_story/ob_story.h"

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

/******************************************************************************
*
* find the chunk corresponding to a given position
*
******************************************************************************/

_Check_return_
extern ARRAY_INDEX
chunk_ix_from_inline_ix(
    P_FORMATTED_TEXT p_formatted_text,
    _InVal_     S32 string_ix)
{
    const ARRAY_INDEX n_chunks = array_elements(&p_formatted_text->h_chunks);
    ARRAY_INDEX chunk_ix;
    PC_CHUNK p_chunk;

    for(chunk_ix = 0, p_chunk = array_rangec(&p_formatted_text->h_chunks, CHUNK, 0, n_chunks);
        chunk_ix < n_chunks;
        ++chunk_ix, ++p_chunk)
    {
        if((string_ix >= p_chunk->input_ix) && (string_ix < p_chunk->input_ix + p_chunk->input_len))
            break;
    }

    return(chunk_ix);
}

/******************************************************************************
*
* return the bounding box for some formatted text
*
* _para gives box for whole paragraph (inc start
*       and end space
* _text gives box for text only
*
******************************************************************************/

extern void
formatted_text_box(
    _OutRef_    P_SKEL_RECT p_skel_rect_para,
    _OutRef_    P_SKEL_RECT p_skel_rect_text,
    P_FORMATTED_TEXT p_formatted_text,
    _InRef_     PC_PAGE_NUM p_page_offset)
{
    /* take a copy of page offset (it might be in output skel_rect) */
    PAGE_NUM page_offset = *p_page_offset;

    *p_skel_rect_text = p_formatted_text->skel_rect;
    p_skel_rect_text->tl.page_num.x += page_offset.x;
    p_skel_rect_text->tl.page_num.y += page_offset.y;
    p_skel_rect_text->br.page_num.x += page_offset.x;
    p_skel_rect_text->br.page_num.y += page_offset.y;
    *p_skel_rect_para = *p_skel_rect_text;

    p_skel_rect_para->tl.pixit_point.y -= p_formatted_text->para_start;
    p_skel_rect_para->br.pixit_point.y += p_formatted_text->para_end;
}

/******************************************************************************
*
* dipose of some or all chunks
*
******************************************************************************/

/*ncr*/
extern S32 /* 1 == all disposed of */
formatted_text_dispose_chunks(
    _InoutRef_  P_FORMATTED_TEXT p_formatted_text,
    _InVal_     ARRAY_INDEX chunk_ix /* place to start chunk disposal */)
{
    S32 res = 0;
    const ARRAY_INDEX n_chunks = array_elements(&p_formatted_text->h_chunks);

    if((chunk_ix <= 0) || (0 == n_chunks))
    {
        al_array_dispose(&p_formatted_text->h_chunks);
        res = 1;
    }
    else
    {
        ARRAY_INDEX n_chunks_new = MIN(n_chunks - 1, chunk_ix);
        al_array_shrink_by(&p_formatted_text->h_chunks, n_chunks_new - n_chunks);
    }

    return(res);
}

/******************************************************************************
*
* dispose of segments
*
******************************************************************************/

extern void
formatted_text_dispose_segments(
    _InoutRef_  P_FORMATTED_TEXT p_formatted_text)
{
    al_array_dispose(&p_formatted_text->h_segments);
}

/******************************************************************************
*
* duplicate a formatted text entry
*
******************************************************************************/

_Check_return_
extern STATUS
formatted_text_duplicate(
    _OutRef_    P_FORMATTED_TEXT p_formatted_text_out,
    _InRef_     P_FORMATTED_TEXT p_formatted_text_in)
{
    *p_formatted_text_out = *p_formatted_text_in;

    p_formatted_text_out->h_segments = 0;
    p_formatted_text_out->h_chunks = 0;
    status_return(al_array_duplicate(&p_formatted_text_out->h_segments, &p_formatted_text_in->h_segments));
    return(al_array_duplicate(&p_formatted_text_out->h_chunks, &p_formatted_text_in->h_chunks));
}

/******************************************************************************
*
* initialise the formatted_text structure
*
******************************************************************************/

extern void
formatted_text_init(
    _OutRef_    P_FORMATTED_TEXT p_formatted_text)
{
    zero_struct_ptr(p_formatted_text);
}

/******************************************************************************
*
* adjust segment position of formatted text for vertical justification
* do after formatting, just prior to redraw
*
******************************************************************************/

/*ncr*/
extern S32 /* text moved vertically */
formatted_text_justify_vertical(
    P_FORMATTED_TEXT p_formatted_text,
    P_TEXT_FORMAT_INFO p_text_format_info,
    _InRef_     PC_PAGE_NUM p_page_offset)
{
    PIXIT old_justify_v_offset_y = p_formatted_text->justify_v_offset_y;

    switch(p_text_format_info->style_text_global.para_style.justify_v)
    {
    default:
        break;

    case SF_JUSTIFY_V_CENTRE:
    case SF_JUSTIFY_V_BOTTOM:
        {
        SKEL_RECT skel_rect_para, skel_rect_text;

        formatted_text_box(&skel_rect_para, &skel_rect_text, p_formatted_text, p_page_offset);

        /* work out vertical position */
        if(skel_rect_text.br.page_num.y == p_text_format_info->skel_rect_object.tl.page_num.y)
        {
            PIXIT offset_y;
            PIXIT vertical_space = p_text_format_info->skel_rect_object.br.pixit_point.y
                                 - skel_rect_text.br.pixit_point.y
                                 - p_text_format_info->style_text_global.para_style.para_end;

            if(SF_JUSTIFY_V_CENTRE == p_text_format_info->style_text_global.para_style.justify_v)
                vertical_space /= 2;

            vertical_space = MAX(0, vertical_space);

            offset_y = vertical_space;

            if(offset_y != p_formatted_text->justify_v_offset_y)
            {
                ARRAY_INDEX seg_ix;
                const ARRAY_INDEX n_segments = array_elements(&p_formatted_text->h_segments);
                P_SEGMENT p_segment;
                PIXIT offset_delta = offset_y - p_formatted_text->justify_v_offset_y;

                for(seg_ix = 0, p_segment = array_range(&p_formatted_text->h_segments, SEGMENT, 0, n_segments);
                    seg_ix < n_segments;
                    seg_ix += 1, p_segment += 1)
                {
                    p_segment->skel_point.pixit_point.y += offset_delta;
                }

                p_formatted_text->justify_v_offset_y += offset_delta;
            }
        }

        break;
        }
    }

    return(old_justify_v_offset_y != p_formatted_text->justify_v_offset_y);
}

/******************************************************************************
*
* find the total width of some formatted text
*
******************************************************************************/

extern PIXIT
formatted_text_width_max(
    P_FORMATTED_TEXT p_formatted_text,
    P_TEXT_FORMAT_INFO p_text_format_info)
{
    PIXIT width = 0, width_max = 0;

    if(p_formatted_text->h_chunks)
    {
        const ARRAY_INDEX n_chunks = array_elements(&p_formatted_text->h_chunks);
        ARRAY_INDEX chunk_ix;
        PC_CHUNK p_chunk;
        PIXIT trail_space = 0;

        width = p_text_format_info->style_text_global.para_style.margin_left +
                p_text_format_info->style_text_global.para_style.margin_para +
                p_text_format_info->style_text_global.para_style.margin_right;

        for(chunk_ix = 0, p_chunk = array_rangec(&p_formatted_text->h_chunks, CHUNK, 0, n_chunks);
            chunk_ix < n_chunks;
            ++chunk_ix, ++p_chunk)
        {
            switch(p_chunk->type)
            {
            case CHUNK_SOFT_HYPHEN:
            case CHUNK_HYPHEN:
                trail_space = 0;
                break;

            case CHUNK_RETURN:
                width_max = MAX(width - trail_space, width_max);
                trail_space = 0;
                width = p_text_format_info->style_text_global.para_style.margin_left +
                        p_text_format_info->style_text_global.para_style.margin_right;
                break;

            default: default_unhandled();
#if CHECKING
            case CHUNK_FREE:
            case CHUNK_TAB:
            case CHUNK_DATE:
            case CHUNK_FILE_DATE:
            case CHUNK_PAGE_Y:
            case CHUNK_PAGE_X:
            case CHUNK_SS_NAME:
            case CHUNK_MS_FIELD:
            case CHUNK_WHOLENAME:
            case CHUNK_LEAFNAME:
            case CHUNK_UTF8:
            case CHUNK_UTF8_AS_TEXT:
#endif
                width += p_chunk->width;
                trail_space = p_chunk->fonty_chunk.trail_space;
                break;
            }
        }

        width -= trail_space;
    }

    return(MAX(width, width_max));
}

/******************************************************************************
*
* produce plain text output from a chunk of formatted text
*
******************************************************************************/

_Check_return_
extern STATUS
plain_text_from_formatted_text(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_plain_text,
    P_FORMATTED_TEXT p_formatted_text,
    _In_z_      PC_USTR_INLINE ustr_inline,
    _InRef_     PC_PAGE_NUM p_page_num,
    _InRef_     PC_STYLE p_style_text_global)
{
    STATUS status = STATUS_OK;
    const ARRAY_INDEX n_segments = array_elements(&p_formatted_text->h_segments);
    ARRAY_INDEX seg_ix;
    P_SEGMENT p_segment;
    PAGE page_y = 0;
    U8 effects[PLAIN_EFFECT_COUNT];
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
    quick_ublock_with_buffer_setup(quick_ublock);

    zero_array(effects);

    *p_h_plain_text = 0;

    for(seg_ix = 0, p_segment = array_range(&p_formatted_text->h_segments, SEGMENT, 0, n_segments);
        seg_ix < n_segments;
        ++seg_ix, ++p_segment)
    {
        ARRAY_INDEX chunk_ix;
        PC_CHUNK p_chunk;
        PIXIT queued_pixit_space = 0;

        if(!seg_ix)
            page_y = p_segment->skel_point.page_num.y;
        else if(page_y != p_segment->skel_point.page_num.y)
        {
            status_break(status = inline_quick_ublock_from_data(&quick_ublock, IL_PAGE_BREAK, IL_TYPE_NONE, NULL, 0));
            status = plain_text_segment_out(p_h_plain_text, &quick_ublock);
            quick_ublock_dispose(&quick_ublock);
            status_break(status);
            page_y = p_segment->skel_point.page_num.y;
        }

        /*********************** output chunks from segment *************************/

        for(chunk_ix = p_segment->start_chunk, p_chunk = array_ptrc(&p_formatted_text->h_chunks, CHUNK, chunk_ix);
            chunk_ix < p_segment->end_chunk;
            ++chunk_ix, ++p_chunk)
        {
            const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(p_chunk->fonty_chunk.fonty_handle);

            queued_pixit_space += p_chunk->lead_space;

            switch(p_chunk->type)
            {
            case CHUNK_FREE:
#if 0 /* MRJC 24.3.95 switched off to make underlining continuous */
                status_break(status = plain_text_effects_off(&quick_ublock, effects));
#endif
                status_break(status = plain_text_spaces_out(&quick_ublock,
                                                            chars_from_pixit(queued_pixit_space, p_font_context->space_width)));
                queued_pixit_space -= status * p_font_context->space_width;
                status_break(status = plain_text_effects_update(&quick_ublock, effects, &p_font_context->font_spec));
                status_break(status = plain_text_chars_out(&quick_ublock,
                                                           uchars_AddBytes(ustr_inline, p_chunk->input_ix),
                                                           p_chunk->input_len - p_chunk->trail_spaces));
                queued_pixit_space += p_chunk->width - status * p_font_context->space_width;
                break;

            case CHUNK_TAB:
                queued_pixit_space += p_chunk->width;
                break;

            case CHUNK_SOFT_HYPHEN:
                break;

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
            case CHUNK_UTF8:
            case CHUNK_UTF8_AS_TEXT:
#endif
#if 0 /* MRJC 24.3.95 switched off to make underlining continuous */
                status_break(status = plain_text_effects_off(&quick_ublock, effects));
#endif
                status_break(status = plain_text_spaces_out(&quick_ublock,
                                                            chars_from_pixit(queued_pixit_space, p_font_context->space_width)));
                queued_pixit_space -= status * p_font_context->space_width;
                status_break(status = plain_text_effects_update(&quick_ublock, effects, &p_font_context->font_spec));
                status_break(status = text_from_field_uchars(p_docu, &quick_ublock,
                                                             uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix) CODE_ANALYSIS_ONLY_ARG(p_chunk->input_len),
                                                             p_page_num, p_style_text_global));
                queued_pixit_space += p_chunk->width - status * p_font_context->space_width;
                break;

            /* should only be encountered at segment end anyway */
            case CHUNK_RETURN:
                assert(chunk_ix + 1 == p_segment->end_chunk);
                break;
            }

            status_break(status);
        }

        status_break(status);

        /* switch off all effects at end of segment */
        status_break(status = plain_text_effects_off(&quick_ublock, effects));

        /*********************** output segment to output array *************************/
        status_break(status = plain_text_segment_out(p_h_plain_text, &quick_ublock));
        quick_ublock_dispose(&quick_ublock);

        if(p_segment->leading >= 2 * p_segment->base_line) /* output another new line for draft double space */
        {
            status_break(status = plain_text_segment_out(p_h_plain_text, &quick_ublock));
            quick_ublock_dispose(&quick_ublock);
        }
    }

    quick_ublock_dispose(&quick_ublock);

    /* chuck it all away if we failed */
    if(status_fail(status))
        plain_text_dispose(p_h_plain_text);

    return(status);
}

/******************************************************************************
*
* calculate which segment contains the
* given inline index
*
******************************************************************************/

_Check_return_
extern ARRAY_INDEX
segment_ix_from_inline_ix(
    P_FORMATTED_TEXT p_formatted_text,
    _InVal_     S32 string_ix)
{
    const ARRAY_INDEX n_segments = array_elements(&p_formatted_text->h_segments);
    ARRAY_INDEX seg_ix;
    PC_SEGMENT p_segment;

    for(seg_ix = 0, p_segment = array_rangec(&p_formatted_text->h_segments, SEGMENT, 0, n_segments);
        seg_ix < n_segments;
        ++seg_ix, ++p_segment)
    {
        ARRAY_INDEX chunk_ix;
        PC_CHUNK p_chunk;

        for(chunk_ix = p_segment->start_chunk, p_chunk = array_rangec(&p_formatted_text->h_chunks, CHUNK, chunk_ix, p_segment->end_chunk - chunk_ix);
            chunk_ix < p_segment->end_chunk;
            ++chunk_ix, ++p_chunk)
        {
            if((string_ix >= p_chunk->input_ix) && (string_ix < p_chunk->input_ix + p_chunk->input_len))
                return(seg_ix);
        }
    }

    return(n_segments - 1);
}

/******************************************************************************
*
* calculate the bounding box of a segment
*
******************************************************************************/

extern P_SKEL_RECT
skel_rect_from_segment(
    _OutRef_    P_SKEL_RECT p_skel_rect,
    _InRef_     PC_SEGMENT p_segment,
    _InRef_     PC_PAGE_NUM p_page_offset)
{
    /* take copy of input page offset */
    PAGE_NUM page_offset = *p_page_offset;

    p_skel_rect->tl = p_segment->skel_point;

    /* add in offset */
    p_skel_rect->tl.page_num.x += page_offset.x;
    p_skel_rect->tl.page_num.y += page_offset.y;

    p_skel_rect->br = p_skel_rect->tl;
    p_skel_rect->br.pixit_point.x += p_segment->width;
    p_skel_rect->br.pixit_point.y += p_segment->leading;

    return(p_skel_rect);
}

/******************************************************************************
*
* given a tab list and a position, return
* the tab information for the next tab stop
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_TAB_INFO
tab_next_from_pixit_position(
    P_ARRAY_HANDLE p_h_tab_list,
    _InVal_     PIXIT position,
    _InVal_     PIXIT margin_left)
{
    const ARRAY_INDEX n_elements = array_elements(p_h_tab_list);
    ARRAY_INDEX i;
    P_TAB_INFO p_tab_info;

    for(i = 0, p_tab_info = array_range(p_h_tab_list, TAB_INFO, 0, n_elements); i < n_elements; ++i, ++p_tab_info)
        if(position < margin_left + p_tab_info->offset)
            return(p_tab_info);

    return(NULL);
}

/******************************************************************************
*
* take an inline string and convert it to chunks ready for segmentation later
*
* a chunk is at most a word; it may be less where inline style changes cut the word
*
* --out--
* array of chunks
*
******************************************************************************/

#if (WINDOWS && TSTR_IS_SBSTR) || (RISCOS && USTR_IS_SBSTR)

_Check_return_
static STATUS
text_chunkify_CHUNK_UTF8(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      P_USTR_INLINE ustr_inline_base,
    _InoutRef_  P_CHUNK p_chunk,
    _InVal_     FONTY_HANDLE fonty_handle)
{
    STATUS status = STATUS_OK;
    const PC_UCHARS_INLINE uchars_inline = uchars_inline_AddBytes(ustr_inline_base, p_chunk->input_ix);
    PC_UTF8 utf8 = inline_data_ptr(PC_UTF8, uchars_inline);
    const U32 il_data_size = inline_data_size(uchars_inline);

#if RISCOS && USTR_IS_SBSTR

    HOST_FONT host_font_utf8 = fonty_host_font_utf8_from_fonty_handle_formatting(fonty_handle, p_docu->flags.draft_mode);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(HOST_FONT_NONE == host_font_utf8)
    {
        /*reportf("CHUNK_UTF8: no host font handle for formatting");*/
        return(STATUS_OK); /* quick out for older non-UCS FM systems */
    }

    /* Convert from UTF8 inline sequence (already UTF-8) */
    status = quick_ublock_uchars_add(&quick_ublock, (PC_UCHARS) utf8, il_data_size);

    status_return(status_wrap(status));

    /* read all about it */
    fonty_chunk_info_read_utf8(
        p_docu,
        &p_chunk->fonty_chunk,
        fonty_handle,
        quick_ublock_uchars(&quick_ublock),
        quick_ublock_bytes(&quick_ublock),
        p_chunk->trail_spaces);

    quick_ublock_dispose(&quick_ublock);

    if(0 == p_chunk->fonty_chunk.width_mp)
    {
        /*reportf("CHUNK_UTF8: zero-width glyph - display as U+XXXX");*/
        return(STATUS_OK); /* zero-width glyph */
    }

#elif WINDOWS && TSTR_IS_SBSTR

    QUICK_WBLOCK_WITH_BUFFER(quick_wblock, 100);
    quick_wblock_with_buffer_setup(quick_wblock);

    /* Convert from UTF8 inline sequence (UTF-8) to UTF-16 */
    status = quick_wblock_utf8_add(&quick_wblock, utf8, il_data_size);

    status_return(status_wrap(status));

    /* read all about it */
    fonty_chunk_info_read_wchars(
        p_docu,
        &p_chunk->fonty_chunk,
        fonty_handle,
        quick_wblock_wchars(&quick_wblock),
        quick_wblock_chars(&quick_wblock),
        p_chunk->trail_spaces);

    quick_wblock_dispose(&quick_wblock);

#if 0
    /*reportf("CHUNK_UTF8: pretend to be zero-width glyph - display as U+XXXX");*/
    return(STATUS_OK); /* pretend not to be able to do it */
#endif

#endif /* WINDOWS && TSTR_IS_SBSTR */

    return(STATUS_DONE);
}

#endif /* (WINDOWS && TSTR_IS_SBSTR) || (RISCOS && USTR_IS_SBSTR) */

_Check_return_
static STATUS
text_chunkify(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_chunk_array,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object)
{
    STATUS status = STATUS_OK;
    P_USTR_INLINE ustr_inline;
    SC_ARRAY_INIT_BLOCK chunk_init_block = aib_init(10, sizeof32(CHUNK), TRUE /* need this */);
    PIXIT lhs_white_space;
    P_STYLE_SUB_CHANGE p_style_sub_change;
    ARRAY_INDEX style_sub_change_ix, chunk_ix, n_style_sub_changes;
    ARRAY_HANDLE h_style_sub_changes;
    U8 need_chunk = 0;
    P_USTR_INLINE ustr_inline_base = p_inline_object->object_data.u.ustr_inline;

    assert(p_text_format_info->h_style_list != 0);

    {
    /* we hope that we've been given an object_position_start set up in object_data, but
     * if not, we'll use the object_id in object_data in a desperate attempt to find
     * any regions which apply to this cell/data
     */
    OBJECT_POSITION object_position = p_inline_object->object_data.object_position_start;

    if(OBJECT_ID_NONE == object_position.object_id)
        object_position.object_id = p_inline_object->object_data.object_id;

    h_style_sub_changes = style_sub_changes(p_docu,
                                            &style_selector_font_spec,
                                            &p_inline_object->object_data.data_ref,
                                            &object_position,
                                            &p_text_format_info->h_style_list);
    assert(h_style_sub_changes);
    n_style_sub_changes = array_elements(&h_style_sub_changes);
    } /*block*/

    ustr_inline = ustr_inline_base;
    lhs_white_space = p_text_format_info->style_text_global.para_style.margin_left;

    /* find end of chunk array */
    if((chunk_ix = array_elements(p_h_chunk_array)) != 0)
    {
        PC_CHUNK p_chunk = array_ptrc(p_h_chunk_array, CHUNK, chunk_ix - 1);
        ustr_inline_IncBytes_wr(ustr_inline, p_chunk->input_ix + p_chunk->input_len);
    }
    else
        lhs_white_space += p_text_format_info->style_text_global.para_style.margin_para;

    style_sub_change_ix = 0;
    p_style_sub_change = array_ptr(&h_style_sub_changes, STYLE_SUB_CHANGE, style_sub_change_ix);

    if(p_inline_object->inline_len != 0)
    {
        while(status_ok(status) && (PtrGetByte(ustr_inline) || need_chunk))
        {
            P_CHUNK p_chunk;
            P_USTR_INLINE p_trail;
            U8 consume_trailing_spaces = 1;

            /* skip any unused style changes */
            while(style_sub_change_ix + 1 < n_style_sub_changes
                  &&
                  PtrDiffBytesS32(ustr_inline, ustr_inline_base) >= p_style_sub_change[1].position.object_position.data)
            {
                p_style_sub_change += 1;
                style_sub_change_ix += 1;
            }

            /************ make a chunk ***********/
            if(NULL == (p_chunk = al_array_extend_by(p_h_chunk_array, CHUNK, 1, &chunk_init_block, &status)))
                break;

            /* set chunk data index */
            need_chunk = 0;
            p_chunk->type = CHUNK_FREE;
            p_chunk->input_ix = PtrDiffBytesU32(ustr_inline, ustr_inline_base);

            /* skip any unused style changes */
            while(style_sub_change_ix + 1 < n_style_sub_changes
                  &&
                  PtrDiffBytesS32(ustr_inline, ustr_inline_base) >= p_style_sub_change[1].position.object_position.data)
            {
                p_style_sub_change += 1;
                style_sub_change_ix += 1;
            }

            /* munge data into chunk */
            if(is_inline(ustr_inline))
            {
                switch(inline_code(ustr_inline))
                {
                case IL_DATE:
                    p_chunk->type = CHUNK_DATE;
                    break;

                case IL_FILE_DATE:
                    p_chunk->type = CHUNK_FILE_DATE;
                    break;

                case IL_PAGE_X:
                    p_chunk->type = CHUNK_PAGE_X;
                    break;

                case IL_PAGE_Y:
                    p_chunk->type = CHUNK_PAGE_Y;
                    break;

                case IL_SS_NAME:
                    p_chunk->type = CHUNK_SS_NAME;
                    break;

                case IL_MS_FIELD:
                    p_chunk->type = CHUNK_MS_FIELD;
                    break;

                case IL_WHOLENAME:
                    p_chunk->type = CHUNK_WHOLENAME;
                    break;

                case IL_LEAFNAME:
                    p_chunk->type = CHUNK_LEAFNAME;
                    break;

                case IL_RETURN:
                    p_chunk->type = CHUNK_RETURN;
                    /* ensure we get a chunk following a return */
                    need_chunk = 1;
                    break;

                case IL_SOFT_HYPHEN:
                    p_chunk->type = CHUNK_SOFT_HYPHEN;
                    break;

                case IL_TAB:
                    p_chunk->type = CHUNK_TAB;
                    consume_trailing_spaces = 0;
                    break;

                case IL_UTF8:
                    p_chunk->type = CHUNK_UTF8;
                    break;

                default: default_unhandled(); break;
                }

                inline_advance(P_USTR_INLINE, ustr_inline);
            }
            else
            {
                U8 stop = 0;

                /* consume leading spaces into chunk */
                while(CH_SPACE == PtrGetByte(ustr_inline))
                {
                    ustr_inline_IncByte_wr(ustr_inline);

                    if(style_sub_change_ix + 1 < n_style_sub_changes
                       &&
                       PtrDiffBytesS32(ustr_inline, ustr_inline_base) == p_style_sub_change[1].position.object_position.data)
                    {
                        stop = 1;
                        break;
                    }
                }

                if(!stop)
                {
                    U8 done_some = 0;

                    while(PtrGetByte(ustr_inline))
                    {
                        if((CH_SPACE == PtrGetByte(ustr_inline)) || (is_inline(ustr_inline)))
                            break;

                        /* subsume trailing hyphens */
                        if(done_some && (PtrGetByte(ustr_inline) == CH_HYPHEN_MINUS))
                        {
                            while(PtrGetByte(ustr_inline) == CH_HYPHEN_MINUS)
                                ustr_inline_IncByte_wr(ustr_inline);
                            break;
                        }

                        inline_advance(P_USTR_INLINE, ustr_inline);
                        done_some = 1;

                        if(style_sub_change_ix + 1 < n_style_sub_changes
                           &&
                           PtrDiffBytesS32(ustr_inline, ustr_inline_base) == p_style_sub_change[1].position.object_position.data)
                            break;
                    }
                }
            }

            /* note end of word */
            p_trail = ustr_inline;

            /* consume trailing spaces */
            if(consume_trailing_spaces)
                while(CH_SPACE == PtrGetByte(ustr_inline))
                    ustr_inline_IncByte_wr(ustr_inline);

            /* set chunk length */
            p_chunk->input_len = PtrDiffBytesS32(ustr_inline, ustr_inline_base) - p_chunk->input_ix;
            p_chunk->trail_spaces = PtrDiffBytesU32(ustr_inline, p_trail);

            { /* set up chunk data */
            FONTY_HANDLE fonty_handle = fonty_handle_from_font_spec(&p_style_sub_change->style.font_spec, p_docu->flags.draft_mode);

            if(status_ok(fonty_handle))
            {
                if(p_chunk->input_len)
                {
                    PIXIT width_text = p_text_format_info->skel_rect_object.br.pixit_point.x
                                     - p_text_format_info->skel_rect_object.tl.pixit_point.x
                                     - p_text_format_info->style_text_global.para_style.margin_right;

                    switch(p_chunk->type)
                    {
                    case CHUNK_FREE:
                        fonty_chunk_info_read_uchars(
                            p_docu,
                            &p_chunk->fonty_chunk,
                            fonty_handle,
                            uchars_AddBytes(ustr_inline_base, p_chunk->input_ix),
                            p_chunk->input_len - p_chunk->trail_spaces,
                            p_chunk->trail_spaces);
                        break;

                    case CHUNK_RETURN:
                    case CHUNK_TAB:
                        fonty_chunk_init(&p_chunk->fonty_chunk, fonty_handle);
                        break;

                    case CHUNK_HYPHEN:
                        assert0();
                        break;

#if (WINDOWS && TSTR_IS_SBSTR) || (RISCOS && USTR_IS_SBSTR)
                    case CHUNK_UTF8:
                        status = text_chunkify_CHUNK_UTF8(p_docu, ustr_inline_base, p_chunk, fonty_handle);
                        if(STATUS_OK == status)
                        {
                            /*reportf("CHUNK_UTF8: failed formatting - mutate to CHUNK_UTF8_AS_TEXT");*/
                            p_chunk->type = CHUNK_UTF8_AS_TEXT; /* mutate for consistent handling */
                            goto text_from_field;
                        }
                        break;
#endif /* (WINDOWS && TSTR_IS_SBSTR) || (RISCOS && USTR_IS_SBSTR) */

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
                    case CHUNK_SOFT_HYPHEN:
#if !(WINDOWS && TSTR_IS_SBSTR) && !(RISCOS && USTR_IS_SBSTR)
                    case CHUNK_UTF8:
#endif
                    case CHUNK_UTF8_AS_TEXT:
#endif
#if (WINDOWS && TSTR_IS_SBSTR) || (RISCOS && USTR_IS_SBSTR)
                    text_from_field:;
#endif
                        {
                        STATUS status;
                        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
                        quick_ublock_with_buffer_setup(quick_ublock);

                        status_assert(status = text_from_field_uchars(p_docu, &quick_ublock,
                                                                      uchars_inline_AddBytes(ustr_inline_base, p_chunk->input_ix) CODE_ANALYSIS_ONLY_ARG(p_chunk->input_len),
                                                                      &p_text_format_info->skel_rect_object.tl.page_num, &p_text_format_info->style_text_global));

                        /* read all about it */
                        fonty_chunk_info_read_uchars(
                            p_docu,
                            &p_chunk->fonty_chunk,
                            fonty_handle,
                            quick_ublock_uchars(&quick_ublock),
                            status,
                            p_chunk->trail_spaces);

                        quick_ublock_dispose(&quick_ublock);
                        break;
                        }
                    }

                    /* does the chunk need subdividing ? */
                    if(p_chunk->type == CHUNK_FREE
                       &&
                       p_chunk->fonty_chunk.width - p_chunk->fonty_chunk.trail_space > width_text - lhs_white_space)
                    {
                        S32 sub_len;
                        PIXIT pos = width_text - lhs_white_space;

                        sub_len = fonty_chunk_index_from_pixit(p_docu,
                                                               &p_chunk->fonty_chunk,
                                                               uchars_AddBytes(ustr_inline_base, p_chunk->input_ix),
                                                               p_chunk->input_len,
                                                               &pos);

                        /* make sure chunk is the narrower of the two possibilities */
                        if(sub_len && pos > width_text - lhs_white_space)
                            sub_len -= 1;

                        PtrDecBytes(P_USTR_INLINE, ustr_inline, (p_chunk->input_len - sub_len));
                        p_chunk->input_len = sub_len;

                        if(p_chunk->input_len == 0)
                        {
                            p_chunk->input_len = inline_bytecount(ustr_inline);
                            ustr_inline_IncBytes_wr(ustr_inline, p_chunk->input_len);
                        }

                        /* consume trailing spaces */
                        p_chunk->trail_spaces = 0;
                        while(CH_SPACE == PtrGetByte(ustr_inline))
                        {
                            ustr_inline_IncByte_wr(ustr_inline);
                            p_chunk->trail_spaces += 1;
                            p_chunk->input_len += 1;
                        }

                        /* re-read chunk details */
                        fonty_chunk_info_read_uchars(
                            p_docu,
                            &p_chunk->fonty_chunk,
                            fonty_handle,
                            uchars_AddBytes(ustr_inline_base, p_chunk->input_ix),
                            p_chunk->input_len,
                            p_chunk->trail_spaces);
                    }
                }
                else
                    /* zero length chunk */
                    fonty_chunk_init(&p_chunk->fonty_chunk, fonty_handle);

                /* calculate leading and base line position */
                p_chunk->leading = style_leading_from_style(&p_text_format_info->style_text_global,
                                                            &p_style_sub_change->style.font_spec,
                                                            p_docu->flags.draft_mode);

                p_chunk->base_line = fonty_base_line(p_chunk->leading,
                                                     p_style_sub_change->style.font_spec.size_y,
                                                     p_chunk->fonty_chunk.ascent);
            }
            } /*block*/

            lhs_white_space = p_text_format_info->style_text_global.para_style.margin_left;
        }
#if 0 /* 16.8.94 this appears superfluous */
        while(status_ok(status)
              &&
              (*ustr_inline || need_chunk));
#endif
    }

    al_array_auto_compact_set(p_h_chunk_array);

    { /* garbage collect unused chunks */
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.shrink = 1;
    consume(S32, al_array_garbage_collect(p_h_chunk_array, 0, NULL, al_garbage_flags));
    } /*block*/

    return(status);
}

/******************************************************************************
*
* convert an embedded field indirection into replacement text
*
******************************************************************************/

_Check_return_
extern STATUS
text_from_field_uchars(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 uchars_n),
    _InRef_opt_ PC_PAGE_NUM p_page_num,
    _InRef_opt_ PC_STYLE p_style_text_global)
{
    STATUS status = STATUS_OK;
    const U32 len_before = quick_ublock_bytes(p_quick_ublock);
    const IL_CODE il_code = inline_code(uchars_inline);

    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(uchars_n));
    CODE_ANALYSIS_ONLY(assert(uchars_n >= inline_bytecount(uchars_inline)));

    switch(il_code)
    {
    case IL_DATE:
    case IL_FILE_DATE:
        {
        EV_DATA ev_data;
        NUMFORM_PARMS numform_parms;

        ev_data.did_num = RPN_DAT_DATE;

        if(IL_DATE == il_code)
            ss_local_time_as_ev_date(&ev_data.arg.ev_date);
        else
            ev_data.arg.ev_date = p_docu->file_date;

        zero_struct(numform_parms);
        numform_parms.ustr_numform_datetime = inline_data_ptr(PC_USTR, uchars_inline);
        numform_parms.p_numform_context = get_p_numform_context(p_docu);

        if(status_ok(status = numform(p_quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms)))
            quick_ublock_nullch_strip(p_quick_ublock);
        break;
        }

    case IL_PAGE_X:
    case IL_PAGE_Y:
        {
        EV_DATA ev_data;
        NUMFORM_PARMS numform_parms;

        if(IS_P_DATA_NONE(p_page_num))
            ev_data_set_integer(&ev_data, 1);
        else if(IL_PAGE_Y == il_code)
            ev_data_set_integer(&ev_data, page_number_from_page_y(p_docu, p_page_num->y) + 1);
        else /* IL_PAGE_X */
            ev_data_set_integer(&ev_data, p_page_num->x + 1);

        zero_struct(numform_parms);
        numform_parms.ustr_numform_numeric = inline_data_ptr(PC_USTR, uchars_inline);
        numform_parms.p_numform_context = get_p_numform_context(p_docu);

        if(status_ok(status = numform(p_quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms)))
            quick_ublock_nullch_strip(p_quick_ublock);
        break;
        }

    case IL_WHOLENAME:
        {
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
        quick_tblock_with_buffer_setup(quick_tblock);

        if(status_ok(status = name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE)))
            status = quick_ublock_tstr_add(p_quick_ublock, quick_tblock_tstr(&quick_tblock));

        quick_tblock_dispose(&quick_tblock);
        break;
        }

    case IL_LEAFNAME:
        status = quick_ublock_tstr_add(p_quick_ublock, p_docu->docu_name.leaf_name);
        break;

    case IL_SOFT_HYPHEN:
        status = quick_ublock_a7char_add(p_quick_ublock, CH_HYPHEN_MINUS);
        break;

    case IL_MS_FIELD:
        {
        READ_MAIL_TEXT read_mail_text;

        read_mail_text.field_no = data_from_inline_s32(uchars_inline);
        read_mail_text.p_quick_ublock = p_quick_ublock;
        read_mail_text.responded = 0;

        status = object_call_id_load(p_docu, T5_MSG_READ_MAIL_TEXT, &read_mail_text, OBJECT_ID_MAILSHOT);

        if(!read_mail_text.responded && status_ok(status))
            status = quick_ublock_printf(p_quick_ublock, resource_lookup_ustr(MSG_FIELD), read_mail_text.field_no + 1);

        break;
        }

    case IL_SS_NAME:
        {
        SS_NAME_READ ss_name_read;

        ss_name_read.ev_handle = (EV_HANDLE) data_from_inline_s32(uchars_inline);
        ss_name_read.follow_indirection = 1;
        ev_data_set_blank(&ss_name_read.ev_data);

        if(object_present(OBJECT_ID_SS)) /* SKS 27sep94 allow for no SS module but load file with duff fields */
            status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_READ, &ss_name_read);
        else
            status = STATUS_MODULE_NOT_FOUND;

        if(status_fail(status))
        {
            ev_data_set_error(&ss_name_read.ev_data, status);
            status = STATUS_OK;
        }

        {
        NUMFORM_PARMS numform_parms;

        if(!IS_P_STYLE_NONE(p_style_text_global))
        {
            /*zero_struct(numform_parms);*/
            numform_parms.ustr_numform_numeric = array_ustr(&p_style_text_global->para_style.h_numform_nu);
            numform_parms.ustr_numform_datetime = array_ustr(&p_style_text_global->para_style.h_numform_dt);
            numform_parms.ustr_numform_texterror = array_ustr(&p_style_text_global->para_style.h_numform_se);
        }
        else
        {
            zero_struct(numform_parms);
        }

        numform_parms.p_numform_context = get_p_numform_context(p_docu);

        if(status_ok(status = numform(p_quick_ublock, P_QUICK_TBLOCK_NONE, &ss_name_read.ev_data, &numform_parms)))
            quick_ublock_nullch_strip(p_quick_ublock);
        } /*block*/

        ss_data_free_resources(&ss_name_read.ev_data);

        break;
        }

    case IL_UTF8:
        {
        const PC_UTF8 utf8 = inline_data_ptr(PC_UTF8, uchars_inline);
        const U32 il_data_size = (U32) inline_data_size(uchars_inline);
        U32 offset = 0;

        while(offset < il_data_size)
        {
            U32 bytes_of_char;
            const UCS4 ucs4 = utf8_char_decode_off(utf8, offset, bytes_of_char);

            status_break(status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("[U+%.4X]"), ucs4));

            offset += bytes_of_char;
        }

        break;
        }

    default: default_unhandled(); break;
    }

    {
    U32 len_add = quick_ublock_bytes(p_quick_ublock) - len_before;
    return(status_ok(status) ? (S32) len_add : status);
    } /*block*/
}

/******************************************************************************
*
* given some formatted text and a skel_point, find the
* segment, offset and string position of the skel_point
*
******************************************************************************/

/*ncr*/
extern BOOL
text_location_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_LOCATION p_text_location,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_FORMATTED_TEXT p_formatted_text,
    P_SKEL_POINT p_skel_point,
    _InRef_     PC_PAGE_NUM p_page_offset)
{
    const ARRAY_INDEX n_segments = array_elements(&p_formatted_text->h_segments);
    ARRAY_INDEX seg_ix;
    PC_SEGMENT p_segment;

    for(seg_ix = 0, p_segment = array_rangec(&p_formatted_text->h_segments, SEGMENT, seg_ix, n_segments);
        seg_ix < n_segments;
        ++seg_ix, ++p_segment)
    {
        SKEL_EDGE seg_bot;
        BOOL last_segment;

        edge_set_from_skel_point(&seg_bot, &p_segment->skel_point, y);
        seg_bot.page += p_page_offset->y;
        seg_bot.pixit += p_segment->leading;

        last_segment = ((seg_ix + 1) >= array_elements(&p_formatted_text->h_segments)) ? 1 : 0;

        /* use segment array to find vertical position */
        if(last_segment || skel_point_edge_compare(p_skel_point, &seg_bot, y) < 0)
        {
            SKEL_EDGE chunk_left;
            ARRAY_INDEX chunk_ix;
            PC_CHUNK p_chunk;

            edge_set_from_skel_point(&chunk_left, &p_segment->skel_point, x);
            chunk_left.page += p_page_offset->x;

            /* use chunk array to find horizontal position */
            for(chunk_ix = p_segment->start_chunk, p_chunk = array_rangec(&p_formatted_text->h_chunks, CHUNK, chunk_ix, p_segment->end_chunk - chunk_ix);
                chunk_ix < p_segment->end_chunk;
                ++chunk_ix, ++p_chunk)
            {
                SKEL_EDGE chunk_right;

                chunk_left.pixit += p_chunk->lead_space;
                chunk_right = chunk_left;
                chunk_right.pixit += p_chunk->width;

                if(chunk_ix + 1 >= p_segment->end_chunk ||
                   skel_point_edge_compare(p_skel_point, &chunk_right, x) < 0)
                {
                    S32 chunk_offset = 0;
                    PIXIT pos;

                    /* is position in space area ? */
                    if((pos = p_skel_point->pixit_point.x - chunk_left.pixit) <= 0)
                        pos = 0;
                    else
                    {
                        if(!last_segment && chunk_ix + 1 == p_segment->end_chunk)
                            pos = MIN(pos, p_chunk->width - p_chunk->fonty_chunk.trail_space);

                        switch(p_chunk->type)
                        {
                        case CHUNK_FREE:
                            chunk_offset = fonty_chunk_index_from_pixit(p_docu,
                                                                        &p_chunk->fonty_chunk,
                                                                        uchars_AddBytes(ustr_inline, p_chunk->input_ix),
                                                                        p_chunk->input_len,
                                                                        &pos);
                            break;

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
                        case CHUNK_RETURN:
                        case CHUNK_SOFT_HYPHEN:
                        case CHUNK_TAB:
                        case CHUNK_UTF8:
                        case CHUNK_UTF8_AS_TEXT:
#endif
                            {
                            PIXIT inline_width = p_chunk->width - p_chunk->fonty_chunk.trail_space;

                            if(inline_width && pos >= inline_width / 2)
                            {
                                if(p_chunk->trail_spaces)
                                {
                                    S32 space_start = p_chunk->input_len - p_chunk->trail_spaces;

                                    /* look for offset in trailing space part */
                                    pos -= inline_width;
                                    chunk_offset = fonty_chunk_index_from_pixit(p_docu,
                                                                                &p_chunk->fonty_chunk,
                                                                                uchars_AddBytes(ustr_inline, p_chunk->input_ix + space_start),
                                                                                p_chunk->trail_spaces,
                                                                                &pos)
                                                   + space_start;
                                    pos += inline_width;
                                }
                                else
                                    chunk_offset = p_chunk->input_len;
                            }
                            else
                            {
                                pos = 0;
                                chunk_offset = 0;
                            }

                            break;
                            }
                        }
                    }

                    if(!last_segment && chunk_offset >= p_chunk->input_len)
                    { /* 2.11.93 */
                        PC_UCHARS_INLINE uchars_inline_last = uchars_inline_AddBytes(ustr_inline, p_chunk->input_ix + chunk_offset);
                        S32 charb = inline_b_bytecount(uchars_inline_last);
                        assert(charb == (S32) inline_b_bytecount_off(ustr_inline, p_chunk->input_ix + chunk_offset));
                        chunk_offset = MAX(0, chunk_offset - charb);
                    }

                    p_text_location->seg_ix = seg_ix;

                    p_text_location->skel_point = p_segment->skel_point;
                    skel_point_update_from_edge(&p_text_location->skel_point, &chunk_left, x);
                    p_text_location->skel_point.pixit_point.x += pos;
                    p_text_location->skel_point.page_num.y += p_page_offset->y;

                    p_text_location->caret_height = p_segment->leading;
                    p_text_location->string_ix = p_chunk->input_ix + chunk_offset;

                    trace_2(TRACE_APP_SKEL_DRAW,
                            TEXT("text_location_from_skel_point pixit x: ") S32_TFMT TEXT(", ix: ") S32_TFMT,
                            p_text_location->skel_point.pixit_point.x,
                            p_text_location->string_ix);

                    return(TRUE);
                }

                /* advance to next chunk */
                chunk_left.pixit = chunk_right.pixit;
            }
        }
    }

    return(FALSE);
}

typedef struct PENDING_TAB
{
    TAB_INFO tab_info;
    ARRAY_INDEX chunk;
    PIXIT position;
    S32 chunk_type;
}
PENDING_TAB, * P_PENDING_TAB;

/******************************************************************************
*
* process a pending TAB
*
******************************************************************************/

_Check_return_
static PIXIT
pending_tab_proc(
    _DocuRef_   P_DOCU p_docu,
    P_FORMATTED_TEXT p_formatted_text,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_PENDING_TAB p_pending_tab,
    _InVal_     PIXIT position_now,
    _InVal_     ARRAY_INDEX chunk_ix,
    P_INLINE_OBJECT p_inline_object)
{
    PIXIT extra_lead_space = 0;

    if(p_pending_tab->chunk_type != CHUNK_FREE)
    {
        PIXIT tab_text_width, tab_width;

        tab_width = p_text_format_info->style_text_global.para_style.margin_left
                    + p_pending_tab->tab_info.offset
                    - p_pending_tab->position;
        tab_text_width = position_now - p_pending_tab->position;

        switch(p_pending_tab->tab_info.type)
        {
        case TAB_CENTRE:
            {
            PIXIT tab_text_offset_centre;

            tab_text_offset_centre = tab_text_width / 2;
            if(tab_text_offset_centre < tab_width)
                extra_lead_space = tab_width - tab_text_offset_centre;
            break;
            }

        case TAB_RIGHT:
            if(tab_text_width < tab_width)
                extra_lead_space = tab_width - tab_text_width;
            break;

        case TAB_DECIMAL:
            {
            extra_lead_space = tab_width - tab_text_width;

            if(p_pending_tab->chunk + 1 < array_elements(&p_formatted_text->h_chunks))
            {
                PC_CHUNK p_chunk = array_ptrc(&p_formatted_text->h_chunks, CHUNK, p_pending_tab->chunk + 1);
                PC_UCHARS p_text_chunk = NULL;
                S32 dot = -1;

                if(p_chunk->type == CHUNK_FREE && p_chunk->input_len)
                {
                    S32 i;

                    p_text_chunk = PtrAddBytes(PC_UCHARS, p_inline_object->object_data.u.p_object, p_chunk->input_ix);

                    for(i = p_chunk->input_len - 1; i >= 0 && dot < 0; i -= 1)
                    {
                        switch(PtrGetByteOff(p_text_chunk, i))
                        {
                        case CH_COLON:
                        case CH_SEMICOLON:
                        case CH_COMMA:
                        case CH_FULL_STOP:
                        case UCH_MIDDLE_DOT: /* Latin-1 decimal point - SKS 16may96 added */
                            dot = i;
                            break;

                        default:
                            break;
                        }
                    }
                }

                if(dot >= 0)
                {
                    FONTY_CHUNK fonty_chunk;
                    fonty_chunk_info_read_uchars(p_docu, &fonty_chunk, p_chunk->fonty_chunk.fonty_handle, p_text_chunk, dot, 0);
                    extra_lead_space = tab_width - fonty_chunk.width;
                }
            }

            extra_lead_space = MAX(0, extra_lead_space);
            break;
            }

        default: default_unhandled(); break;
        }

        /* now set width of original TAB chunk after calculation */
        array_ptr(&p_formatted_text->h_chunks, CHUNK, p_pending_tab->chunk)->width += extra_lead_space;
        p_pending_tab->chunk_type = CHUNK_FREE;
    }

    return(chunk_ix > p_pending_tab->chunk ? extra_lead_space : 0);
}

/******************************************************************************
*
* segment an inline string in the width available
* taking account of style changes etc
*
******************************************************************************/

_Check_return_
extern STATUS
text_segment(
    _DocuRef_   P_DOCU p_docu,
    P_FORMATTED_TEXT p_formatted_text,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object)
{
    STATUS status = STATUS_OK;

    /* get a load of chunks */
    if(status_ok(status = text_chunkify(p_docu, &p_formatted_text->h_chunks, p_text_format_info, p_inline_object)))
    {
        ARRAY_INDEX chunk_ix = 0, seg_ix = 0;
        SKEL_POINT skel_point = p_text_format_info->skel_rect_object.tl;
        const ARRAY_INDEX n_chunks = array_elements(&p_formatted_text->h_chunks);
        BOOL first_segment_on_page = (skel_point.pixit_point.y == 0) ? 1 : 0;
        SC_ARRAY_INIT_BLOCK seg_init_block = aib_init(3, sizeof32(SEGMENT), TRUE /* need this */);
        P_SEGMENT p_segment = NULL;
        PIXIT format_width;

        /* make into relative page offsets */
        skel_point.page_num.x = 0;
        skel_point.page_num.y = 0;
        skel_point.pixit_point.y += p_text_format_info->style_text_global.para_style.para_start;
        p_formatted_text->skel_rect.tl = skel_point;

        format_width = p_text_format_info->skel_rect_object.br.pixit_point.x
                     - p_text_format_info->skel_rect_object.tl.pixit_point.x
                     - p_text_format_info->style_text_global.para_style.margin_right;

        while(chunk_ix < n_chunks)
        {
            P_CHUNK p_chunk, p_start_chunk;
            PIXIT last_chunk_trail_space, hyphen_space = 0;
            S32 gap_count;
            PENDING_TAB pending_tab;
            ARRAY_INDEX chunk_space_ix;
            U8 format_break = 0;

            /****** allocate us a segment ******/
            if(NULL == (p_segment = al_array_extend_by(&p_formatted_text->h_segments, SEGMENT, 1, &seg_init_block, &status)))
                break;

            /* accumulate as many chunks as possible
             * into the segment (before the format width is hit)
             */
            p_segment->start_chunk = chunk_ix;
            p_start_chunk = array_ptr(&p_formatted_text->h_chunks, CHUNK, chunk_ix);
            chunk_space_ix = chunk_ix;
            last_chunk_trail_space = 0;
            gap_count = 0;
            zero_struct(pending_tab); /* Keep dataflower happy */
            pending_tab.chunk_type = CHUNK_FREE;

            for(p_chunk = p_start_chunk; chunk_ix < n_chunks; ++chunk_ix, ++p_chunk)
            {
                PIXIT tab_space = 0;

                /* set lead space */
                if(chunk_ix == p_segment->start_chunk)
                {
                    if(chunk_ix == 0 &&
                       (p_text_format_info->style_text_global.para_style.justify == SF_JUSTIFY_LEFT ||
                        p_text_format_info->style_text_global.para_style.justify == SF_JUSTIFY_BOTH))
                        p_chunk->lead_space = p_text_format_info->style_text_global.para_style.margin_left +
                                              p_text_format_info->style_text_global.para_style.margin_para;
                    else
                        p_chunk->lead_space = p_text_format_info->style_text_global.para_style.margin_left;
                }
                else
                    p_chunk->lead_space = 0;

                /* start off chunk width accumulator */
                p_chunk->width = p_chunk->type == CHUNK_SOFT_HYPHEN ? 0 : p_chunk->fonty_chunk.width;

                /* work out size of space from TAB type... */
                if(p_text_format_info->style_text_global.para_style.justify == SF_JUSTIFY_LEFT ||
                   p_text_format_info->style_text_global.para_style.justify == SF_JUSTIFY_BOTH)
                {
                    if(p_chunk->type == CHUNK_TAB)
                    {
                        P_TAB_INFO p_tab_info;

                        /* process any pending tab */
                        p_segment->width += pending_tab_proc(p_docu,
                                                             p_formatted_text,
                                                             p_text_format_info,
                                                             &pending_tab,
                                                             p_segment->width,
                                                             chunk_ix,
                                                             p_inline_object
                                                             );

                        p_tab_info =
                            tab_next_from_pixit_position(&p_text_format_info->style_text_global.para_style.h_tab_list,
                                                         p_segment->width,
                                                         p_text_format_info->style_text_global.para_style.margin_left);

                        if(NULL != p_tab_info)
                        {
                            switch(p_tab_info->type)
                            {
                            case TAB_LEFT:
                                tab_space = p_text_format_info->style_text_global.para_style.margin_left +
                                            p_tab_info->offset - p_segment->width - p_chunk->lead_space;
                                break;

                            case TAB_RIGHT:
                            case TAB_CENTRE:
                            case TAB_DECIMAL:
                                pending_tab.tab_info = *p_tab_info;
                                pending_tab.chunk = chunk_ix;
                                pending_tab.position = p_segment->width
                                                       + (chunk_ix == p_segment->start_chunk
                                                             ? p_text_format_info->style_text_global.para_style.margin_left
                                                             : 0);
                                pending_tab.chunk_type = p_chunk->type;
                                break;

                            default: default_unhandled(); break;
                            }
                        }

                        gap_count = 0;
                        chunk_space_ix = chunk_ix + 1;
                    }
                    /* pending decimal TABS get processed after one more chunk */
                    else if(pending_tab.chunk_type == CHUNK_TAB
                            &&
                            pending_tab.tab_info.type == TAB_DECIMAL
                            &&
                            chunk_ix > pending_tab.chunk + 1)
                        p_segment->width += pending_tab_proc(p_docu,
                                                             p_formatted_text,
                                                             p_text_format_info,
                                                             &pending_tab,
                                                             p_segment->width,
                                                             chunk_ix,
                                                             p_inline_object);
                }

                /* tab can't pend if this is the last chunk */
                if(pending_tab.chunk_type == CHUNK_TAB
                   &&
                   (chunk_ix + 1 == n_chunks
                    ||
                    p_chunk->type == CHUNK_RETURN))
                {
                    p_segment->width += pending_tab_proc(p_docu,
                                                         p_formatted_text,
                                                         p_text_format_info,
                                                         &pending_tab,
                                                         p_segment->width + p_chunk->width - p_chunk->fonty_chunk.trail_space,
                                                         chunk_ix,
                                                         p_inline_object);
                    p_chunk->lead_space = 0;
                }

                /* have we got a wrap ? */
                if(chunk_ix != p_segment->start_chunk
                   &&
                   p_chunk->type != CHUNK_RETURN)
               {
                    PIXIT hyphen_trail;

                    if(chunk_ix + 1 < n_chunks && p_chunk[1].type == CHUNK_SOFT_HYPHEN)
                        hyphen_trail = p_chunk[1].fonty_chunk.width;
                    else
                        hyphen_trail = 0;

                    if(p_chunk->lead_space
                       + tab_space
                       + p_segment->width
                       + p_chunk->width
                       + hyphen_trail
                       - p_chunk->fonty_chunk.trail_space > format_width)
                        break;
               }

                hyphen_space = p_chunk->type == CHUNK_SOFT_HYPHEN ? p_chunk->fonty_chunk.width : 0;

                p_chunk->width += tab_space;
                p_segment->width += p_chunk->width + p_chunk->lead_space;
                p_segment->base_line = MAX(p_segment->base_line, p_chunk->base_line);
                p_segment->leading = MAX(p_segment->leading, p_chunk->leading);
                p_segment->format_width = format_width;

                /* forced return ? */
                if(p_chunk->type == CHUNK_RETURN)
                {
                    chunk_ix += 1;
                    format_break = 1;
                    break;
                }

                /* set trail width of last chunk */
                last_chunk_trail_space = p_chunk->fonty_chunk.trail_space;

                /* count gaps */
                if(last_chunk_trail_space)
                    gap_count += 1;

            } /* end of for */

            /************************************ chunks now accumulated ********************************/

            /* flip hyphen into a real one */
            if(hyphen_space)
            {
                array_ptr(&p_formatted_text->h_chunks, CHUNK, chunk_ix - 1)->type = CHUNK_HYPHEN;
                p_segment->width += hyphen_space;
            }

            p_segment->end_chunk = chunk_ix;

            assert(p_segment->end_chunk > p_segment->start_chunk);

            /* work out lead space for segment for
             * justification style
             */
            switch(p_text_format_info->style_text_global.para_style.justify)
            {
            case SF_JUSTIFY_LEFT:
                /* nothing to do */
                break;

            case SF_JUSTIFY_CENTRE:
                {
                PIXIT extra_space;

                extra_space = (format_width - (p_segment->width - last_chunk_trail_space)) / 2;
                p_start_chunk->lead_space += extra_space;
                p_segment->width += extra_space;
                break;
                }

            case SF_JUSTIFY_RIGHT:
                {
                PIXIT extra_space;

                extra_space = format_width - (p_segment->width - last_chunk_trail_space);
                p_start_chunk->lead_space += extra_space;
                p_segment->width += extra_space;
                break;
                }

            case SF_JUSTIFY_BOTH:
                {
                /* the last bit of space does not count as a gap */
                if(last_chunk_trail_space && gap_count)
                    gap_count -= 1;

                /* we must have at least one gap, and not be on the last segment */
                if(gap_count && !format_break && chunk_ix < n_chunks)
                {
                    PIXIT total_extra_space, extra_space_per_gap, odd_extra_space;
                    S32 gap_ix;
                    P_CHUNK p_chunk;

                    total_extra_space = format_width - (p_segment->width - last_chunk_trail_space);
                    extra_space_per_gap = total_extra_space / gap_count;
                    odd_extra_space = total_extra_space - (extra_space_per_gap * gap_count);

                    /* distribute space amongst gaps */
                    for(gap_ix = gap_count, p_chunk = array_ptr(&p_formatted_text->h_chunks, CHUNK, chunk_space_ix);
                        gap_ix;
                        p_chunk += 1)
                    {
                        if(p_chunk->fonty_chunk.trail_space)
                        {
                            p_chunk->width += extra_space_per_gap + (gap_ix ? 0 : odd_extra_space);
                            gap_ix -= 1;
                        }
                    }

                    p_segment->width += total_extra_space;
                }

                break;
                }
            }

            /**************************************** justification over ************************************/

            /* now position the segment in document space
             * - does the segment fit on this page ?
             * but don't move the first segment onto another page (== loop forever)
             */

            if(!first_segment_on_page
               &&
               p_text_format_info->paginate
               &&
               skel_point.pixit_point.y
               + p_segment->leading
               + (chunk_ix >= n_chunks ? p_text_format_info->style_text_global.para_style.para_end : 0)
               > p_text_format_info->skel_rect_work.br.pixit_point.y - p_text_format_info->text_area_border_y)
            {
                /* avoid grid space at top of page 24.2.93 */
                skel_point.pixit_point.y = seg_ix ? p_text_format_info->text_area_border_y : p_text_format_info->style_text_global.para_style.para_start;
                skel_point.page_num.y += 1;
                first_segment_on_page = 1;

                /* 20.1.95 make sure we report back the new rectangle */
                if(!seg_ix)
                    p_formatted_text->skel_rect.tl = skel_point;
            }
            else
                first_segment_on_page = 0;

            p_segment->skel_point = skel_point;
            skel_point.pixit_point.y += p_segment->leading;
            seg_ix += 1;

        } /* end of while loop for chunks */

        /* save formatted text details */
        p_formatted_text->para_start = p_text_format_info->style_text_global.para_style.para_start;
        p_formatted_text->para_end = p_text_format_info->style_text_global.para_style.para_end;
        p_formatted_text->justify_v_offset_y = 0;

        if(NULL != p_segment)
        {
            p_formatted_text->skel_rect.br = p_segment->skel_point;
            p_formatted_text->skel_rect.br.pixit_point.y += p_segment->leading;

            if(seg_ix > 1)
                p_formatted_text->skel_rect.br.pixit_point.x += format_width;
            else
                p_formatted_text->skel_rect.br.pixit_point.x += p_segment->width;
        }
        else
        {
            p_formatted_text->skel_rect.br = p_formatted_text->skel_rect.tl;
            p_formatted_text->skel_rect.br.pixit_point.y += style_leading_from_style(&p_text_format_info->style_text_global,
                                                                                     &p_text_format_info->style_text_global.font_spec,
                                                                                     p_docu->flags.draft_mode);
        }
    }

#if TRACE_ALLOWED
    if(p_inline_object->object_data.data_ref.data_space == DATA_SLOT)
        trace_3(TRACE_APP_SKEL,
                TEXT("text_segment made ") S32_TFMT TEXT(" segments, col: ") COL_TFMT TEXT(", row: ") ROW_TFMT,
                array_elements(&p_formatted_text->h_segments),
                p_inline_object->object_data.data_ref.arg.slr.col,
                p_inline_object->object_data.data_ref.arg.slr.row);
#endif

    al_array_auto_compact_set(&p_formatted_text->h_segments);

    { /* garbage collect unused segments */
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.shrink = 1;
    consume(S32, al_array_garbage_collect(&p_formatted_text->h_segments, 0, NULL, al_garbage_flags));
    } /*block*/

    return(status);
}

/******************************************************************************
*
* given a physical location in formatted
* text, find the skel point for that location
*
* --in--
* end flag tells whether to return the end of a
* segment (TRUE) or the start of the next (FALSE)
* where the index lies on a segment boundary
*
******************************************************************************/

/*ncr*/
extern BOOL
text_skel_point_from_location(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_LOCATION p_text_location,
    P_USTR_INLINE ustr_inline,
    P_FORMATTED_TEXT p_formatted_text,
    _InVal_     S32 end,
    _InRef_     PC_PAGE_NUM p_page_offset)
{
    const ARRAY_INDEX n_segments = array_elements(&p_formatted_text->h_segments);
    ARRAY_INDEX seg_ix;
    PC_SEGMENT p_segment;

    for(seg_ix = 0, p_segment = array_rangec(&p_formatted_text->h_segments, SEGMENT, 0, n_segments);
        seg_ix < n_segments;
        ++seg_ix, ++p_segment)
    {
        PC_CHUNK p_chunk;
        ARRAY_INDEX chunk_ix;

        p_text_location->skel_point = p_segment->skel_point;
        p_text_location->skel_point.page_num.x += p_page_offset->x;
        p_text_location->skel_point.page_num.y += p_page_offset->y;

        for(chunk_ix = p_segment->start_chunk, p_chunk = array_rangec(&p_formatted_text->h_chunks, CHUNK, chunk_ix, p_segment->end_chunk - chunk_ix);
            chunk_ix < p_segment->end_chunk;
            ++chunk_ix, ++p_chunk)
        {
            S32 chunk_end = p_chunk->input_ix + p_chunk->input_len;

            if((!end && p_text_location->string_ix <  chunk_end) ||
               ( end && p_text_location->string_ix <= chunk_end) ||
               ((chunk_ix + 1 >= p_segment->end_chunk) && (seg_ix + 1 >= n_segments)) )
            {
                S32 string_len;

                string_len = p_text_location->string_ix - p_chunk->input_ix;

                if(string_len > 0)
                {
                    PIXIT chunk_part_width = p_chunk->width;

                    /* is the point in the midst of the chunk ? */
                    if(string_len < p_chunk->input_len)
                    {
                        switch(p_chunk->type)
                        {
                        case CHUNK_FREE:
                            {
                            FONTY_CHUNK fonty_chunk;

                            fonty_chunk_info_read_uchars(
                                p_docu,
                                &fonty_chunk,
                                p_chunk->fonty_chunk.fonty_handle,
                                uchars_AddBytes(ustr_inline, p_chunk->input_ix),
                                string_len,
                                0);

                            chunk_part_width = fonty_chunk.width;

                            break;
                            }

                        /* these chunks might have trailing spaces subsumed into them */
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
                        case CHUNK_RETURN:
                        case CHUNK_SOFT_HYPHEN:
                        case CHUNK_TAB:
                        case CHUNK_UTF8:
                        case CHUNK_UTF8_AS_TEXT:
#endif
                            {
                            if(p_chunk->trail_spaces)
                            {
                                S32 n_chars_less = MIN(p_chunk->input_len - string_len, p_chunk->trail_spaces);
                                chunk_part_width = p_chunk->width -
                                                   p_chunk->fonty_chunk.trail_space * n_chars_less / p_chunk->trail_spaces;
                            }
                            else
                                chunk_part_width = 0;

                            break;
                            }
                        }
                    }

                    p_text_location->skel_point.pixit_point.x += chunk_part_width;
                }

                p_text_location->skel_point.pixit_point.x += p_chunk->lead_space;
                p_text_location->seg_ix = seg_ix;
                p_text_location->caret_height = p_segment->leading;

                trace_2(TRACE_APP_SKEL_DRAW,
                        TEXT("text_skel_point_from_location pixit x: ") S32_TFMT TEXT(", ix: ") S32_TFMT,
                        p_text_location->skel_point.pixit_point.x,
                        p_text_location->string_ix);

                return(TRUE);
            }

            p_text_location->skel_point.pixit_point.x += p_chunk->lead_space + p_chunk->width;
        }
    }

    return(FALSE);
}

/* end of tx_form.c */
