/* redraw.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#include "cmodules/gr_diag.h"

#include "cmodules/gr_rdia3.h"

extern DRAG_STATE drag_state;

/******************************************************************************

     Redrawing code

******************************************************************************/

static void
rec_frame_outline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    P_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_STYLE p_style)
{
    BORDER_LINE_FLAGS border_line_flags_template;
    CLEAR_BORDER_LINE_FLAGS(border_line_flags_template);

    if(SF_BORDER_NONE == p_style->para_style.border)
        return;

    border_line_flags_template.border_style = p_style->para_style.border;

    { /* left border */
    BORDER_LINE_FLAGS border_line_flags = border_line_flags_template;
    PIXIT_LINE pixit_line;

    pixit_line.tl = p_pixit_rect->tl;
    pixit_line.br = p_pixit_rect->br;
    pixit_line.br.x = pixit_line.tl.x;
    pixit_line.horizontal = 0;

    /*border_line_flags.sub_gw_from_b = 1;*/
    /*border_line_flags.add_lw_to_b   = 1;*/

    host_paint_border_line(p_redraw_context, &pixit_line, &p_style->para_style.rgb_border, border_line_flags);
    } /*block*/

    { /* top border */
    BORDER_LINE_FLAGS border_line_flags = border_line_flags_template;
    PIXIT_LINE pixit_line;

    pixit_line.tl = p_pixit_rect->tl;
    pixit_line.br = p_pixit_rect->br;
    pixit_line.br.y = pixit_line.tl.y;
    pixit_line.horizontal = 1;

    /*border_line_flags.add_lw_to_l   = 1;*/
    /*border_line_flags.sub_gw_from_r = 1;*/

    host_paint_border_line(p_redraw_context, &pixit_line, &p_style->para_style.rgb_border, border_line_flags);
    } /*block*/

    { /* right border */
    BORDER_LINE_FLAGS border_line_flags = border_line_flags_template;
    PIXIT_LINE pixit_line;

    pixit_line.tl = p_pixit_rect->tl;
    pixit_line.br = p_pixit_rect->br;
    pixit_line.tl.x = pixit_line.br.x;
    pixit_line.horizontal = 0;

    /*border_line_flags.sub_gw_from_l = 1;*/
    /*border_line_flags.sub_gw_from_r = 1;*/
    /*border_line_flags.sub_gw_from_b = 1;*/
    /*border_line_flags.add_lw_to_b   = 1;*/
    /*border_line_flags.add_lw_to_t   = 1;*/

    host_paint_border_line(p_redraw_context, &pixit_line, &p_style->para_style.rgb_border, border_line_flags);
    } /*block*/

    { /* bottom border */
    BORDER_LINE_FLAGS border_line_flags = border_line_flags_template;
    PIXIT_LINE pixit_line;

    pixit_line.tl = p_pixit_rect->tl;
    pixit_line.br = p_pixit_rect->br;
    pixit_line.tl.y = pixit_line.br.y;
    pixit_line.horizontal = 1;

    /*border_line_flags.sub_gw_from_t = 1;*/
    /*border_line_flags.sub_gw_from_b = 1;*/
    /*border_line_flags.add_lw_to_b   = 1;*/
    /*border_line_flags.sub_gw_from_r = 1;*/
    /*border_line_flags.add_gw_to_r = 1;*/

    host_paint_border_line(p_redraw_context, &pixit_line, &p_style->para_style.rgb_border, border_line_flags);
    } /*block*/
}

/* Redraw a resource */

