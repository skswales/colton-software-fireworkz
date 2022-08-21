/* sk_draft.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1995-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Draft printing for Fireworkz */

/* SKS April 1995 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

_Check_return_
static STATUS
plain_text_is_page_break(
    _InVal_     ARRAY_HANDLE h_chars)
{
    PC_UCHARS uchars;
    if(array_elements(&h_chars) < INLINE_OVH)
        return(FALSE);
    uchars = (PC_UCHARS) array_rangec(&h_chars, UCHARB, 0, INLINE_OVH);
    return(is_inline(uchars) && (inline_code(uchars) == IL_PAGE_BREAK));
}

_Check_return_
static STATUS
plain_text_output_escape_to_file(
    _InoutRef_  FILE_HANDLE file_out,
    _InVal_     U8 ch)
{
    status_return(file_putc(0x1B, file_out));
    return(file_putc(ch, file_out));
}

/******************************************************************************
*
* output chars to file
*
******************************************************************************/

_Check_return_
static STATUS /* error; chars output */
plain_text_output_chars_to_file(
    _InoutRef_  FILE_HANDLE file_out,
    _InVal_     ARRAY_HANDLE h_chars)
{
    STATUS status = STATUS_OK;
    S32 count = 0;
    U8 effects[PLAIN_EFFECT_COUNT];

    zero_array(effects);

    if(array_elements(&h_chars) != 0)
    {
        PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&h_chars);
        U32 offset = 0;

        while(status_ok(status))
        {
            if(is_inline_off(ustr_inline, offset))
            {   /* extract U8 argument */
                U8 arg = *(inline_data_ptr_off(PC_U8, ustr_inline, offset));
                int escape_arg;

                switch(inline_code_off(ustr_inline, offset))
                {
                default:
                    break;

                case IL_STYLE_FS_BOLD:
                    effects[PLAIN_EFFECT_BOLD] = arg;
                    break;

                case IL_STYLE_FS_UNDERLINE:
                    effects[PLAIN_EFFECT_UNDERLINE] = arg;
                    break;

                case IL_STYLE_FS_ITALIC:
                    effects[PLAIN_EFFECT_ITALIC] = arg;
                    break;

                case IL_STYLE_FS_SUPERSCRIPT:
                    effects[PLAIN_EFFECT_SUPER] = arg;
                    break;

                case IL_STYLE_FS_SUBSCRIPT:
                    effects[PLAIN_EFFECT_SUB] = arg;
                    break;
                }

                escape_arg = '\x80';
                escape_arg |= (effects[PLAIN_EFFECT_SUB] << 5);
                escape_arg |= (effects[PLAIN_EFFECT_SUPER] << 4);
                escape_arg |= (effects[PLAIN_EFFECT_UNDERLINE] << 3);
                escape_arg |= (effects[PLAIN_EFFECT_ITALIC] << 2);
                escape_arg |= (effects[PLAIN_EFFECT_BOLD]);

                status = plain_text_output_escape_to_file(file_out, (U8) escape_arg);

                offset += inline_bytecount_off(ustr_inline, offset);
            }
            else
            {
                U8 u8 = PtrGetByteOff(ustr_inline, offset);

                if(CH_NULL == u8)
                    break;

                status = file_putc(u8, file_out);

                count += 1;

                ++offset;
            }
        }
    }

    return(status_ok(status) ? count : status);
}

_Check_return_
static STATUS
plain_text_output_spaces_to_file(
    _InoutRef_  FILE_HANDLE file_out,
    _InVal_     S32 count)
{
    S32 i;
    S32 output_count = i = MAX(0, count);

    assert(count >= 0);

    while(i--)
        status_return(file_putc(CH_SPACE, file_out));

    return(output_count);
}

_Check_return_
static STATUS
plain_text_output_returns_to_file(
    _InoutRef_  FILE_HANDLE file_out,
    _InVal_     S32 count)
{
    S32 i;
    S32 output_count = i = MAX(0, count);

    assert(count >= 0);

    while(i--)
        status_return(file_putc(CR, file_out));

    return(output_count);
}

