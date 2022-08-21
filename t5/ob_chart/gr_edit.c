/* gr_edit.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart editing interface for Fireworkz */

/* SKS July 1993 */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#include "ob_skel/ff_io.h"

/*
internal routines
*/

static void
chart_selection_display(
    P_CHART_HEADER p_chart_header);

extern void
chart_selection_kill_repr(
    P_CHART_HEADER p_chart_header);

extern void
chart_selection_make_repr(
    P_CHART_HEADER p_chart_header);

extern void
callback_from_gr_chart(
    P_ANY client_handle,
    _In_        GR_CHART_CALLBACK_RC rc)
{
    switch(rc)
    {
    case GR_CHART_CALLBACK_RC_SELECTION_KILL_REPR:
        chart_selection_kill_repr((P_CHART_HEADER) client_handle);
        break;

    case GR_CHART_CALLBACK_RC_SELECTION_MAKE_REPR:
        chart_selection_make_repr((P_CHART_HEADER) client_handle);
        break;

    default:
        break;
    }
}

_Check_return_
static STATUS
chart_load_image_file_to_selection(
    P_CHART_HEADER p_chart_header,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InVal_     BOOL embed_file)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);
    GR_FILLSTYLEB gr_fillstyleb;
    IMAGE_CACHE_HANDLE image_cache_handle;
    STATUS status;
    ARRAY_HANDLE h_data;

    status_return(image_cache_entry_ensure(&image_cache_handle, filename, t5_filetype));

    h_data = image_cache_loaded_ensure(image_cache_handle);

    if(!h_data)
        return(image_cache_error_query(image_cache_handle));

    if(embed_file)
        image_cache_embedded_updating_entry(&image_cache_handle);

    chart_modify_docu(p_chart_header);

    gr_chart_objid_fillstyleb_query(cp, p_chart_header->selection.id, &gr_fillstyleb);

    if(!gr_fillstyleb.bits.pattern)
    {
        gr_fillstyleb.bits.pattern = 1;
        gr_fillstyleb.bits.isotropic = 0;
        gr_fillstyleb.bits.norecolour = 1;
        gr_fillstyleb.bits.notsolid = 1;
    }

    * (P_U32) &gr_fillstyleb.pattern = * (P_U32) &image_cache_handle;

    status = gr_chart_objid_fillstyleb_set(cp, p_chart_header->selection.id, &gr_fillstyleb);

    return(status);
}

/******************************************************************************
*
* clear selection - queue a redraw to take place later
*
******************************************************************************/

extern void
chart_selection_clear(
    P_CHART_HEADER p_chart_header,
    P_NOTE_INFO p_note_info)
{
    /* XOR off now */
    if((NULL != p_chart_header->selection.p_gr_riscdiag) && p_chart_header->selection.p_gr_riscdiag->draw_diag.length && p_note_info)
    {
        NOTE_UPDATE_NOW note_update_now;
        note_update_now.p_note_info = p_note_info;
        REDRAW_FLAGS_CLEAR(note_update_now.redraw_flags);
#if RISCOS
        /* XOR still works */
        note_update_now.redraw_flags.show_selection = TRUE;
#elif WINDOWS
        /* XOR no longer works with GDI+ */
        note_update_now.redraw_flags.show_content = TRUE;
#endif
        status_consume(object_call_id(OBJECT_ID_NOTE, p_docu_from_docno(p_chart_header->docno), T5_MSG_NOTE_UPDATE_NOW, &note_update_now));
    }

    chart_selection_kill_repr(p_chart_header);

    p_chart_header->selection.id = gr_chart_objid_anon;

    chart_selection_display(p_chart_header);
}

static void
chart_selection_display(
    P_CHART_HEADER p_chart_header)
{
    const P_DOCU p_docu = p_docu_from_docno(p_chart_header->docno);
    UCHARZ buffer[BUF_MAX_GR_CHART_OBJID_REPR];
    UI_TEXT ui_text;

    if(p_chart_header->selection.id.name == GR_CHART_OBJNAME_ANON)
    {
        ui_text.type = UI_TEXT_TYPE_RESID;
        ui_text.text.resource_id = CHART_MSG_STATUS_LINE_NO_SELECTION;
    }
    else
    {
        gr_chart_object_name_from_id(p_gr_chart_from_chart_handle(p_chart_header->ch), p_chart_header->selection.id, ustr_bptr(buffer), elemof32(buffer));
        ui_text.type = UI_TEXT_TYPE_USTR_TEMP;
        ui_text.text.ustr = ustr_bptr(buffer);
    }

    status_line_set(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS(OBJECT_ID_CHART), &ui_text);
}

extern void
chart_selection_kill_repr(
    P_CHART_HEADER p_chart_header)
{
    gr_riscdiag_diagram_dispose(&p_chart_header->selection.p_gr_riscdiag);
}

/******************************************************************************
*
* make a selection - best to do it snappily
*
******************************************************************************/

_Check_return_
static STATUS
chart_selection_make(
    P_CHART_HEADER p_chart_header,
    /*const*/ P_GR_CHART_OBJID id,
    P_NOTE_INFO p_note_info)
{
    /* worthwhile doing it again? */
    if(0 != memcmp32(&p_chart_header->selection.id, id, sizeof32(*id)))
    {
        /* remove the old one */

        /* XOR off now */
        if((NULL != p_chart_header->selection.p_gr_riscdiag) && p_chart_header->selection.p_gr_riscdiag->draw_diag.length)
        {
            NOTE_UPDATE_NOW note_update_now;
            note_update_now.p_note_info = p_note_info;
            REDRAW_FLAGS_CLEAR(note_update_now.redraw_flags);
#if RISCOS
            /* XOR still works */
            note_update_now.redraw_flags.show_selection = TRUE;
#elif WINDOWS
            /* XOR no longer works with GDI+ */
            note_update_now.redraw_flags.show_content = TRUE;
#endif
            status_consume(object_call_id(OBJECT_ID_NOTE, p_docu_from_docno(p_chart_header->docno), T5_MSG_NOTE_UPDATE_NOW, &note_update_now));
        }

        chart_selection_kill_repr(p_chart_header);

        p_chart_header->selection.id = *id;

        chart_selection_display(p_chart_header);

        /* build a new selection representation */
        chart_selection_make_repr(p_chart_header);

        /* XOR on now */
        if((NULL != p_chart_header->selection.p_gr_riscdiag) && p_chart_header->selection.p_gr_riscdiag->draw_diag.length)
        {
            NOTE_UPDATE_NOW note_update_now;
            note_update_now.p_note_info = p_note_info;
            REDRAW_FLAGS_CLEAR(note_update_now.redraw_flags);
#if RISCOS
            /* XOR still works */
            note_update_now.redraw_flags.show_selection = TRUE;
#elif WINDOWS
            /* XOR no longer works with GDI+ */
            note_update_now.redraw_flags.show_selection = TRUE; /* yes, that is what I mean */
#endif
            status_consume(object_call_id(OBJECT_ID_NOTE, p_docu_from_docno(p_chart_header->docno), T5_MSG_NOTE_UPDATE_NOW, &note_update_now));
        }

        return(1);
    }

    return(0);
}

