/* fs_csv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* CSV save object module for Fireworkz */

/* SKS January 1993 */

#include "common/gflags.h"

#include "fs_csv/fs_csv.h"

#include "ob_skel/ff_io.h"

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_FS_CSV)
extern PC_U8 rb_fs_csv_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_FS_CSV &rb_fs_csv_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FS_CSV DONT_LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FS_CSV DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FS_CSV DONT_LOAD_RESOURCES

#define CSV_FIELD_SEP_CH  CH_COMMA
#define CSV_FIELD_SEP_STR ","

static U8
g_csv_save_field_sep_ch = CSV_FIELD_SEP_CH;

#define QUOTE_STR     "\""

/******************************************************************************
*
* outputs CSV text / numbers and skips over unhandled inlines
*
******************************************************************************/

_Check_return_
static STATUS
csv_save_cell_data(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_USTR_INLINE ustr_inline,
    _InVal_     U8 type)
{
    U32 offset = 0;

    if(type == OWNFORM_DATA_TYPE_TEXT)
        status_return(plain_write_ucs4_character(p_ff_op_format, CH_QUOTATION_MARK));

    for(;;)
    {
        if(is_inline_off(ustr_inline, offset))
        {
            switch(inline_code_off(ustr_inline, offset))
            {
            default:
                break;

            case IL_TAB:
                /* TABs become new columns */
                if(type == OWNFORM_DATA_TYPE_TEXT)
                    status_return(plain_write_ustr(p_ff_op_format, USTR_TEXT(QUOTE_STR CSV_FIELD_SEP_STR QUOTE_STR)));
                else
                    status_return(plain_write_ucs4_character(p_ff_op_format, g_csv_save_field_sep_ch));
                break;
            }

            offset += inline_bytecount_off(ustr_inline, offset);
        }
        else
        {
            U32 bytes_of_char;
            UCS4 ucs4 = ustr_char_decode_off((PC_USTR) ustr_inline, offset, /*ref*/bytes_of_char);

            if(CH_NULL == ucs4)
                break;

            if((ucs4 == CH_QUOTATION_MARK) && (type == OWNFORM_DATA_TYPE_TEXT))
                status_return(plain_write_ustr(p_ff_op_format, USTR_TEXT(QUOTE_STR QUOTE_STR))); /* escape quote */
            else
                status_return(plain_write_ucs4_character(p_ff_op_format, ucs4));

            offset += bytes_of_char;
        }
    }

    if(type == OWNFORM_DATA_TYPE_TEXT)
        status_return(plain_write_ucs4_character(p_ff_op_format, CH_QUOTATION_MARK));

    return(STATUS_OK);
}

/******************************************************************************
*
* Save all cells (only) in CSV format
*
******************************************************************************/

T5_MSG_PROTO(static, csv_msg_save_foreign, _InoutRef_ P_MSG_SAVE_FOREIGN p_msg_save_foreign)
{
    P_FF_OP_FORMAT p_ff_op_format = p_msg_save_foreign->p_ff_op_format;
    SCAN_BLOCK scan_block;
    OBJECT_DATA object_data;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        SLR init_slr, last_slr;

        /*p_ff_op_format->size = calc_output_size_from(&scan_block);*/

        /* retrieve the tl of the scan_block now and
         * each time tl of range changes as we go down the markers list
        */
        init_slr.col = p_ff_op_format->of_op_format.save_docu_area.tl.slr.col;
        init_slr.row = p_ff_op_format->of_op_format.save_docu_area.tl.slr.row;

        last_slr.col = init_slr.col;
        last_slr.row = init_slr.row;

        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            P_DOCU_AREA p_current_scan_area;

            save_reflect_status((P_OF_OP_FORMAT) p_ff_op_format, cells_scan_percent(&scan_block));

            p_current_scan_area = p_docu_area_from_scan_block(&scan_block);

            if( (p_current_scan_area->tl.slr.col != init_slr.col) ||
                (p_current_scan_area->tl.slr.row != init_slr.row) )
            {
                init_slr.col = p_current_scan_area->tl.slr.col;
                init_slr.row = p_current_scan_area->tl.slr.row;

                last_slr.col = init_slr.col;
                last_slr.row = init_slr.row;
            }

            if(last_slr.row < object_data.data_ref.arg.slr.row)
            {
                /* output as many record separators as needed */
                do  {
                    status_return(plain_write_newline(p_ff_op_format));

                    /*p_ff_op_format->of_op_format.offset +=
                        (p_current_scan_area->br.slr.col - p_current_scan_area->tl.slr.col);*/
                }
                while(++last_slr.row < object_data.data_ref.arg.slr.row);

                /* have to output all leading field separators for new record */
                last_slr.col = init_slr.col;
            }

            if(last_slr.col < object_data.data_ref.arg.slr.col)
            {
                /* output as many field separators as needed */
                do  {
                    status_return(plain_write_ucs4_character(p_ff_op_format, g_csv_save_field_sep_ch));

                    /*p_ff_op_format->of_op_format.offset++;*/
                }
                while(++last_slr.col < object_data.data_ref.arg.slr.col);
            }

            last_slr = object_data.data_ref.arg.slr;

            {
            SAVE_CELL_OWNFORM save_cell_ownform;
            UCHARZ contents_buffer[256];
            UCHARZ formula_buffer[256];

            zero_struct(save_cell_ownform);

            save_cell_ownform.object_data = object_data;
            save_cell_ownform.p_of_op_format = NULL; /* not required for foreign */

            quick_ublock_setup(&save_cell_ownform.contents_data_quick_ublock, contents_buffer);
            quick_ublock_setup(&save_cell_ownform.formula_data_quick_ublock, formula_buffer);

            if(status_ok(status = object_call_id(object_data.object_id, p_docu, T5_MSG_SAVE_CELL_OWNFORM, &save_cell_ownform)))
            if(0 != quick_ublock_bytes(&save_cell_ownform.contents_data_quick_ublock))
            if(status_ok(status = quick_ublock_nullch_add(&save_cell_ownform.contents_data_quick_ublock)))
                 status = csv_save_cell_data(p_ff_op_format, quick_ublock_ustr_inline(&save_cell_ownform.contents_data_quick_ublock), save_cell_ownform.data_type);

            quick_ublock_dispose(&save_cell_ownform.contents_data_quick_ublock);
            quick_ublock_dispose(&save_cell_ownform.formula_data_quick_ublock);
            } /*block*/

            status_return(status);

            /*p_ff_op_format->of_op_format.offset++;*/
        }
    }

    status_return(status);

    /* SKS 20apr93 after 1.03 - output a final record separator too */
    return(plain_write_newline(p_ff_op_format));
}

/******************************************************************************
*
* CSV save object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, csv_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FS_CSV, P_BOUND_MESSAGES_OBJECT_ID_FS_CSV, P_BOUND_RESOURCES_OBJECT_ID_FS_CSV));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FS_CSV));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fs_csv);
OBJECT_PROTO(extern, object_fs_csv)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(csv_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_SAVE_FOREIGN:
        return(csv_msg_save_foreign(p_docu, t5_message, (P_MSG_SAVE_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fs_csv.c */
