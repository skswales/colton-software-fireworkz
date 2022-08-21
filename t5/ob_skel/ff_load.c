/* ff_load.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Foreign format file load routines for Fireworkz */

/* JAD Mar 1992; SKS Jun 2014 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ff_io.h"

#include "cmodules/collect.h"

#include "cmodules/cfbf.h"

#include "ob_skel/ho_print.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#ifndef          __utf8_h
#include "cmodules/utf8.h"
#endif

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

/*
foreign
*/

static ARRAY_HANDLE g_bound_filetype_handle;

static ARRAY_HANDLE g_installed_load_objects_handle;

/*
reading
*/

static inline void
plain_unread_ucs4_character(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _InVal_     UCS4 char_read)
{
    if(char_read != EOF_READ)
    {
        assert(EOF_READ == p_ip_format_input->ungotchar);
        p_ip_format_input->ungotchar = char_read;
    }
}

_Check_return_
extern STATUS
plain_read_ucs4_character(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _OutRef_    P_UCS4 p_char_read)
{
    P_IP_FORMAT_INPUT p_ip_format_input = &p_ff_ip_format->of_ip_format.input;
    UCS4 char_read, next_char_read;
    STATUS status, next_status;

    if(EOF_READ != p_ip_format_input->ungotchar) /* plain_unread_ucs4_character() is only now called from plain_read_ucs4_character() */
    {
        char_read = p_ip_format_input->ungotchar;
        p_ip_format_input->ungotchar = EOF_READ;
        status = STATUS_OK; /* consider case of xx,LF,LF,CR,yy */
    }
    else
    {
        status = (* p_ff_ip_format->read_ucs4_character_fn) (p_ip_format_input, &char_read);
    }

    if((LF != char_read) && (CR != char_read)) /* i.e. or error */
    {
        *p_char_read = char_read;
        return(status);
    }

    /* handles admixtures of LF, CR, LF/CR, CR/LF as line separators, returning EOF_READ instead of the last line separator */
    next_status = (* p_ff_ip_format->read_ucs4_character_fn) (p_ip_format_input, &next_char_read);

    if(status_fail(next_status) || (EOF_READ == next_status))
    {   /* always return EOF_READ instead of last line separator read */
        *p_char_read = EOF_READ;
        return(next_status);
    }

    if((LF ^ CR) != (next_char_read ^ char_read))
    {   /* not the expected possible complementary character, put it back to be read next time round */
        plain_unread_ucs4_character(p_ip_format_input, next_char_read);
        *p_char_read = LF;
        return(STATUS_OK);
    }

    /* found the expected possible complementary character: now check that it's not at EOF */
    next_status = (* p_ff_ip_format->read_ucs4_character_fn) (p_ip_format_input, &next_char_read);

    if(status_fail(next_status) || (EOF_READ == next_status))
    {   /* always return EOF_READ instead of last line separator read */
        *p_char_read = EOF_READ;
        return(next_status);
    }

    /* not yet at EOF, put it back to be read next time round */
    plain_unread_ucs4_character(p_ip_format_input, next_char_read);
    *p_char_read = LF;
    return(STATUS_OK);
}

_Check_return_
static STATUS
read_ucs4_character_ASCII(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status;
    UCS4 ucs4;
    U8 byte_read = binary_read_byte(p_ip_format_input, &status);

    if(status_fail(status) || (EOF_READ == status))
    {
        *p_char_read = EOF_READ;
        return(status);
    }

    ucs4 = ((UCS4) byte_read & 0x7F); /* ignore bit 7 */

    *p_char_read = ucs4;
    return(STATUS_OK);
}

_Check_return_
static STATUS
read_ucs4_character_Latin_N(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status;
    UCS4 ucs4;
    U8 byte_read = binary_read_byte(p_ip_format_input, &status);

    if(status_fail(status) || (EOF_READ == status))
    {
        *p_char_read = EOF_READ;
        return(status);
    }

    ucs4 = (UCS4) byte_read;

    if(!ucs4_is_ascii7(ucs4) && (0 != p_ip_format_input->sbchar_codepage)) /* leave top bit set characters untranslated unless explicitly set for now */
        ucs4 = ucs4_from_sbchar_with_codepage(byte_read, p_ip_format_input->sbchar_codepage);

    *p_char_read = ucs4;
    return(STATUS_OK);
}

_Check_return_
static STATUS
read_ucs4_character_Translate_8(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status;
    UCS4 ucs4;
    U8 byte_read = binary_read_byte(p_ip_format_input, &status);

    if(status_fail(status) || (EOF_READ == status))
    {
        *p_char_read = EOF_READ;
        return(status);
    }

    ucs4 = ucs4_from_sbchar_with_codepage(byte_read, p_ip_format_input->sbchar_codepage);

    *p_char_read = ucs4;
    return(STATUS_OK);
}

_Check_return_
static STATUS
read_ucs4_character_UTF_8(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status = STATUS_OK;
    UCS4 ucs4 = 0;
    UTF8B byte_read[4];
    U32 bytes_of_char = 0;
    int i;

    /*byte_read[0] = CH_NULL;*/

    for(i = 0; i <= 3; ++i) /* might have to read up to four 8-bit values */
    {
        byte_read[i] = binary_read_byte(p_ip_format_input, &status);

        if(status_fail(status) || (EOF_READ == status))
        {
            if((EOF_READ == status) && (0 != i))
                /* insufficient UTF-8 trail bytes read */
                reportf(TEXT("UTF-8: EOF when only %u bytes read when %u were needed"), i, bytes_of_char);
            *p_char_read = EOF_READ;
            return(status);
        }

        if(0 != i)
        {   /* should only encounter UTF-8 trail bytes */
            if(!u8_is_utf8_trail_byte(byte_read[i]))
            {
                reportf(TEXT("UTF-8: 0x%.2X is not a UTF-8 trail byte: junking %d bytes read up to this point and retrying"), byte_read[i], i);
                /* can't work out how we could output CH_REPLACEMENT_CHARACTER followed by the new char without overcomplicating */
                byte_read[0] = byte_read[i];
                i = 0;
            }
        }

        if(0 == i)
        {   /* should only encounter top-bit-clear or UTF-8 lead bytes */
            if((0 != (0x80 & byte_read[0])) && !u8_is_utf8_lead_byte(byte_read[0]))
            {   /* simply treat as Latin-1 */
                reportf(TEXT("UTF-8: 0x%.2X is a top-bit-set byte that's not a UTF-8 lead byte: treating as Latin-N"), byte_read[i]);
                ucs4 = ucs4_from_sbchar_with_codepage(byte_read[0], get_system_codepage());
                *p_char_read = ucs4;
                return(STATUS_OK);
            }

            bytes_of_char = utf8_bytes_of_char(utf8_bptr(byte_read));
        }

        if((U32) (i + 1) == bytes_of_char)
            /* got enough bytes to try making a character */
            break;
    }

    ucs4 = utf8_char_decode_NULL(utf8_bptr(byte_read));
    assert(utf8_bytes_of_char_encoding(ucs4) == bytes_of_char); /* overlong? */

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-8: U+%.4X not valid: junked"), ucs4);
        *p_char_read = UCH_REPLACEMENT_CHARACTER;
        return(STATUS_OK);
    }

    *p_char_read = ucs4;
    return(STATUS_OK);
}