/* build a selection representation */

static DRAW_DIAG_OFFSET selectionPathStart;

extern void
chart_selection_make_repr(
    P_CHART_HEADER p_chart_header)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);
    GR_DIAG_OFFSET hitObject;
    GR_DIAG_PROCESS_T process;
    GR_LINESTYLE linestyle;
    STATUS simple_box;
    STATUS status;

    p_chart_header->selection.box.x0 = 0;
    p_chart_header->selection.box.y0 = 0;
    p_chart_header->selection.box.x1 = 0;
    p_chart_header->selection.box.y1 = 0;

    if(p_chart_header->selection.id.name == GR_CHART_OBJNAME_ANON)
        return;

    zero_struct(linestyle);

    /* Draw in an unmottled colour */
#if RISCOS
    {
    const RGB rgb = rgb_stash[11];
    gr_colour_set_RGB(linestyle.fg, rgb.r, rgb.g, rgb.b);
    } /*block*/
#else
    linestyle.fg.visible = 1;
    linestyle.fg.reserved = 0;
    linestyle.fg.red = 0xFF;
    linestyle.fg.green = 0;
    linestyle.fg.blue = 0;
    linestyle.fg.manual = 0;
#endif
#ifdef GR_CHART_FILL_LINE_INTERSTICES
    gr_colour_set_NONE(linestyle.bg);
#endif

    /* search **actual** diagram for first object of right class OR ONE OF ITS CHILDREN */
    * (int *) &process = 0;
    process.recurse = 1;
    process.find_children = 1;

    hitObject = gr_diag_object_search_between(cp->core.p_gr_diag, p_chart_header->selection.id, GR_DIAG_OBJECT_FIRST, GR_DIAG_OBJECT_LAST, process);

    if(hitObject != GR_DIAG_OBJECT_NONE)
    {
        PC_BYTE pObject = array_ptr(&cp->core.p_gr_diag->handle, BYTE, hitObject);
        U32 objectSize = * PtrAddBytes(PC_U32, pObject, offsetof32(GR_DIAG_OBJHDR, n_bytes));
        GR_DIAG_OBJHDR objhdr;

        memcpy32(&objhdr, pObject, sizeof32(objhdr));

        p_chart_header->selection.box = objhdr.bbox;

        simple_box = 1;

        /* scan rest of actual diagram and accumulate bboxes for objects of same class OR THEIR CHILDREN */

        for(;;)
        {
            hitObject = gr_diag_object_search_between(cp->core.p_gr_diag, p_chart_header->selection.id, hitObject + objectSize, GR_DIAG_OBJECT_LAST, process);

            if(hitObject == GR_DIAG_OBJECT_NONE)
                break;

            pObject = array_ptr(&cp->core.p_gr_diag->handle, BYTE, hitObject);
            objectSize = * PtrAddBytes(PC_U32, pObject, offsetof32(GR_DIAG_OBJHDR, n_bytes));

            memcpy32(&objhdr, pObject, sizeof32(objhdr));

            gr_box_union(&p_chart_header->selection.box, &p_chart_header->selection.box, &objhdr.bbox);
        }
    }
    else
    {
        simple_box = 0;
    }

    if(simple_box)
    {
        linestyle.width = 0;

        /* never, even if the program is buggy, make a huge dot-dashed path
         * as it takes forever to render; limit to twice the 'paper' size
        */
        linestyle.pattern = GR_LINE_PATTERN_DASH;

        if((p_chart_header->selection.box.x1 - p_chart_header->selection.box.x0) > 2 * cp->core.layout.width)
            linestyle.pattern = GR_LINE_PATTERN_THIN;

        if((p_chart_header->selection.box.y1 - p_chart_header->selection.box.y0) > 2 * cp->core.layout.height)
            linestyle.pattern = GR_LINE_PATTERN_THIN;

        if(status_ok(status = gr_riscdiag_diagram_new(&p_chart_header->selection.p_gr_riscdiag, "selection", 0, 0, FALSE /*options*/)))
        {
            if((p_chart_header->selection.box.x1 - p_chart_header->selection.box.x0) < 2 * GR_PIXITS_PER_RISCOS)
                p_chart_header->selection.box.x1 = p_chart_header->selection.box.x0  + 2 * GR_PIXITS_PER_RISCOS;

            if((p_chart_header->selection.box.y1 - p_chart_header->selection.box.y0) < 2 * GR_PIXITS_PER_RISCOS)
                p_chart_header->selection.box.y1 = p_chart_header->selection.box.y0  + 2 * GR_PIXITS_PER_RISCOS;

            { /* blat in a transparent background corresponding to the diagram */
            PC_DRAW_FILE_HEADER pDrawFileHdr;
            if(NULL != (pDrawFileHdr = gr_riscdiag_diagram_lock(cp->core.p_gr_diag->p_gr_riscdiag)))
            {
                DRAW_DIAG_OFFSET dummy_sys_off;
                DRAW_POINT draw_point;
                DRAW_SIZE draw_size;
                draw_point.x = pDrawFileHdr->bbox.x0;
                draw_point.y = pDrawFileHdr->bbox.y0;
                draw_size.cx = pDrawFileHdr->bbox.x1 - pDrawFileHdr->bbox.x0;
                draw_size.cy = pDrawFileHdr->bbox.y1 - pDrawFileHdr->bbox.y0;
                status = gr_riscdiag_rectangle_new(p_chart_header->selection.p_gr_riscdiag, &dummy_sys_off, &draw_point, &draw_size, NULL, NULL);
                gr_riscdiag_diagram_unlock(cp->core.p_gr_diag->p_gr_riscdiag);
            }
            } /*block*/

            if(status_ok(status))
            {
                DRAW_BOX draw_box;
                DRAW_POINT draw_point;
                DRAW_SIZE draw_size;
                draw_box_from_gr_box(&draw_box, &p_chart_header->selection.box);
                draw_point.x = draw_box.x0;
                draw_point.y = draw_box.y0;
                draw_size.cx = draw_box.x1 - draw_box.x0;
                draw_size.cy = draw_box.y1 - draw_box.y0;
                status = gr_riscdiag_rectangle_new(p_chart_header->selection.p_gr_riscdiag, &selectionPathStart, &draw_point, &draw_size, &linestyle, NULL);
            }

            if(status_ok(status))
                gr_riscdiag_diagram_end(p_chart_header->selection.p_gr_riscdiag);
            else
                gr_riscdiag_diagram_dispose(&p_chart_header->selection.p_gr_riscdiag);
        }

        status_assertc(status);
        /*if(status_fail(status))
            gr_chart_edit_winge(status);*/
    }
}

