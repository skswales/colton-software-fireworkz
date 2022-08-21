/* ff_save.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Foreign format file save routines for Fireworkz */

/* JAD Mar 1992; MRJC October 1992; SKS June 2014 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ff_io.h"

#include "cmodules/collect.h"

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

_Check_return_
extern STATUS
plain_write_newline(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
#if WINDOWS
    status_return(plain_write_ucs4_character(p_ff_op_format, CR));
           return(plain_write_ucs4_character(p_ff_op_format, LF));
#else
    return(plain_write_ucs4_character(p_ff_op_format, LF));
#endif
}

_Check_return_
static STATUS
write_ucs4_character_ASCII(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    if(ucs4_is_ascii7(ucs4))
    {
        return(binary_write_byte(p_op_format_output, (U8) ucs4));
    }

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("ASCII: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    {
    UCS4 ucs4_try = ucs4_to_sbchar_try_with_codepage(ucs4, p_op_format_output->sbchar_codepage);

    if(ucs4_is_ascii7(ucs4_try))
    {
        return(binary_write_byte(p_op_format_output, (U8) ucs4_try));
    }
    } /*block*/

    reportf(TEXT("ASCII: no representation for U+%.4X"), ucs4);
    return(binary_write_byte(p_op_format_output, CH_SPACE)); /* no 7-bit ASCII representation possible */
}

_Check_return_
static STATUS
write_ucs4_character_Latin_N(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    if(ucs4_is_ascii7(ucs4))
    {
        return(binary_write_byte(p_op_format_output, (U8) ucs4));
    }

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("Latin-N: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    {
    UCS4 ucs4_try = ucs4_to_sbchar_try_with_codepage(ucs4, p_op_format_output->sbchar_codepage);

    if(ucs4_is_sbchar(ucs4_try))
    {
        return(binary_write_byte(p_op_format_output, (U8) ucs4_try));
    }
    } /*block*/

    reportf(TEXT("Latin-N: no representation for U+%.4X"), ucs4);
    return(binary_write_byte(p_op_format_output, CH_SPACE)); /* no 8-bit Latin-N representation possible */
}

