/* of_save.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Own format file save routines for Fireworkz */

/* JAD Mar 1992; MRJC October 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ff_io.h"

#include "cmodules/collect.h"

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

/*
internal routines
*/

_Check_return_
static STATUS
ownform_save_object_inline(
    _In_z_      PC_USTR_INLINE ustr_inline,
    _InVal_     OBJECT_ID object_id,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

#define SAVE_OUTPUT_BUFFER_INC 4096

_Check_return_
_Ret_z_
extern PCTSTR
localise_filename(
    _In_z_      PCTSTR name_to_make_relative_to,
    _In_z_      PCTSTR filename)
{
    TCHARZ buffer[BUF_MAX_PATHSTRING];

    if(file_get_cwd(buffer, name_to_make_relative_to))
    {
        U32 len = tstrlen32(buffer);

        if(0 == tstrnicmp(buffer, filename, len))
            filename += len;
    }

    return(filename);
}

/*
ownform
*/

_Check_return_
extern STATUS
arglist_prepare_with_construct(
    _OutRef_    P_ARGLIST_HANDLE p_arglist_handle,
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message,
    _OutRef_    P_PC_CONSTRUCT_TABLE p_p_construct_table)
{
    if(NULL == (*p_p_construct_table = construct_table_lookup_message(object_id, t5_message)))
    {
        *p_arglist_handle = 0;
        return(STATUS_FAIL);
    }

    return(arglist_prepare(p_arglist_handle, (*p_p_construct_table)->args));
}

/******************************************************************************
*
* Finalise a save from of_op_format (tidy up stuff e.g. close file etc.)
*
******************************************************************************/

_Check_return_
extern STATUS
ownform_finalise_save(
    _OutRef_opt_ P_P_DOCU p_p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     STATUS save_status)
{
    STATUS status = STATUS_OK;
    STATUS status1;
    P_DOCU p_docu;

    process_status_end(&p_of_op_format->process_status);

    p_docu = p_docu_from_docno(p_of_op_format->docno);

    if(NULL != p_p_docu)
        *p_p_docu = p_docu;

    if(p_of_op_format->output.state == OP_OUTPUT_FILE)
    {
#if RISCOS
        if(status_fail(save_status))
            status_assert(file_set_risc_os_filetype(p_of_op_format->output.u.file.file_handle, FILETYPE_DATA));
#endif

        status = file_close_date(&p_of_op_format->output.u.file.file_handle, &p_of_op_format->output.u.file.ev_date);
    }
    else
    {
        if(status_fail(save_status))
            al_array_dispose(p_of_op_format->output.u.mem.p_array_handle);
    }

    { /* kosher even in NULL p_docu case 'cos state should be stored in the passed of_op_format's list */
    MSG_SAVE msg_save;
    OBJECT_ID object_id = OBJECT_ID_ENUM_START;

    msg_save.p_of_op_format = p_of_op_format;
    msg_save.t5_msg_save_message = T5_MSG_SAVE__SAVE_ENDED;

    while(object_id < p_of_op_format->stopped_object_id)
    {
        if(status_fail(status1 = object_call_between(&object_id, p_of_op_format->stopped_object_id, p_docu, T5_MSG_SAVE, &msg_save)))
        {
            if(status_ok(status))
                status = status1;
        }
    }
    } /*block*/

    collect_delete(&p_of_op_format->object_data_list);

    if(p_of_op_format->output.state == OP_OUTPUT_FILE)
        tstr_clr(&p_of_op_format->output.u.file.filename);

    return(status_fail(save_status) ? save_status : status);
}

_Check_return_
static STATUS
op_output_initialise_save(
    _OutRef_    P_OP_FORMAT_OUTPUT p_op_format_output,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _In_opt_z_  PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status = STATUS_OK;

    p_op_format_output->state = (NULL == filename) ? OP_OUTPUT_MEM : OP_OUTPUT_FILE;

    if(p_op_format_output->state == OP_OUTPUT_FILE)
    {
        assert((NULL == p_array_handle) || (0 == *p_array_handle));

        p_op_format_output->u.file.t5_filetype = t5_filetype;

        if(status_ok(status = tstr_set(&p_op_format_output->u.file.filename, filename)))
        {
            status_assert(status = t5_file_open(filename, file_open_write, &p_op_format_output->u.file.file_handle, TRUE));

            assert(p_op_format_output->u.file.file_handle != 0);

#if RISCOS
            if(status_ok(status))
                status_assert(file_set_risc_os_filetype(p_op_format_output->u.file.file_handle, t5_filetype));

            if(status_ok(status))
                status = alloc_ensure_froth(0x6000 + 16384);

            if(status_ok(status)) /* SKS 26feb97 fix the status-losing bug */
#endif
                status_assert(file_buffer(p_op_format_output->u.file.file_handle, NULL, 16384)); /* SKS 19apr95 big buffering for saves */
        }
    }
    else
    {
        assert((NULL != p_array_handle) /*&& (0 == *p_array_handle)*/);

        p_op_format_output->u.mem.p_array_handle = p_array_handle; /* allows append - do we really want this? */

        if(0 == *p_op_format_output->u.mem.p_array_handle)
        {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(SAVE_OUTPUT_BUFFER_INC, sizeof32(U8), FALSE);
            status = al_array_preallocate_zero(p_op_format_output->u.mem.p_array_handle, &array_init_block);
        }
    }

    return(status);
}

_Check_return_
extern STATUS
ownform_initialise_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _In_opt_z_  PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InRef_opt_ PC_DOCU_AREA p_docu_area)
{
    STATUS status;

    status = op_output_initialise_save(&p_of_op_format->output, p_array_handle, filename, t5_filetype);

    p_of_op_format->docno = docno_from_p_docu(p_docu);

    if(NULL != p_docu_area)
        p_of_op_format->save_docu_area = *p_docu_area;
    else
    {
        docu_area_init(&p_of_op_format->save_docu_area);
        p_of_op_format->save_docu_area.whole_col =
        p_of_op_format->save_docu_area.whole_row = 1;
    }

    /*p_of_op_format->offset = 0;*/

    p_of_op_format->stopped_object_id = OBJECT_ID_ENUM_START;

    p_of_op_format->save_docu_area_no_frags = p_of_op_format->save_docu_area;

    if(status_ok(status))
    {
#if defined(VALIDATE_MAIN_ALLOCS) && 0
        alloc_validate((void *) 0, "SK_SAVE");
        alloc_validate((void *) 1, "SK_SAVE");
#endif

        if(UI_TEXT_TYPE_NONE == p_of_op_format->process_status.reason.type)
        {
            p_of_op_format->process_status.reason.type = UI_TEXT_TYPE_RESID;
            p_of_op_format->process_status.reason.text.resource_id = MSG_STATUS_SAVING;
        }

        process_status_begin(p_docu, &p_of_op_format->process_status, PROCESS_STATUS_PERCENT);

        { /* call all the objects to say a save has started */
        MSG_SAVE msg_save;

        msg_save.p_of_op_format = p_of_op_format;
        msg_save.t5_msg_save_message = T5_MSG_SAVE__SAVE_STARTED;

        /* kosher even in NULL p_docu case 'cos state should be stored in the passed of_op_format's list */
        status = object_call_between(&p_of_op_format->stopped_object_id, OBJECT_ID_MAX, p_docu, T5_MSG_SAVE, &msg_save);
        } /*block*/
    }

    if(status_fail(status))
        status = ownform_finalise_save(&p_docu, p_of_op_format, status);

    return(status);
}