_Check_return_
static STATUS
read_ucs4_character_UTF_16LE(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status = STATUS_OK;
    UCS4 ucs4 = 0;
    WCHAR char_read[2];
    int i;

    for(i = 0; i <= 1; i++) /* might have to read two 16-bit values */
    {
        U8 byte_read[2];

        byte_read[0] = binary_read_byte(p_ip_format_input, &status);

        if(status_fail(status) || (EOF_READ == status))
        {
            if((EOF_READ == status) && (0 != i))
                reportf(TEXT("UTF-16LE: EOF when only %u bytes read when %u were needed"), 2*i + 0, 2*i + 2);
            *p_char_read = EOF_READ;
            return(status);
        }

        byte_read[1] = binary_read_byte(p_ip_format_input, &status);

        if(status_fail(status) || (EOF_READ == status))
        {
            if(EOF_READ == status)
                reportf(TEXT("UTF-16LE: EOF when only %u bytes read when %u were needed"), 2*i + 1, 2*i + 2);
            *p_char_read = EOF_READ;
            return(status);
        }

        char_read[i] = /* NB UTF-16LE */
            ((WCHAR) byte_read[1] << 8) |
            ((WCHAR) byte_read[0] << 0) ;

        if(0 == i)
        {
            if((char_read[0] >= UCH_SURROGATE_HIGH) && (char_read[0] <= UCH_SURROGATE_HIGH_END))
            {   /* high-surrogate: must loop to get another low-surrogate character to go with this */
                continue;
            }

            if((char_read[0] >= UCH_SURROGATE_LOW) && (char_read[0] <= UCH_SURROGATE_LOW_END))
            {
                reportf(TEXT("UTF-16LE: lone low-surrogate 0x%.4X: junked"), char_read[0]);
                *p_char_read = UCH_REPLACEMENT_CHARACTER;
                return(STATUS_OK);
            }

            /* !surrogate */
            ucs4 = char_read[0];
            break;
        }

        if((char_read[i] >= UCH_SURROGATE_LOW) && (char_read[i] <= UCH_SURROGATE_LOW_END))
        {   /* combine together */
            ucs4 = utf16_char_decode_surrogates(char_read[0], char_read[1]);
            break;
        }

        reportf(TEXT("UTF-16LE: high-surrogate 0x%.4X followed by !low-surrogate 0x%.4X : junked both"), char_read[0], char_read[1]);
        *p_char_read = UCH_REPLACEMENT_CHARACTER;
        return(STATUS_OK);
    }

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-16LE: U+%.4X not valid: junked"), ucs4);
        *p_char_read = UCH_REPLACEMENT_CHARACTER;
        return(STATUS_OK);
    }

    *p_char_read = ucs4;
    return(STATUS_OK);
}

_Check_return_
static STATUS
read_ucs4_character_UTF_16BE(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status = STATUS_OK;
    UCS4 ucs4 = 0;
    WCHAR char_read[2];
    int i;

    for(i = 0; i <= 1; i++) /* might have to read two 16-bit values */
    {
        U8 byte_read[2];

        byte_read[0] = binary_read_byte(p_ip_format_input, &status);

        if(status_fail(status) || (EOF_READ == status))
        {
            if((EOF_READ == status) && (0 != i))
                reportf(TEXT("UTF-16BE: EOF when only %u bytes read when %u were needed"), 2*i + 0, 2*i + 2);
            *p_char_read = EOF_READ;
            return(status);
        }

        byte_read[1] = binary_read_byte(p_ip_format_input, &status);

        if(status_fail(status) || (EOF_READ == status))
        {
            if(EOF_READ == status)
                reportf(TEXT("UTF-16BE: EOF when only %u bytes read when %u were needed"), 2*i + 1, 2*i + 2);
            *p_char_read = EOF_READ;
            return(status);
        }

        char_read[i] = /* NB UTF-16BE */
            ((WCHAR) byte_read[0] << 8) |
            ((WCHAR) byte_read[1] << 0) ;

        if(0 == i)
        {
            if((char_read[0] >= UCH_SURROGATE_HIGH) && (char_read[0] <= UCH_SURROGATE_HIGH_END))
            {   /* high-surrogate: must loop to get another low-surrogate character to go with this */
                continue;
            }

            if((char_read[0] >= UCH_SURROGATE_LOW) && (char_read[0] <= UCH_SURROGATE_LOW_END))
            {
                reportf(TEXT("UTF-16BE: lone low-surrogate 0x%.4X: junked"), char_read[0]);
                *p_char_read = UCH_REPLACEMENT_CHARACTER;
                return(STATUS_OK);
            }

            /* !surrogate */
            ucs4 = char_read[0];
            break;
        }

        if((char_read[i] >= UCH_SURROGATE_LOW) && (char_read[i] <= UCH_SURROGATE_LOW_END))
        {   /* combine together */
            ucs4 = utf16_char_decode_surrogates(char_read[0], char_read[1]);
            break;
        }

        reportf(TEXT("UTF-16BE: high-surrogate 0x%.4X followed by !low-surrogate 0x%.4X: junked both"), char_read[0], char_read[1]);
        *p_char_read = UCH_REPLACEMENT_CHARACTER;
        return(STATUS_OK);
    }

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-16BE: U+%.4X not valid: junked"), ucs4);
        *p_char_read = UCH_REPLACEMENT_CHARACTER;
        return(STATUS_OK);
    }

    *p_char_read = ucs4;
    return(STATUS_DONE);
}

_Check_return_
static STATUS
read_ucs4_character_UTF_32LE(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status = STATUS_OK;
    UCS4 ucs4;
    U8 byte_read[4];
    int i;

    /* simply have to read four 8-bit values */
    for(i = 0; i < 4; i++)
    {
        byte_read[i] = binary_read_byte(p_ip_format_input, &status);

        if(status_fail(status) || (EOF_READ == status))
        {
            if((EOF_READ == status) && (0 != i))
                reportf(TEXT("UTF-32LE: EOF when only %u bytes read when 4 were needed"), i);
            *p_char_read = EOF_READ;
            return(status);
        }
    }

    ucs4 = /* NB UTF-32LE */
        ((UCS4) byte_read[3] << 24) |
        ((UCS4) byte_read[2] << 16) |
        ((UCS4) byte_read[1] <<  8) |
        ((UCS4) byte_read[0] <<  0) ;

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-32LE: U+%.4X [0:0x%.2X 1:0x%.2X 2:0x%.2X 3:0x%.2X] not valid: junked"), ucs4, byte_read[0], byte_read[1], byte_read[2], byte_read[3]);
        *p_char_read = UCH_REPLACEMENT_CHARACTER;
        return(STATUS_OK);
    }

    *p_char_read = ucs4;
    return(STATUS_DONE);
}

_Check_return_
static STATUS
read_ucs4_character_UTF_32BE(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
    _OutRef_    P_UCS4 p_char_read)
{
    STATUS status = STATUS_OK;
    UCS4 ucs4;
    U8 byte_read[4];
    int i;

    /* simply have to read four 8-bit values */
    for(i = 0; i < 4; i++)
    {
        byte_read[i] = binary_read_byte(p_ip_format_input, &status);

        if(status_fail(status) || (EOF_READ == status))
        {
            if((EOF_READ == status) && (0 != i))
                reportf(TEXT("UTF-32BE: EOF when only %u bytes read when 4 were needed"), i);
            *p_char_read = EOF_READ;
            return(status);
        }
    }

    ucs4 = /* NB UTF-32BE */
        ((UCS4) byte_read[0] << 24) |
        ((UCS4) byte_read[1] << 16) |
        ((UCS4) byte_read[2] <<  8) |
        ((UCS4) byte_read[3] <<  0) ;

    if(status_fail(ucs4_validate(ucs4)))
    {
        reportf(TEXT("UTF-32BE: 0x%.8X [0:0x%.2X 1:0x%.2X 2:0x%.2X 3:0x%.2X] not valid: junked"), ucs4, byte_read[0], byte_read[1], byte_read[2], byte_read[3]);
        *p_char_read = UCH_REPLACEMENT_CHARACTER;
        return(STATUS_OK);
    }

    *p_char_read = ucs4;
    return(STATUS_DONE);
}

/*
bound filetype handing
*/

typedef struct BOUND_FILETYPE
{
    QUICK_UBLOCK description_quick_ublock;    /* NB Can't have associated static_buffer as we will get realloc()ed */
    QUICK_UBLOCK extension_srch_quick_ublock; /* ditto */

    S32 mask;
    T5_FILETYPE t5_filetype;
}
BOUND_FILETYPE, * P_BOUND_FILETYPE; typedef const BOUND_FILETYPE * PC_BOUND_FILETYPE;

extern void
ff_load_msg_exit2(void)
{
    /* Ensure we dispose of all the quick_tblocks in the list */
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&g_bound_filetype_handle); i++)
    {
        P_BOUND_FILETYPE p_bound_filetype = array_ptr(&g_bound_filetype_handle, BOUND_FILETYPE, i);

        quick_ublock_dispose(&p_bound_filetype->description_quick_ublock);
        quick_ublock_dispose(&p_bound_filetype->extension_srch_quick_ublock);
    }

    al_array_dispose(&g_bound_filetype_handle);

    al_array_dispose(&g_installed_load_objects_handle);
}

/* Enumerate the bound file types within the system.
 *
 * When a file type is bound a mask, file type, textual
 * description, wildcard spec for the type is stored
 * within the array.
 *
 * When a client requests an enumeration, they specify
 * a mask which is then used for searching for the next type.
*/

