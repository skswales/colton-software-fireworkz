/* ob_story.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Story object module */

/* MRJC July 1994 */

#include "common/gflags.h"

#include "ob_story/ob_story.h"

#if RISCOS
#define MSG_WEAK &rb_story_msg_weak
extern PC_U8 rb_story_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_STORY NULL

/******************************************************************************
*
* redisplay text object efficiently after mods
*
******************************************************************************/

static void
text_redisplay_from_info(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_INLINE_REDISPLAY p_text_inline_redisplay,
    P_TEXT_REDISPLAY_INFO p_text_redisplay_info,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object)
{
    P_TEXT_CACHE p_text_cache;

    if(!p_text_redisplay_info->object_visible)
        return;

    p_text_cache = p_text_cache_from_data_ref(p_docu, p_text_format_info, p_inline_object);

    if(NULL != p_text_cache)
    {
        SKEL_RECT new_rect_para, new_rect_text, old_rect_para, old_rect_text;

        formatted_text_justify_vertical(&p_text_cache->formatted_text,
                                        p_text_format_info,
                                        &p_text_redisplay_info->skel_rect_object.tl.page_num);

        formatted_text_box(&new_rect_para,
                           &new_rect_text,
                           &p_text_cache->formatted_text,
                           &p_text_redisplay_info->skel_rect_object.tl.page_num);

        formatted_text_box(&old_rect_para,
                           &old_rect_text,
                           &p_text_redisplay_info->formatted_text,
                           &p_text_redisplay_info->skel_rect_object.tl.page_num);

        /* if object was at top of object rectangle and has now moved down, reformat */
        if( skel_point_compare(&p_text_redisplay_info->skel_rect_object.tl, &old_rect_text.tl, y)
           ||
           !skel_point_compare(&new_rect_text.tl, &old_rect_text.tl, y) )
        {
            if(p_text_inline_redisplay->do_redraw)
            {
                text_redisplay_fast(p_docu,
                                    p_inline_object->object_data.u.ustr_inline,
                                    p_text_redisplay_info,
                                    p_text_cache,
                                    &old_rect_text,
                                    &new_rect_text,
                                    p_text_inline_redisplay->redraw_tag,
                                    &p_text_format_info->style_text_global);
                p_text_inline_redisplay->redraw_done = TRUE;
            }

            p_text_inline_redisplay->size_changed = FALSE;
        }

        p_text_inline_redisplay->skel_rect_para_after = new_rect_para;
        p_text_inline_redisplay->skel_rect_para_before = old_rect_para;
        p_text_inline_redisplay->skel_rect_text_before = old_rect_text;
    }
}

/******************************************************************************
*
* story object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, story_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_STORY, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_STORY));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_STORY));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, story_cmd_setc, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    STATUS status = STATUS_OK;
    const P_TEXT_INLINE_REDISPLAY p_text_inline_redisplay = (P_TEXT_INLINE_REDISPLAY) p_text_message_block->p_data;
    TEXT_REDISPLAY_INFO text_redisplay_info;
    S32 start, end;

    offsets_from_object_data(&start, &end, &p_text_message_block->inline_object.object_data, p_text_message_block->inline_object.inline_len);

    text_redisplay_info_save(p_docu,
                             &text_redisplay_info,
                             &p_text_message_block->text_format_info,
                             &p_text_message_block->inline_object,
                             start);

    {
    const P_USTR_INLINE ustr_inline = p_text_message_block->inline_object.object_data.u.ustr_inline;
    U32 offset = (U32) start;
    U32 offset_in_word = 0;

    while((S32) offset < end)
    {
        U32 bytes_of_char;
        UCS4 ucs4;

        if(is_inline_off(ustr_inline, offset))
        {
            offset_in_word = 0;
            offset += inline_bytecount_off(ustr_inline, offset);
            continue;
        }

        ucs4 = ustr_char_decode_off((PC_USTR) ustr_inline, offset, /*ref*/bytes_of_char);

        if(!t5_ucs4_is_alphabetic(ucs4) && !t5_ucs4_is_decimal_digit(ucs4))
        {
            offset_in_word = 0;
        }
        else
        {
            UCS4 ucs4_x;

            switch(t5_message)
            {
            default: default_unhandled();
#if CHECKING
            case T5_CMD_SETC_UPPER:
#endif
                ucs4_x = t5_ucs4_uppercase(ucs4);
                break;

            case T5_CMD_SETC_LOWER:
                ucs4_x = t5_ucs4_lowercase(ucs4);
                break;

            case T5_CMD_SETC_INICAP:
                if(offset_in_word == 0)
                    ucs4_x = t5_ucs4_uppercase(ucs4);
                else
                    ucs4_x = t5_ucs4_lowercase(ucs4);
                break;

            case T5_CMD_SETC_SWAP:
                if(t5_ucs4_is_uppercase(ucs4))
                    ucs4_x = t5_ucs4_lowercase(ucs4);
                else
                    ucs4_x = t5_ucs4_uppercase(ucs4);
                break;
            }

            if(ucs4_x != ucs4)
            {
                const U32 new_bytes_of_char = uchars_bytes_of_char_encoding(ucs4_x);
                assert(new_bytes_of_char == bytes_of_char);
                if(new_bytes_of_char == bytes_of_char)
                    (void) uchars_char_encode_off((P_UCHARS) ustr_inline, (U32) end, offset, ucs4_x);
            }

            offset_in_word++;
        }

        offset += bytes_of_char;
    }
    } /*block*/

    text_redisplay_from_info(p_docu,
                             p_text_inline_redisplay,
                             &text_redisplay_info,
                             &p_text_message_block->text_format_info,
                             &p_text_message_block->inline_object);

    text_redisplay_info_dispose(&text_redisplay_info);

    return(status);
}