_Check_return_
static STATUS
rec_render_resource(
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_PIXIT_RECT p_pixit_rect)
{
    BOOL config_display_pictures = !global_preferences.dont_display_pictures; /* SKS for 1.30 */
    BOOL display_picture = config_display_pictures;

    if(p_redraw_context->flags.printer)
        display_picture = 1;

    if(!display_picture)
        host_paint_rectangle_crossed(p_redraw_context, p_pixit_rect, &rgb_stash[3]);
    else
    {
        REC_RESOURCE rec_resource;
        T5_FILETYPE t5_filetype;
        PC_U8 p_data = P_U8_NONE;
        ARRAY_HANDLE h_data = 0;
        S32 length = 0;

        status_return(rec_read_resource(&rec_resource, p_rec_projector, p_data_ref, FALSE));

        t5_filetype = rec_resource_filetype(&rec_resource);

        if(FILETYPE_LINKED_FILE == rec_resource.t5_filetype)
        {
            TCHARZ buffer[BUF_MAX_PATHSTRING];

            memcpy32(buffer, rec_resource.ev_data.arg.string.uchars, rec_resource.ev_data.arg.string.size);
            buffer[rec_resource.ev_data.arg.string.size] = CH_NULL;

            trace_1(TRACE_APP_DPLIB, TEXT("Name is %s"), buffer);

            if(status_ok(file_memory_load(p_docu_from_docno(p_rec_projector->docno), &h_data, buffer, NULL, 0)))
            {
                length = array_elements(&h_data);
                p_data = array_rangec(&h_data, U8, 0, length);
            }
            else
            {
                assert0();
                t5_filetype = FILETYPE_DATA; /* Data file */
            }
        }
        else if(NULL != rec_resource.ev_data.arg.db_blob.data)
        {
            length = rec_resource.ev_data.arg.db_blob.size;
            p_data = rec_resource.ev_data.arg.db_blob.data;
        }
        else
        {
            assert0();
            t5_filetype = FILETYPE_DATA; /* Data file */
        }

        switch(t5_filetype)
        {
        default:
            {
#if RISCOS
            P_SCB p_scb;
            SBCHARZ name_buffer[16];
            RESOURCE_BITMAP_ID resource_bitmap_id;

            /* There be an unknown filetype here! perhaps we should render its filetype iconsprite */
            riscos_filesprite(name_buffer /*filled*/, elemof32(name_buffer), t5_filetype);

            resource_bitmap_id.bitmap_name = name_buffer;

            p_scb = resource_bitmap_find_system(&resource_bitmap_id).p_scb;

            if(NULL != p_scb)
                host_paint_sprite(p_redraw_context, p_pixit_rect, p_scb, FALSE);
#else
            assert0();
#endif
            break;
            }

        case FILETYPE_DRAW:
            {
            DRAW_DIAG draw_diag;
            PIXIT_SIZE drawing_pixit_size, frame_pixit_size;
            GR_SCALE_PAIR gr_scale_pair;

            myassert0x(((length)&3)==0, TEXT("Bad size for drawfile - not a multiple of 4"));

            frame_pixit_size.cx = pixit_rect_width(p_pixit_rect);
            frame_pixit_size.cy = pixit_rect_height(p_pixit_rect);

            /* Find the bounding box to compute some reasonable scale factors */
            host_read_drawfile_size(p_data, &drawing_pixit_size);

            gr_scale_pair.x = muldiv64(GR_SCALE_ONE, frame_pixit_size.cx, drawing_pixit_size.cx);
            gr_scale_pair.y = muldiv64(GR_SCALE_ONE, frame_pixit_size.cy, drawing_pixit_size.cy);

            if(gr_scale_pair.y > gr_scale_pair.x) /* choose the smaller scale factor */
                gr_scale_pair.y = gr_scale_pair.x;
            else
                gr_scale_pair.x = gr_scale_pair.y;

            draw_diag.data = de_const_cast(P_BYTE, p_data);
            draw_diag.length = length;

            host_paint_drawfile(p_redraw_context, &p_pixit_rect->tl, &gr_scale_pair, &draw_diag, 0);
            break;
            }

#if RISCOS
        case FILETYPE_WINDOWS_BMP:
            host_paint_bitmap(p_redraw_context, p_pixit_rect, p_data, TRUE);
            break;

        case FILETYPE_SPRITE:
            {
            P_SCB p_scb = PtrAddBytes(P_SCB, p_data, 12 /*sizeof32 spritefileheader*/);
            host_paint_sprite(p_redraw_context, p_pixit_rect, p_scb, TRUE);
            break;
            }
#endif
        }

        al_array_dispose(&h_data);

        ss_data_free_resources(&rec_resource.ev_data);
    }

    return(STATUS_DONE); /* rendered ok */
}

_Check_return_
static STATUS
text_redraw_card(
    _DocuRef_   P_DOCU p_docu,
    /*inout*/ P_OBJECT_DATA p_object_data,
    _OutRef_    P_ARRAY_HANDLE p_array_handle)
{
    STATUS status = STATUS_OK;
    ARRAY_HANDLE h_text = 0;
    S32 modified;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 40);
    quick_ublock_with_buffer_setup(quick_ublock);

    *p_array_handle = 0;

    assert((p_object_data->data_ref.data_space == DATA_DB_FIELD) || (p_object_data->data_ref.data_space == DATA_DB_TITLE));

    /* Examine the cache state */
    if(status_done(status = rec_cache_enquire(p_docu, &p_object_data->data_ref, &h_text, &modified)))
    {
        p_object_data->object_id = OBJECT_ID_REC;
        p_object_data->u.ustr_inline = ustr_inline_from_h_ustr(&h_text);
        return(STATUS_DONE);
    }

    /* Go fetch it */
    if(status_ok(status = rec_object_read_card(p_docu, &p_object_data->data_ref, &quick_ublock)))
    {
        U32 length = quick_ublock_bytes(&quick_ublock);

        if(0 != length)
        {
            P_USTR_INLINE ustr_text_out;

            if(NULL != (ustr_text_out = al_array_alloc(p_array_handle, _UCHARS_INLINE, length + 1, &array_init_block_uchars, &status)))
            {
                memcpy32(ustr_text_out, quick_ublock_uchars(&quick_ublock), length);
                PtrPutByteOff(ustr_text_out, length, CH_NULL);

                p_object_data->object_id = OBJECT_ID_REC;
                p_object_data->u.ustr_inline = ustr_text_out;

                quick_ublock_dispose(&quick_ublock);
                return(STATUS_DONE);
            }

            status_assert(status);
        }
    }

    p_object_data->u.p_object = P_DATA_NONE;

    quick_ublock_dispose(&quick_ublock);
    return(status);
}