_Check_return_
extern T5_FILETYPE
enumerate_bound_filetypes(
    _InoutRef_  P_ARRAY_INDEX p_array_index,
    _InVal_     S32 mask)
{
    if(*p_array_index == ENUMERATE_BOUND_FILETYPES_START)
        *p_array_index = 0;

    while(*p_array_index < array_elements(&g_bound_filetype_handle))
    {
        PC_BOUND_FILETYPE p_bound_filetype = array_ptr(&g_bound_filetype_handle, BOUND_FILETYPE, *p_array_index);

        *p_array_index += 1;

        if((p_bound_filetype->mask & mask) == mask)
            return(p_bound_filetype->t5_filetype);
    }

    *p_array_index = ENUMERATE_BOUND_FILETYPES_START;
    return(FILETYPE_UNDETERMINED);
}

_Check_return_
_Ret_z_
extern PC_USTR
description_ustr_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_BOOL p_found)
{
    static UCHARZ description_buffer[32];
    ARRAY_INDEX i = array_elements(&g_bound_filetype_handle);

    while(--i >= 0)
    {
        PC_BOUND_FILETYPE p_bound_filetype = array_ptrc(&g_bound_filetype_handle, BOUND_FILETYPE, i);

        if(p_bound_filetype->t5_filetype == t5_filetype)
        {
            assert(0 != quick_ublock_array_handle_ref(&p_bound_filetype->description_quick_ublock));
            *p_found = TRUE;
            return(quick_ublock_ustr(&p_bound_filetype->description_quick_ublock));
        }
    }

    *p_found = FALSE;
#if RISCOS
    {
    U8Z var_name_buffer[32];
    consume_int(xsnprintf(var_name_buffer, elemof32(var_name_buffer), "File$Type_%03X", t5_filetype));
    if(NULL == _kernel_getenv(var_name_buffer, description_buffer, elemof32(description_buffer)))
        consume_int(ustr_xsnprintf(ustr_bptr(description_buffer), elemof32(description_buffer), USTR_TEXT("Unknown [RISC OS &x03X]"), t5_filetype));
    } /*block*/
#else
    consume_int(ustr_xsnprintf(ustr_bptr(description_buffer), elemof32(description_buffer), USTR_TEXT("Unknown [RISC OS &%03X] (*.%3x)"), t5_filetype, t5_filetype));
#endif /* OS */
    return(ustr_bptr(description_buffer));
}

_Check_return_
_Ret_z_
extern PC_USTR
extension_srch_ustr_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_BOOL p_found)
{
    static UCHARZ extension_buffer[8];
    ARRAY_INDEX i = array_elements(&g_bound_filetype_handle);

    while(--i >= 0)
    {
        PC_BOUND_FILETYPE p_bound_filetype = array_ptrc(&g_bound_filetype_handle, BOUND_FILETYPE, i);

        if(p_bound_filetype->t5_filetype == t5_filetype)
        {
            assert(0 != quick_ublock_array_handle_ref(&p_bound_filetype->extension_srch_quick_ublock));
            *p_found = TRUE;
            return(quick_ublock_ustr(&p_bound_filetype->extension_srch_quick_ublock));
        }
    }

    *p_found = FALSE;
    consume_int(ustr_xsnprintf(ustr_bptr(extension_buffer), elemof32(extension_buffer), RISCOS ? USTR_TEXT("/x%03x") : USTR_TEXT("*.%03x"), t5_filetype));
    return(ustr_bptr(extension_buffer));
}

/* Bind a file type to its textual description.
 * This is used internally when displaying save boxes
 * and all that goo to map a RISC OS 12-bit file type
 * onto something that can be displayed to the user.
 */

T5_CMD_PROTO(extern, t5_cmd_bind_file_type)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 5);
    P_BOUND_FILETYPE p_bound_filetype;
    SC_ARRAY_INIT_BLOCK type_binding_init_block = aib_init(1, sizeof32(*p_bound_filetype), TRUE);
    STATUS status;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL != (p_bound_filetype = al_array_extend_by(&g_bound_filetype_handle, BOUND_FILETYPE, 1, &type_binding_init_block, &status)))
    {
        PC_USTR ustr_filetype_name = p_args[2].val.ustr;
        PC_USTR ustr_file_extension_desc = NULL;
        PC_USTR ustr_file_extension_srch = NULL;

        if(arg_is_present(p_args, 3))
            ustr_file_extension_desc = p_args[3].val.ustr;

        if(arg_is_present(p_args, 4))
            ustr_file_extension_srch = p_args[4].val.ustr;

        quick_ublock_setup_using_array(&p_bound_filetype->description_quick_ublock, 0); /* force it to use array as it will move */
        quick_ublock_setup_using_array(&p_bound_filetype->extension_srch_quick_ublock, 0); /* ditto */

        p_bound_filetype->mask = p_args[0].val.s32;
        p_bound_filetype->t5_filetype = p_args[1].val.t5_filetype;

        status = quick_ublock_ustr_add(&p_bound_filetype->description_quick_ublock, ustr_filetype_name);

#if WINDOWS
        if(status_ok(status) && (NULL != ustr_file_extension_desc))
        {
            if(status_ok(status = quick_ublock_a7char_add(&p_bound_filetype->description_quick_ublock, CH_SPACE)))
                status = quick_ublock_ustr_add(&p_bound_filetype->description_quick_ublock, ustr_file_extension_desc);
        }
#else
        UNREFERENCED_PARAMETER(ustr_file_extension_desc);
#endif

        if(status_ok(status))
            status = quick_ublock_nullch_add(&p_bound_filetype->description_quick_ublock);

#if WINDOWS
        if(status_ok(status) && (NULL != ustr_file_extension_srch))
            status = quick_ublock_ustr_add_n(&p_bound_filetype->extension_srch_quick_ublock, ustr_file_extension_srch, strlen_with_NULLCH);
#else
        UNREFERENCED_PARAMETER(ustr_file_extension_srch);
#endif

        if(status_fail(status))
            al_array_shrink_by(&g_bound_filetype_handle, -1);
    }

    return(status);
}

/*
installed load object handling
*/

typedef struct INSTALLED_LOAD_OBJECT
{
    OBJECT_ID object_id;
    T5_FILETYPE t5_filetype;
    TCHARZ tstr_buf_extension[3 + 1 /*for CH_NULL*/];
    PTSTR tstr_template;
}
INSTALLED_LOAD_OBJECT, * P_INSTALLED_LOAD_OBJECT; typedef const INSTALLED_LOAD_OBJECT * PC_INSTALLED_LOAD_OBJECT;

_Check_return_
_Ret_maybenull_
extern PCTSTR
foreign_template_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&g_installed_load_objects_handle); ++i)
    {
        PC_INSTALLED_LOAD_OBJECT p_installed_load_object = array_ptrc(&g_installed_load_objects_handle, INSTALLED_LOAD_OBJECT, i);

        if(p_installed_load_object->t5_filetype == t5_filetype)
            return(p_installed_load_object->tstr_template);
    }

    return(NULL);
}

_Check_return_
extern OBJECT_ID
object_id_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype)
{
    /* Can we load this filetype? */
    ARRAY_INDEX i;

    switch(t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
    case FILETYPE_T5_TEMPLATE:
    case FILETYPE_T5_COMMAND:
        return(OBJECT_ID_SKEL);

    default:
        break;
    }

    for(i = 0; i < array_elements(&g_installed_load_objects_handle); ++i)
    {
        PC_INSTALLED_LOAD_OBJECT p_installed_load_object = array_ptrc(&g_installed_load_objects_handle, INSTALLED_LOAD_OBJECT, i);

        if(p_installed_load_object->t5_filetype == t5_filetype)
            return(p_installed_load_object->object_id);
    }

    /* ugly temporary hack so we don't have to set huge config and it's dynamic */
    if(image_cache_can_import_with_image_convert(t5_filetype))
        return(OBJECT_ID_DRAW);

    return(OBJECT_ID_NONE);
}