#define ownform_write_byte(p_of_of_format, byte_out) \
    binary_write_byte(&(p_of_of_format)->output, (byte_out))

#define ownform_write_bytes(p_of_of_format, p_any, n_bytes) \
    binary_write_bytes(&(p_of_of_format)->output, (p_any), (n_bytes))

_Check_return_
static STATUS
ownform_save_string_s32(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     S32 s32)
{
    char stringbuf[OF_DATA_MAX_BUF];
    int n;

__pragma(warning(push)) __pragma(warning(disable:4996)) /* This function or variable may be unsafe */
    n = sprintf(stringbuf, S32_FMT, s32);
__pragma(warning(pop))

    if(n <= 0)
        return(STATUS_FAIL);

    return(ownform_write_bytes(p_of_op_format, stringbuf, n));
}

_Check_return_
static STATUS
ownform_save_string_x32(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     U32 u32)
{
    char stringbuf[OF_DATA_MAX_BUF];
    int n;

__pragma(warning(push)) __pragma(warning(disable:4996)) /* This function or variable may be unsafe */
    n = sprintf(stringbuf, U32_XFMT, u32); /* requires 0x prefix for load code */
__pragma(warning(pop))

    if(n <= 0)
        return(STATUS_FAIL);

    return(ownform_write_bytes(p_of_op_format, stringbuf, n));
}