/******************************************************************************
*
* rec_story_redraw_frame
*
* Called from ob_rec's main redraw_frame to get ob_story to draw a field or title of a card
* It returns STATUS_DONE iff it drew the text for the frame
*            STATUS_OK iff the given data_ref was not the one currently under editing
*            STATUS_FAIL etc if it does not like its args.
*
* Note that the clipping rectangle needs to be set up to be the frame
*
* THIS IS NOT CALLED VIA proc_event_ed_rec()
*
******************************************************************************/

_Check_return_
static STATUS
rec_story_redraw_frame(
    P_REC_PROJECTOR p_rec_projector,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_DATA_REF p_data_ref,
    _InVal_     BOOL is_picture)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    BOOL is_text = !is_picture;
    OBJECT_REDRAW my_object_redraw = *p_object_redraw; /* Copy the redraw context so that we can change things in our copy */
    OBJECT_DATA object_data_rec = my_object_redraw.object_data;
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect_frame;
    ARRAY_HANDLE array_handle_text = 0;
    STATUS status;

    if(!my_object_redraw.flags.show_content && !my_object_redraw.flags.show_selection)
        return(STATUS_DONE);

    object_data_rec.data_ref = *p_data_ref;

    object_data = object_data_rec;

    if(is_text)
        if(!status_done(status = text_redraw_card(p_docu, &object_data, &array_handle_text)))
            return(status);

    if(status_ok(status = rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_frame, p_data_ref)))
    {
        SKEL_RECT skel_rect_frame_abs = skel_rect_frame; /* Take a copy to work on */
        PIXIT_RECT pixit_rect;
        STYLE border_style;
        RECT_FLAGS rect_flags;
        RECT_FLAGS_CLEAR(rect_flags);

        {
        STYLE_SELECTOR selector;
        style_selector_clear(&selector);
        style_selector_bit_set(&selector, STYLE_SW_PS_RGB_BORDER);
        style_selector_bit_set(&selector, STYLE_SW_PS_BORDER);
        rec_style_for_data_ref(p_docu, p_data_ref, &border_style, &selector);
        } /*block*/

        if(SF_BORDER_NONE != border_style.para_style.border) /* SKS 06may95 allow completely borderless objects */
        {
            rect_flags.reduce_left_by_1  = 1; /* maybe should be _by_2 */
            rect_flags.reduce_up_by_1    = 1;
          /*rect_flags.reduce_right_by_1 = 1;*/
          /*rect_flags.reduce_down_by_1  = 1;*/
        }

        /* Lets try and sort out the clip rectangle */
        my_object_redraw.skel_rect_clip.tl.pixit_point.x = MAX(my_object_redraw.skel_rect_object.tl.pixit_point.x, skel_rect_frame_abs.tl.pixit_point.x);
        my_object_redraw.skel_rect_clip.br.pixit_point.x = MIN(my_object_redraw.skel_rect_object.br.pixit_point.x, skel_rect_frame_abs.br.pixit_point.x);

        my_object_redraw.skel_rect_clip.tl.pixit_point.y = MAX(my_object_redraw.skel_rect_object.tl.pixit_point.y, skel_rect_frame_abs.tl.pixit_point.y);
        my_object_redraw.skel_rect_clip.br.pixit_point.y = MIN(my_object_redraw.skel_rect_object.br.pixit_point.y, skel_rect_frame_abs.br.pixit_point.y);

        pixit_rect.tl = my_object_redraw.skel_rect_clip.tl.pixit_point;
        pixit_rect.br = my_object_redraw.skel_rect_clip.br.pixit_point;

        if(my_object_redraw.flags.show_content) /* best to do this before setting the clip rectangle which would exclude it, huh? */
            rec_frame_outline(&my_object_redraw.redraw_context, &pixit_rect, &border_style);

        if(host_set_clip_rectangle(&my_object_redraw.redraw_context, &pixit_rect, rect_flags))
        {
            TEXT_MESSAGE_BLOCK text_message_block;

            rec_text_message_block_init(p_docu, &text_message_block, &my_object_redraw, &skel_rect_frame, &object_data);

            my_object_redraw.rgb_back = text_message_block.text_format_info.style_text_global.para_style.rgb_back;

            if(my_object_redraw.flags.show_content)
            {
                host_paint_rectangle_filled(&my_object_redraw.redraw_context, &pixit_rect, &my_object_redraw.rgb_back);

                if(is_picture)
                {
                    PIXIT_RECT picture_pixit_rect = pixit_rect;

                    if(SF_BORDER_NONE != border_style.para_style.border) /* SKS 06may95 allow for borders on objects. 05feb96 get it right */
                    {
                        picture_pixit_rect.tl.x += my_object_redraw.redraw_context.line_width_eff.x;
                        picture_pixit_rect.tl.y += my_object_redraw.redraw_context.line_width_eff.y;
                        picture_pixit_rect.br.x -= my_object_redraw.redraw_context.line_width_eff.x;
                        picture_pixit_rect.br.y -= my_object_redraw.redraw_context.line_width_eff.y;
                    }

                    status = rec_render_resource(p_rec_projector, &my_object_redraw.redraw_context, p_data_ref, &picture_pixit_rect);
                }
            }

             if(is_text)
             {
                 if(text_message_block.inline_object.inline_len != 0)
                 {
                     if(status_ok(status = object_call_id(OBJECT_ID_STORY, p_docu, T5_EVENT_REDRAW, &text_message_block)))
                        status = STATUS_DONE;
                 }
                 else
                {
                    BOOL do_invert = FALSE; /* NULL object - nothing to paint, but invert if necessary !!!! */

                    if(my_object_redraw.flags.show_selection)
                    {
                        if(!my_object_redraw.flags.show_content)
                        {
                            if(my_object_redraw.flags.marked_now != my_object_redraw.flags.marked_screen)
                                do_invert = TRUE;
                        }
                        else if(my_object_redraw.flags.marked_now)
                            do_invert = TRUE;
                    }

                    if(do_invert)
                        host_invert_rectangle_filled(&my_object_redraw.redraw_context, &pixit_rect, &my_object_redraw.rgb_fore, &my_object_redraw.rgb_back);

                    status = STATUS_DONE;
                }
            }

            host_restore_clip_rectangle(&p_object_redraw->redraw_context);
        }
        else
            status = STATUS_DONE;
    }
    else
        status = STATUS_OK; /* Couldn't really do it */

    al_array_dispose(&array_handle_text);

    return(status);
}