_Check_return_
extern T5_FILETYPE
t5_filetype_from_extension(
    _In_opt_z_  PCTSTR tstr)
{
    if(tstr)
    {
        ARRAY_INDEX i = array_elements(&g_installed_load_objects_handle);

        while(--i >= 0)
        {
            PC_INSTALLED_LOAD_OBJECT p_installed_load_object = array_ptrc(&g_installed_load_objects_handle, INSTALLED_LOAD_OBJECT, i);

            if(0 == tstricmp(p_installed_load_object->tstr_buf_extension, tstr))
                return(p_installed_load_object->t5_filetype);
        }
    }

    return(FILETYPE_UNDETERMINED);
}

T5_CMD_PROTO(extern, t5_cmd_object_bind_loader)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 4);
    INSTALLED_LOAD_OBJECT installed_load_object;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(installed_load_object), 0);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    installed_load_object.object_id = p_args[0].val.object_id;
    installed_load_object.t5_filetype = p_args[1].val.t5_filetype;
    tstr_xstrkpy(installed_load_object.tstr_buf_extension, elemof32(installed_load_object.tstr_buf_extension), p_args[2].val.tstr);

    if(NULL == p_args[3].val.tstr)
        installed_load_object.tstr_template = NULL;
    else
        status_return(alloc_block_tstr_set(&installed_load_object.tstr_template, p_args[3].val.tstr, &p_docu->general_string_alloc_block));

    return(al_array_add(&g_installed_load_objects_handle, INSTALLED_LOAD_OBJECT, 1, &array_init_block, &installed_load_object));
}

_Check_return_
static T5_FILETYPE
detect_compound_document_file(
    _InoutRef_  FILE_HANDLE file_handle)
{
    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;
    STATUS status;
    P_COMPOUND_FILE p_compound_file = compound_file_create_from_file_handle(file_handle, &status);
    U32 directory_index;
    COMPOUND_FILE_DECODED_DIRECTORY * decoded_directory;

    if(NULL == p_compound_file)
        return(FILETYPE_UNDETERMINED);

    for(directory_index = 0, decoded_directory = p_compound_file->decoded_directory_list;
        directory_index < p_compound_file->decoded_directory_count;
        directory_index++, decoded_directory++)
    {
        /* Do wide string match (can't use L"" as wchar_t is int on Norcroft) */
        const WCHAR workbook_name[] = { 'W', 'o', 'r', 'k', 'b', 'o', 'o', 'k', CH_NULL }; /*L"Workbook"*/
        const WCHAR book_name[]     = { 'B', 'o', 'o', 'k', CH_NULL }; /*L"Book"*/

        if(0 == C_wcsicmp(decoded_directory->name.wchar, workbook_name))
        {
            t5_filetype = FILETYPE_MS_XLS;
            break;
        }

        if(0 == C_wcsicmp(decoded_directory->name.wchar, book_name))
        {
            t5_filetype = FILETYPE_MS_XLS;
            break;
        }
    }

    compound_file_dispose(&p_compound_file);

    return(t5_filetype);
}

_Check_return_
static int
try_memcmp32(
    _In_reads_bytes_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes,
    _In_reads_bytes_(n_bytes_compare) PC_BYTE p_data_compare,
    _InVal_     U32 n_bytes_compare)
{
    if(n_bytes < n_bytes_compare)
        return(-1); /* not enough data to compare with pattern */

    return(memcmp32(p_data, p_data_compare, n_bytes_compare));
}

_Check_return_
extern T5_FILETYPE
t5_filetype_from_data(
    _In_reads_bytes_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes)
{
    /* compare longest first */
    static const BYTE buffer_fireworkz[]    = { '{',    'V',    'e',    'r',    's',    'i',    'o',    'n',   ':' };
    static const BYTE buffer_jfif_00[]      = { '\xFF', '\xD8', /*SOI*/ '\xFF', '\xE0' /*APP0*/ };
    static const BYTE buffer_jfif_06[]      = { 'J',    'F',    'I',    'F',    '\x00' /*JFIF*/ };
    static const BYTE buffer_dib_00[]       = { 'B',    'M' };
    static const BYTE buffer_dib_0E[]       = { '\x28', '\x00', '\x00', '\x00' /*BITMAPINFOHEADER*/ };
    static const BYTE buffer_dibv4_0E[]     = { '\x6C', '\x00', '\x00', '\x00' /*BITMAPV4HEADER*/ };
    static const BYTE buffer_dibv5_0E[]     = { '\x7C', '\x00', '\x00', '\x00' /*BITMAPV5HEADER*/ };
    static const BYTE buffer_acorn_sprite[] = { '\x01', '\x00', '\x00', '\x00', '\x10', '\x00', '\x00', '\x00' }; /* assumes only one sprite and no extension area */
    static const BYTE buffer_excel_biff4w[] = { '\x09', '\x04', '\x06', '\x00', '\x00', '\x04', '\x00', '\x01' };
    static const BYTE buffer_excel_biff4[]  = { '\x09', '\x04', '\x06', '\x00', '\x00', '\x00', '\x02', '\x00' };
    static const BYTE buffer_excel_biff3[]  = { '\x09', '\x02', '\x06', '\x00', '\x00', '\x00', '\x02', '\x00' };
    static const BYTE buffer_excel_biff2[]  = { '\x09', '\x00', '\x06', '\x00', '\x00', '\x00', '\x02', '\x00' };
    static const BYTE buffer_png[]          = { '\x89', 'P',    'N',    'G',    '\x0D', '\x0A', '\x1A', '\x0A' };
    static const BYTE buffer_gif87a[]       = { 'G',    'I',    'F',    '8',    '7',    'a' };
    static const BYTE buffer_gif89a[]       = { 'G',    'I',    'F',    '8',    '9',    'a' };
    static const BYTE buffer_rtf[]          = { '{',    '\\',   'r',    't',    'f' };
    static const BYTE buffer_acorn_draw[]   = { 'D',    'r',    'a',    'w',    '\xC9' };
    static const BYTE buffer_pipedream[]    = { '%',    'O',    'P',    '%' };
    static const BYTE buffer_pipedream_2[]  = { '%',    'C',    'O',    ':' };
    static const BYTE buffer_tiff_LE[]      = { 'I',    'I',    '*',    '\x00' };
    static const BYTE buffer_tiff_BE[]      = { 'M',    'M',    '\x00', '*'    };
    static const BYTE buffer_lotus_wk1[]    = { '\x00', '\x00', '\x02', '\x00' /*BOF*/ };
    static const BYTE buffer_lotus_wk3[]    = { '\x00', '\x00', '\x1A', '\x00' /*BOF*/ };
    static const BYTE buffer_acorn_sid[]    = { '%',    '%' };

    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;
    
    if(0 == try_memcmp32(p_data, n_bytes, buffer_fireworkz, sizeof32(buffer_fireworkz)))
        t5_filetype = FILETYPE_T5_FIREWORKZ;
    else if( (n_bytes > (0x06 + sizeof32(buffer_jfif_06))) &&
             (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_jfif_00, sizeof32(buffer_jfif_00))) &&
             (0 == try_memcmp32(&p_data[0x06], n_bytes - 0x06, buffer_jfif_06, sizeof32(buffer_jfif_06))) )
        t5_filetype = FILETYPE_JPEG;
    else if( (n_bytes > (0x0E + sizeof32(buffer_dib_0E))) &&
             (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_dib_00, sizeof32(buffer_dib_00))) &&
             (0 == try_memcmp32(&p_data[0x0E], n_bytes - 0x0E, buffer_dib_0E, sizeof32(buffer_dib_0E))) )
        t5_filetype = FILETYPE_BMP;
    else if( (n_bytes > (0x0E + sizeof32(buffer_dibv4_0E))) &&
             (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_dib_00,   sizeof32(buffer_dib_00)))   &&
             (0 == try_memcmp32(&p_data[0x0E], n_bytes - 0x0E, buffer_dibv4_0E, sizeof32(buffer_dibv4_0E))) )
        t5_filetype = FILETYPE_BMP;
    else if( (n_bytes > (0x0E + sizeof32(buffer_dibv5_0E))) &&
             (0 == try_memcmp32(&p_data[0x00], n_bytes,        buffer_dib_00,   sizeof32(buffer_dib_00)))   &&
             (0 == try_memcmp32(&p_data[0x0E], n_bytes - 0x0E, buffer_dibv5_0E, sizeof32(buffer_dibv5_0E))) )
        t5_filetype = FILETYPE_BMP;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_acorn_sprite, sizeof32(buffer_acorn_sprite)))
        t5_filetype = FILETYPE_SPRITE;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff4w, sizeof32(buffer_excel_biff4w)))
        t5_filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff4, sizeof32(buffer_excel_biff4)))
        t5_filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff3, sizeof32(buffer_excel_biff3)))
        t5_filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_excel_biff2, sizeof32(buffer_excel_biff2)))
        t5_filetype = FILETYPE_MS_XLS;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_png, sizeof32(buffer_png)))
        t5_filetype = FILETYPE_PNG;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_gif87a, sizeof32(buffer_gif87a)))
        t5_filetype = FILETYPE_GIF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_gif89a, sizeof32(buffer_gif89a)))
        t5_filetype = FILETYPE_GIF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_rtf, sizeof32(buffer_rtf)))
        t5_filetype = FILETYPE_RTF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_acorn_draw, sizeof32(buffer_acorn_draw)))
        t5_filetype = FILETYPE_DRAW;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_pipedream, sizeof32(buffer_pipedream)))
        t5_filetype = FILETYPE_PIPEDREAM;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_pipedream_2, sizeof32(buffer_pipedream_2)))
        t5_filetype = FILETYPE_PIPEDREAM;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_tiff_LE, sizeof32(buffer_tiff_LE)))
        t5_filetype = FILETYPE_TIFF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_tiff_BE, sizeof32(buffer_tiff_BE)))
        t5_filetype = FILETYPE_TIFF;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_lotus_wk1, sizeof32(buffer_lotus_wk1)))
        t5_filetype = FILETYPE_LOTUS123;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_lotus_wk3, sizeof32(buffer_lotus_wk3)))
        t5_filetype = FILETYPE_LOTUS123;
    else if(0 == try_memcmp32(p_data, n_bytes, buffer_acorn_sid, sizeof32(buffer_acorn_sid)))
        t5_filetype = FILETYPE_SID;

    return(t5_filetype);
}