extern void
gr_chart_object_name_from_id(
    _InRef_opt_ PC_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer)
{
    P_USTR out = ustr_buf;
    PC_USTR format;

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
        resource_lookup_ustr_buffer(out, elemof_buffer, CHART_MSG_OBJNAME_CHART);
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        {
        STATUS resource_id;

        if(id.no == 1)
            resource_id = CHART_MSG_OBJNAME_PLOTAREA_1;
        else if(id.no == 2)
            resource_id = CHART_MSG_OBJNAME_PLOTAREA_2;
        else
            resource_id = CHART_MSG_OBJNAME_PLOTAREA_3;

        resource_lookup_ustr_buffer(out, elemof_buffer, resource_id);

        break;
        }

    case GR_CHART_OBJNAME_LEGEND:
        resource_lookup_ustr_buffer(out, elemof_buffer, CHART_MSG_OBJNAME_LEGEND);
        break;

    case GR_CHART_OBJNAME_TEXT:
        format = resource_lookup_ustr(CHART_MSG_OBJNAME_TEXT);
        consume_int(ustr_xsnprintf(out, elemof_buffer, format, (U16) id.no /*.text*/));
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSERIES:
        format = resource_lookup_ustr((GR_CHART_OBJNAME_SERIES == id.name) ? CHART_MSG_OBJNAME_SERIES : CHART_MSG_OBJNAME_DROPSERIES);
        consume_int(ustr_xsnprintf(out, elemof_buffer, format, (U16) id.no /*.series*/));
        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
        format = resource_lookup_ustr((GR_CHART_OBJNAME_POINT == id.name) ? CHART_MSG_OBJNAME_POINT : CHART_MSG_OBJNAME_DROPPOINT);
        consume_int(ustr_xsnprintf(out, elemof_buffer, format, (U16) id.no /*.series*/, (U16) id.subno));
        break;

    case GR_CHART_OBJNAME_AXIS:
        if(NULL == cp)
        {
            format = resource_lookup_ustr(CHART_MSG_OBJNAME_AXIS);
            consume_int(ustr_xsnprintf(out, elemof_buffer, format, (U16) id.no /*.axis*/));
        }
        else
        {
            GR_AXES_IDX axes_idx;
            GR_AXIS_IDX axis_idx = gr_axes_idx_from_external(cp, id.no, &axes_idx);

            if(cp->axes[axes_idx].chart_type == GR_CHART_TYPE_SCAT)
            {
                if(axis_idx == X_AXIS_IDX)
                    resource_lookup_ustr_buffer(out, elemof_buffer, CHART_MSG_OBJNAME_AXIS_X);
                else
                {
                    resource_lookup_ustr_buffer(out, elemof_buffer, CHART_MSG_OBJNAME_AXIS_Y);
                    assert(axis_idx == Y_AXIS_IDX);
                }
            }
            else
            {
                if(axis_idx == X_AXIS_IDX)
                    resource_lookup_ustr_buffer(out, elemof_buffer, CHART_MSG_OBJNAME_AXIS_X_CAT);
                else if(axis_idx == Y_AXIS_IDX)
                    resource_lookup_ustr_buffer(out, elemof_buffer, CHART_MSG_OBJNAME_AXIS_Y_VAL);
                else
                {
                    resource_lookup_ustr_buffer(out, elemof_buffer, CHART_MSG_OBJNAME_AXIS_Z_SER);
                    assert(axis_idx == Z_AXIS_IDX);
                }
            }

            if(0 != cp->axes_idx_max)
                ustr_xstrkat(out, elemof_buffer, (axes_idx == 0) ? USTR_TEXT(" 1") : USTR_TEXT(" 2"));
        }
        break;

    case GR_CHART_OBJNAME_AXISGRID:
        format = resource_lookup_ustr(CHART_MSG_OBJNAME_AXISGRID);
        ustr_IncBytes_wr(out, ustr_xsnprintf(out, elemof_buffer, format, (U16) id.no /*.axis*/));
        if(id.subno > 1)
            consume_int(ustr_xsnprintf(out, elemof_buffer - PtrDiffBytesU32(out, ustr_buf), USTR_TEXT("." U16_FMT), (U16) id.subno));
        break;

    case GR_CHART_OBJNAME_AXISTICK:
        format = resource_lookup_ustr(CHART_MSG_OBJNAME_AXISTICK);
        ustr_IncBytes_wr(out, ustr_xsnprintf(out, elemof_buffer, format, (U16) id.no /*.axis*/));
        if(id.subno > 1)
            consume_int(ustr_xsnprintf(out, elemof_buffer - PtrDiffBytesU32(out, ustr_buf), USTR_TEXT("." U16_FMT), (U16) id.subno));
        break;

    case GR_CHART_OBJNAME_BESTFITSER:
        format = resource_lookup_ustr(CHART_MSG_OBJNAME_BESTFITSER);
        consume_int(ustr_xsnprintf(out, elemof_buffer, format, (U16) id.no /*.series*/));
        break;

    default:
        consume_int(
            ustr_xsnprintf(out, elemof_buffer,
                           id.has_subno
                               ? USTR_TEXT("O" U32_FMT "(" U32_FMT "." U32_FMT ")")
                               : id.has_no
                               ? USTR_TEXT("O" U32_FMT "(" U32_FMT ")")
                               : USTR_TEXT("O" U32_FMT),
                           (U32) id.name, (U32) id.no, (U32) id.subno));
        break;
    }
}