_Check_return_
static STATUS /* no of returns output */
plain_text_hefo_output(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  FILE_HANDLE file_out,
    _InRef_     PC_PAGE_NUM p_page_num,
    _InVal_     OBJECT_ID focus,
    _InVal_     S32 offset_lines,
    _InVal_     S32 margin_lines,
    _InVal_     S32 margin_left)
{
    STATUS status = STATUS_OK, count = 0;

    if(margin_lines)
    {
        OBJECT_READ_TEXT_DRAFT object_read_text_draft;

        object_data_init(&object_read_text_draft.object_data);
        object_read_text_draft.h_plain_text = 0;
        object_read_text_draft.skel_rect_object.tl.page_num = *p_page_num;
        object_read_text_draft.skel_rect_object.tl.pixit_point.x =
        object_read_text_draft.skel_rect_object.tl.pixit_point.y = 0;

        status = object_call_id(focus, p_docu, T5_MSG_OBJECT_READ_TEXT_DRAFT, &object_read_text_draft);

        while(status_ok(status))
        {
            S32 line_ix, hefo_lines = margin_lines - offset_lines;

            status_break(status = plain_text_output_returns_to_file(file_out, offset_lines));
            count += status;
            for(line_ix = 0; line_ix < hefo_lines; line_ix += 1)
            {
                /* is there an entry for this object ? */
                if(object_read_text_draft.h_plain_text && line_ix < array_elements(&object_read_text_draft.h_plain_text))
                {
                    status_break(status = plain_text_output_spaces_to_file(file_out, margin_left));
                    status_break(status = plain_text_output_chars_to_file(file_out, *array_ptr(&object_read_text_draft.h_plain_text, ARRAY_HANDLE, line_ix)));
                }

                status_break(status = plain_text_output_returns_to_file(file_out, 1));
                count += 1;
            }

            break;
            /*NOTREACHED*/
        }

        plain_text_dispose(&object_read_text_draft.h_plain_text);
    }

    return(status_ok(status) ? count : status);
}

/******************************************************************************
*
* output a given page of text to a file
*
******************************************************************************/