_Check_return_
static STATUS
rec_story_redraw_field(
    P_REC_PROJECTOR p_rec_projector,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_DATA_REF p_data_ref)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    OBJECT_REDRAW my_object_redraw = *p_object_redraw; /* Copy the redraw context so that we can change things in our copy */
    OBJECT_DATA object_data_rec = my_object_redraw.object_data;
    OBJECT_DATA object_data;
    ARRAY_HANDLE array_handle_text = 0;
    SKEL_RECT skel_rect_frame;
    STATUS status;

    object_data_rec.data_ref = *p_data_ref;

    object_data = object_data_rec;

    if(!status_done(status = text_redraw_card(p_docu, &object_data, &array_handle_text)))
        return(status);

    if(status_ok(status = rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_frame, p_data_ref)))
    {
        PIXIT_RECT pixit_rect;
        TEXT_MESSAGE_BLOCK text_message_block;

        pixit_rect.tl = my_object_redraw.skel_rect_clip.tl.pixit_point;
        pixit_rect.br = my_object_redraw.skel_rect_clip.br.pixit_point;

        rec_text_message_block_init(p_docu, &text_message_block, &my_object_redraw, &skel_rect_frame, &object_data);

        my_object_redraw.rgb_back = text_message_block.text_format_info.style_text_global.para_style.rgb_back;

        if(my_object_redraw.flags.show_content) /* has been done higher up by sk_draw but that can't get the right style so test here */
            if(rgb_compare_not_equals(&my_object_redraw.rgb_back, &p_object_redraw->rgb_back))
                host_paint_rectangle_filled(&my_object_redraw.redraw_context, &pixit_rect, &my_object_redraw.rgb_back);

        if(text_message_block.inline_object.inline_len != 0)
        {
            if(status_ok(status = object_call_id(OBJECT_ID_STORY, p_docu, T5_EVENT_REDRAW, &text_message_block)))
                status = STATUS_DONE;
        }
        else
        {
            BOOL do_invert = FALSE; /* NULL object - nothing to paint, but invert if necessary !!!! */

            if(my_object_redraw.flags.show_selection)
            {
                if(!my_object_redraw.flags.show_content)
                {
                    if(my_object_redraw.flags.marked_now != my_object_redraw.flags.marked_screen)
                        do_invert = TRUE;
                }
                else if(my_object_redraw.flags.marked_now)
                    do_invert = TRUE;
            }

            if(do_invert)
                host_invert_rectangle_filled(&my_object_redraw.redraw_context, &pixit_rect, &my_object_redraw.rgb_fore, &my_object_redraw.rgb_back);

            status = STATUS_DONE;
        }
    }

    al_array_dispose(&array_handle_text);

    return(status);
}