T5_MSG_PROTO(static, chart_edit_msg_note_object_click, _InoutRef_ P_NOTE_OBJECT_CLICK p_note_object_edit_click)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_object_edit_click->object_data_ref;
    P_GR_DIAG p_gr_diag;
    P_GR_CHART cp;
    BOOL tighter_selection;
    GR_CHART_OBJID id;
    GR_DIAG_OFFSET hitObject[64];
    U32 hitObjectDepth = elemof32(hitObject);
    P_BYTE pObject;
    GR_POINT selpoint;
    GR_SIZE selsize;
    U32 hitIndex;
    GR_DIAG_OFFSET endObject;
    GR_POINT origin_offset = { 0, 0 };
    T5_MESSAGE t5_message_effective = p_note_object_edit_click->t5_message;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    CODE_ANALYSIS_ONLY(zero_struct(hitObject));

    switch(t5_message_effective)
    {
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
        t5_message_effective = right_message_if_ctrl(t5_message_effective, T5_EVENT_CLICK_RIGHT_SINGLE, p_note_object_edit_click->p_skelevent_click);
        break;

    case T5_EVENT_FILEINSERT_DOINSERT:
    case T5_EVENT_CLICK_LEFT_DRAG:
        break;

    default:
        return(STATUS_OK);
    }

    p_note_object_edit_click->processed = 1;

    tighter_selection = (T5_EVENT_CLICK_RIGHT_SINGLE == t5_message_effective);

    if( !p_chart_header ||
        !gr_chart_diagram(p_chart_header->ch, &p_gr_diag) ||
        !p_gr_diag->p_gr_riscdiag ||
        !p_gr_diag->p_gr_riscdiag->draw_diag.length )
    {
        host_bleep();
        return(STATUS_OK);
    }

    cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

    {
    P_DRAW_FILE_HEADER pDiagHdr = (P_DRAW_FILE_HEADER) gr_riscdiag_diagram_lock(p_gr_diag->p_gr_riscdiag);
    if(NULL != pDiagHdr)
    {
        origin_offset.x = - pDiagHdr->bbox.x0;
        origin_offset.y = + pDiagHdr->bbox.y1;

        origin_offset.x *= PIXITS_PER_RISCOS;
        origin_offset.y *= PIXITS_PER_RISCOS;

        origin_offset.x /= GR_RISCDRAW_PER_RISCOS;
        origin_offset.y /= GR_RISCDRAW_PER_RISCOS;

        gr_riscdiag_diagram_unlock(p_gr_diag->p_gr_riscdiag);
    }
    } /*block*/

    if(tighter_selection)
    {
        /* tighter click box if ADJUST clicked (but remember some objects may be truly subpixel ...) */
        selsize.cx = 2 * MAX(p_note_object_edit_click->p_skelevent_click->click_context.one_real_pixel.x, p_note_object_edit_click->p_skelevent_click->click_context.one_real_pixel.y);
        selsize.cy = PIXITS_PER_PIXEL;
    }
    else
    {
        /* larger (but still small) click box if SELECT clicked */
        selsize.cx = 4 * MAX(p_note_object_edit_click->p_skelevent_click->click_context.one_real_pixel.x, p_note_object_edit_click->p_skelevent_click->click_context.one_real_pixel.y);
        selsize.cy = selsize.cx;
    }

    selpoint.x = p_note_object_edit_click->pixit_point.x;
    selpoint.y = p_note_object_edit_click->pixit_point.y;

    /* invert coordinate space again */
    selpoint.x = selpoint.x - origin_offset.x;
    selpoint.y = origin_offset.y - selpoint.y;

    /* scanning backwards from the front of the diagram */
    endObject = GR_DIAG_OBJECT_LAST;

    id = gr_chart_objid_anon;

    for(;;)
    {
        status_consume(gr_diag_object_correlate_between(cp->core.p_gr_diag, &selpoint, &selsize, hitObject, hitObjectDepth - 1, GR_DIAG_OBJECT_FIRST, endObject));

        if(hitObject[0] == GR_DIAG_OBJECT_NONE)
        {
            hitIndex = 0;
            break;
        }

        /* find deepest named hit */
        for(hitIndex = 1; hitIndex < hitObjectDepth; ++hitIndex)
        {
            if(hitObject[hitIndex] == GR_DIAG_OBJECT_NONE)
            {
                --hitIndex;
                break;
            }
        }

        pObject = array_ptr(&cp->core.p_gr_diag->handle, BYTE, hitObject[hitIndex]);

        /* read back the id field of this object */
        * (P_U32) &id = * (P_U32) (pObject + offsetof32(GR_DIAG_OBJHDR, objid));

        /* if hit anonymous object at end then loop up to but not including this object */
        if(id.name == GR_CHART_OBJNAME_ANON)
        {
            endObject = hitObject[hitIndex];
            continue;
        }

        break;
    }

    consume_bool(gr_chart_objid_find_parent(&id, tighter_selection));

    if(hitObject[0] == GR_DIAG_OBJECT_NONE)
    {
        /* clear selection only on left click and if there was one */
        if((T5_EVENT_CLICK_LEFT_SINGLE == t5_message_effective) && (p_chart_header->selection.id.name != GR_CHART_OBJNAME_ANON))
            chart_selection_clear(p_chart_header, p_note_object_edit_click->p_note_info);
    }
    else
    {
        /* change selection unless file insert or drag start or if there wasn't one */
        BOOL change_selection = FALSE;

        switch(t5_message_effective)
        {
        case T5_EVENT_FILEINSERT_DOINSERT:
        case T5_EVENT_CLICK_LEFT_DRAG:
            if(GR_CHART_OBJNAME_ANON == p_chart_header->selection.id.name)
                change_selection = TRUE;
            break;

        default:
            change_selection = TRUE;
            break;
        }

        if(change_selection)
            (void) chart_selection_make(p_chart_header, &id, p_note_object_edit_click->p_note_info);
    }

    switch(t5_message_effective)
    {
    case T5_EVENT_FILEINSERT_DOINSERT:
        {
        PCTSTR filename = p_note_object_edit_click->p_skelevent_click->data.fileinsert.filename;
        T5_FILETYPE t5_filetype = p_note_object_edit_click->p_skelevent_click->data.fileinsert.t5_filetype;
        const BOOL file_is_not_safe = !p_note_object_edit_click->p_skelevent_click->data.fileinsert.safesource;
        const BOOL ctrl_pressed = p_note_object_edit_click->p_skelevent_click->click_context.ctrl_pressed;
        BOOL embed_file = global_preferences.embed_inserted_files;

        if(file_is_not_safe)
        {   /* if source is a scrapfile, we must embed, regardless of the checkbox/Ctrl key states */
            embed_file = TRUE;
        }
        else
        {   /* SKS 10/13oct99 take note of Ctrl key state too - inverts the preference */
            if(ctrl_pressed) embed_file = !embed_file;
        }

        if(GR_CHART_OBJNAME_ANON == p_chart_header->selection.id.name)
        {
            host_bleep();
            return(STATUS_OK);
        }

        if(!image_cache_can_import_with_image_convert(p_note_object_edit_click->p_skelevent_click->data.fileinsert.t5_filetype))
            return(create_error(CHART_ERR_FILETYPE_BAD));

        status = chart_load_image_file_to_selection(p_chart_header, filename, t5_filetype, embed_file);

        break;
        }

    case T5_EVENT_CLICK_LEFT_DRAG:
        {
        switch(p_chart_header->selection.id.name)
        {
        case GR_CHART_OBJNAME_LEGEND:
        case GR_CHART_OBJNAME_TEXT:
            /* these are the only ones you can drag */
            p_note_object_edit_click->processed = CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT;
            p_note_object_edit_click->subselection_pixit_rect.tl.x = p_chart_header->selection.box.x0 * PIXITS_PER_RISCOS;
            p_note_object_edit_click->subselection_pixit_rect.tl.y = 256 /*<<<*/ /*p_chart_header->selection.box.y0 * PIXITS_PER_RISCOS*/;
            p_note_object_edit_click->subselection_pixit_rect.br.x = p_note_object_edit_click->subselection_pixit_rect.tl.x + (p_chart_header->selection.box.x1 - p_chart_header->selection.box.x0) * PIXITS_PER_RISCOS;
            p_note_object_edit_click->subselection_pixit_rect.br.y = p_note_object_edit_click->subselection_pixit_rect.tl.y + 1440 /*<<<*/ /*(p_chart_header->selection.box.y1 - p_chart_header->selection.box.y0) * PIXITS_PER_RISCOS*/;
            break;

        case GR_CHART_OBJNAME_ANON:
            host_bleep();

            /*FALLTHRU*/

        default:
            p_note_object_edit_click->processed = FALSE;
            break;
        }

        break;
        }

    default:
        break;
    }

    return(status);
}

