/* ob_skel/ff_io.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* ownform / foreign file format exports */

/*
raw
*/

_Check_return_
static inline U8
binary_read_byte(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_STATUS p_status)
{
    STATUS status;

    *p_status = STATUS_OK;

    if(NULL != p_ip_format_input->file.file_handle /*IP_INPUT_FILE == p_ip_format_input->state*/)
    {
        const FILE_HANDLE file_handle = p_ip_format_input->file.file_handle;

        /*++p_ip_format_input->file.file_offset_in_bytes;*/ /* absolute accuracy is not needed for this wrt multiple reads at EOF etc! */

        if(file_getc_fast_ready(file_handle))
            return(file_getc_fast(file_handle));

        status = file_getc(file_handle);

        if(status_ok(status) && (EOF_READ != status))
        {
            p_ip_format_input->file.file_offset_in_bytes += (file_handle->count + 1); /* even more approximate! */
            return((U8) status);
        }

        /* status_fail or EOF_READ */
    }
    else /* if(IP_INPUT_MEM == p_ip_format_input->state) */
    {
        if(array_offset_valid(p_ip_format_input->mem.p_array_handle, p_ip_format_input->mem.array_offset))
        {
            const U32 array_offset = p_ip_format_input->mem.array_offset++;
            return(*array_ptr32c(p_ip_format_input->mem.p_array_handle, U8, array_offset));
        }

        status = EOF_READ;
    }

    *p_status = status;
    return(CH_NULL /* == (U8) EOF_READ */); /* caller must test for EOF_READ as well as status_fail */
}

_Check_return_
static inline STATUS
binary_write_byte(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     U8 byte_out)
{
    if(OP_OUTPUT_FILE == p_op_format_output->state)
    {
        const FILE_HANDLE file_handle = p_op_format_output->u.file.file_handle;

        if(file_putc_fast_ready(file_handle))
        {
            file_putc_fast(byte_out, file_handle);
            return(STATUS_OK);
        }

        status_return(file_putc(byte_out, file_handle));
        return(STATUS_OK); /* normalise */
    }

    assert(OP_OUTPUT_MEM == p_op_format_output->state);
    return(al_array_add(p_op_format_output->u.mem.p_array_handle, BYTE, 1, PC_ARRAY_INIT_BLOCK_NONE, &byte_out));
}

_Check_return_
static inline STATUS
binary_write_bytes(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _In_reads_bytes_(n_bytes) PC_ANY p_any,
    _InVal_     U32 n_bytes)
{
    /*if(0 == n_bytes)
        return(STATUS_OK);*/

    if(OP_OUTPUT_FILE == p_op_format_output->state)
    {
        status_return(file_write_bytes(p_any, n_bytes, p_op_format_output->u.file.file_handle));

        /* NB file_write returns +ve non-zero number on success, normalise return */
        return(STATUS_OK);
    }

    assert(OP_OUTPUT_MEM == p_op_format_output->state);
    return(al_array_add(p_op_format_output->u.mem.p_array_handle, BYTE, n_bytes, PC_ARRAY_INIT_BLOCK_NONE, p_any));
}

/*
exported routines - foreign
*/

_Check_return_
extern STATUS
plain_read_ucs4_character(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _OutRef_    P_UCS4 p_char_read);

_Check_return_
extern STATUS
plain_write_newline(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format);

_Check_return_
extern STATUS
plain_write_ucs4_character(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     UCS4 ucs4);

_Check_return_
extern STATUS
plain_write_uchars(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
plain_write_ustr(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_USTR ustr);

_Check_return_
extern STATUS
plain_write_tchars(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n);

_Check_return_
extern STATUS
plain_write_tstr(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PCTSTR tstr);

#define plain_write_a7char plain_write_ucs4_character

_Check_return_
static inline STATUS
plain_write_a7chars(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(a7chars_n) PC_U8Z a7chars,
    _InVal_     U32 a7chars_n)
{
    return(plain_write_uchars(p_ff_op_format, (PC_UCHARS) a7chars, a7chars_n));
}

_Check_return_
static inline STATUS
plain_write_a7str(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_A7STR a7str)
{
    return(plain_write_uchars(p_ff_op_format, (PC_UCHARS) a7str, strlen32(a7str)));
}

_Check_return_
extern STATUS
foreign_save_set_io_type(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     IO_TYPE io_type);

_Check_return_
extern STATUS
foreign_finalise_save(
    _OutRef_opt_ P_P_DOCU p_p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     STATUS save_status);

_Check_return_
extern STATUS
foreign_initialise_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _In_opt_z_  PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InRef_opt_ PC_DOCU_AREA p_docu_area);

_Check_return_
extern STATUS
foreign_load_file_apply_style_table(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area_table,
    _InVal_     COL lhs_extra_cols,
    _InVal_     ROW top_extra_rows,
    _InVal_     ROW bot_extra_rows);

_Check_return_
extern STATUS
foreign_load_file_auto_width_table(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area_table);

/*
cross-exports internal to of_load/ff_load
*/

/*
of_load.c
*/

_Check_return_
extern /*for ff_load*/ STATUS
load_is_file_loaded(
    _OutRef_    P_DOCNO p_docno /*DOCNO_NONE->not loaded, +ve->docno of thunk or file*/,
    _InRef_     PC_DOCU_NAME p_docu_name);

_Check_return_
extern /*for ff_load*/ STATUS
new_docno_using(
    _OutRef_    P_DOCNO p_docno,
    _In_z_      PCTSTR leafname, /* might not exist */
    _InRef_     PC_DOCU_NAME p_docu_name /*copied*/);

_Check_return_
extern /*for ff_load*/ STATUS
select_a_template(
    _DocuRef_   P_DOCU cur_p_docu,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/,
    /*out*/ P_BOOL p_just_the_one,
    _InVal_     BOOL allow_dirs,
    _InRef_     PC_UI_TEXT p_ui_text_caption);

/*
ff_load.c
*/

extern /*for of_load*/ void
ff_load_msg_exit2(void);

/* end of ff_io.h */