/* Draw the clickable ears on the red drag box thing for card design

   There are eight of them at the corners an edge-midpoints of the given rect
*/

#define rgb_frame      &rgb_stash[11 /*red*/]
#define rgb_ear_outer  &rgb_stash[11 /*red*/]
#define rgb_ear_inner  &rgb_stash[0 /*white*/]

static void
rec_redraw_frame_borders_ears(
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    P_PIXIT_RECT_EARS p_pixit_rect_ears)
{
    const PC_REDRAW_CONTEXT p_redraw_context = &p_object_redraw->redraw_context;
    STATUS note_position = PIXIT_RECT_EAR_CENTRE;

    host_paint_rectangle_outline(p_redraw_context, &p_pixit_rect_ears->ear[note_position], rgb_frame);

    while(++note_position < PIXIT_RECT_EAR_COUNT)
    {
        if(p_pixit_rect_ears->ear_active[note_position])
        {
            host_paint_rectangle_filled(p_redraw_context, &p_pixit_rect_ears->ear[note_position], rgb_ear_inner);
            host_paint_rectangle_outline(p_redraw_context, &p_pixit_rect_ears->ear[note_position], rgb_ear_outer);
        }
    }
}

static void
rec_redraw_frame_borders(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_REC_FRAME p_rec_frame,
    _InRef_     PC_DATA_REF p_data_ref)
{
    const PC_REDRAW_CONTEXT p_redraw_context = &p_object_redraw->redraw_context;
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);

    if(!p_rec_instance->frame.on)
        return;

    if( (p_rec_instance->frame.data_ref.data_space == p_data_ref->data_space)  &&
        (db_id_from_rec_data_ref(&p_rec_instance->frame.data_ref) == db_id_from_rec_data_ref(p_data_ref) &&
        (field_id_from_rec_data_ref(&p_rec_instance->frame.data_ref) == field_id_from_rec_data_ref(p_data_ref)) &&
        (get_record_from_rec_data_ref(&p_rec_instance->frame.data_ref) == get_record_from_rec_data_ref(p_data_ref))) )
    {
        PIXIT_RECT pixit_rect;

        {
        PC_PIXIT_RECT p_pixit_rect = (p_data_ref->data_space == DATA_DB_FIELD) ? &p_rec_frame->pixit_rect_field : &p_rec_frame->pixit_rect_title;
        pixit_rect.tl.x = p_object_redraw->skel_rect_object.tl.pixit_point.x + p_pixit_rect->tl.x;
        pixit_rect.tl.y = p_object_redraw->skel_rect_object.tl.pixit_point.y + p_pixit_rect->tl.y;
        pixit_rect.br.x = p_object_redraw->skel_rect_object.tl.pixit_point.x + p_pixit_rect->br.x;
        pixit_rect.br.y = p_object_redraw->skel_rect_object.tl.pixit_point.y + p_pixit_rect->br.y;
        } /*block*/

        if(p_object_redraw->flags.show_selection && !drag_state.rec_static_flag /* needs expansion of show_selection state eventually <<< */)
        {
            PIXIT_RECT_EARS ears;
            pixit_rect_get_ears(&ears, &pixit_rect, &p_redraw_context->one_program_pixel);
            rec_redraw_frame_borders_ears(p_object_redraw, &ears);
        }

        if(drag_state.flag != 0)
        {   /* We are dragging so draw the thing at the corrected position */
            PIXIT_RECT pixit_rect_temp = pixit_rect;
            pixit_rect_temp.tl.x += drag_state.pixit_rect_deltas.tl.x;
            pixit_rect_temp.tl.y += drag_state.pixit_rect_deltas.tl.y;
            pixit_rect_temp.br.x += drag_state.pixit_rect_deltas.br.x;
            pixit_rect_temp.br.y += drag_state.pixit_rect_deltas.br.y;
            host_invert_rectangle_outline(p_redraw_context, &pixit_rect_temp, &rgb_stash[2 /*grey*/], &rgb_stash[0 /*white*/]);
        }
    }
}