T5_MSG_PROTO(static, chart_edit_msg_note_object_size_query, _InoutRef_ P_NOTE_OBJECT_SIZE p_note_object_size)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_object_size->object_data_ref;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_note_object_size->pixit_size.cx = cp->core.layout.width  + CHART_NOTE_SIZE_FUDGE_X;
    p_note_object_size->pixit_size.cy = cp->core.layout.height + CHART_NOTE_SIZE_FUDGE_Y;

    p_note_object_size->processed = 1;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_edit_msg_note_object_size_set_possible, _InoutRef_ P_NOTE_OBJECT_SIZE p_note_object_size)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_note_object_size->processed = 1;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_edit_msg_note_object_size_set, _InoutRef_ P_NOTE_OBJECT_SIZE p_note_object_size)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_object_size->object_data_ref;
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    cp->core.layout.width  = p_note_object_size->pixit_size.cx - CHART_NOTE_SIZE_FUDGE_X;
    cp->core.layout.height = p_note_object_size->pixit_size.cy - CHART_NOTE_SIZE_FUDGE_Y;

    p_note_object_size->processed = 1;

    chart_rebuild_after_modify(p_chart_header);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_edit_msg_save_picture, _InRef_ P_MSG_SAVE_PICTURE p_msg_save_picture)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_msg_save_picture->extra;
    P_GR_DIAG p_gr_diag;
    P_GR_RISCDIAG p_gr_riscdiag;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(p_msg_save_picture->t5_filetype == FILETYPE_DRAW);

    if( gr_chart_diagram(p_chart_header->ch, &p_gr_diag) &&
        (NULL != (p_gr_riscdiag = p_gr_diag->p_gr_riscdiag)) &&
        p_gr_diag->p_gr_riscdiag->draw_diag.length )
    {
        P_DRAW_FILE_HEADER pDrawFileHdr;

        if(NULL != (pDrawFileHdr = gr_riscdiag_diagram_lock(p_gr_riscdiag)))
        {
            STATUS status = binary_write_bytes(&p_msg_save_picture->p_ff_op_format->of_op_format.output, pDrawFileHdr, p_gr_riscdiag->draw_diag.length);
            gr_riscdiag_diagram_unlock(p_gr_riscdiag);
            return(status);
        }
    }

    return(STATUS_FAIL);
}