_Check_return_
extern STATUS
plain_text_page_to_file(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  FILE_HANDLE file_out,
    _InRef_     PC_PAGE_NUM p_page_num)
{
    STATUS status = STATUS_OK;
    ARRAY_HANDLE h_col_info = 0, h_col_array = 0;
    ROW row = 0; /* Keep dataflower happy */
    STYLE style;
    PIXIT pixit_per_char = 0, pixit_per_line = 0, pixit_y = 0;
    HEADFOOT_BOTH headfoot_both;

    /* read base style to get character sizes */
    style_init(&style);
    style_copy_defaults(p_docu, &style, &style_selector_all);

    if(status_ok(status = fonty_handle_from_font_spec(&style.font_spec, p_docu->flags.draft_mode)))
    {
        const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;
        const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);

        pixit_per_char = p_font_context->space_width;

        pixit_per_line = style_leading_from_style(&style, &style.font_spec, p_docu->flags.draft_mode);

        row = row_from_page_y(p_docu, p_page_num->y);
    }

    style_dispose(&style);

    for(;;) /* loop for structure */
    {
        status_break(status);

        headfoot_both_from_page_y(p_docu, &headfoot_both, p_page_num->y);
        status = plain_text_hefo_output(p_docu,
                                        file_out,
                                        p_page_num,
                                        OBJECT_ID_HEADER,
                                        lines_from_pixit(headfoot_both.header.offset, pixit_per_line),
                                        lines_from_pixit(headfoot_both.header.margin, pixit_per_line),
                                        chars_from_pixit(margin_left_from(&p_docu->page_def, p_page_num->y), pixit_per_char));
        if(status_ok(status))
            pixit_y += pixit_per_line * (S32) status;

        /* loop for each row */
        while(status_ok(status))
        {
            COL col_start, col_end;
            SLR slr;
            PC_COL_INFO p_col_info;
            P_ARRAY_HANDLE p_h_plain_text;
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_h_plain_text), TRUE);
            S32 n_lines_this_row = 0;
            ROW_ENTRY row_entry;

            if(row >= n_rows(p_docu))
                break; /* SKS 23nov94 */

            row_entry_from_row(p_docu, &row_entry, row);

            if(row_entry.rowtab.edge_top.page > p_page_num->y)
                break;

            /* check for a page break */
            if(row_entry.rowtab.edge_top.pixit && page_break_row(p_docu, row))
                break;

            /* get details for all columns on this page for this row */
            status_assert(status = skel_col_enum(p_docu, row, p_page_num->x, (COL) -1, &h_col_info));
            status_break(status);

            assert(0 != array_elements32(&h_col_info));
            col_start = array_basec(&h_col_info, COL_INFO)->col;
            col_end = col_start + (COL) array_elements(&h_col_info);

            if(NULL == (p_h_plain_text = al_array_alloc_ARRAY_HANDLE(&h_col_array, col_end - col_start, &array_init_block, &status)))
                break;

            /* output returns to get up to the place */
            if(row_entry.rowtab.edge_top.page == p_page_num->y)
            {
                status_break(status = plain_text_output_returns_to_file(file_out,
                                                                        lines_from_pixit(headfoot_both.header.margin
                                                                                         + row_entry.rowtab.edge_top.pixit
                                                                                         - pixit_y,
                                                                                         pixit_per_line)));
                pixit_y += pixit_per_line * (S32) status;
            }

            /* get the plain text from the objects */
            slr.row = row;
            for(slr.col = col_start, p_col_info = array_ptrc(&h_col_info, COL_INFO, slr.col - col_start);
                slr.col < col_end;
                ++slr.col, ++p_col_info, ++p_h_plain_text)
            {
                OBJECT_READ_TEXT_DRAFT object_read_text_draft;

                status_consume(object_data_from_slr(p_docu, &object_read_text_draft.object_data, &slr));

                /* set up object rectangle */
                skel_point_update_from_edge(&object_read_text_draft.skel_rect_object.tl, &p_col_info->edge_left, x);
                skel_point_update_from_edge(&object_read_text_draft.skel_rect_object.tl, &row_entry.rowtab.edge_top, y);
                skel_point_update_from_edge(&object_read_text_draft.skel_rect_object.br, &p_col_info->edge_right, x);
                skel_point_update_from_edge(&object_read_text_draft.skel_rect_object.br, &row_entry.rowtab.edge_bot, y);

                object_read_text_draft.h_plain_text = 0;
                status_break(status = object_call_id(object_read_text_draft.object_data.object_id, p_docu, T5_MSG_OBJECT_READ_TEXT_DRAFT, &object_read_text_draft));
                *p_h_plain_text = object_read_text_draft.h_plain_text;
                n_lines_this_row = MAX(n_lines_this_row, array_elements(&object_read_text_draft.h_plain_text));
            }

            if(status_ok(status))
            {
                PAGE page_y_cur = row_entry.rowtab.edge_top.page;
                ARRAY_INDEX line_ix;

                /* loop over lines inside row */
                for(line_ix = 0; line_ix < n_lines_this_row && status_ok(status); line_ix += 1)
                {
                    S32 queued_space = 0, need_return = 0;

                    for(slr.col = col_start, p_col_info = array_ptr(&h_col_info, COL_INFO, slr.col - col_start),
                        p_h_plain_text = array_ptr(&h_col_array, ARRAY_HANDLE, 0);
                        slr.col < col_end && status_ok(status);
                        ++slr.col, ++p_col_info, ++p_h_plain_text)
                    {
                        ARRAY_HANDLE h_chars = 0;

                        if(*p_h_plain_text && line_ix < array_elements(p_h_plain_text))
                            h_chars = *array_ptr(p_h_plain_text, ARRAY_HANDLE, line_ix);

                        if(plain_text_is_page_break(h_chars))
                        {
                            page_y_cur += 1;
                            break;
                        }
                        else if(page_y_cur == p_page_num->y)
                        {
                            S32 n_output = 0;

                            /* is there an entry for this object ? */
                            if(0 != h_chars)
                            {
                                status_break(status = plain_text_output_spaces_to_file(file_out, queued_space));
                                queued_space = 0;
                                status_break(status = plain_text_output_chars_to_file(file_out, h_chars));
                                n_output = (S32) status;
                            }

                            /* space out to next object */
                            queued_space += chars_from_pixit(p_col_info->edge_right.pixit - p_col_info->edge_left.pixit,
                                                             pixit_per_char) - n_output;
                            need_return = 1;
                        }
                    }

                    if(page_y_cur > p_page_num->y)
                        break;

                    if(need_return)
                    {
                        status_break(status = plain_text_output_returns_to_file(file_out, 1));
                        pixit_y += pixit_per_line * (S32) status;
                    }
                }
            }

            { /* dispose of it all ! */
            ARRAY_INDEX i;

            for(i = 0; i < array_elements(&h_col_array); i += 1)
                plain_text_dispose(array_ptr(&h_col_array, ARRAY_HANDLE, i));
            al_array_dispose(&h_col_array);
            } /*block*/

            status_break(status);

            slr.row += 1;
        } /* od */

        status_break(status);

        /* output the footer */

        /* output returns to get up to the place */
        status = plain_text_output_returns_to_file(file_out,
                                                   lines_from_pixit(p_docu->page_def.cells_usable_y
                                                   - headfoot_both.footer.margin
                                                   - pixit_y,
                                                   pixit_per_line));
        status_break(status);
        pixit_y += pixit_per_line * (S32) status;

        status = plain_text_hefo_output(p_docu,
                                        file_out,
                                        p_page_num,
                                        OBJECT_ID_FOOTER,
                                        lines_from_pixit(headfoot_both.footer.offset, pixit_per_line),
                                        lines_from_pixit(headfoot_both.footer.margin, pixit_per_line),
                                        chars_from_pixit(margin_left_from(&p_docu->page_def, p_page_num->y), pixit_per_char));
        status_break(status);
        pixit_y += pixit_per_line * (S32) status;

        /* output returns to get to the end of page */
        status = plain_text_output_returns_to_file(file_out,
                                                   lines_from_pixit(p_docu->page_def.cells_usable_y
                                                                    - pixit_y,
                                                                    pixit_per_line));

        break; /* end of loop for structure */
    }

    return(status);
}