_Check_return_
static T5_FILETYPE
t5_filetype_from_file_header_test(
    FILE_HANDLE file_handle)
{
    static const BYTE buffer_compound_file[]= { '\xD0', '\xCF', '\x11', '\xE0', '\xA1', '\xB1', '\x1A', '\xE1' };

    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;
    U32 bytesread;
    BYTE buffer[0x20]; /* >= MAX(CFBF_FILE_HEADER_ID_BYTES, all-the-above) */
    zero_struct(buffer);

    if(status_ok(file_read_bytes(buffer, sizeof32(buffer), &bytesread, file_handle)))
    {   /* NB Not an error if not all bytes read! */
        if(0 == memcmp32(buffer, buffer_compound_file, sizeof32(buffer_compound_file)))
            t5_filetype = detect_compound_document_file(file_handle);
        else
        {
            t5_filetype = t5_filetype_from_data(buffer, bytesread);

            if(FILETYPE_UNDETERMINED == t5_filetype)
                t5_filetype = FILETYPE_TEXT;
        }
    }

    return(t5_filetype);
}

_Check_return_
extern T5_FILETYPE
t5_filetype_from_file_header(
    _In_z_      PCTSTR filename)
{
    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;
    FILE_HANDLE file_handle;
    STATUS status;

    if(status_ok(status = t5_file_open(filename, file_open_read, &file_handle, TRUE)))
    {
        t5_filetype = t5_filetype_from_file_header_test(file_handle);

        status_assert(t5_file_close(&file_handle));
    }

    return(t5_filetype);
}

#if RISCOS

/* RISC OS file type is paramount, then extension, then file content */

_Check_return_
extern T5_FILETYPE
t5_filetype_from_filename(
    _In_z_      PCTSTR filename)
{
    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;
    FILE_HANDLE file_handle;
    STATUS status = t5_file_open(filename, file_open_read, &file_handle, FALSE);

    if(status_done(status))
    {
        status = status_wrap(file_get_risc_os_filetype(file_handle));

        t5_filetype = (T5_FILETYPE) status;

        if(status_ok(status))
        {
            switch(t5_filetype)
            {
            case FILETYPE_TEXT:
                t5_filetype = t5_filetype_from_extension(file_extension(filename));
                break;

            case FILETYPE_DOS:
            case FILETYPE_DATA:
            case FILETYPE_UNTYPED:
            case FILETYPE_UNDETERMINED:
                /* if wooly, look for any known extension then grok file content while file is still open */
                t5_filetype = t5_filetype_from_extension(file_extension(filename));

                if(FILETYPE_UNDETERMINED == t5_filetype)
                    t5_filetype = t5_filetype_from_file_header_test(file_handle);
                break;

            default:
                break;
            }
        }

        status_assert(t5_file_close(&file_handle));
    }

    return(t5_filetype);
}

#elif WINDOWS

_Check_return_
extern T5_FILETYPE
t5_filetype_from_filename(
    _In_z_      PCTSTR filename)
{
    T5_FILETYPE t5_filetype = t5_filetype_from_extension(file_extension(filename));

    if(FILETYPE_UNDETERMINED == t5_filetype)
        t5_filetype = t5_filetype_from_file_header(filename);

    return(t5_filetype);
}

#endif /* OS */

/******************************************************************************
*
* load non-Fireworkz format file
*
******************************************************************************/