T5_MSG_PROTO(static, chart_edit_msg_save_picture_filetypes_request, _InoutRef_ P_MSG_SAVE_PICTURE_FILETYPES_REQUEST p_msg_save_picture_filetypes_request)
{
    SAVE_FILETYPE save_filetype;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* Indicate that we can save charts as Drawfiles */
    save_filetype.object_id = OBJECT_ID_CHART;
    save_filetype.t5_filetype = FILETYPE_DRAW;
    save_filetype.suggested_leafname.type = UI_TEXT_TYPE_RESID;
    save_filetype.suggested_leafname.text.resource_id = CHART_MSG_SUGGESTED_LEAFNAME;

    return(al_array_add(&p_msg_save_picture_filetypes_request->h_save_filetype, SAVE_FILETYPE, 1, PC_ARRAY_INIT_BLOCK_NONE, &save_filetype));
}

T5_MSG_PROTO(static, chart_edit_msg_caret_show_claim, P_CARET_SHOW_CLAIM p_caret_show_claim)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_caret_show_claim);

    /* switch off caret */
    view_show_caret(p_docu, UPDATE_PANE_CELLS_AREA, &p_docu->caret, 0);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_edit_msg_focus_changed, _InRef_ P_OBJECT_ID p_object_id)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(OBJECT_ID_CHART == *p_object_id)
    {
#if WINDOWS
        ho_prefer_menu_bar(p_docu, MENU_ROOT_DOCU);
#endif
        status_line_clear(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS(OBJECT_ID_CHART));
    }
    else if(OBJECT_ID_CHART == p_docu->focus_owner)
    {
#if WINDOWS
        ho_prefer_menu_bar(p_docu, MENU_ROOT_CHART_EDIT);
#else
        /*EMPTY*/
#endif
        /* status line info will already be kosher */
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, chart_edit_cmd_toggle_marks)
{
    const P_CHART_HEADER p_chart_header = p_chart_header_from_docu_last(p_docu);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(docu_focus_owner_object_call(p_docu, p_chart_header && (p_chart_header->selection.id.name != GR_CHART_OBJNAME_ANON) ? T5_CMD_SELECTION_CLEAR : T5_CMD_SELECT_DOCUMENT, de_const_cast(P_T5_CMD, p_t5_cmd)));
}

T5_MSG_PROTO(static, chart_edit_msg_mark_info_read, _InoutRef_ P_MARK_INFO p_mark_info)
{
    const P_CHART_HEADER p_chart_header = p_chart_header_from_docu_last(p_docu);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_mark_info->h_markers = p_chart_header && (p_chart_header->selection.id.name != GR_CHART_OBJNAME_ANON);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_edit_msg_selection_clear, _InRef_ P_NOTE_OBJECT_SELECTION_CLEAR p_note_object_selection_clear)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_object_selection_clear->object_data_ref;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    chart_selection_clear(p_chart_header, p_note_object_selection_clear->p_note_info);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_edit_msg_note_object_snapshot, _InoutRef_ P_NOTE_OBJECT_SNAPSHOT p_note_object_snapshot)
{
    /* attempt to donate chart note to draw note */
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_object_snapshot->object_data_ref;
    P_GR_DIAG p_gr_diag;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(!gr_chart_diagram(p_chart_header->ch, &p_gr_diag))
        return(STATUS_FAIL);

    p_note_object_snapshot->p_more_info = p_gr_diag;

    /* when we return, the chart should be no more */
    return(object_call_id_load(p_docu, T5_MSG_DRAWING_DONATE, p_note_object_snapshot, OBJECT_ID_DRAW));
}

T5_MSG_PROTO(static, chart_edit_cmd_force_recalc, _InRef_ P_CHART_HEADER p_chart_header)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    chart_rebuild_after_modify(p_chart_header);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_edit_msg_note_delete, _In_ P_CHART_HEADER p_chart_header)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    chart_dispose(&p_chart_header);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_chart_gallery)
{
    S32 arg_s32 = -1;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        arg_s32 = p_args[0].val.s32;
    }

    switch(arg_s32)
    {
    default:
    case 0: /* bar chart */
        return(gr_chart_bl_gallery(p_docu, GR_CHART_TYPE_BAR));

    case 1: /* line chart */
        return(gr_chart_bl_gallery(p_docu, GR_CHART_TYPE_LINE));

    case 2: /* pie chart */
        return(gr_chart_bl_gallery(p_docu, GR_CHART_TYPE_PIE));

    case 3: /* X-Y (scatter) chart */
        return(gr_chart_bl_gallery(p_docu, GR_CHART_TYPE_SCAT));

    case 4:
        return(gr_chart_bl_gallery(p_docu, GR_CHART_TYPE_OVER_BL));

    case 5: /* add to existing chart */
        {
        const P_CHART_HEADER p_chart_header = p_chart_header_from_docu_last(p_docu);

        if(NULL == p_chart_header)
        {
            host_bleep();
            return(STATUS_OK);
        }

        return(gr_chart_add_more(p_docu, p_chart_header));
        }
    }
}

/* manipulate selected whatever for 'Edit' */