/******************************************************************************
*
* Draft mode printout.
*
* Shovel ASCII directly at a file
*
******************************************************************************/

_Check_return_
static STATUS
print_document_draft_to_file(
    _DocuRef_   P_DOCU p_docu,
    P_PRINT_CTRL p_print_ctrl,
    _In_z_      PCTSTR filename)
{
    FILE_HANDLE file_out;
    STATUS status, status1;

    if(status_ok(status = t5_file_open(filename, file_open_write, &file_out, TRUE)))
    {
        ARRAY_INDEX inner_copies, outer_copies, outer_copy_idx;

#if RISCOS
        status_assert(file_set_risc_os_filetype(file_out, FILETYPE_ASCII));
#endif

        if(p_print_ctrl->flags.collate)
        {
            inner_copies = 1;
            outer_copies = p_print_ctrl->copies;
        }
        else
        {
            outer_copies = 1;
            inner_copies = p_print_ctrl->copies;
        }

        for(outer_copy_idx = 0; outer_copy_idx < outer_copies; ++outer_copy_idx)
        {
            const ARRAY_INDEX page_count = array_elements(&p_print_ctrl->h_page_list);
            ARRAY_INDEX page_index;
            const ARRAY_INDEX docu_pages_per_printer_page = 1; /* always, cannot do two up! */
            PRINTER_PERCENTAGE printer_percentage;

            print_percentage_initialise(p_docu, &printer_percentage, page_count);

            for(page_index = 0; page_index < page_count; page_index += docu_pages_per_printer_page)
            {
                const PC_PAGE_ENTRY p_page_entry = array_ptrc(&p_print_ctrl->h_page_list, PAGE_ENTRY, page_index);
                ARRAY_INDEX inner_copy_idx;

                for(inner_copy_idx = 0; inner_copy_idx < inner_copies; ++inner_copy_idx)
                {
                    if((p_page_entry->page.x >= 0) && (p_page_entry->page.y >= 0))
                        status = plain_text_page_to_file(p_docu, file_out, &p_page_entry->page);
                  /*else          */
                  /*    blank page*/
                    status_assertc(status);
                    status_break(status);
                }

                status_break(status);

                print_percentage_page_inc(&printer_percentage);
            }

            print_percentage_finalise(&printer_percentage);

            status_break(status);
        }

        status_assert(status1 = t5_file_close(&file_out));

        if(status_ok(status))
            status = status1;
    }

    return(status);
}

T5_MSG_PROTO(extern, t5_msg_draft_print_to_file, P_DRAFT_PRINT_TO_FILE p_draft_print_to_file)
{
    IGNOREPARM_InVal_(t5_message);

    return(print_document_draft_to_file(p_docu, p_draft_print_to_file->p_print_ctrl, p_draft_print_to_file->filename));
}