_Check_return_
static STATUS
ownform_save_string_f64(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InRef_     PC_F64 p_f64)
{
    char stringbuf[OF_DATA_MAX_BUF];
    int n;

__pragma(warning(push)) __pragma(warning(disable:4996)) /* This function or variable may be unsafe */
    n = sprintf(stringbuf, F64_FMT, *p_f64);
__pragma(warning(pop))

    if(n <= 0)
        return(STATUS_FAIL);

    return(ownform_write_bytes(p_of_op_format, stringbuf, n));
}

/* translate C0/C1 CtrlChars into more visible/editable tbs forms */

_Check_return_
static STATUS
ownform_save_ctrl_char(
    _Inout_     P_OF_OP_FORMAT p_of_op_format,
    _InVal_     U8 u8)
{
    STATUS status;

    if(0 == (u8 & 0x80))
    {   /* C0: 0x00..0x1F -> 0x80..0x9F */
        status = ownform_write_byte(p_of_op_format, u8 | 0x80);
    }
    else
    {   /* C1: 0x80..0x9F -> escaped C1 */
        if(status_ok(status = ownform_write_byte(p_of_op_format, OF_ESCAPE_CHAR)))
                     status = ownform_write_byte(p_of_op_format, u8);
    }

    return(status);
}

#if !USTR_IS_SBSTR || defined(ARG_TYPE_TSTR_DISTINCT)

/* save generic UCS-4 character as U (Unicode) construct in file */

_Check_return_
static STATUS
ownform_save_unicode_inline(
    _Inout_     P_OF_OP_FORMAT p_of_op_format,
    _InVal_     UCS4  ucs4)
{
    STATUS status;
    UCHARZ repr_buffer[16];
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 32);
    quick_ublock_with_buffer_setup(quick_ublock);

__pragma(warning(push)) __pragma(warning(disable:4996)) /* This function or variable may be unsafe */
    consume_int(sprintf((char *) repr_buffer, "%.4X", ucs4));
__pragma(warning(pop))

    if(status_ok(status = inline_quick_ublock_from_ustr(&quick_ublock, IL_UTF8, ustr_bptr(repr_buffer))))
        status = ownform_save_object_inline(quick_ublock_ustr_inline(&quick_ublock), OBJECT_ID_TEXT, p_of_op_format);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

#endif /**/