_Check_return_
static STATUS
write_ucs4_character_Translate_8(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("Translate_8: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    assert(0 != p_op_format_output->sbchar_codepage);

    {
    UCS4 ucs4_try = ucs4_to_sbchar_try_with_codepage(ucs4, p_op_format_output->sbchar_codepage);

    if(ucs4_is_sbchar(ucs4_try))
    {
        return(binary_write_byte(p_op_format_output, (U8) ucs4_try));
    }
    } /*block*/

    reportf(TEXT("Translate_8: no representation for U+%.4X"), ucs4);
    return(binary_write_byte(p_op_format_output, CH_SPACE)); /* no 8-bit translated representation possible */
}

_Check_return_
static STATUS
write_ucs4_character_UTF_8(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    if(ucs4_is_ascii7(ucs4))
    {
        return(binary_write_byte(p_op_format_output, (U8) ucs4));
    }

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-8: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    {
    UTF8B utf8_buffer[8];
    U32 bytes_of_char = utf8_char_encode(utf8_bptr(utf8_buffer), elemof32(utf8_buffer), ucs4);

    return(binary_write_bytes(p_op_format_output, utf8_buffer, bytes_of_char));
    } /*block*/
}

_Check_return_
static STATUS
write_ucs4_character_UTF_16LE(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    WCHAR high_surrogate, low_surrogate;
    U8 byte_write[4];
    U32 n_bytes;

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-16LE: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    if(ucs4 >= UCH_UCS2_INVALID)
    {
        utf16_char_encode_surrogates(&high_surrogate, &low_surrogate, ucs4);

        byte_write[0] = (U8) (high_surrogate >> 0);
        byte_write[1] = (U8) (high_surrogate >> 8);

        byte_write[2] = (U8) (low_surrogate >> 0);
        byte_write[3] = (U8) (low_surrogate >> 8);

        n_bytes = 4;
    }
    else
    {
        byte_write[1] = (U8) (ucs4 >> 8);
        byte_write[0] = (U8) (ucs4 >> 0);

        n_bytes = 2;
    }

    return(binary_write_bytes(p_op_format_output, byte_write, n_bytes));
}

_Check_return_
static STATUS
write_ucs4_character_UTF_16BE(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    WCHAR high_surrogate, low_surrogate;
    U8 byte_write[4];
    U32 n_bytes;

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-16BE: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    if(ucs4 >= UCH_UCS2_INVALID)
    {
        utf16_char_encode_surrogates(&high_surrogate, &low_surrogate, ucs4);

        byte_write[0] = (U8) (high_surrogate >> 8);
        byte_write[1] = (U8) (high_surrogate >> 0);

        byte_write[2] = (U8) (low_surrogate >> 8);
        byte_write[3] = (U8) (low_surrogate >> 0);

        n_bytes = 4;
    }
    else
    {
        byte_write[0] = (U8) (ucs4 >> 8);
        byte_write[1] = (U8) (ucs4 >> 0);

        n_bytes = 2;
    }

    return(binary_write_bytes(p_op_format_output, byte_write, n_bytes));
}

_Check_return_
static STATUS
write_ucs4_character_UTF_32LE(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    U8 byte_write[4];

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-32LE: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    byte_write[0] = (U8) (ucs4 >>  0);
    byte_write[1] = (U8) (ucs4 >>  8);
    byte_write[2] = (U8) (ucs4 >> 16);
    byte_write[3] = (U8) (ucs4 >> 24);

    return(binary_write_bytes(p_op_format_output, byte_write, 4));
}

_Check_return_
static STATUS
write_ucs4_character_UTF_32BE(
    _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
    _InVal_     UCS4 ucs4)
{
    U8 byte_write[4];

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-32BE: no representation for invalid U+%.4X"), ucs4);
        return(STATUS_OK);
    }

    byte_write[0] = (U8) (ucs4 >> 24);
    byte_write[1] = (U8) (ucs4 >> 16);
    byte_write[2] = (U8) (ucs4 >>  8);
    byte_write[3] = (U8) (ucs4 >>  0);

    return(binary_write_bytes(p_op_format_output, byte_write, 4));
}

_Check_return_
extern STATUS
plain_write_ucs4_character(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     UCS4 ucs4)
{
    return((* p_ff_op_format->write_ucs4_character_fn) (&p_ff_op_format->of_op_format.output, ucs4));
}

_Check_return_
static STATUS
foreign_save_write_BOM(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    return(plain_write_ucs4_character(p_ff_op_format, 0xFEFFU));
}

_Check_return_
extern STATUS
foreign_save_set_io_type(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     IO_TYPE io_type)
{
    STATUS status = STATUS_OK;

    p_ff_op_format->io_type = io_type;

    switch(io_type)
    {
    default: default_unhandled();
#if CHECKING
        /*FALLTHRU*/
    case IO_TYPE_LATIN_1:
    case IO_TYPE_LATIN_2:
    case IO_TYPE_LATIN_3:
    case IO_TYPE_LATIN_4:
    case IO_TYPE_LATIN_9:
#endif
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_Latin_N;
        break;

    case IO_TYPE_ASCII:
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_ASCII;
        break;

    case IO_TYPE_TRANSLATE_8:
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_Translate_8;
        break;

    case IO_TYPE_UTF_8:
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_UTF_8;
        /* No BOM */
        break;

    case IO_TYPE_UTF_16LE:
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_UTF_16LE;
        status = foreign_save_write_BOM(p_ff_op_format);
        break;

    case IO_TYPE_UTF_16BE:
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_UTF_16BE;
        status = foreign_save_write_BOM(p_ff_op_format);
        break;

    case IO_TYPE_UTF_32LE:
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_UTF_32LE;
        status = foreign_save_write_BOM(p_ff_op_format);
        break;

    case IO_TYPE_UTF_32BE:
        p_ff_op_format->write_ucs4_character_fn = write_ucs4_character_UTF_32BE;
        status = foreign_save_write_BOM(p_ff_op_format);
        break;
    }

    return(status);
}