_Check_return_
static STATUS
load_foreign_core(
    _InVal_     DOCNO docno,
    _InVal_     T5_FILETYPE t5_filetype,
    _In_z_      PCTSTR filename_foreign,
    _InVal_     OBJECT_ID object_id,
    _InoutRef_  P_U32 p_retry_with_this_arg)
{
    STATUS status;
    P_DOCU p_docu = p_docu_from_docno(docno);
    MSG_INSERT_FOREIGN msg_insert_foreign;
    zero_struct(msg_insert_foreign);

    msg_insert_foreign.old_virtual_row_table = p_docu->flags.virtual_row_table;
    status_assert(virtual_row_table_set(p_docu, TRUE)); /* NB loading speed bodge */

    cur_change_before(p_docu);

    msg_insert_foreign.filename = filename_foreign;
    msg_insert_foreign.t5_filetype = t5_filetype;
    msg_insert_foreign.ctrl_pressed = FALSE;
    position_init(&msg_insert_foreign.position);
    msg_insert_foreign.retry_with_this_arg = *p_retry_with_this_arg;
    status = object_call_id_load(p_docu, T5_MSG_INSERT_FOREIGN, &msg_insert_foreign, object_id);
    *p_retry_with_this_arg = msg_insert_foreign.retry_with_this_arg;

    p_docu = p_docu_from_docno(docno);

    cur_change_after(p_docu);

    status_accumulate(status, virtual_row_table_set(p_docu, msg_insert_foreign.old_virtual_row_table));

    p_docu->flags.allow_modified_change = 1;

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_load_foreign)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    PCTSTR filename_foreign = p_args[0].val.tstr;
    T5_FILETYPE t5_filetype = p_args[1].val.t5_filetype;
    PCTSTR filename_template;
    DOCNO docno = DOCNO_NONE; /* Keep dataflower happy */
    STATUS status = STATUS_OK;
    BOOL just_the_one = FALSE;
    const OBJECT_ID object_id = object_id_from_t5_filetype(t5_filetype);
    U32 retry_with_this_arg = 0;
    BOOL looping = FALSE;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 100);
    quick_tblock_with_buffer_setup(quick_tblock);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(OBJECT_ID_NONE == object_id)
        return(create_error(ERR_UNKNOWN_FILETYPE));

    if(arg_is_present(p_args, 2))
    {
        filename_template = p_args[2].val.tstr;

        if(!file_is_rooted(filename_template))
        {
            if(status_ok(status = quick_tblock_tstr_add(&quick_tblock, TEMPLATES_SUBDIR FILE_DIR_SEP_TSTR)))
                status = quick_tblock_tstr_add_n(&quick_tblock, filename_template, strlen_with_NULLCH);

            filename_template = status_ok(status) ? quick_tblock_tstr(&quick_tblock) : NULL;
        }
    }
    else
    {
        static const UI_TEXT caption = UI_TEXT_INIT_RESID(MSG_DIALOG_SELECT_TEMPLATE_CAPTION);

        status = select_a_template(p_docu, &quick_tblock, &just_the_one, FALSE, &caption);

        filename_template = status_ok(status) ? quick_tblock_tstr(&quick_tblock) : NULL;
    }

    do  {

    if(filename_template)
    {
        DOCU_NAME docu_name;

        name_init(&docu_name);

        if(!host_xfer_loaded_file_is_safe())
            status = name_set_untitled(&docu_name);
        else
        {
            TCHARZ filename_buffer[BUF_MAX_PATHSTRING];
            PTSTR tstr_leafname;
            QUICK_TBLOCK_WITH_BUFFER(quick_tblock_derive, BUF_MAX_PATHSTRING);
            quick_tblock_with_buffer_setup(quick_tblock_derive);

            tstr_xstrkpy(filename_buffer, elemof32(filename_buffer), filename_foreign);
            tstr_leafname = file_leafname(filename_buffer);
            { /* strip any extension off this name */
            PTSTR tstr_extension = file_extension(filename_buffer);
            if(tstr_extension)
                *--tstr_extension = CH_NULL;
            } /*block*/
            {
            TCHARZ suffix_buffer[32];
            TCHARZ dirname_buffer[BUF_MAX_PATHSTRING];
            file_dirname(dirname_buffer, filename_buffer);
            /* stick an extension on in any case to discriminate from original foreign file */
            if(0 == retry_with_this_arg)
            {
                tstr_xstrkpy(suffix_buffer, elemof32(suffix_buffer), FILE_EXT_SEP_TSTR);
                tstr_xstrkat(suffix_buffer, elemof32(suffix_buffer), extension_document_tstr);
            }
            else
            {
                consume_int(tstr_xsnprintf(suffix_buffer, elemof32(suffix_buffer),
                                           TEXT("f%d") FILE_EXT_SEP_TSTR TEXT("%s"),
                                           retry_with_this_arg,
                                           extension_document_tstr));
            }
            status = file_derive_name(dirname_buffer, tstr_leafname, suffix_buffer, &quick_tblock_derive, 0 /*ERR_CANT_IMPORT_ISAFILE*/);
            if(FILE_ERR_ISAFILE == status)
            { /* SKS 27jul95 have another go! */
                quick_tblock_dispose(&quick_tblock_derive);
                status = file_tempname(dirname_buffer, tstr_leafname, suffix_buffer, FILE_TEMPNAME_INITIAL_TRY, &quick_tblock_derive);
            }
            } /*block*/
            if(status_ok(status))
            {
                PCTSTR fullname = quick_tblock_tstr(&quick_tblock_derive);

                status = name_read_tstr(&docu_name, fullname);

                if(status_ok(status))
                {
                    status = load_is_file_loaded(&docno, &docu_name);

                    if(STATUS_DONE == status)
                    {   /* SKS 15jun95 we've found the corresponding thunk for this document, which is probably not correctly formed name-wise */
                        reportf(
                            TEXT("t5_cmd_load_foreign: naming thunk docno: ") S32_TFMT TEXT(" path(%s) leaf(%s) extension(%s)"),
                            (S32) docno,
                            report_tstr(docu_name.path_name),
                            report_tstr(docu_name.leaf_name),
                            report_tstr(docu_name.extension));
                        status = maeve_event(p_docu_from_docno(docno), T5_MSG_DOCU_RENAME, (P_ANY) de_const_cast(PTSTR, fullname));
                    }
                }

            }

            quick_tblock_dispose(&quick_tblock_derive);
        }

        if(status_ok(status))
            status = new_docno_using(&docno, filename_template, &docu_name, FALSE /*fReadOnly*/);

        if(status_fail(status))
        {
            reperr(status, filename_foreign);
            status = STATUS_FAIL;
        }

        name_dispose(&docu_name);
    }

    if(status_ok(status))
        status = command_set_current_docu(p_docu_from_docno(docno));

    if(status_ok(status))
        status = load_foreign_core(docno, t5_filetype, filename_foreign, object_id, &retry_with_this_arg);

    looping = (0 != retry_with_this_arg);
    }
    while(looping);

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

_Check_return_
extern STATUS
foreign_load_initialise(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_maybenone_ P_POSITION p_position,
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _In_opt_z_  PCTSTR filename,
    _InRef_opt_ PC_ARRAY_HANDLE p_array_handle)
{
    STATUS status = STATUS_OK;
        
    status_return(ownform_initialise_load(p_docu, p_position, (P_OF_IP_FORMAT) p_ff_ip_format, filename, p_array_handle));

    if(status_ok(status))
        foreign_load_set_io_type(p_ff_ip_format, IO_TYPE_AUTO);

    if(status_fail(status))
        status_consume(foreign_load_finalise(p_docu, p_ff_ip_format));

    return(status);
}

_Check_return_
extern STATUS
foreign_load_finalise(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format)
{
    STATUS status = ownform_finalise_load(p_docu, (P_OF_IP_FORMAT) p_ff_ip_format);

    return(status);
}

extern void
foreign_load_set_io_type(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _InVal_     IO_TYPE io_type)
{
    p_ff_ip_format->io_type = io_type;

    switch(io_type)
    {
    default: default_unhandled();
#if CHECKING
        /*FALLTHRU*/
    case IO_TYPE_AUTO:
    case IO_TYPE_LATIN_1:
    case IO_TYPE_LATIN_2:
    case IO_TYPE_LATIN_3:
    case IO_TYPE_LATIN_4:
    case IO_TYPE_LATIN_9:
#endif
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_Latin_N;
        break;

    case IO_TYPE_ASCII:
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_ASCII;
        break;

    case IO_TYPE_TRANSLATE_8:
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_Translate_8;
        break;

    case IO_TYPE_UTF_8:
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_UTF_8;
        break;

    case IO_TYPE_UTF_16LE:
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_UTF_16LE;
        break;

    case IO_TYPE_UTF_16BE:
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_UTF_16BE;
        break;

    case IO_TYPE_UTF_32LE:
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_UTF_32LE;
        break;

    case IO_TYPE_UTF_32BE:
        p_ff_ip_format->read_ucs4_character_fn = read_ucs4_character_UTF_32BE;
        break;
    }
}

_Check_return_
static STATUS
foreign_load_determine_io_type_from_BOM(
    _Inout_     P_FF_IP_FORMAT p_ff_ip_format)
{
    IO_TYPE io_type = IO_TYPE_AUTO;
    U32 bom_bytes = 0;
    U8 buffer[4];
    U32 bytes_read;
    STATUS status = STATUS_OK;

    memset32(buffer, CH_DOLLAR_SIGN, sizeof32(buffer));

    for(bytes_read = 0; bytes_read < sizeof32(buffer); ++bytes_read)
    {
        buffer[bytes_read] = binary_read_byte(&p_ff_ip_format->of_ip_format.input, &status);

        if(status_fail(status) || (EOF_READ == status))
            break;
    }

    if(status_ok(status))
    {   /* search longer ones first! */
        static const U8 buffer_bom_utf32le[]  = { '\xFF', '\xFE', '\x00', '\x00' };
        static const U8 buffer_bom_utf32be[]  = { '\x00', '\x00', '\xFE', '\xFF' };
        static const U8 buffer_bom_utf8[]     = { '\xEF', '\xBB', '\xBF' }; /* Unicode Standard Section 3.10 advises against using a BOM to signify UTF-8 */
        static const U8 buffer_bom_utf16le[]  = { '\xFF', '\xFE' };
        static const U8 buffer_bom_utf16be[]  = { '\xFE', '\xFF' };

        if(0 == memcmp32(buffer, buffer_bom_utf32le, (bom_bytes = sizeof32(buffer_bom_utf32le))))
            io_type = IO_TYPE_UTF_32LE;
        else if(0 == memcmp32(buffer, buffer_bom_utf32be, (bom_bytes = sizeof32(buffer_bom_utf32be))))
            io_type = IO_TYPE_UTF_32BE;
        else if(0 == memcmp32(buffer, buffer_bom_utf8, (bom_bytes = sizeof32(buffer_bom_utf8))))
            io_type = IO_TYPE_UTF_8;
        else if(0 == memcmp32(buffer, buffer_bom_utf16le, (bom_bytes = sizeof32(buffer_bom_utf16le))))
            io_type = IO_TYPE_UTF_16LE;
        else if(0 == memcmp32(buffer, buffer_bom_utf16be, (bom_bytes = sizeof32(buffer_bom_utf16be))))
            io_type = IO_TYPE_UTF_16BE;
        else
            bom_bytes = 0;

        status = STATUS_OK; /* normalise */
    }

    p_ff_ip_format->of_ip_format.input.bom_bytes = bom_bytes; /* needed for this, and any subsequent, rewind */

    /* rewind in any case, skipping any BOM */
    status_accumulate(status, input_rewind(&p_ff_ip_format->of_ip_format.input));

    p_ff_ip_format->of_ip_format.input.file.file_offset_in_bytes = 0; /* manual reset */
    p_ff_ip_format->of_ip_format.process_status_threshold = 0; /* manual reset */

    if(IO_TYPE_AUTO != io_type)
    {
        /* and only then set the I/O type */
        foreign_load_set_io_type(p_ff_ip_format, io_type);
    }

    return(status);
}