_Check_return_
static STATUS
ownform_save_ustr_inline(
    _Inout_     P_OF_OP_FORMAT p_of_op_format,
    _In_z_      PC_USTR_INLINE ustr_inline,
    _InVal_     OBJECT_ID object_id)
{
    STATUS status = STATUS_OK;

    while(status_ok(status))
    {
        U8 u8 = PtrGetByte(ustr_inline);

#if USTR_IS_SBSTR
        if(CH_NULL == u8)
            break;

        if(CH_INLINE == u8)
        {   /* save out construct representing inline, will go recursive */
            /* NB this first tries with the object owning the containing construct but does go on to try others */
            status = ownform_save_object_inline(ustr_inline, object_id, p_of_op_format);
            inline_advance(PC_USTR_INLINE, ustr_inline);
            continue;
        }

        ustr_inline_IncByte(ustr_inline);
#else
        if(u8_is_ascii7(u8))
        {
            if(CH_NULL == u8)
                break;

            if(CH_INLINE == u8)
            {   /* save out construct representing inline, will go recursive */
                /* NB this first tries with the object owning the containing construct but does go on to try others */
                status = ownform_save_object_inline(ustr_inline, object_id, p_of_op_format);
                inline_advance(PC_USTR_INLINE, ustr_inline);
                continue;
            }

            ustr_inline_IncByte(ustr_inline);
        }
        else
        {
            PC_USTR ustr_next;
            UCS4 ucs4 = ustr_char_next((PC_USTR) ustr_inline, &ustr_next);

            ustr_inline = (PC_USTR_INLINE) ustr_next;

            if(ucs4_is_sbchar(ucs4))
            {
                u8 = (U8) ucs4;
            }
            else
            {   /* try transforming */
                UCS4 ucs4_try = ucs4_to_sbchar_try_with_codepage(ucs4, get_system_codepage());

                if(ucs4_is_sbchar(ucs4_try))
                {
                    u8 = (U8) ucs4_try;
                }
                else
                {   /* save generic UCS-4 character as Unicode inline sequence */
                    status = ownform_save_unicode_inline(p_of_op_format, ucs4);
                    continue;
                }
            }
        }
#endif /* USTR_IS_SBSTR */

        /* NB output Latin-1 bytes coded exactly as before for file compatibility */

        switch(u8)
        {
        case OF_CONSTRUCT_ARG_SEPARATOR:
            status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_ESCAPE_STR OF_ESCAPE_STR OF_CONSTRUCT_ARG_SEPARATOR_STR, 4);
            break;

        case OF_ESCAPE_CHAR:
            status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_ESCAPE_STR OF_ESCAPE_STR OF_ESCAPE_STR, 4);
            break;

        case OF_CONSTRUCT_START:
            status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_CONSTRUCT_START_STR, 2);
            break;

        case OF_CONSTRUCT_END:
            status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_CONSTRUCT_END_STR, 2);
            break;

        /* translate C0/C1/DEL into more visible/editable tbs forms:
         * extreme care as there are other cases of this code around here, in the TSTR/RAW code
        */
        case 0x7F:
            status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR "\xFF", 2); /* this is the odd-one-out */
            break;

        default:
            if((u8 & 0x7F) <= 0x1F) /* C0 and C1 */
                status = ownform_save_ctrl_char(p_of_op_format, u8);
            else
                status = ownform_write_byte(p_of_op_format, u8);
            break;
        }
    }

    return(status);
}

#if defined(ARG_TYPE_TSTR_DISTINCT)

_Check_return_
static STATUS
ownform_save_tstr(
    _Inout_     P_OF_OP_FORMAT p_of_op_format,
    _In_z_      PCTSTR tstr)
{
    STATUS status = STATUS_OK;

    while(status_ok(status))
    {
        TCHAR tchar = *tstr++;

        if(tchar < 256)
        {
            U8 u8 = (U8) tchar;

            if(CH_NULL == u8)
                break;

            assert(CH_INLINE != u8);

            /* NB output Latin-1 bytes coded exactly as before for file compatibility */

            switch(u8)
            {
            case OF_CONSTRUCT_ARG_SEPARATOR:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_ESCAPE_STR OF_ESCAPE_STR OF_CONSTRUCT_ARG_SEPARATOR_STR, 4);
                break;

            case OF_ESCAPE_CHAR:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_ESCAPE_STR OF_ESCAPE_STR OF_ESCAPE_STR, 4);
                break;

            case OF_CONSTRUCT_START:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_CONSTRUCT_START_STR, 2);
                break;

            case OF_CONSTRUCT_END:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_CONSTRUCT_END_STR, 2);
                break;

            /* translate C0/C1/DEL into more visible/editable tbs forms:
             * extreme care as there are other cases of this code around here, in the USTR/RAW code
            */
            case 0x7F:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR "\xFF", 2); /* this is the odd-one-out */
                break;

            default:
                if((u8 & 0x7F) <= 0x1F) /* C0 and C1 */
                    status = ownform_save_ctrl_char(p_of_op_format, u8);
                else
                    status = ownform_write_byte(p_of_op_format, u8);
                break;
            }
        }
        else
        {
            status = ownform_save_unicode_inline(p_of_op_format, tchar);
        }
    }

    return(status);
}