T5_CMD_PROTO(static, t5_cmd_chart_edit)
{
    const P_CHART_HEADER p_chart_header = p_chart_header_from_docu_last(p_docu);
    GR_CHART_OBJID id;
    S32 arg_s32 = -1;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_chart_header)
    {
        host_bleep();
        return(STATUS_OK);
    }

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        arg_s32 = p_args[0].val.s32;
    }

    id = p_chart_header->selection.id;

    if(arg_s32 == -1)
    {
        switch(id.name)
        {
        case GR_CHART_OBJNAME_AXIS:
        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_AXISTICK:
            arg_s32 = 0;
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_DROPSERIES:
        case GR_CHART_OBJNAME_BESTFITSER:
        case GR_CHART_OBJNAME_POINT:
        case GR_CHART_OBJNAME_DROPPOINT:
            arg_s32 = 1;
            break;

        default:
#if CHECKING
        case GR_CHART_OBJNAME_LEGDSERIES:
        case GR_CHART_OBJNAME_LEGDPOINT:
            assert0();

            /*FALLTHRU*/

        case GR_CHART_OBJNAME_ANON:
        case GR_CHART_OBJNAME_CHART:
        case GR_CHART_OBJNAME_PLOTAREA:
        case GR_CHART_OBJNAME_LEGEND:
        case GR_CHART_OBJNAME_TEXT:
#endif
            arg_s32 = 2;
            break;
        }
    }

    switch(arg_s32)
    {
    case 0:
        return(gr_chart_axis_process(p_chart_header, id));

    case 1:
        return(gr_chart_series_process(p_chart_header, id));

    default: default_unhandled();
    case 2:
        return(gr_chart_process(p_chart_header, id));
    }
}

/* manipulate selected whatever for 'Style' */

T5_CMD_PROTO(static, t5_cmd_chart_style)
{
    const P_CHART_HEADER p_chart_header = p_chart_header_from_docu_last(p_docu);
    GR_CHART_OBJID id;
    S32 arg_s32 = -1;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_chart_header)
    {
        host_bleep();
        return(STATUS_OK);
    }

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        arg_s32 = p_args[0].val.s32;
    }

    id = p_chart_header->selection.id;

    if(arg_s32 == -1)
    {
        switch(id.name)
        {
        default:
#if CHECKING
        case GR_CHART_OBJNAME_LEGDSERIES:
        case GR_CHART_OBJNAME_LEGDPOINT:
            assert0();

            /*FALLTHRU*/

        case GR_CHART_OBJNAME_ANON:
        case GR_CHART_OBJNAME_LEGEND:
        case GR_CHART_OBJNAME_TEXT:
        case GR_CHART_OBJNAME_AXISTICK:
#endif
            arg_s32 = 0;
            break;

        case GR_CHART_OBJNAME_DROPPOINT:
        case GR_CHART_OBJNAME_AXIS:
        case GR_CHART_OBJNAME_AXISGRID:
        case GR_CHART_OBJNAME_BESTFITSER:
            arg_s32 = 1;
            break;

        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_DROPSERIES:
        case GR_CHART_OBJNAME_CHART: /* SKS for 1.30 moved from above */
        case GR_CHART_OBJNAME_PLOTAREA:
        case GR_CHART_OBJNAME_POINT:
            arg_s32 = 2;
            break;
        }
    }

    switch(arg_s32)
    {
    case 0:
        return(gr_chart_style_text_process(p_chart_header, id));

    case 1:
        return(gr_chart_style_line_process(p_chart_header, id));

    default: default_unhandled();
    case 2:
        return(gr_chart_style_fill_process(p_chart_header, id));
    }
}

/* manipulate selected whatever */

T5_CMD_PROTO(static, t5_cmd_chart_editx)
{
    const P_CHART_HEADER p_chart_header = p_chart_header_from_docu_last(p_docu);
    S32 arg_s32 = -1;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_chart_header)
    {
        host_bleep();
        return(STATUS_OK);
    }

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        arg_s32 = p_args[0].val.s32;
    }

    switch(arg_s32)
    {
    case 0:
        return(gr_chart_legend_process(p_chart_header));

    default: default_unhandled();
    case 1:
        return(gr_chart_margins_process(p_chart_header));
    }
}

T5_MSG_PROTO(static, chart_edit_msg_chart_edit_insert_picture, _InRef_ P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    const P_CHART_HEADER p_chart_header = p_chart_header_from_docu_last(p_docu);
    PCTSTR filename = p_msg_insert_foreign->filename;
    T5_FILETYPE t5_filetype = p_msg_insert_foreign->t5_filetype;
  /*const BOOL file_is_not_safe = FALSE;*/
    const BOOL ctrl_pressed = p_msg_insert_foreign->ctrl_pressed;
    BOOL embed_file = global_preferences.embed_inserted_files;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* SKS 10/13oct99 take note of Ctrl key state too - inverts the preference */
    if(ctrl_pressed) embed_file = !embed_file;

    if(NULL == p_chart_header)
    {
        host_bleep();
        return(STATUS_OK);
    }

    if(p_chart_header->selection.id.name == GR_CHART_OBJNAME_ANON)
    {
        host_bleep();
        return(STATUS_OK);
    }

    if(!image_cache_can_import_with_image_convert(p_msg_insert_foreign->t5_filetype))
        return(create_error(CHART_ERR_FILETYPE_BAD));

    return(chart_load_image_file_to_selection(p_chart_header, filename, t5_filetype, embed_file));
}

T5_MSG_PROTO(static, chart_edit_msg_note_object_edit_start, _InoutRef_ P_NOTE_OBJECT_EDIT_START p_note_object_edit_start)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_object_edit_start->object_data_ref;
    P_GR_DIAG p_gr_diag;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if((NULL == p_chart_header) || !gr_chart_diagram(p_chart_header->ch, &p_gr_diag))
    {
        host_bleep();
        return(STATUS_OK);
    }

    p_docu->focus_owner_old = p_docu->focus_owner;

    caret_show_claim(p_docu, p_docu->focus_owner, FALSE);

    p_docu->last_chart_edited = p_chart_header->chartdatakey;

    chart_selection_clear(p_chart_header, (P_NOTE_INFO) p_note_object_edit_start->p_note_info);

    p_note_object_edit_start->processed = 1;

    return(STATUS_OK);
}

static void
chart_edit_event_redraw_show_content(
    _InRef_     P_CHART_HEADER p_chart_header,
    _InRef_     P_NOTE_OBJECT_REDRAW p_note_object_redraw)
{
    const P_OBJECT_REDRAW p_object_redraw = &p_note_object_redraw->object_redraw;
    P_GR_DIAG p_gr_diag;

    if(gr_chart_diagram(p_chart_header->ch, &p_gr_diag))
    {
        const P_GR_RISCDIAG p_gr_riscdiag = p_gr_diag->p_gr_riscdiag;

        if((NULL != p_gr_riscdiag) && p_gr_riscdiag->draw_diag.length)
        {
            host_paint_drawfile(&p_object_redraw->redraw_context,
                                &p_object_redraw->pixit_rect_object.tl,
                                &p_note_object_redraw->gr_scale_pair,
                                &p_gr_diag->p_gr_riscdiag->draw_diag, 0);
        }
    }
}

