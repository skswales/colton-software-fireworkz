/* fs_ascii.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* ASCII save object module for Fireworkz */

/* SKS January 1993 */

#include "common/gflags.h"

#include "fs_ascii/fs_ascii.h"

#include "ob_skel/ff_io.h"

#if RISCOS
#define MSG_WEAK &rb_fs_ascii_msg_weak
extern PC_U8 rb_fs_ascii_msg_weak;
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FS_ASCII DONT_LOAD_RESOURCES

static U8
g_ascii_save_field_sep_ch = CH_TAB;

/* Save all cells (only) in ASCII (TAB-delimited) format */

T5_MSG_PROTO(extern, ascii_msg_save_foreign, _InoutRef_ P_MSG_SAVE_FOREIGN p_msg_save_foreign)
{
    P_FF_OP_FORMAT p_ff_op_format = p_msg_save_foreign->p_ff_op_format;
    const BOOL fSuppressNewline = docu_area_is_frag(&p_ff_op_format->of_op_format.save_docu_area);
    SCAN_BLOCK scan_block;
    OBJECT_DATA object_data;
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        SLR init_slr, last_slr;

        init_slr.col = p_ff_op_format->of_op_format.save_docu_area.tl.slr.col;
        init_slr.row = p_ff_op_format->of_op_format.save_docu_area.tl.slr.row;

        last_slr.col = init_slr.col;
        last_slr.row = init_slr.row;

        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            P_DOCU_AREA p_current_scan_area;

            save_reflect_status((P_OF_OP_FORMAT) p_ff_op_format, cells_scan_percent(&scan_block));

            p_current_scan_area = p_docu_area_from_scan_block(&scan_block);

            if((p_current_scan_area->tl.slr.col != init_slr.col) || (p_current_scan_area->tl.slr.row != init_slr.row))
            {
                init_slr.col = p_current_scan_area->tl.slr.col;
                init_slr.row = p_current_scan_area->tl.slr.col;
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
                    status_return(plain_write_ucs4_character(p_ff_op_format, g_ascii_save_field_sep_ch));

                    /*p_ff_op_format->of_op_format.offset++;*/
                }
                while(++last_slr.col < object_data.data_ref.arg.slr.col);
            }

            last_slr = object_data.data_ref.arg.slr;

            {
            /* SKS 18jul94 after 1.08b2 output displayed values - used to save like ownform! */
            OBJECT_READ_TEXT object_read_text;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
            quick_ublock_with_buffer_setup(quick_ublock);

            object_read_text.object_data = object_data;
            object_read_text.p_quick_ublock = &quick_ublock;
            object_read_text.type = OBJECT_READ_TEXT_RESULT;

            if(status_ok(status = object_call_id(object_read_text.object_data.object_id, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text)))
                status = plain_write_uchars(p_ff_op_format, quick_ublock_uchars(object_read_text.p_quick_ublock), quick_ublock_bytes(object_read_text.p_quick_ublock));

            quick_ublock_dispose(&quick_ublock);
            } /*block*/

            status_return(status);

            /*p_ff_op_format->of_op_format.offset++;*/
        }
    }

    if(fSuppressNewline)
        return(STATUS_OK);

    /* SKS 20apr93 after 1.03 - output a final record separator too */
    return(plain_write_newline(p_ff_op_format));
}

/******************************************************************************
*
* ASCII converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, ascii_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FS_ASCII, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FS_ASCII));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FS_ASCII));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fs_ascii);
OBJECT_PROTO(extern, object_fs_ascii)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(ascii_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_SAVE_FOREIGN:
        return(ascii_msg_save_foreign(p_docu, t5_message, (P_MSG_SAVE_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fs_ascii.c */