_Check_return_
extern STATUS
plain_write_uchars(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    U32 offset = 0;

    while(offset < uchars_n)
    {
        U32 bytes_of_char;
        UCS4 ucs4 = uchars_char_decode_off(uchars, offset, /*ref*/bytes_of_char);

        assert(CH_INLINE != ucs4);

        status_return(plain_write_ucs4_character(p_ff_op_format, ucs4));

        offset += bytes_of_char;
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
plain_write_ustr(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_USTR ustr)
{
    return(plain_write_uchars(p_ff_op_format, ustr, ustrlen32(ustr)));
}

_Check_return_
extern STATUS
plain_write_tchars(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(tchars_n) PCTCH tchars,
    _InVal_     U32 tchars_n)
{
    U32 offset = 0;

    while(offset < tchars_n)
    {
        UCS4 ucs4 = tchars[offset];

        status_return(plain_write_ucs4_character(p_ff_op_format, ucs4));

        ++offset;
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
plain_write_tstr(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PCTSTR tstr)
{
    return(plain_write_tchars(p_ff_op_format, tstr, tstrlen32(tstr)));
}

/******************************************************************************
*
* Finalise a save from ff_op_format (tidy up stuff e.g. close file etc.)
*
******************************************************************************/

_Check_return_
extern STATUS
foreign_finalise_save(
    _OutRef_opt_ P_P_DOCU p_p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     STATUS save_status)
{
    return(ownform_finalise_save(p_p_docu, (P_OF_OP_FORMAT) p_ff_op_format, save_status));
}

_Check_return_
extern STATUS
foreign_initialise_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _In_opt_z_  PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InRef_opt_ PC_DOCU_AREA p_docu_area)
{
    STATUS status;

    status = ownform_initialise_save(p_docu, (P_OF_OP_FORMAT) p_ff_op_format, p_array_handle, filename, t5_filetype, p_docu_area);

    if(status_ok(status))
        status = foreign_save_set_io_type(p_ff_op_format, IO_TYPE_LATIN_1); /* default must be a type that doesn't write a BOM! */

    if(status_fail(status))
        (void) foreign_finalise_save(&p_docu, p_ff_op_format, status);

    return(status);
}

/******************************************************************************
*
* Cutting
*
******************************************************************************/

_Check_return_
extern STATUS
save_foreign_to_array_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _InVal_     T5_FILETYPE t5_filetype,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    FF_OP_FORMAT ff_op_format = { OP_OUTPUT_INVALID };
    STATUS status;

    /* lookup the object to use to save this type of data */
    OBJECT_ID object_id = OBJECT_ID_NONE;

    {
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&installed_save_objects_handle); ++i)
    {
        P_INSTALLED_SAVE_OBJECT p_installed_save_object = array_ptr(&installed_save_objects_handle, INSTALLED_SAVE_OBJECT, i);

        if(t5_filetype == p_installed_save_object->t5_filetype)
        {
            object_id = p_installed_save_object->object_id;
            break;
        }
    }
    } /*block*/

    assert(OBJECT_ID_NONE != object_id);

    ff_op_format.of_op_format.process_status.flags.foreground  = 1;
    ff_op_format.of_op_format.process_status.reason.type = UI_TEXT_TYPE_RESID;
    ff_op_format.of_op_format.process_status.reason.text.resource_id = MSG_STATUS_CUTTING;

    status_return(foreign_initialise_save(p_docu, &ff_op_format, p_array_handle, NULL, t5_filetype, p_docu_area));

    {
    MSG_SAVE_FOREIGN msg_save_foreign;
    msg_save_foreign.t5_filetype = t5_filetype;
    msg_save_foreign.p_ff_op_format = &ff_op_format;
    status = object_call_id_load(p_docu, T5_MSG_SAVE_FOREIGN, &msg_save_foreign, object_id);
    } /*block*/

    return(foreign_finalise_save(&p_docu, &ff_op_format, status));
}

/* end of ff_save.c */