#endif /* defined(ARG_TYPE_TSTR_DISTINCT) */

_Check_return_
static STATUS
ownform_save_raw(
    _Inout_     P_OF_OP_FORMAT p_of_op_format,
    _InRef_     PC_ARRAY_HANDLE p_array_handle)
{
    STATUS status = STATUS_OK;
    S32 raw_data_s32 = array_elements32(p_array_handle) * array_element_size32(p_array_handle);

    if(0 == raw_data_s32)
        return(STATUS_OK);

    /* raw data preceded immediately by length and comma */
    if( status_ok(status = ownform_save_string_s32(p_of_op_format, raw_data_s32)) && /* read by fast_strtol - see of_load */
        status_ok(status = ownform_write_byte(p_of_op_format, OF_CONSTRUCT_ARG_SEPARATOR))   )
    {
        PC_U8 val = array_rangec(p_array_handle, U8, 0, raw_data_s32);

        do  {
            U8 ch = *val++;

            switch(ch)
            {
            /* OF_CONSTRUCT_ARG_SEPARATOR: will never be looking for this in RAW arg (which must be at end) */

            case OF_ESCAPE_CHAR:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_ESCAPE_STR, 2);
                break;

            case OF_CONSTRUCT_START:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_CONSTRUCT_START_STR, 2);
                break;

            case OF_CONSTRUCT_END:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR OF_CONSTRUCT_END_STR, 2);
                break;

            /* translate C0/C1/DEL into more visible/editable tbs forms:
             * extreme care as there are other cases of this code around here, in the TSTR/USTR code
            */
            case 0x7F:
                status = ownform_write_bytes(p_of_op_format, OF_ESCAPE_STR "\xFF", 2); /* this is the odd-one-out */
                break;

            default:
                if((ch & 0x7F) <= 0x1F) /* C0 and C1 */
                    status = ownform_save_ctrl_char(p_of_op_format, ch);
                else
                    status = ownform_write_byte(p_of_op_format, ch);
                break;
            }

            status_break(status);
        }
        while(--raw_data_s32 > 0);
    }

    return(status);
}

_Check_return_
extern STATUS
ownform_save_arglist(
    _InVal_     ARGLIST_HANDLE arglist_handle,
    _InVal_     OBJECT_ID object_id,
    _InRef_     PC_CONSTRUCT_TABLE p_construct_table,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    U32 n_args = n_arglist_args(&arglist_handle);

    status_return(ownform_write_byte( p_of_op_format, OF_CONSTRUCT_START));
    status_return(ownform_write_bytes(p_of_op_format, p_construct_table->a7str_construct_name, strlen32(p_construct_table->a7str_construct_name)));

    /* trim excess args from end */
    if(0 != n_args)
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&arglist_handle, n_args);
        BOOL looping = TRUE;

        for(;;)
            {
            const PC_ARGLIST_ARG p_arg = &p_args[n_args - 1];

            switch(p_arg->type & ~ARG_ALLOC)
            {
            case ARG_TYPE_NONE:
                break;

            /* SKS 27may93 after 1.03: strip trailing 'empty' non-mandatory strings too */
            case ARG_TYPE_USTR:
            case ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK: /* yes, even these - it's the load code that has to bother */
                if((P_DATA_NONE != p_arg->val.ustr) && (NULL != p_arg->val.ustr) && (CH_NULL != PtrGetByte(p_arg->val.ustr)))
                    looping = FALSE;
                break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
            case ARG_TYPE_TSTR:
            case ARG_TYPE_TSTR | ARG_MANDATORY_OR_BLANK: /* yes, even these - it's the load code that has to bother */
                if((P_DATA_NONE != p_arg->val.tstr) && (NULL != p_arg->val.tstr) && (CH_NULL != *p_arg->val.tstr))
                    looping = FALSE;
                break;
#endif /* ARG_TYPE_TSTR_DISTINCT */

            default:
                looping = FALSE;
                break;
            }

            if(!looping)
                break;

            if(0 == n_args)
                break;

            --n_args; /* that one was trimmed */
        }
    }