_Check_return_
static STATUS
foreign_load_determine_io_type_from_content(
    _Inout_     P_FF_IP_FORMAT p_ff_ip_format)
{
    IO_TYPE io_type = IO_TYPE_AUTO;
    U8 buffer[1024];
    U32 bytes_read, i;
    U32 hi_zero, hi_nonzero;
    STATUS status = STATUS_OK;

    memset32(buffer, CH_DOLLAR_SIGN, sizeof32(buffer));

    for(bytes_read = 0; bytes_read < sizeof32(buffer); ++bytes_read)
    {
        buffer[bytes_read] = binary_read_byte(&p_ff_ip_format->of_ip_format.input, &status);

        if(status_fail(status) || (EOF_READ == status))
            break;
    }

    /* rewind in any case, skipping any BOM */
    status_accumulate(status, input_rewind(&p_ff_ip_format->of_ip_format.input));

    p_ff_ip_format->of_ip_format.input.file.file_offset_in_bytes = 0; /* manual reset */
    p_ff_ip_format->of_ip_format.process_status_threshold = 0; /* manual reset */

    status_return(status);

    status = STATUS_OK; /* normalise */

    /* try detecting higher-order zero bytes that will be found in 'normal' characters */
    if((IO_TYPE_AUTO == io_type) && (bytes_read >= 4))
    {
        hi_zero = hi_nonzero = 0;

        for(i = 0; i <= bytes_read - 4; i++)
        {
            if((CH_NULL != buffer[i + 2]) || (CH_NULL != buffer[i + 3]))
                ++hi_nonzero;
            else
                ++hi_zero;
        }

        if(hi_zero > hi_nonzero * 2)
            io_type = IO_TYPE_UTF_32LE;
    }

    if((IO_TYPE_AUTO == io_type) && (bytes_read >= 4))
    {
        hi_zero = hi_nonzero = 0;

        for(i = 0; i <= bytes_read - 4; i++)
        {
            if((CH_NULL != buffer[i + 0]) || (CH_NULL != buffer[i + 1]))
                ++hi_nonzero;
            else
                ++hi_zero;
        }

        if(hi_zero > hi_nonzero * 2)
            io_type = IO_TYPE_UTF_32BE;
    }

    if((IO_TYPE_AUTO == io_type) && (bytes_read >= 2))
    {
        hi_zero = hi_nonzero = 0;

        for(i = 0; i <= bytes_read - 2; i++)
        {
            if(CH_NULL != buffer[i + 1])
                ++hi_nonzero;
            else
                ++hi_zero;
        }

        if(hi_zero > hi_nonzero * 2)
            io_type = IO_TYPE_UTF_16LE;
    }

    if((IO_TYPE_AUTO == io_type) && (bytes_read >= 2))
    {
        hi_zero = hi_nonzero = 0;

        for(i = 0; i <= bytes_read - 2; i++)
        {
            if(CH_NULL != buffer[i + 0])
                ++hi_nonzero;
            else
                ++hi_zero;
        }

        if(hi_zero > hi_nonzero * 2)
            io_type = IO_TYPE_UTF_16BE;
    }

    if((IO_TYPE_AUTO == io_type) && (bytes_read >= 2))
    {
        /* try sussing whether it appears to be UTF-8 encoded */
        BOOL valid = TRUE;
        BOOL had_lead_byte = FALSE;
        U32 bytes_of_char = 1;

        for(i = 0; (i < bytes_read) && valid; i += bytes_of_char)
        {
            if(u8_is_utf8_trail_byte(buffer[i]))
            {
                valid = FALSE;
                break;
            }

            bytes_of_char = utf8_bytes_of_char_off(utf8_bptr(buffer), i);

            if(bytes_of_char > 1)
            {   /* it must be a lead byte! */
                U32 byte_index;

                had_lead_byte = TRUE;

                for(byte_index = 1; (byte_index < bytes_of_char) && (i + byte_index < bytes_read); ++byte_index) /* can't be fussy about things half-read! */
                {
                    if(!u8_is_utf8_trail_byte(buffer[i + byte_index]))
                        valid = FALSE;
                }
            }
        }

        if(valid && had_lead_byte)
            io_type = IO_TYPE_UTF_8;
    }

    if(IO_TYPE_AUTO != io_type)
    {
        /* and only then set the I/O type */
        foreign_load_set_io_type(p_ff_ip_format, io_type);
    }

    return(status);
}

_Check_return_
extern STATUS
foreign_load_determine_io_type(
    _Inout_     P_FF_IP_FORMAT p_ff_ip_format)
{
    /* been specified at a higher level? */
    if(IO_TYPE_AUTO != p_ff_ip_format->io_type)
        return(STATUS_OK);

    status_return(foreign_load_determine_io_type_from_BOM(p_ff_ip_format));

    /* now determined? */
    if(IO_TYPE_AUTO != p_ff_ip_format->io_type)
        return(STATUS_OK);

    status_return(foreign_load_determine_io_type_from_content(p_ff_ip_format));

    /* now determined? */
    if(IO_TYPE_AUTO != p_ff_ip_format->io_type)
        return(STATUS_OK);

    /* set the default I/O type */
#if 1
    foreign_load_set_io_type(p_ff_ip_format, IO_TYPE_LATIN_1);
#else
    p_ff_ip_format->of_ip_format.input.sbchar_codepage = get_system_codepage();
    switch(p_ff_ip_format->of_ip_format.input.sbchar_codepage)
    {
    default:
    case SBCHAR_CODEPAGE_WINDOWS_1252:
    case SBCHAR_CODEPAGE_WINDOWS_28591:
    case SBCHAR_CODEPAGE_ALPHABET_LATIN1:
        foreign_load_set_io_type(p_ff_ip_format, IO_TYPE_LATIN_1);
        break;

    case SBCHAR_CODEPAGE_WINDOWS_1250:
    case SBCHAR_CODEPAGE_WINDOWS_28592:
    case SBCHAR_CODEPAGE_ALPHABET_LATIN2:
        foreign_load_set_io_type(p_ff_ip_format, IO_TYPE_LATIN_2);
        break;

    case SBCHAR_CODEPAGE_WINDOWS_28593:
    case SBCHAR_CODEPAGE_ALPHABET_LATIN3:
        foreign_load_set_io_type(p_ff_ip_format, IO_TYPE_LATIN_3);
        break;

    case SBCHAR_CODEPAGE_WINDOWS_28594:
    case SBCHAR_CODEPAGE_ALPHABET_LATIN4:
        foreign_load_set_io_type(p_ff_ip_format, IO_TYPE_LATIN_4);
        break;

    case SBCHAR_CODEPAGE_WINDOWS_28605:
    case SBCHAR_CODEPAGE_ALPHABET_LATIN9:
        foreign_load_set_io_type(p_ff_ip_format, IO_TYPE_LATIN_9);
        break;
    }
#endif

    return(STATUS_OK);
}

/* check that the specified template file will be usable */

_Check_return_
static STATUS
foreign_template_check(
    _In_z_      PCTSTR tstr_foreign_template)
{
    STATUS status = STATUS_OK;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 100);
    quick_tblock_with_buffer_setup(quick_tblock);

    if(status_ok(status = quick_tblock_tstr_add(&quick_tblock, TEMPLATES_SUBDIR FILE_DIR_SEP_TSTR)))
        status = quick_tblock_tstr_add_n(&quick_tblock, tstr_foreign_template, strlen_with_NULLCH);

    if(status_ok(status))
    {
        PCTSTR composite_filename = quick_tblock_tstr(&quick_tblock);
        TCHARZ filename_buffer[BUF_MAX_PATHSTRING];
        status = file_find_on_path(filename_buffer, elemof32(filename_buffer), composite_filename);
    } /*block*/

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