static void
rec_redraw_popup(
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_REC_FRAME p_rec_frame)
{
    const PC_REDRAW_CONTEXT p_redraw_context = &p_object_redraw->redraw_context;
    PIXIT_RECT pixit_rect;

    pixit_rect.tl = p_object_redraw->skel_rect_object.tl.pixit_point;

    pixit_rect.tl.x += p_rec_frame->pixit_rect_field.br.x;
    pixit_rect.tl.y += p_rec_frame->pixit_rect_field.tl.y;

    pixit_rect.tl.x += p_redraw_context->one_program_pixel.x; /* same as for click */

    pixit_rect.br = pixit_rect.tl;

    pixit_rect.br.x += DEFAULT_GRIGHT_SIZE;
    pixit_rect.br.y += DEFAULT_GRIGHT_SIZE;

#if RISCOS
    {
    PIXIT_RECT pixit_rect_adjusted;
    GDI_RECT gdi_rect;

    /* NB host_ploticon() plots window relative */
    pixit_rect_adjusted.tl.x = pixit_rect.tl.x + p_redraw_context->pixit_origin.x;
    pixit_rect_adjusted.tl.y = pixit_rect.tl.y + p_redraw_context->pixit_origin.y;
    pixit_rect_adjusted.br.x = pixit_rect.br.x + p_redraw_context->pixit_origin.x;
    pixit_rect_adjusted.br.y = pixit_rect.br.y + p_redraw_context->pixit_origin.y;

    status_consume(window_rect_from_pixit_rect(&gdi_rect, &pixit_rect_adjusted, &p_redraw_context->host_xform));

    if((gdi_rect.tl.y - gdi_rect.br.y) < 16) /* SKS 06apr95 in really small views draw little grey rectangle */
        host_paint_rectangle_filled(p_redraw_context, &pixit_rect, &rgb_stash[1 /*lt gray*/]);
    else
    {
        static const RESOURCE_BITMAP_ID gright_id = { OBJECT_ID_SKEL, BITMAP_NAME_COMBO };
        WimpIconBlockWithBitset gright_icon;

        BBox_from_gdi_rect(gright_icon.bbox, gdi_rect);

        gright_icon.flags.u32 = 0;

        gright_icon.flags.bits.sprite   = 1;
        gright_icon.flags.bits.indirect = 1;

        gright_icon.data.is.sprite = resource_bitmap_find(&gright_id).p_u8;
        gright_icon.data.is.sprite_area = (void *) 1; /* Window Manager's sprite area - shouldn't be needed */
        gright_icon.data.is.sprite_name_length = 0;

        host_ploticon(&gright_icon);
    }
    } /*block*/
#endif
}

_Check_return_
static BOOL
intersect_pixit_rect(
    _InRef_     PC_PIXIT_RECT p_pixit_rect_1,
    _InRef_     PC_SKEL_RECT p_skel_rect)
{
    PIXIT_RECT pixit_rect_clip;

    pixit_rect_clip.tl.x = MAX(p_pixit_rect_1->tl.x, p_skel_rect->tl.pixit_point.x);
    pixit_rect_clip.tl.y = MAX(p_pixit_rect_1->tl.y, p_skel_rect->tl.pixit_point.y);
    pixit_rect_clip.br.x = MIN(p_pixit_rect_1->br.x, p_skel_rect->br.pixit_point.x);
    pixit_rect_clip.br.y = MIN(p_pixit_rect_1->br.y, p_skel_rect->br.pixit_point.y);

    if((pixit_rect_clip.tl.x >= pixit_rect_clip.br.x) || (pixit_rect_clip.tl.y >= pixit_rect_clip.br.y))
        return(FALSE);

    return(TRUE);
}

_Check_return_
static STATUS
rec_card_redraw_field(
    P_REC_PROJECTOR p_rec_projector,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_REC_FRAME p_rec_frame,
    _InRef_     PC_DATA_REF p_data_ref)
{
    const PC_REDRAW_CONTEXT p_redraw_context = &p_object_redraw->redraw_context;
    STATUS status = STATUS_OK;
    PIXIT_RECT pixit_rect;
    BOOL is_picture;

    pixit_rect.tl.x = p_object_redraw->skel_rect_object.tl.pixit_point.x + p_rec_frame->pixit_rect_field.tl.x;
    pixit_rect.tl.y = p_object_redraw->skel_rect_object.tl.pixit_point.y + p_rec_frame->pixit_rect_field.tl.y;
    pixit_rect.br.x = p_object_redraw->skel_rect_object.tl.pixit_point.x + p_rec_frame->pixit_rect_field.br.x;
    pixit_rect.br.y = p_object_redraw->skel_rect_object.tl.pixit_point.y + p_rec_frame->pixit_rect_field.br.y;

    pixit_rect.br.x += p_redraw_context->border_width.x;
    pixit_rect.br.y += p_redraw_context->border_width.y;

    if(!intersect_pixit_rect(&pixit_rect, &p_object_redraw->skel_rect_clip))
        return(STATUS_OK);

    {
    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);

    switch(p_fielddef->type)
    {
    case FIELD_FILE:
    case FIELD_PICTURE:
        is_picture = TRUE;
        break;

    default:
        is_picture = FALSE;
        break;
    }
    } /*block*/

    status = rec_story_redraw_frame(p_rec_projector, p_object_redraw, p_data_ref, is_picture);
    /* This will be STATUS_DONE if it drew it, STATUS_OK if there was nothing to be drawn or otherwise a failure code */

    if(STATUS_OK == status)
    {
        /* Record not available...*/
        host_paint_rectangle_filled(p_redraw_context, &pixit_rect, &rgb_stash[4 /*mid gray*/]);
        status = STATUS_OK;
    }

    rec_redraw_frame_borders(p_docu_from_docno(p_rec_projector->docno), p_object_redraw, p_rec_frame, p_data_ref);

    return(status);
}