#if RISCOS

#define PRINTING_START 1

typedef struct PRINTING
{
    P_DOCU       p_docu;
    P_PRINT_CTRL p_print_ctrl;
    STATUS       printing_state;
}
PRINTING, * P_PRINTING;

/******************************************************************************
*
* output draft representation to a file and send it to the printer app
*
******************************************************************************/

_Check_return_
static STATUS
host_print_document_draft_and_send(
    P_PRINTING p_printing,
    _In_        const WimpMessage * const p_wimp_messsage /*DataSaveAck*/)
{
    WimpMessage msg = *p_wimp_messsage;
    PCTSTR filename = msg.data.data_load.leaf_name;
    STATUS status;

    {
    DRAFT_PRINT_TO_FILE draft_print_to_file;
    draft_print_to_file.p_print_ctrl = p_printing->p_print_ctrl;
    draft_print_to_file.filename = filename;
    status = object_skel(p_printing->p_docu, T5_MSG_DRAFT_PRINT_TO_FILE, &draft_print_to_file);
    } /*block*/

    if(status_ok(status))
    {
        _kernel_osfile_block osfile_block;

        if(_kernel_ERROR == _kernel_osfile(OSFile_ReadNoPath, filename, &osfile_block))
            assert0();

        msg.data.data_load.file_type = (osfile_block.load & 0x000FFF00) >> 8;
        msg.data.data_load.estimated_size = osfile_block.start;

        msg.hdr.size  = offsetof32(WimpMessage, data.data_load.leaf_name);
        msg.hdr.size += strlen32p1(filename) /*CH_NULL*/;
        msg.hdr.size  = (msg.hdr.size + (4-1)) & ~(4-1);
        msg.hdr.my_ref = p_wimp_messsage->hdr.your_ref;
        msg.hdr.action_code = Wimp_MDataLoad;

        void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessageRecorded, &msg, p_wimp_messsage->hdr.sender, BAD_WIMP_I, NULL));
    }
    else
        p_printing->printing_state = status;

    return(status);
}

_Check_return_
static BOOL
draft_print_message(
    _In_        const WimpMessage * const p_wimp_message,
    P_PRINTING p_printing)
{
    switch(p_wimp_message->hdr.action_code)
    {
    case Wimp_MPrintFile:
        /* naff all on RISC OS 3.1 and later, as directed by PRM */
        return(TRUE);

    case Wimp_MPrintError:
        if(p_wimp_message->hdr.size == 20)
            /* RISC OS 2 style driver (can even happen on RISC OS 3) */
            p_printing->printing_state = ERR_PRINT_BUSY;
        else
        {
            wimp_reporterror_simple((_kernel_oserror *) &p_wimp_message->data);

            p_printing->printing_state = STATUS_FAIL;
        }

        return(TRUE);

    case Wimp_MDataSaveAck:
        status_assert(host_print_document_draft_and_send(p_printing, p_wimp_message));
        return(TRUE);

    case Wimp_MDataLoadAck:
        p_printing->printing_state = STATUS_OK;
        return(TRUE);

    default:
        break;
    }

    return(FALSE); /* we don't want it - pass it on to the real handler */
}

_Check_return_
static BOOL
draft_print_message_bounced(
    _In_        const WimpMessage * const p_wimp_message,
    P_PRINTING p_printing)
{
    switch(p_wimp_message->hdr.action_code)
    {
    case Wimp_MPrintSave:
        /* our message bounced back; give up */
        myassert0(TEXT("Wimp_MPrintSave bounced - print protocol borked (no printer driver)"));
        p_printing->printing_state = ERR_PRINT_NO_DRIVER;
        return(TRUE);

    case Wimp_MDataLoad:
        /* our message bounced back; give up */
        myassert0(TEXT("Wimp_MDataLoad bounced - print protocol borked (printer driver fooked)"));
        p_printing->printing_state = ERR_PRINT_NO_DRIVER;
        return(TRUE);

    default:
        break;
    }

    return(FALSE); /* we don't want it - pass it on to the real handler */
}