_Check_return_
static STATUS
load_foreign_file_core(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status = ensure_memory_froth();

    if(status_ok(status))
    {
        T5_MESSAGE t5_message = T5_CMD_LOAD_FOREIGN;
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
            QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 128);
            quick_tblock_with_buffer_setup(quick_tblock);

            if(NULL == filename)
            {
                arg_dispose(&arglist_handle, 0);
            }
            else if(status_ok(status = quick_tblock_tstr_add_n(&quick_tblock, filename, strlen_with_NULLCH)))
            {
                p_args[0].val.tstr = quick_tblock_tstr(&quick_tblock);
            }

            p_args[1].val.t5_filetype = t5_filetype;

            if(status_ok(status))
            {
                PCTSTR tstr_foreign_template = foreign_template_from_t5_filetype(t5_filetype);

                if(NULL != tstr_foreign_template)
                    if(!status_done(status = foreign_template_check(tstr_foreign_template)))
                        tstr_foreign_template = NULL;

                if(NULL != tstr_foreign_template)
                    p_args[2].val.tstr = tstr_foreign_template;
                else
                    arg_dispose(&arglist_handle, 2);
            }

            if(status_ok(status))
                status = execute_command(cur_p_docu, t5_message, &arglist_handle, object_id);

            quick_tblock_dispose(&quick_tblock);

            arglist_dispose(&arglist_handle);
        }
    }

    return(status);
}

/* Errors reported locally */

_Check_return_
extern STATUS /*reported*/
load_foreign_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status = load_foreign_file_core(cur_p_docu, filename, t5_filetype);

    if(status_fail(status))
    {
        if(status != STATUS_FAIL)
            reperr_null(status);

        return(STATUS_FAIL);
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
foreign_load_file_apply_style_table(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area_table,
    _InVal_     COL lhs_extra_cols,
    _InVal_     ROW top_extra_rows,
    _InVal_     ROW bot_extra_rows)
{
    STATUS status = STATUS_OK;

    {
    STYLE style_table;
    STYLE_HANDLE style_table_handle = 0;

    style_init(&style_table);

    style_table.para_style.para_start = style_default_measurement(p_docu, STYLE_SW_PS_PARA_START);
    style_bit_set(&style_table, STYLE_SW_PS_PARA_START);

    style_table.para_style.para_end = style_default_measurement(p_docu, STYLE_SW_PS_PARA_END);
    style_bit_set(&style_table, STYLE_SW_PS_PARA_END);

    style_table.para_style.grid_left =
    style_table.para_style.grid_top =
    style_table.para_style.grid_right =
    style_table.para_style.grid_bottom = SF_BORDER_STANDARD;
    style_bit_set(&style_table, STYLE_SW_PS_GRID_LEFT);
    style_bit_set(&style_table, STYLE_SW_PS_GRID_TOP);
    style_bit_set(&style_table, STYLE_SW_PS_GRID_RIGHT);
    style_bit_set(&style_table, STYLE_SW_PS_GRID_BOTTOM);

    rgb_set(&style_table.para_style.rgb_grid_left, 0, 0, 0); /* true black */
    style_table.para_style.rgb_grid_top =
    style_table.para_style.rgb_grid_right =
    style_table.para_style.rgb_grid_bottom =
    style_table.para_style.rgb_grid_left;
    style_bit_set(&style_table, STYLE_SW_PS_RGB_GRID_LEFT);
    style_bit_set(&style_table, STYLE_SW_PS_RGB_GRID_TOP);
    style_bit_set(&style_table, STYLE_SW_PS_RGB_GRID_RIGHT);
    style_bit_set(&style_table, STYLE_SW_PS_RGB_GRID_BOTTOM);

    { /* invent a new, appropriately named, style for this table (which will be 'renumbered' on file insertion) */
    PCTSTR tstr_format = resource_lookup_tstr(MSG_DIALOG_INSERT_TABLE_TABLE);
    S32 i = 1;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 16);
    quick_tblock_with_buffer_setup(quick_tblock);

    for(;;)
    {
        BOOL do_break = FALSE;

        status_break(status = quick_tblock_printf(&quick_tblock, tstr_format, i));
        status_break(status = quick_tblock_nullch_add(&quick_tblock));

        if(0 == style_handle_from_name(p_docu, quick_tblock_tstr(&quick_tblock)))
        {
            status = al_tstr_set(&style_table.h_style_name_tstr, quick_tblock_tstr(&quick_tblock));
            do_break = TRUE;
        }

        quick_tblock_dispose(&quick_tblock);

        if(do_break)
            break;

        ++i;
    }
    } /*block*/

    status_return(status);

    style_bit_set(&style_table, STYLE_SW_NAME);
    status_return(style_table_handle = style_handle_add(p_docu, &style_table));

    {
    DOCU_AREA docu_area_interior = *p_docu_area_table;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_table_handle);
    docu_area_interior.tl.slr.col += lhs_extra_cols;
    docu_area_interior.tl.slr.row += top_extra_rows;
    docu_area_interior.br.slr.row -= bot_extra_rows;
    status_return(style_docu_area_add(p_docu, &docu_area_interior, &style_docu_area_add_parm));
    } /*block*/
    } /*block*/

    {
    DOCU_AREA docu_area_cols = *p_docu_area_table;
    STYLE style_cols;

    style_init(&style_cols);

    style_cols.para_style.margin_para = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_PARA);
    style_bit_set(&style_cols, STYLE_SW_PS_MARGIN_PARA);

    style_cols.para_style.margin_left = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_LEFT);
    style_bit_set(&style_cols, STYLE_SW_PS_MARGIN_LEFT);

    style_cols.para_style.margin_right = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_RIGHT);
    style_bit_set(&style_cols, STYLE_SW_PS_MARGIN_RIGHT);

    style_cols.para_style.h_tab_list = 0;
    style_bit_set(&style_cols, STYLE_SW_PS_TAB_LIST);

    style_cols.col_style.width = style_default_measurement(p_docu, STYLE_SW_CS_WIDTH);
    style_bit_set(&style_cols, STYLE_SW_CS_WIDTH);

    while(docu_area_cols.tl.slr.col < docu_area_cols.br.slr.col)
    {
        DOCU_AREA docu_area_inner = docu_area_cols;
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style_cols);
        style_docu_area_add_parm.add_without_subsume = TRUE;
        docu_area_inner.br.slr.col = docu_area_inner.tl.slr.col + 1;
        status_return(style_docu_area_add(p_docu, &docu_area_inner, &style_docu_area_add_parm));
        ++docu_area_cols.tl.slr.col;
    }
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
foreign_load_file_auto_width_table(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area_table)
{
    DOCU_AREA docu_area = *p_docu_area_table;
    COL col_s = docu_area.tl.slr.col;
    COL col_e = docu_area.br.slr.col;
    COL col   = col_s;
    PROCESS_STATUS process_status;
    STATUS status = STATUS_OK;
    STYLE style;

    /* do this in background? if so we'd better have hacked the other cols in and can then use modify source */
    process_status_init(&process_status);
    process_status.flags.foreground = TRUE;
    process_status_begin(p_docu, &process_status, PROCESS_STATUS_PERCENT);
    process_status.reason.type = UI_TEXT_TYPE_RESID;
    process_status.reason.text.resource_id = MSG_STATUS_REFORMATTING;

    process_status.data.percent.current = (99 * (1)) / (col_e - col_s + 1); /* put up a little lie to indicate some change */
    process_status_reflect(&process_status);

    style_init(&style);
    style_bit_set(&style, STYLE_SW_CS_WIDTH);

    while(col < col_e)
    {
        COL_AUTO_WIDTH col_auto_width;
        col_auto_width.col = col;
        col_auto_width.row_s = docu_area.tl.slr.row;
        col_auto_width.row_e = docu_area.br.slr.row;
        col_auto_width.allow_special = TRUE;
        status_consume(object_call_id(p_docu->object_id_flow, p_docu, T5_MSG_COL_AUTO_WIDTH, &col_auto_width));
        style.col_style.width = col_auto_width.width;

        if(style.col_style.width != 0)
        {
            STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
            STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
            docu_area.tl.slr.col = col;
            docu_area.br.slr.col = docu_area.tl.slr.col + 1;
            status_break(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
        }

        ++col;

        process_status.data.percent.current = (99 * (col - col_s + 1)) / (col_e - col_s + 1);
        process_status_reflect(&process_status);
    }

    process_status_end(&process_status);

    return(status);
}

/* end of ff_load.c */