T5_MSG_PROTO(static, story_msg_object_how_big, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_OBJECT_HOW_BIG p_object_how_big = (P_OBJECT_HOW_BIG) p_text_message_block->p_data;
    const P_OBJECT_DATA p_object_data = &p_text_message_block->inline_object.object_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_object_data->u.p_object)
    {
        const P_TEXT_CACHE p_text_cache = p_text_cache_from_data_ref(p_docu, &p_text_message_block->text_format_info, &p_text_message_block->inline_object);
        PTR_ASSERT(p_text_cache);
        {
        SKEL_RECT skel_rect_text;
        formatted_text_box(&p_object_how_big->skel_rect,
                           &skel_rect_text,
                           &p_text_cache->formatted_text,
                           &p_object_how_big->skel_rect.tl.page_num);
        } /*block*/
    }
    else
    {
        /* object is blank - work out a sensible size */
        p_object_how_big->skel_rect.br.pixit_point.y = p_object_how_big->skel_rect.tl.pixit_point.y;
        p_object_how_big->skel_rect.br.pixit_point.y += style_leading_from_style(&p_text_message_block->text_format_info.style_text_global,
                                                                                 &p_text_message_block->text_format_info.style_text_global.font_spec,
                                                                                 p_docu->flags.draft_mode)
                                                       + p_text_message_block->text_format_info.style_text_global.para_style.para_start
                                                       + p_text_message_block->text_format_info.style_text_global.para_style.para_end;
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, story_msg_object_how_wide, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_OBJECT_HOW_WIDE p_object_how_wide = (P_OBJECT_HOW_WIDE) p_text_message_block->p_data;
    const P_OBJECT_DATA p_object_data = &p_text_message_block->inline_object.object_data;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_object_data->u.p_object)
    {
        const P_TEXT_CACHE p_text_cache = p_text_cache_from_data_ref(p_docu, &p_text_message_block->text_format_info, &p_text_message_block->inline_object);
        PTR_ASSERT(p_text_cache);
        p_object_how_wide->width = formatted_text_width_max(&p_text_cache->formatted_text, &p_text_message_block->text_format_info);
    }
    else
        p_object_how_wide->width = 0;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, story_msg_insert_inline_redisplay, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_TEXT_INLINE_REDISPLAY p_text_inline_redisplay = (P_TEXT_INLINE_REDISPLAY) p_text_message_block->p_data;
    const P_OBJECT_DATA p_object_data = &p_text_message_block->inline_object.object_data;
    TEXT_REDISPLAY_INFO text_redisplay_info;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    text_redisplay_info_save(p_docu,
                             &text_redisplay_info,
                             &p_text_message_block->text_format_info,
                             &p_text_message_block->inline_object,
                             p_object_data->object_position_start.data);

    { /* actually insert the data */
    S32 ins_size;

    PTR_ASSERT(p_text_inline_redisplay->p_quick_ublock);
    ins_size = quick_ublock_bytes(p_text_inline_redisplay->p_quick_ublock);

    if(ins_size)
    {
        P_USTR_INLINE ustr_inline = p_object_data->u.ustr_inline;
        S32 start, end;

        offsets_from_object_data(&start, &end, p_object_data, p_text_message_block->inline_object.inline_len);

        if(p_text_message_block->inline_object.inline_len)
            memmove32(uchars_inline_AddBytes_wr(ustr_inline, start + ins_size),
                      uchars_inline_AddBytes(ustr_inline, start),
                      p_text_message_block->inline_object.inline_len - start);

        memcpy32(uchars_inline_AddBytes_wr(ustr_inline, start),
                 quick_ublock_uchars(p_text_inline_redisplay->p_quick_ublock),
                 ins_size);

        PtrPutByteOff(ustr_inline, p_text_message_block->inline_object.inline_len + ins_size, CH_NULL);
        p_text_message_block->inline_object.inline_len += ins_size;

        if(p_text_inline_redisplay->do_position_update)
        {
            OBJECT_POSITION_UPDATE object_position_update;
            object_position_update.data_ref = p_object_data->data_ref;
            object_position_update.object_position = p_object_data->object_position_start;
            object_position_update.data_update = ins_size;
            style_docu_area_position_update(p_docu, &p_text_message_block->text_format_info.h_style_list, &object_position_update);
        }

        /* update output position */
        p_object_data->object_position_end.data = start + ins_size;
        p_object_data->object_position_end.object_id = p_object_data->object_id;

        text_redisplay_from_info(p_docu,
                                 p_text_inline_redisplay,
                                 &text_redisplay_info,
                                 &p_text_message_block->text_format_info,
                                 &p_text_message_block->inline_object);
    }
    } /*block*/

    text_redisplay_info_dispose(&text_redisplay_info);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, story_msg_delete_inline_redisplay, /**/ P_TEXT_MESSAGE_BLOCK p_text_message_block)
{
    const P_TEXT_INLINE_REDISPLAY p_text_inline_redisplay = (P_TEXT_INLINE_REDISPLAY) p_text_message_block->p_data;
    TEXT_REDISPLAY_INFO text_redisplay_info;
    S32 start, end;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    offsets_from_object_data(&start, &end, &p_text_message_block->inline_object.object_data, p_text_message_block->inline_object.inline_len);

    /* save redisplay info */
    text_redisplay_info_save(p_docu,
                             &text_redisplay_info,
                             &p_text_message_block->text_format_info,
                             &p_text_message_block->inline_object,
                             p_text_message_block->inline_object.object_data.object_position_start.data);

    {
    const P_USTR_INLINE ustr_inline = p_text_message_block->inline_object.object_data.u.ustr_inline;
    P_UCHARS_INLINE p_text_e = uchars_inline_AddBytes_wr(ustr_inline, p_text_message_block->inline_object.inline_len);
    P_UCHARS_INLINE p_del_s = uchars_inline_AddBytes_wr(ustr_inline, start);
    P_UCHARS_INLINE p_del_e = uchars_inline_AddBytes_wr(ustr_inline, end);
    S32 del_size = end - start;

    if(del_size)
    {
        /* delete data */
        memmove32(p_del_s, p_del_e, PtrDiffBytesU32(p_text_e, p_del_e) + 1);

        /* adjust object size */
        p_text_message_block->inline_object.inline_len -= del_size;

        {
        OBJECT_POSITION_UPDATE object_position_update;
        object_position_update.data_ref = p_text_message_block->inline_object.object_data.data_ref;
        object_position_update.object_position = p_text_message_block->inline_object.object_data.object_position_start;
        object_position_update.data_update = -del_size;
        style_docu_area_position_update(p_docu, &p_text_message_block->text_format_info.h_style_list, &object_position_update);
        /* SKS 13jun95 was &p_docu->h_style_docu_area - caused hefo styles to go slidey */
        } /*block*/

        text_redisplay_from_info(p_docu,
                                 p_text_inline_redisplay,
                                 &text_redisplay_info,
                                 &p_text_message_block->text_format_info,
                                 &p_text_message_block->inline_object);
    }
    } /*block*/

    text_redisplay_info_dispose(&text_redisplay_info);

    return(STATUS_OK);
}

OBJECT_PROTO(extern, object_story);
OBJECT_PROTO(extern, object_story)
{
    switch(t5_message)
    {
    default:
        return(proc_event_tx_main(p_docu, t5_message, p_data));

    case T5_MSG_INITCLOSE:
        return(story_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_OBJECT_HOW_BIG:
        return(story_msg_object_how_big(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_OBJECT_HOW_WIDE:
        return(story_msg_object_how_wide(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_INSERT_INLINE_REDISPLAY:
        return(story_msg_insert_inline_redisplay(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_MSG_DELETE_INLINE_REDISPLAY:
        return(story_msg_delete_inline_redisplay(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:
        return(story_cmd_setc(p_docu, t5_message, (P_TEXT_MESSAGE_BLOCK) p_data));
    }
}

/* end of ob_story.c */