static BOOL
host_print_document_draft_message_filter(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    CLIENT_HANDLE client_handle)
{
    trace_2(TRACE_RISCOS_HOST, TEXT("%s: %s"), __Tfunc__, report_wimp_event(event_code, p_event_data));

    switch(event_code)
    {
    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        return(draft_print_message(&p_event_data->user_message, (P_PRINTING) client_handle));

    case Wimp_EUserMessageAcknowledge:
        return(draft_print_message_bounced(&p_event_data->user_message_acknowledge, (P_PRINTING) client_handle));

    default:
        break;
    }

    return(FALSE); /* we don't want it - pass it on to the real handler */
}

T5_MSG_PROTO(extern, t5_msg_draft_print, P_PRINT_CTRL p_print_ctrl)
{
    WimpMessage msg;
    PRINTING printing;

    IGNOREPARM_InVal_(t5_message);

    printing.p_docu         = p_docu;
    printing.p_print_ctrl   = p_print_ctrl;
    printing.printing_state = PRINTING_START;

    msg.data.data_save.destination_window = 0;
    msg.data.data_save.destination_icon = BAD_WIMP_I;
    msg.data.data_save.destination_x = 0;
    msg.data.data_save.destination_y = 0;
    msg.data.data_save.estimated_size = 42; /* we haven't a clue what the answer is ... */
    msg.data.data_save.file_type = FILETYPE_ASCII;
    xstrkpy(msg.data.data_save.leaf_name, elemof32(msg.data.data_save.leaf_name), p_docu->docu_name.leaf_name); /* SKS 1.03 15mar93 suggest name to printer app */
    if(NULL != p_docu->docu_name.extension)
    {
        xstrkat(msg.data.data_save.leaf_name, elemof32(msg.data.data_save.leaf_name), FILE_EXT_SEP_TSTR);
        xstrkat(msg.data.data_save.leaf_name, elemof32(msg.data.data_save.leaf_name), p_docu->docu_name.extension);
    }

    msg.hdr.size  = offsetof32(WimpMessage, data.data_save.leaf_name);
    msg.hdr.size += strlen32p1(msg.data.data_save.leaf_name); /* SKS 1.03 16mar93 send correct size msg to printer app */
    msg.hdr.size  = (msg.hdr.size + (4-1)) & ~(4-1);
    msg.hdr.my_ref = 0; /* fresh msg */
    msg.hdr.action_code = Wimp_MPrintSave;

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessageRecorded, &msg, 0 /* broadcast */, BAD_WIMP_I, NULL));

    host_message_filter_register(host_print_document_draft_message_filter, (CLIENT_HANDLE) &printing);

    do { (void) wm_event_get(FALSE); } while(printing.printing_state == PRINTING_START);

    host_message_filter_register(NULL, 0);

    status_return(printing.printing_state);

    return(STATUS_OK);
}

#endif /* RISCOS */

#if WINDOWS

/* Within the windows world we have no concept of draft printing, as all
 * windows printing is quite nippy and makes use of printing via the
 * resident printer fonts where applicable, and/or, possible
 * So as you can see this is a bit of a duff functions.
 *
 * You're off your trolley says SKS Come back to planet Earth for a change
 */

T5_MSG_PROTO(extern, t5_msg_draft_print, P_PRINT_CTRL p_print_ctrl)
{
    PCTSTR temp_filename = TEXT("c:\\temp\\draftout.txt");

    IGNOREPARM_InVal_(t5_message);

    {
    DRAFT_PRINT_TO_FILE draft_print_to_file;
    draft_print_to_file.p_print_ctrl = p_print_ctrl;
    draft_print_to_file.filename = temp_filename;
    status_return(object_skel(p_docu, T5_MSG_DRAFT_PRINT_TO_FILE, &draft_print_to_file));
    } /*block*/

    {
    const HOST_WND hwnd = /*!IS_VIEW_NONE(p_view) ? p_view->main[WIN_BACK].hwnd :*/ HOST_WND_NONE;
    SHELLEXECUTEINFO sei;

    zero_struct(sei);
    sei.cbSize = sizeof32(sei);
    sei.hwnd = hwnd;
    sei.lpVerb = TEXT("Open");
    sei.lpFile = temp_filename;
    sei.nShow = SW_SHOW;

    if(!WrapOsBoolChecking(ShellExecuteEx(&sei)))
        return(create_error(STATUS_FAIL));
    } /*block*/
    
    return(STATUS_OK);
}

#endif /* WINDOWS */

/* end of sk_draft.c */