static void
chart_edit_event_redraw_show_selection(
    _InRef_     P_CHART_HEADER p_chart_header,
    _InRef_     P_NOTE_OBJECT_REDRAW p_note_object_redraw)
{
    const P_OBJECT_REDRAW p_object_redraw = &p_note_object_redraw->object_redraw;
    const P_GR_RISCDIAG p_gr_riscdiag = p_chart_header->selection.p_gr_riscdiag;

    if((NULL != p_gr_riscdiag) && p_gr_riscdiag->draw_diag.length)
    {
        const BOOL eor_paths = RISCOS; /* not GDI+ on Windows */

        host_paint_drawfile(&p_object_redraw->redraw_context,
                            &p_object_redraw->pixit_rect_object.tl,
                            &p_note_object_redraw->gr_scale_pair,
                            &p_gr_riscdiag->draw_diag, eor_paths);
    }
}

T5_MSG_PROTO(static, chart_edit_event_redraw, _InRef_ P_NOTE_OBJECT_REDRAW p_note_object_redraw)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_object_redraw->object_redraw.object_data.u.p_object; /* (puke) */

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_chart_header)
        return(create_error(ERR_NOTE_NOT_LOADED));

    if(p_note_object_redraw->object_redraw.flags.show_content)
        chart_edit_event_redraw_show_content(p_chart_header, p_note_object_redraw);

    if(p_note_object_redraw->object_redraw.flags.show_selection)
        chart_edit_event_redraw_show_selection(p_chart_header, p_note_object_redraw);

    return(STATUS_OK);
}

OBJECT_PROTO(extern, object_chart_edit_sideways)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(chart_edit_event_redraw(p_docu, t5_message, (P_NOTE_OBJECT_REDRAW) p_data));

    case T5_MSG_CHART_EDIT_INSERT_PICTURE:
        return(chart_edit_msg_chart_edit_insert_picture(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    case T5_MSG_NOTE_OBJECT_EDIT_START:
        return(chart_edit_msg_note_object_edit_start(p_docu, t5_message, (P_NOTE_OBJECT_EDIT_START) p_data));

    case T5_MSG_NOTE_OBJECT_CLICK:
        return(chart_edit_msg_note_object_click(p_docu, t5_message, (P_NOTE_OBJECT_CLICK) p_data));

    case T5_MSG_NOTE_OBJECT_SIZE_QUERY:
        return(chart_edit_msg_note_object_size_query(p_docu, t5_message, (P_NOTE_OBJECT_SIZE) p_data));

    case T5_MSG_NOTE_OBJECT_SIZE_SET_POSSIBLE:
        return(chart_edit_msg_note_object_size_set_possible(p_docu, t5_message, (P_NOTE_OBJECT_SIZE) p_data));

    case T5_MSG_NOTE_OBJECT_SIZE_SET:
        return(chart_edit_msg_note_object_size_set(p_docu, t5_message, (P_NOTE_OBJECT_SIZE) p_data));

    case T5_MSG_SAVE_PICTURE:
        return(chart_edit_msg_save_picture(p_docu, t5_message, (P_MSG_SAVE_PICTURE) p_data));

    case T5_MSG_SAVE_PICTURE_FILETYPES_REQUEST:
        return(chart_edit_msg_save_picture_filetypes_request(p_docu, t5_message, (P_MSG_SAVE_PICTURE_FILETYPES_REQUEST) p_data));

    /*********************************************************************************************/

    case T5_MSG_CARET_SHOW_CLAIM:
        return(chart_edit_msg_caret_show_claim(p_docu, t5_message, (P_CARET_SHOW_CLAIM) p_data));

    case T5_MSG_FOCUS_CHANGED:
        return(chart_edit_msg_focus_changed(p_docu, t5_message, (P_OBJECT_ID) p_data));

    case T5_MSG_MARK_INFO_READ:
        return(chart_edit_msg_mark_info_read(p_docu, t5_message, (P_MARK_INFO) p_data));

    case T5_MSG_SELECTION_CLEAR:
        return(chart_edit_msg_selection_clear(p_docu, t5_message, (P_NOTE_OBJECT_SELECTION_CLEAR) p_data));

    case T5_MSG_NOTE_DELETE:
        return(chart_edit_msg_note_delete(p_docu, t5_message, (P_CHART_HEADER) p_data));

    case T5_MSG_NOTE_OBJECT_SNAPSHOT:
        return(chart_edit_msg_note_object_snapshot(p_docu, t5_message, (P_NOTE_OBJECT_SNAPSHOT) p_data));

    /*********************************************************************************************/

    case T5_CMD_CHART_GALLERY:
        return(t5_cmd_chart_gallery(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_EDIT:
        return(t5_cmd_chart_edit(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_STYLE:
        return(t5_cmd_chart_style(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_EDITX:
        return(t5_cmd_chart_editx(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_TOGGLE_MARKS:
        return(chart_edit_cmd_toggle_marks(p_docu, t5_message, (P_T5_CMD) p_data));

    case T5_CMD_FORCE_RECALC:
        return(chart_edit_cmd_force_recalc(p_docu, t5_message, (P_CHART_HEADER) p_data));

    case T5_CMD_DELETE_CHARACTER_LEFT:
    case T5_CMD_DELETE_CHARACTER_RIGHT:
        /* try to delete whatever we have selected; probably just series,legend */

    case T5_CMD_SELECTION_COPY:
    case T5_CMD_SELECTION_CUT:
    case T5_CMD_SELECTION_DELETE:
    case T5_CMD_PASTE_AT_CURSOR:

    default:
        return(STATUS_OK);
    }
}

/* end of gr_edit.c */
