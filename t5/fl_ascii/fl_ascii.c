/* fl_ascii.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* ASCII load object module for Fireworkz */

/* SKS January 1993 */

#include "common/gflags.h"

#include "fl_ascii/fl_ascii.h"

#include "ob_skel/ff_io.h"

#include "cmodules/unicode/u2000.h" /* 2000..206F General Punctuation */

#if RISCOS
#define MSG_WEAK &rb_fl_ascii_msg_weak
extern PC_U8 rb_fl_ascii_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_FL_ASCII NULL

#define ASCII_LOAD_ROWS_INCREMENT     8
#define ASCII_LOAD_ROWS_INCREMENT_MAX 256

/******************************************************************************
*
* read a line from a file, returning error or terminator
*
******************************************************************************/

static UCS4 ascii_readahead_char = EOF_READ;

_Check_return_
static inline STATUS
ascii_load_ucs4_character(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _OutRef_    P_UCS4 p_char_read)
{
    if(EOF_READ != ascii_readahead_char)
    {
        *p_char_read = ascii_readahead_char;
        ascii_readahead_char = EOF_READ;
        return(STATUS_OK);
    }

    for(;;)
    {
        status_return(plain_read_ucs4_character(p_ff_ip_format, p_char_read));

        if(*p_char_read <= 0x1F)
        {
            if((LF != *p_char_read) && (CH_TAB != *p_char_read))
                continue; /* ignore other C0 CtrlChars */
        }

        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
ascii_load_line(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_contents,
    _InVal_     BOOL load_as_paragraphs)
{
    for(;;)
    {
        UCS4 char_read;

        status_return(ascii_load_ucs4_character(p_ff_ip_format, &char_read));

        if(EOF_READ == char_read)
        {
            status_return(quick_ublock_nullch_add(p_quick_ublock_contents));
            return(EOF_READ);
        }

        if(CH_TAB == char_read)
        {
            status_return(inline_quick_ublock_IL_TAB(p_quick_ublock_contents));
            continue;
        }

        if(UCH_PARAGRAPH_SEPARATOR == char_read)
        {
            status_return(quick_ublock_nullch_add(p_quick_ublock_contents));
            return(STATUS_OK);
        }

        if((LF == char_read) || (UCH_LINE_SEPARATOR == char_read))
        {
            if(!load_as_paragraphs)
            {
                status_return(quick_ublock_nullch_add(p_quick_ublock_contents));
                return(STATUS_OK);
            }

            /* if loading paragraph then we need to read ahead to see what we're up to */
            status_return(ascii_load_ucs4_character(p_ff_ip_format, &ascii_readahead_char));

            if((LF == ascii_readahead_char) || (UCH_LINE_SEPARATOR == ascii_readahead_char))
            {   /* line-separator,line-separator -> end-of-paragraph */
                ascii_readahead_char = EOF_READ; /* we've used it */
                status_return(quick_ublock_nullch_add(p_quick_ublock_contents));
                return(STATUS_OK);
            }

            if(CH_SPACE != ascii_readahead_char)
            {   /* substitute a space for this line separator if one is needed */
                U32 len = quick_ublock_bytes(p_quick_ublock_contents);

                if(0 != len)
                {
                    PC_UCHARS uchars = quick_ublock_uchars(p_quick_ublock_contents);

                    if(CH_SPACE != PtrGetByteOff(uchars, len-1))
                    {
                        status_return(quick_ublock_a7char_add(p_quick_ublock_contents, CH_SPACE));
                    }
                }
            }

            continue;
        }

        status_return(quick_ublock_ucs4_add_aiu(p_quick_ublock_contents, char_read));
    }
}

static ROW n_rows_inserted = 0; /* fix */
static ROW n_rows_inserted_in_docu = 0;
static SLR slr, i_slr;

_Check_return_
static STATUS
ascii_msg_insert_foreign_core(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format)
{
    const BOOL load_as_paragraphs = global_preferences.ascii_load_as_paragraphs;
    BOOL maybe_frag = TRUE; /* first bit of text could be a fragment */
    BOOL first_split = TRUE;
    ROW n_rows_increment = ASCII_LOAD_ROWS_INCREMENT;
    LOAD_CELL_FOREIGN load_cell_foreign;
    STATUS status = STATUS_OK;

    zero_struct(load_cell_foreign);

    for(;;)
    {
        STATUS load_line_status;
        POSITION position_load;
        U32 total_line_length;
        U32 used_so_far = 0;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_contents, 128);
        quick_ublock_with_buffer_setup(quick_ublock_contents);

        status = ascii_load_line(p_ff_ip_format, &quick_ublock_contents, load_as_paragraphs);

        load_reflect_status(&p_ff_ip_format->of_ip_format);

        if(status_fail(status))
        {
            quick_ublock_dispose(&quick_ublock_contents);
            break;
        }

        load_line_status = status;

        p_docu = p_docu_from_docno(p_ff_ip_format->of_ip_format.docno);

        total_line_length = quick_ublock_bytes(&quick_ublock_contents); /* includes CH_NULL */

        while(used_so_far < total_line_length) /* won't actually keep looping in 99.9% of cases */
        {
            PC_USTR_INLINE ustr_inline = ustr_inline_AddBytes(quick_ublock_ustr(&quick_ublock_contents), used_so_far);
            BOOL needs_splitting = ((total_line_length - used_so_far) >= 32700);

            if(maybe_frag && (EOF_READ == load_line_status) && !needs_splitting)
            {   /* a single short enough line has been read, can try inserting as fragment at insertion position */
                position_load = p_ff_ip_format->of_ip_format.insert_position;
            }
            else
            {
                if(n_rows_inserted >= n_rows_inserted_in_docu)
                {
                    /* insert some more rows for this data to go into */
                    if(first_split)
                    {
                        /* first time through, split the document at insertion point (inserting rows too) */
                        DOCU_AREA docu_area;
                        POSITION position = p_ff_ip_format->of_ip_format.insert_position;

                        first_split = maybe_frag = FALSE;

                        docu_area_init(&docu_area);
                        docu_area.br.slr.col = 1;
                        docu_area.br.slr.row = n_rows_increment;

                        status = cells_docu_area_insert(p_docu, &position, &docu_area, &position);

                        slr.row = i_slr.row = position.slr.row;
                    }
                    else
                        status = cells_block_insert(p_docu, 0, all_cols(p_docu), slr.row, n_rows_increment, 0);

                    if(status_ok(status))
                    {
                        n_rows_inserted_in_docu += n_rows_increment;

                        if(n_rows_increment < ASCII_LOAD_ROWS_INCREMENT_MAX)
                            n_rows_increment <<= 1;
                    }
                }

                status_break(status);

                n_rows_inserted++;

                position_load.slr = slr;
                position_load.object_position.data = 0;
                position_load.object_position.object_id = OBJECT_ID_NONE;
            }

            if(needs_splitting)
            {   /* find a suitable point in this line to truncate as Fireworkz cell size is currently rather limited */
                const U32 start_offset = 0;
                const U32 end_offset = start_offset + 32000;
                U32 offset = end_offset;
                BOOL found_suitable = FALSE;

                for(;;)
                {
                    U32 bytes_of_thing = inline_b_bytecount_off(ustr_inline, offset);

                    if(CH_SPACE == PtrGetByteOff(ustr_inline, offset))
                    {
                        found_suitable = TRUE;
                        break;
                    }

                    if(bytes_of_thing >= (offset - start_offset))
                        break;

                    offset -= bytes_of_thing;
                }

                if(!found_suitable)
                {
                    offset = end_offset;

                    /* avoid truncating in the middle of something */
                    for(;;)
                    {
                        if(is_inline_off(ustr_inline, offset))
                        {
                            offset += inline_bytecount_off(ustr_inline, offset);
                            continue;
                        }
#if !USTR_IS_SBSTR
                        if(u8_is_utf8_lead_byte(PtrGetByteOff(ustr_inline, offset)))
                        {
                            offset += utf8_bytes_of_char_off(ustr_inline, offset);
                            continue;
                        }
                        if(u8_is_utf8_trail_byte(PtrGetByteOff(ustr_inline, offset)))
                        {
                            offset++;
                            continue;
                        }
#endif
                        break;
                    }
                }

                PtrPutByteOff(de_const_cast(P_USTR_INLINE, ustr_inline), offset, CH_NULL);

                used_so_far += offset + 1 /* start next time round past the CH_NULL */;
            }
            else
            {   /* insert everything that we have accumulated for this line, then this line is done */
                used_so_far = total_line_length;
            }

            consume_bool(object_data_from_position(p_docu, &load_cell_foreign.object_data, &position_load, NULL));
            load_cell_foreign.original_slr = position_load.slr;

            load_cell_foreign.data_type = OWNFORM_DATA_TYPE_TEXT;
            load_cell_foreign.ustr_inline_contents = ustr_inline;
          /*load_cell_foreign.ustr_formula  = NULL;*/

            status_break(status = insert_cell_contents_foreign(p_docu, OBJECT_ID_TEXT, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign));

            slr.row++;
        }

        quick_ublock_dispose(&quick_ublock_contents);

        status_break(status);

        if(EOF_READ == load_line_status)
        {
            status = STATUS_OK;
            break;
        }
    }

    return(status);
}

T5_MSG_PROTO(static, ascii_msg_insert_foreign, _InoutRef_ P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    FF_IP_FORMAT ff_ip_format = FF_IP_FORMAT_INIT;
    P_FF_IP_FORMAT p_ff_ip_format = &ff_ip_format;
    STATUS status = STATUS_OK;

    if(global_preferences.ascii_load_as_delimited)
    {
#if 1
        /* If a delimiter is set, load as delimited text using CSV module */
#else
        /* If it's a Letter-derived document, insert as plain text (TABs -> inlines) */
        /* If it's somewhere in a Text cell in a Sheet-derived document, insert as plain text */
        /* Otherwise try to load as TAB-delimited text using CSV module */
        if(!p_docu->flags.base_single_col)
            if(OBJECT_ID_TEXT != p_msg_insert_foreign->position.object_position.object_id)
#endif
                return(object_call_id_load(p_docu, t5_message, p_msg_insert_foreign, OBJECT_ID_FL_CSV));
    }

    status_return(ensure_memory_froth());

    n_rows_inserted = 0;
    n_rows_inserted_in_docu = 0;

    ascii_readahead_char = EOF_READ;

    ff_ip_format.of_ip_format.flags.insert = p_msg_insert_foreign->insert;

    ff_ip_format.of_ip_format.process_status.flags.foreground = 1;

    status_return(foreign_load_initialise(p_docu, &p_msg_insert_foreign->position, p_ff_ip_format, p_msg_insert_foreign->filename, &p_msg_insert_foreign->array_handle));

    i_slr = p_ff_ip_format->of_ip_format.insert_position.slr;
    slr   = p_ff_ip_format->of_ip_format.insert_position.slr;

    status = foreign_load_determine_io_type(p_ff_ip_format);

    if(status_ok(status))
        status = ascii_msg_insert_foreign_core(p_docu, p_ff_ip_format);

    status_accumulate(status, foreign_load_finalise(p_docu, p_ff_ip_format));

    /* delete out spuriously inserted rows from end */
    if((n_rows_inserted_in_docu - n_rows_inserted) > 0)
        cells_block_delete(p_docu, 0, all_cols(p_docu), slr.row, n_rows_inserted_in_docu - n_rows_inserted);

    status_accumulate(status, format_col_row_extents_set(p_docu, n_cols_logical(p_docu), MAX(slr.row, n_rows(p_docu))));

    status_return(status);

    /* reposition caret */
    p_docu->cur.slr = i_slr;
    p_docu->cur.object_position.object_id = OBJECT_ID_NONE;

    caret_show_claim(p_docu, OBJECT_ID_CELLS, FALSE);

    return(status);
}

/******************************************************************************
*
* ASCII load object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, ascii_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FL_ASCII, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FL_ASCII));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FL_ASCII));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fl_ascii);
OBJECT_PROTO(extern, object_fl_ascii)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(ascii_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(ascii_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of flf_ascii.c */