_Check_return_
static STATUS
rec_card_redraw_title(
    P_REC_PROJECTOR p_rec_projector,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_REC_FRAME p_rec_frame,
    _InRef_     PC_DATA_REF p_data_ref)
{
    STATUS status;
    PIXIT_RECT pixit_rect;

    pixit_rect.tl.x = p_object_redraw->skel_rect_object.tl.pixit_point.x + p_rec_frame->pixit_rect_title.tl.x;
    pixit_rect.tl.y = p_object_redraw->skel_rect_object.tl.pixit_point.y + p_rec_frame->pixit_rect_title.tl.y;
    pixit_rect.br.x = p_object_redraw->skel_rect_object.tl.pixit_point.x + p_rec_frame->pixit_rect_title.br.x;
    pixit_rect.br.y = p_object_redraw->skel_rect_object.tl.pixit_point.y + p_rec_frame->pixit_rect_title.br.y;

    if(!intersect_pixit_rect(&pixit_rect, &p_object_redraw->skel_rect_clip))
        return(STATUS_OK);

    status = rec_story_redraw_frame(p_rec_projector, p_object_redraw, p_data_ref, FALSE);
    /* This will be STATUS_DONE if it drew it, STATUS_OK if there was nothing to be drawn or otherwise a failure code */

    rec_redraw_frame_borders(p_docu_from_docno(p_rec_projector->docno), p_object_redraw, p_rec_frame, p_data_ref);

    return(status);
}

/* Strategy for marking is :-

   Impose a notional ordering on the fields in a card
   Span all the fields in this order
   switch on the marking when you hit the start
   switch off the marking after you do the end

   pass nun-object positions for the start and end markers
   unless this field is the start or end field
   in which case pass the start for the start
   and the end for the end or indeed both!

    my_flag = FALSE;

    for(all fields)
    {
        my_start = NONE;
        my_end   = NONE;

        if(compare_data_ref_with_op(&data_ref, &start)
        {
            my_flag  =  TRUE
            my_start = start;
        }

        if(compare_data_ref_with_op(&data_ref, &end)
            my_end =   end;

        ob_story(my_flag, my_start, my_end)

        if(compare_data_ref_with_op(&data_ref, &end)
            my_flag = FALSE;
    }
*/

_Check_return_
static BOOL
compare_ops(
    _InRef_     PC_OBJECT_POSITION p_op1,
    _InRef_     PC_OBJECT_POSITION p_op2)
{
    return(p_op1->more_data == p_op2->more_data);
}

/* redraw the card contained in SLR in p_object_redraw->object_data.data_ref
   In the Card View we draw all the frames in turn, using the field id in the rec_frame structure
   to determine which field and is used
*/