#if CHECKING && 1
    {
    U32 arg_idx = 0;
    for(;;)
    {
        if(!p_construct_table->args || (p_construct_table->args[arg_idx] == ARG_TYPE_NONE))
        {
            myassert3x(n_arglist_args(&arglist_handle) <= arg_idx,
                      TEXT("Construct %s has got more args (") U32_TFMT TEXT(") than is required (") U32_TFMT TEXT(")"), report_sbstr(p_construct_table->a7str_construct_name), n_arglist_args(&arglist_handle), arg_idx);
            break;
        }

        if(p_construct_table->args[arg_idx] & ARG_MANDATORY)
        {
            if(pc_arglist_arg(&arglist_handle, arg_idx)->type == ARG_TYPE_NONE)
            {
                myassert2(TEXT("Construct %s mandatory arg ") U32_TFMT TEXT(" is missing (ARG_TYPE_NONE)"), report_sbstr(p_construct_table->a7str_construct_name), arg_idx);
            }
            else if(arg_idx >= n_args)
            {
                switch(p_construct_table->args[arg_idx])
                {
                default:
                    myassert3(TEXT("Construct %s mandatory arg ") U32_TFMT TEXT(" is missing (off the end ") U32_TFMT TEXT(")"), report_sbstr(p_construct_table->a7str_construct_name), arg_idx, n_args);
                    break;

                case ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK:
                    break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
                case ARG_TYPE_TSTR | ARG_MANDATORY_OR_BLANK:
                    break;
#endif /* ARG_TYPE_TSTR_DISTINCT */
                }
            }
        }

        ++arg_idx;
    }
    } /*block*/
#endif

    if(OBJECT_ID_SKEL != object_id)
    {
        status_return(ownform_write_byte(p_of_op_format, OF_CONSTRUCT_OBJECT_SEPARATOR));
        status_return(ownform_write_byte(p_of_op_format, construct_id_from_object_id(object_id)));
    }

    {
    U8 sep_ch = OF_CONSTRUCT_ARGLIST_SEPARATOR;
    STATUS status = STATUS_OK;
    U32 current_arg_idx;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&arglist_handle, n_args);

    for(current_arg_idx = 0; current_arg_idx < n_args; current_arg_idx++)
    {
        const PC_ARGLIST_ARG p_arg = &p_args[current_arg_idx];

        status_break(status = ownform_write_byte(p_of_op_format, sep_ch));

        sep_ch = OF_CONSTRUCT_ARG_SEPARATOR;

        switch(p_arg->type & ARG_TYPE_MASK)
        {
        default:
            myassert2(TEXT("Unknown data type ") S32_TFMT TEXT(" in arglist for %s"), p_arg->type, report_sbstr(p_construct_table->a7str_construct_name));
            status = STATUS_OK;
            break;

        case ARG_TYPE_NONE:
            /* NULL arg? */
            continue;

        case ARG_TYPE_U8C:
            assert(OF_CONSTRUCT_ARG_SEPARATOR != p_arg->val.u8c);
            assert(OF_ESCAPE_CHAR != p_arg->val.u8c);
            assert(OF_CONSTRUCT_START != p_arg->val.u8c);
            assert(OF_CONSTRUCT_END != p_arg->val.u8c);
            status = ownform_write_byte(p_of_op_format, p_arg->val.u8c);
            break;

        case ARG_TYPE_U8N:
            status = ownform_save_string_s32(p_of_op_format, (S32) p_arg->val.u8n); /* read by fast_strtol - see of_load */
            break;

        case ARG_TYPE_BOOL:
            status = ownform_write_byte(p_of_op_format, p_arg->val.fBool ? '1' : CH_DIGIT_ZERO); /* read by fast_strtol - see of_load */
            break;

        case ARG_TYPE_S32:
            status = ownform_save_string_s32(p_of_op_format, p_arg->val.s32); /* read by fast_strtol - see of_load */
            break;

        case ARG_TYPE_X32:
            status = ownform_save_string_x32(p_of_op_format, p_arg->val.x32); /* read by strtoul - see of_load */
            break;

        case ARG_TYPE_COL: /* offseted at load time - small-bit cols need different save! */
            status = ownform_save_string_s32(p_of_op_format, (S32) p_arg->val.col); /* read by fast_strtol - see of_load */
            break;

        case ARG_TYPE_ROW: /* offseted at load time */
            status = ownform_save_string_s32(p_of_op_format, p_arg->val.s32); /* read by fast_strtol - see of_load */
            break;

        case ARG_TYPE_F64:
            status = ownform_save_string_f64(p_of_op_format, &p_arg->val.f64);
            break;

        case ARG_TYPE_RAW:
        case ARG_TYPE_RAW_DS:
            status = ownform_save_raw(p_of_op_format, &p_arg->val.raw);
            break;

        case ARG_TYPE_USTR:
            if(NULL == p_arg->val.ustr_inline)
                continue;

            status = ownform_save_ustr_inline(p_of_op_format, p_arg->val.ustr_inline, object_id);
            break;

#if defined(ARG_TYPE_TSTR_DISTINCT)
        case ARG_TYPE_TSTR:
            if(NULL == p_arg->val.tstr)
                continue;

            status = ownform_save_tstr(p_of_op_format, p_arg->val.tstr);
            break;
#endif /* ARG_TYPE_TSTR_DISTINCT */
        }

        status_break(status);
    }

    if(status_ok(status))
        if(status_ok(status = ownform_write_byte(p_of_op_format, OF_CONSTRUCT_END)))
            if(!p_construct_table->bits.supress_newline)
                status = ownform_output_newline(p_of_op_format);

    return(status);
    } /*block*/
}