_Check_return_
extern STATUS
rec_card_redraw(
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    OBJECT_REDRAW my_object_redraw = *p_object_redraw; /* Copy the redraw context so that we can change things in our copy */
    const PC_REDRAW_CONTEXT p_redraw_context = &my_object_redraw.redraw_context;
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    DATA_REF data_ref = *p_data_ref;
    P_FIELDDEF p_fielddef;
    ARRAY_INDEX i;
    BOOL my_marked_now = FALSE;
    BOOL my_marked_screen = FALSE;
    OBJECT_POSITION my_start_now, my_end_now, my_start_screen, my_end_screen;
    OBJECT_POSITION object_position;
    STATUS status = STATUS_OK;
    DRAW_DIAG_OFFSET groupStart = 0; /* keep dataflower happy */

    if(p_redraw_context->flags.drawfile)
    {   /* create a group object to contain this card's objects */
        U8Z groupName[16];
        consume_int(xsnprintf(groupName, elemof32(groupName), "Card " S32_FMT, p_data_ref->arg.db_field.record + 1));
        status_return(gr_riscdiag_group_new(p_redraw_context->p_gr_riscdiag, &groupStart, groupName));
    }

    object_position.object_id = OBJECT_ID_REC;

    /* draw frames starting with the ones at the back */
    for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
    {
        PC_REC_FRAME p_rec_frame = array_ptrc(&p_rec_projector->h_rec_frames, REC_FRAME, i);

        set_field_id_for_rec_data_ref(&data_ref, p_rec_frame->field_id);

        p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);

        if(P_DATA_NONE == p_fielddef)
            continue;

        my_object_redraw.flags.marked_now = FALSE;
        my_object_redraw.flags.marked_screen = FALSE;

        if(p_rec_frame->title_show)
        {
            DATA_REF data_ref_title;

            title_data_ref_from_rec_data_ref(&data_ref_title, &data_ref);

            status = rec_card_redraw_title(p_rec_projector, &my_object_redraw, p_rec_frame, &data_ref_title);

            if(status_fail(status))
            {
                myassert0(TEXT("rec card title redraw failed"));
                status = STATUS_OK;
            }
        }

        set_rec_object_position_from_data_ref(p_docu, &object_position, &data_ref);

        object_position_init(&my_start_now);
        object_position_init(&my_end_now);
        object_position_init(&my_start_screen);
        object_position_init(&my_end_screen);

        if(compare_ops(&object_position, &p_object_redraw->start_now))
        {
            my_marked_now  =  TRUE;
            my_start_now   =  p_object_redraw->start_now;
        }

        if(compare_ops(&object_position, &p_object_redraw->end_now))
            my_end_now     =  p_object_redraw->end_now;

        if(compare_ops(&object_position, &p_object_redraw->start_screen))
        {
            my_marked_screen  =  TRUE;
            my_start_screen   =  p_object_redraw->start_screen;
        }

        if(compare_ops(&object_position, &p_object_redraw->end_screen))
            my_end_screen     =  p_object_redraw->end_screen;

        my_object_redraw.flags.marked_now = my_marked_now && p_object_redraw->flags.marked_now;
        my_object_redraw.start_now        = my_start_now;
        my_object_redraw.end_now          = my_end_now;

        my_object_redraw.flags.marked_screen = my_marked_screen && p_object_redraw->flags.marked_screen;
        my_object_redraw.start_screen        = my_start_screen;
        my_object_redraw.end_screen          = my_end_screen;

        status = rec_card_redraw_field(p_rec_projector, &my_object_redraw, p_rec_frame, &data_ref);

        if(status_fail(status))
        {
            trace_0(TRACE_APP_DPLIB, TEXT("rec redraw frame failed"));
            status = STATUS_OK;
        }

        if(compare_ops(&object_position, &p_object_redraw->end_now))
            my_marked_now = FALSE;

        if(compare_ops(&object_position, &p_object_redraw->end_screen))
            my_marked_screen = FALSE;

        /* Lets see if there is a value list popup to be drawn */
        if(p_object_redraw->flags.show_content)
            if(!p_redraw_context->flags.printer && !p_redraw_context->flags.drawfile && strlen(p_fielddef->value_list))
                rec_redraw_popup(p_object_redraw, p_rec_frame);
    }

    if(p_redraw_context->flags.drawfile)
    {   /* complete this card's group object */
        gr_riscdiag_group_end(p_redraw_context->p_gr_riscdiag, groupStart);
    }

    return(status);
}

_Check_return_
extern STATUS
rec_sheet_redraw_field(
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_data_ref->arg.db_field.field_id);
    REC_FRAME rec_frame;
    PIXIT_RECT pixit_rect;
    STATUS status;

    if((P_DATA_NONE == p_fielddef) || p_fielddef->hidden) /* in SHEET we won't get requests for fields we aren't displaying - surely */
        return(STATUS_OK);

    if(status_fail(get_frame_by_field_id(p_rec_projector, p_fielddef->id, &rec_frame)))
    {
        myassert1(TEXT("There is an unhidden field without a frame") S32_TFMT, 0);
        return(STATUS_OK);
    }

    assert(rec_frame.field_id == field_id_from_rec_data_ref(p_data_ref));

    pixit_rect.tl = p_object_redraw->skel_rect_object.tl.pixit_point;
    pixit_rect.br = p_object_redraw->skel_rect_object.br.pixit_point;

    if(!intersect_pixit_rect(&pixit_rect, &p_object_redraw->skel_rect_clip))
        return(STATUS_OK);

    switch(p_fielddef->type)
    {
    case FIELD_FILE:
    case FIELD_PICTURE:
        status = rec_render_resource(p_rec_projector, &p_object_redraw->redraw_context, p_data_ref, &pixit_rect);
        break;

    default:
    case FIELD_TEXT:
        if(status_done(status = rec_story_redraw_field(p_rec_projector, p_object_redraw, p_data_ref)))
            break;

        /* Record not available...*/
        host_paint_rectangle_filled(&p_object_redraw->redraw_context, &pixit_rect, &rgb_stash[4 /*mid_grey*/]);
        status = STATUS_OK;
        break;
    }

    return(status);
}

/* end of redraw.c */