/******************************************************************************
*
* Calls an object to decode an inline,
* returning an ARGLIST_HANDLE with arguments, object id and construct pointer
*
******************************************************************************/

_Check_return_
static STATUS
ownform_save_object_inline(
    _In_z_      PC_USTR_INLINE ustr_inline,
    _InVal_     OBJECT_ID object_id,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    const P_DOCU p_docu = p_docu_from_docno(p_of_op_format->docno);
    SAVE_CONSTRUCT_OWNFORM save_construct_of;
    STATUS status;

    save_construct_of.ustr_inline    = ustr_inline;
    save_construct_of.arglist_handle = 0;
    save_construct_of.p_construct    = NULL;
    save_construct_of.object_id      = object_id;

    if(status_fail(status = object_call_id(save_construct_of.object_id, p_docu, T5_MSG_SAVE_CONSTRUCT_OWNFORM, &save_construct_of)) && (status != STATUS_FAIL))
        save_construct_of.p_construct = NULL;

    if(!save_construct_of.p_construct)
    {
        /* inline not convertible by data owner - try others */
        OBJECT_ID try_object_id = OBJECT_ID_ENUM_START;

        while(status_ok(object_next(&try_object_id)))
        {
            if(try_object_id == object_id)
                continue;

            save_construct_of.object_id = try_object_id;

            if(status_fail(status = object_call_id(save_construct_of.object_id, p_docu, T5_MSG_SAVE_CONSTRUCT_OWNFORM, &save_construct_of)) && (status != STATUS_FAIL))
                save_construct_of.p_construct = NULL;

            if(save_construct_of.p_construct)
                break;
        }
    }

    myassert3x(status_ok(status), TEXT("inline ") S32_TFMT TEXT(" in object ") S32_TFMT TEXT(" data not converted; status = ") S32_TFMT, (S32) inline_code(ustr_inline), (S32) object_id, status);

    if(save_construct_of.p_construct)
        status = ownform_save_arglist(save_construct_of.arglist_handle, save_construct_of.object_id, save_construct_of.p_construct, p_of_op_format);

    arglist_dispose(&save_construct_of.arglist_handle);

    return((status != STATUS_FAIL) ? status : STATUS_OK /* don't stop the save process */);
}

_Check_return_
extern STATUS
ownform_output_newline(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
#if WINDOWS
    static const BYTE crlf[] = { CR, LF };
    return(ownform_write_bytes(p_of_op_format, crlf, sizeof32(crlf)));
#else
    return(ownform_write_byte(p_of_op_format, LF));
#endif
}

_Check_return_
extern STATUS
ownform_output_group_stt(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    PC_CONSTRUCT_TABLE p_construct_table = construct_table_lookup_message(OBJECT_ID_SKEL, T5_CMD_OF_GROUP);
    PTR_ASSERT(p_construct_table);
    status_return(ownform_write_byte( p_of_op_format, OF_CONSTRUCT_START));
    status_return(ownform_write_bytes(p_of_op_format, p_construct_table->a7str_construct_name, strlen32(p_construct_table->a7str_construct_name)));
           return(ownform_write_byte( p_of_op_format, OF_CONSTRUCT_ARGLIST_SEPARATOR));
}

_Check_return_
extern STATUS
ownform_output_group_end(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    status_return(ownform_write_byte(p_of_op_format, OF_CONSTRUCT_END));

    return(ownform_output_newline(p_of_op_format));
}

extern void
save_reflect_status(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     S32 percent)
{
    p_of_op_format->process_status.data.percent.current = percent;

    process_status_reflect(&p_of_op_format->process_status);
}

_Check_return_
static STATUS
save_cmd_of_data(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     T5_MESSAGE t5_message)
{
    STATUS status = STATUS_OK;
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    status_return(arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table));

    status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

    arglist_dispose(&arglist_handle);

    return(status);
}

/******************************************************************************
*
* save a file in ownform
*
******************************************************************************/

_Check_return_
extern STATUS
save_ownform_file(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    MSG_SAVE msg_save;

    msg_save.p_of_op_format = p_of_op_format;

    msg_save.t5_msg_save_message = T5_MSG_SAVE__PRE_SAVE_1;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    msg_save.t5_msg_save_message = T5_MSG_SAVE__PRE_SAVE_2;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    msg_save.t5_msg_save_message = T5_MSG_SAVE__PRE_SAVE_3;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    status_return(save_cmd_of_data(p_of_op_format, T5_CMD_OF_START_OF_DATA));

    msg_save.t5_msg_save_message = T5_MSG_SAVE__DATA_SAVE_1;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    msg_save.t5_msg_save_message = T5_MSG_SAVE__DATA_SAVE_2;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    msg_save.t5_msg_save_message = T5_MSG_SAVE__DATA_SAVE_3;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    status_return(save_cmd_of_data(p_of_op_format, T5_CMD_OF_END_OF_DATA));

    msg_save.t5_msg_save_message = T5_MSG_SAVE__POST_SAVE_1;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    msg_save.t5_msg_save_message = T5_MSG_SAVE__POST_SAVE_2;
    status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));

    /*msg_save.t5_msg_save_message = T5_MSG_SAVE__POST_SAVE_3; as yet unused */
    /*status_return(maeve_event(p_docu, T5_MSG_SAVE, &msg_save));*/

    return(STATUS_OK);
}

/******************************************************************************
*
* Cutting
*
******************************************************************************/

_Check_return_
extern STATUS
save_ownform_to_array_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _InRef_     PC_OF_TEMPLATE p_of_template,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    OF_OP_FORMAT of_op_format = OF_OP_FORMAT_INIT;
    STATUS status;

    of_op_format.process_status.flags.foreground  = 1;
    of_op_format.process_status.reason.type = UI_TEXT_TYPE_RESID;
    of_op_format.process_status.reason.text.resource_id = MSG_STATUS_CUTTING;

    of_op_format.of_template = *p_of_template;

    status_return(ownform_initialise_save(p_docu, &of_op_format, p_array_handle, NULL, FILETYPE_T5_FIREWORKZ, p_docu_area));

    status = save_ownform_file(p_docu, &of_op_format);

    return(ownform_finalise_save(&p_docu, &of_op_format, status));
}

/* end of of_save.c */
