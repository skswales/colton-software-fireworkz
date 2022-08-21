/* of_load.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* File load routines for Fireworkz */

/* JAD Mar 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/ff_io.h"

#include "cmodules/collect.h"

#include "cmodules/cfbf.h"

#include "ob_skel/ho_print.h"

#include "ob_file/xp_file.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

/*
internal routines
*/

_Check_return_
static STATUS
of_load_obtain_and_process_construct(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_patch,
    _InVal_     BOOL top_level_command_file);

_Check_return_
static STATUS
of_load_process_construct(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_patch,
    _InRef_     P_QUICK_BLOCK p_quick_block_construct_line /*in, poked*/);

_Check_return_
static STATUS
of_load_process_construct_end(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_patch,
    _InRef_     PC_CONSTRUCT_TABLE p_construct,
    _In_        OBJECT_ID object_id,
    P_U8 p_construct_body);

typedef struct REGISTERED_CONSTRUCT_TABLE
{
    PC_CONSTRUCT_TABLE p_construct_table;
    U32 construct_table_entries;
    U32 short_construct_table_entries;
    U8 construct_id;
    U8 needs_help;
}
REGISTERED_CONSTRUCT_TABLE, * P_REGISTERED_CONSTRUCT_TABLE; typedef const REGISTERED_CONSTRUCT_TABLE * PC_REGISTERED_CONSTRUCT_TABLE;

#define SHORT_CONSTRUCT_NAME_LENGTH 3 /* as construct name length is primary key, can lookup short ones even faster */

typedef struct OF_LOAD_STATICS
{
    REGISTERED_CONSTRUCT_TABLE registered_construct_table[MAX_OBJECTS];
}
OF_LOAD_STATICS;

static OF_LOAD_STATICS of_load_statics;

static BOOL first_time_through = 0;

extern void
of_load_prepare_first_template(void)
{
    first_time_through = 1;
}

#ifndef LOAD_QB_SIZE
#define LOAD_QB_SIZE 512
#endif /* styles are broken up more these days */

/******************************************************************************
*
* load whole file into memory
*
******************************************************************************/

_Check_return_
extern STATUS
file_memory_load(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_data,
    _In_z_      PCTSTR p_pathname,
    _OutRef_opt_ P_PROCESS_STATUS p_process_status,
    _InVal_     U32 min_file_size)
{
    STATUS status = STATUS_OK;
    FILE_HANDLE file_handle = 0;

    *p_h_data = 0;

    if(NULL != p_process_status)
        process_status_init(p_process_status);

    if(status_ok(status = t5_file_open(p_pathname, file_open_read, &file_handle, TRUE)))
    {
        /* structure loop */
        for(;;)
        {
            filelength_t filelength;

            status_break(file_length(file_handle, &filelength));

            if(filelength.u.words.hi != 0)
            {
                status = create_error(ERR_FILE_TOO_LARGE);
                break;
            }

            if((0 != min_file_size) && (filelength.u.words.lo < min_file_size))
            {
                status = create_error(ERR_FILE_TOO_SMALL);
                break;
            }

            {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(U8), FALSE);
            P_U8 p_u8;
            if(NULL == (p_u8 = al_array_alloc_U8(p_h_data, filelength.u.words.lo, &array_init_block, &status)))
                break;
            status_break(status = file_read_bytes_requested(p_u8, filelength.u.words.lo, file_handle));
            } /*block*/

            if(NULL != p_process_status)
            {
                p_process_status->flags.foreground = TRUE;

                if(UI_TEXT_TYPE_NONE == p_process_status->reason.type)
                {
                    p_process_status->reason.type = UI_TEXT_TYPE_RESID;
                    p_process_status->reason.text.resource_id = MSG_STATUS_CONVERTING; /* that's what we're about to do */
                }

                process_status_begin(p_docu, p_process_status, PROCESS_STATUS_PERCENT);
            }

            break; /* out of structure loop */
            /*NOTREACHED*/
        }

        status_accumulate(status, t5_file_close(&file_handle));
    }

    return(status);
}

_Check_return_
extern STATUS
input_rewind(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input)
{
    p_ip_format_input->ungotchar = EOF_READ;

    if(IP_INPUT_FILE == p_ip_format_input->state)
    {
        U32 i;

        status_return(file_rewind(p_ip_format_input->file.file_handle));

        for(i = 0; i < p_ip_format_input->bom_bytes; ++i)
            status_return(file_getc(p_ip_format_input->file.file_handle));
    }
    else /* if(IP_INPUT_MEM == p_ip_format_input->state) */
    {
        assert(IP_INPUT_MEM == p_ip_format_input->state);
        assert(array_index_is_valid(p_ip_format_input->mem.p_array_handle, p_ip_format_input->bom_bytes));
        p_ip_format_input->mem.array_offset = p_ip_format_input->bom_bytes;
    }

    return(STATUS_OK);
}

/*
Construct character-> ownform_data_type table
*/

static const
struct OWNFORM_DATA_TYPE
{
    U8 data_char;
    U8 data_type;
}
ownform_data_type[OWNFORM_DATA_TYPE_MAX] =
{
    { OWNFORM_DATA_CHAR_TEXT,     OWNFORM_DATA_TYPE_TEXT     },
    { OWNFORM_DATA_CHAR_DATE,     OWNFORM_DATA_TYPE_DATE     },
    { OWNFORM_DATA_CHAR_CONSTANT, OWNFORM_DATA_TYPE_CONSTANT },
    { OWNFORM_DATA_CHAR_FORMULA,  OWNFORM_DATA_TYPE_FORMULA  },
    { OWNFORM_DATA_CHAR_ARRAY,    OWNFORM_DATA_TYPE_ARRAY    },
    { OWNFORM_DATA_CHAR_DRAWFILE, OWNFORM_DATA_TYPE_DRAWFILE },
    { OWNFORM_DATA_CHAR_OWNER,    OWNFORM_DATA_TYPE_OWNER    }
};

/******************************************************************************
*
* Returns the ownform character to identify the cell contents
* data type, from internal DATA_TYPEs
*
******************************************************************************/

_Check_return_
extern U8
construct_id_from_data_type(
    _InVal_     S32 data_type)
{
    U32 offset;

    for(offset = 0; offset < elemof32(ownform_data_type); ++offset)
        if(data_type == ownform_data_type[offset].data_type)
            return(ownform_data_type[offset].data_char);

    assert0();
    return(CH_NULL);
}

_Check_return_
extern U8
construct_id_from_object_id(
    _InVal_     OBJECT_ID object_id)
{
    PC_REGISTERED_CONSTRUCT_TABLE p = &of_load_statics.registered_construct_table[object_id];
    return(p->construct_id);
}

typedef struct CONSTRUCT_TABLE_SEARCH
{
    PC_A7STR a7str_construct_name;
    U32 construct_name_length;
}
CONSTRUCT_TABLE_SEARCH, * P_CONSTRUCT_TABLE_SEARCH;

#define CONSTRUCT_NAME_LENGTH_PRIMARY_KEY 1

#if RISCOS
#define construct_table_compare short_memcmp32
#else
#define construct_table_compare memcmp /* use native function */
#endif /* OS */

PROC_BSEARCH_PROTO(static, construct_table_compare_bsearch, CONSTRUCT_TABLE_SEARCH, CONSTRUCT_TABLE)
{
    BSEARCH_KEY_VAR_DECL(P_CONSTRUCT_TABLE_SEARCH, p_construct_table_search_key);
    BSEARCH_DATUM_VAR_DECL(PC_CONSTRUCT_TABLE, p_construct_table_datum);

    PC_A7STR a7str_construct_name_key;
    PC_A7STR a7str_construct_name_datum;

#if defined(CONSTRUCT_NAME_LENGTH_PRIMARY_KEY)
    const U32 construct_name_length_key   = p_construct_table_search_key->construct_name_length;
    const U32 construct_name_length_datum = p_construct_table_datum->bits.construct_name_length;

    if(construct_name_length_key != construct_name_length_datum)
        return((int) construct_name_length_key - (int) construct_name_length_datum);

    a7str_construct_name_key   = p_construct_table_search_key->a7str_construct_name;
    a7str_construct_name_datum = p_construct_table_datum->a7str_construct_name;

    return(construct_table_compare(a7str_construct_name_key, a7str_construct_name_datum, construct_name_length_key));
#else
    a7str_construct_name_key   = p_construct_table_search_key->a7strconstruct_name;
    a7str_construct_name_datum = p_construct_table_datum->a7str_construct_name;

    return(/*"C"*/strcmp(a7str_construct_name_key, a7str_construct_name_datum));
#endif
}

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, construct_table_compare_qsort, CONSTRUCT_TABLE)
{
    int res;

    QSORT_ARG1_VAR_DECL(PC_CONSTRUCT_TABLE, p_construct_table_1);
    QSORT_ARG2_VAR_DECL(PC_CONSTRUCT_TABLE, p_construct_table_2);

    PC_A7STR a7str_construct_name_1;
    PC_A7STR a7str_construct_name_2;

#if defined(CONSTRUCT_NAME_LENGTH_PRIMARY_KEY)
    const U32 construct_name_length_1 = p_construct_table_1->bits.construct_name_length;
    const U32 construct_name_length_2 = p_construct_table_2->bits.construct_name_length;

    if(construct_name_length_1 != construct_name_length_2)
        return((int) construct_name_length_1 - (int) construct_name_length_2);

    a7str_construct_name_1 = p_construct_table_1->a7str_construct_name;
    a7str_construct_name_2 = p_construct_table_2->a7str_construct_name;

    res = construct_table_compare(a7str_construct_name_1, a7str_construct_name_2, construct_name_length_1);
#else
    a7str_construct_name_1 = p_construct_table_1->a7str_construct_name;
    a7str_construct_name_2 = p_construct_table_2->a7str_construct_name;

    res = /*"C"*/strcmp(a7str_construct_name_1, a7str_construct_name_2);
#endif

    myassert0x((res != 0) || (p_construct_table_1 == p_construct_table_2), TEXT("construct table has identical entries"));

    return(res);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* given an object_id, find the table
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern PC_CONSTRUCT_TABLE
construct_table_from_object_id(
    _InVal_     OBJECT_ID object_id)
{
    PC_REGISTERED_CONSTRUCT_TABLE p = &of_load_statics.registered_construct_table[object_id];
    return(p->p_construct_table);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* given an object_id and t5_message, find the table entry
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern PC_CONSTRUCT_TABLE
construct_table_lookup_message(
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message)
{
    PC_REGISTERED_CONSTRUCT_TABLE p = &of_load_statics.registered_construct_table[object_id];
    PC_CONSTRUCT_TABLE p_construct_table = p->p_construct_table;

    if(NULL == p_construct_table)
    {
        myassert2(TEXT("construct_table_lookup_message(command=") S32_TFMT TEXT("): ")
                  TEXT("Object ") S32_TFMT TEXT(" unexpectedly returned p_construct_table=NULL for T5_MSG_CONSTRUCT_TABLE_RQ"), t5_message, (S32) object_id);
        return(NULL);
    }

    for(; NULL != p_construct_table->a7str_construct_name; ++p_construct_table)
        if(t5_message == p_construct_table->t5_message)
            return(p_construct_table);

#if 0 /* SKS 03.03.93 - this situation is kosher: consider hefo saving out region of skel constructs */
    myassert2(TEXT("construct_table_lookup_message(command=") S32_TFMT TEXT("): ")
                 "command not found in object ") S32_TFMT TEXT("'s command construct table"), t5_message, (S32) object_id);
#endif

    return(NULL);
}

/******************************************************************************
*
* Type of data contained in cell
*
******************************************************************************/

_Check_return_
extern STATUS
data_type_from_construct_id(
    _InVal_     U8 data_type_char,
    _OutRef_    P_S32 data_type)
{
    U32 offset;

    for(offset = 0; offset < elemof32(ownform_data_type); ++offset)
    {
        if(data_type_char == ownform_data_type[offset].data_char)
        {
            *data_type = ownform_data_type[offset].data_type;
            return(STATUS_OK);
        }
    }

    assert0();
    *data_type = CH_NULL;
    return(STATUS_FAIL);
}

_Check_return_
extern STATUS
ownform_finalise_load(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format)
{
    STATUS status = STATUS_OK;

    if(IP_INPUT_FILE == p_of_ip_format->input.state)
    {
        (void) t5_file_close(&p_of_ip_format->input.file.file_handle);
    }
    else
    {
        p_of_ip_format->input.mem.p_array_handle = NULL;
    }

    tstr_clr(&p_of_ip_format->input_filename);

    process_status_end(&p_of_ip_format->process_status);

    { /* call all the objects to finish up */
    OBJECT_ID object_id = OBJECT_ID_ENUM_START;

    while(object_id < p_of_ip_format->stopped_object_id)
        status_accumulate(status, object_call_between(&object_id, p_of_ip_format->stopped_object_id, p_docu, T5_MSG_LOAD_ENDED, p_of_ip_format));
    } /*block*/

    collect_delete(&p_of_ip_format->object_data_list);

    arglist_cache_reduce();

#if CHECKING
    if(p_of_ip_format->flags.unknown_object == 1)
        myassert0(TEXT("Unknown object header(s) found in file (Data may be loaded by another object)"));

    if(p_of_ip_format->flags.unknown_data_type == 1)
        myassert0(TEXT("Unknown data type(s) found in file (Ignored)"));

    if(p_of_ip_format->flags.unknown_construct == 1)
        myassert0(TEXT("Unknown construct(s) found in file (Ignored)"));

    if(p_of_ip_format->flags.unknown_font_spec == 1)
        myassert0(TEXT("Unknown font name(s) found in file (Ignored)"));

    if(p_of_ip_format->flags.unknown_named_style == 1)
        myassert0(TEXT("Unknown named style(s) found in file (Ignored)"));
#endif

    return(status);
}

static STATUS
input_initialise_input(
    _OutRef_    P_IP_FORMAT_INPUT p_ip_format_input,
    _In_opt_z_  PCTSTR filename,
    _InRef_opt_ PC_ARRAY_HANDLE p_array_handle)
{
    STATUS status = STATUS_OK;

    p_ip_format_input->bom_bytes = 0;

    p_ip_format_input->ungotchar = EOF_READ;

    p_ip_format_input->state = (NULL == filename) ? IP_INPUT_MEM : IP_INPUT_FILE;

    if(IP_INPUT_FILE == p_ip_format_input->state)
    {
        p_ip_format_input->file.file_offset_in_bytes = 0;
        p_ip_format_input->file.file_size_in_bytes = 0;

        if(status_ok(status = t5_file_open(filename, file_open_read, &p_ip_format_input->file.file_handle, TRUE)))
        {
            filelength_t filelength;

            status = file_length(p_ip_format_input->file.file_handle, &filelength);

            if(status_ok(status))
            {
                if(0 != filelength.u.words.hi)
                    p_ip_format_input->file.file_size_in_bytes = 0xFFFFFF00U; /* look as if it's nearly 4GB - percentage counter will wrap */
                else
                    p_ip_format_input->file.file_size_in_bytes = filelength.u.words.lo;

                status = file_buffer(p_ip_format_input->file.file_handle, NULL, 16384); /* SKS big buffering for loads */
            }
        }
    }
    else /* if(IP_INPUT_MEM == p_ip_format_input->state) */
    {
        assert(IP_INPUT_MEM == p_ip_format_input->state);

        PTR_ASSERT(p_array_handle);
        assert(0 != *p_array_handle);

        p_ip_format_input->mem.p_array_handle = p_array_handle;
        p_ip_format_input->mem.array_offset = 0;
    }

    return(status);
}

_Check_return_
extern STATUS
ownform_initialise_load(
    _DocuRef_   P_DOCU p_docu,
    _InRef_maybenone_ PC_POSITION p_position,
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _In_opt_z_  PCTSTR filename,
    _InRef_opt_ PC_ARRAY_HANDLE p_array_handle)
{
    STATUS status = STATUS_OK;

    p_of_ip_format->input_filename = NULL;

    if(filename && host_xfer_loaded_file_is_safe())
        status = tstr_set(&p_of_ip_format->input_filename, filename);

    if(status_ok(status))
        status = input_initialise_input(&p_of_ip_format->input, filename, p_array_handle);

    p_of_ip_format->docno = docno_from_p_docu(p_docu);
    p_of_ip_format->directed_object_id = OBJECT_ID_NONE;
    p_of_ip_format->stopped_object_id = OBJECT_ID_ENUM_START;

    p_of_ip_format->insert_position = IS_P_DATA_NONE(p_position) ? p_docu->cur : *p_position;

    docu_area_init(&p_of_ip_format->original_docu_area);

    if(status_ok(status))
    {
#if defined(VALIDATE_MAIN_ALLOCS) && defined(VALIDATE_LOAD_ALLOCS)
        alloc_validate((P_ANY) 1, "VALIDATE_LOAD_ALLOCS");
#endif

        if(UI_TEXT_TYPE_NONE == p_of_ip_format->process_status.reason.type)
        {
            p_of_ip_format->process_status.reason.type             = UI_TEXT_TYPE_RESID;
            p_of_ip_format->process_status.reason.text.resource_id = MSG_STATUS_LOADING;
        }

        process_status_begin(p_docu, &p_of_ip_format->process_status, PROCESS_STATUS_PERCENT);

        /* call all the objects to say a load has started */
        status = object_call_between(&p_of_ip_format->stopped_object_id, OBJECT_ID_MAX, p_docu, T5_MSG_LOAD_STARTED, P_DATA_NONE);
    }

    if(status_fail(status))
        status_consume(ownform_finalise_load(p_docu, p_of_ip_format));

    return(status);
}

_Check_return_
extern /*for ff_load*/  STATUS
load_is_file_loaded(
    _OutRef_    P_DOCNO p_docno /*DOCNO_NONE->not loaded, +ve->docno of thunk or file*/,
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    if(DOCNO_NONE != (*p_docno = docno_find_name(p_docu_name)))
    {
        const P_DOCU p_docu = p_docu_from_docno(*p_docno);

        if(!p_docu->flags.has_data)
            return(STATUS_DONE); /* 'tis a thunk */

        /* test for exact match */
        if(0 == name_compare(&p_docu->docu_name, p_docu_name, TRUE))
            return(create_error(ERR_DUPLICATE_FILE));

        return(create_error(ERR_DUPLICATE_LEAFNAME));
    }

    return(STATUS_OK); /* 'tis no there */
}

/******************************************************************************
*
* Load the config file for the specified object, if the file is present
*
******************************************************************************/

_Check_return_
extern STATUS
load_object_config_file(
    _InVal_     OBJECT_ID object_id)
{
    TCHARZ filename[BUF_MAX_PATHSTRING];
    STATUS status;

    {
    TCHARZ config_leaf[elemof32(CHOICES_FILE_EXAMPLE_STR)];
    consume_int(tstr_xsnprintf(config_leaf, elemof32(config_leaf),
                               CHOICES_FILE_FORMAT_STR,
                               (S32) object_id));
    status = file_find_on_path(filename, elemof32(filename), file_get_search_path(), config_leaf);
    } /*block*/

    if(status_done(status))
    {
        const P_DOCU p_docu_config = p_docu_from_config_wr();
        OF_IP_FORMAT of_ip_format = OF_IP_FORMAT_INIT;

        of_ip_format.process_status.flags.foreground = 1;

        if(status_ok(status = ownform_initialise_load(p_docu_config, &p_docu_config->cur, &of_ip_format, filename, NULL)))
        {
            status = load_ownform_file(&of_ip_format);

            status_accumulate(status, ownform_finalise_load(p_docu_config, &of_ip_format));
        }
    }

    return(status);
}

_Check_return_
extern STATUS
load_ownform_file(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format)
{
    STATUS status;
    U8 byte_read;
    QUICK_BLOCK_WITH_BUFFER(top_quick_block, 16); /* this only needs to be microscopic to account for file junk */
    quick_block_with_buffer_setup(top_quick_block);

    for(;;)
    {
        byte_read = binary_read_byte(&p_of_ip_format->input, &status);

        if(OF_CONSTRUCT_START == byte_read)
        {
            /*status_break(status = ensure_memory_froth());*/

            /* do prior to loading construct so that last construct loaded won't have another after it;
             * an aesthetic decision so that views come up nicely as opposed to any specific code need
            */
            load_reflect_status(p_of_ip_format);

            status = of_load_obtain_and_process_construct(p_of_ip_format, &top_quick_block, 0);

            /* discard data returned from top-level constructs! */
            quick_block_dispose(&top_quick_block);

            status_break(status);

            continue;
        }

        if(status_fail(status) || (EOF_READ == status))
            break;

        /* discard all other bytes read at top-level (e.g. any whitespace that we may add to prettify output) */
    }

    return(status);
}

_Check_return_
extern STATUS
load_ownform_command_file(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format)
{
    STATUS status;
    QUICK_BLOCK_WITH_BUFFER(top_quick_block, 16); /* this only needs to be microscopic */
    quick_block_with_buffer_setup(top_quick_block);

    status = of_load_obtain_and_process_construct(p_of_ip_format, &top_quick_block, 1);

    /* discard data returned from top-level constructs! */
    quick_block_dispose(&top_quick_block);

    return(status);
}

/******************************************************************************
*
* Pasting!
*
******************************************************************************/

_Check_return_
extern STATUS
load_ownform_from_array_handle(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InRef_maybenone_ PC_POSITION p_position,
    _InVal_     BOOL clip_data_from_cut_operation)
{
    STATUS status;
    OF_IP_FORMAT of_ip_format = OF_IP_FORMAT_INIT;

    of_ip_format.flags.insert = 1;

    of_ip_format.process_status.flags.foreground = 1;

    of_ip_format.process_status.reason.type             = UI_TEXT_TYPE_RESID;
    of_ip_format.process_status.reason.text.resource_id = MSG_STATUS_PASTING;

    of_ip_format.clip_data_from_cut_operation = (U8) clip_data_from_cut_operation;

    status_return(ownform_initialise_load(p_docu, p_position, &of_ip_format, NULL, p_array_handle));

    if(P_DATA_NONE == p_position)
    {
        OBJECT_AT_CURRENT_POSITION object_at_current_position;
        object_at_current_position.object_id = OBJECT_ID_NONE;
        status_consume(object_call_id(p_docu->object_id_flow, p_docu, T5_MSG_OBJECT_AT_CURRENT_POSITION, &object_at_current_position));
        of_ip_format.directed_object_id = object_at_current_position.object_id;
    }

    status = load_ownform_file(&of_ip_format);

    status_accumulate(status, ownform_finalise_load(p_docu, &of_ip_format));

    return(status);
}

extern void
load_reflect_status(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format)
{
    S32 percent = 0;
    U32 current_offset;
    U32 total_bytes;

    /* don't waste time doing muldiv64 unless really really needed */
    if(IP_INPUT_FILE == p_of_ip_format->input.state)
    {
        current_offset = p_of_ip_format->input.file.file_offset_in_bytes;
        if(current_offset < p_of_ip_format->process_status_threshold)
            return;
        total_bytes = p_of_ip_format->input.file.file_size_in_bytes;
    }
    else /* if(IP_INPUT_MEM == p_of_ip_format->input.state) */
    {
        assert(IP_INPUT_MEM == p_of_ip_format->input.state);
        current_offset = p_of_ip_format->input.mem.array_offset;
        if(current_offset < p_of_ip_format->process_status_threshold)
            return;
        total_bytes = array_elements32(p_of_ip_format->input.mem.p_array_handle);
    }

    if(0 == total_bytes)
        return;

    percent = muldiv64((S32) current_offset, 100, (S32) total_bytes);

    p_of_ip_format->process_status_threshold = ((U32) percent + 1) * (total_bytes / 100U);

    p_of_ip_format->process_status.data.percent.current = percent;

    process_status_reflect(&p_of_ip_format->process_status);
}

static DOCNO last_docno_loaded = DOCNO_NONE;

extern DOCNO
last_docno_loaded_query(void)
{
    return(last_docno_loaded);
}

/* Creates a docno using a template file on the templates resource path */

_Check_return_
static STATUS
new_docno_using_core(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     BOOL fReadOnly)
{
    STATUS status;
    POSITION position;
    OF_IP_FORMAT of_ip_format;

    zero_struct(of_ip_format);
    of_ip_format.flags.full_file_load = 1;
    of_ip_format.process_status.flags.foreground = 1;

    position_init_from_col_row(&position, 0, 0);

    if(status_ok(status = ownform_initialise_load(p_docu, &position, &of_ip_format, filename, NULL)))
    {
        status_assert(file_date(of_ip_format.input.file.file_handle, &p_docu->file_date));

        status = load_ownform_file(&of_ip_format);

        if(p_docu->flags.has_data)
            /* SKS 04jun96 this is needed for ancient Wordz documents without EndOfData (intro'd 1.04); SKS 06mar13 but do take care with no_data! */
            status_accumulate(status, t5_cmd_of_end_of_data(&of_ip_format));

        if(fReadOnly)
        {
            p_docu->flags.read_only = fReadOnly;
            status_assert(maeve_event(p_docu, T5_MSG_DOCU_READWRITE, P_DATA_NONE));
        }

        status_accumulate(status, ownform_finalise_load(p_docu, &of_ip_format));
    }

    return(status);
}

_Check_return_
static STATUS
new_docno_using_existing_file(
    _OutRef_    P_DOCNO p_docno,
    _In_z_      PCTSTR filename, /* already checked to exist */
    _InRef_     PC_DOCU_NAME p_docu_name /*copied*/,
    _InVal_     BOOL fReadOnly,
    _InVal_     BOOL allow_no_data)
{
    STATUS status;

    *p_docno = DOCNO_NONE;

    /* docno_new_create will return an existing docno if the names match and no data is loaded */
    if(DOCNO_NONE == (*p_docno = docno_new_create(p_docu_name)))
        status = status_nomem();
    else
    {
        const P_DOCU p_docu = p_docu_from_docno(*p_docno);

        last_docno_loaded = *p_docno;

#if !RISCOS
        /* if we really don't have an extension on this document then damn well make sure we don't */
        if((NULL == p_docu_name->extension) && (NULL != p_docu->docu_name.extension))
            tstr_clr(&p_docu->docu_name.extension);
#endif

        status = new_docno_using_core(p_docu, filename, fReadOnly);

        if(status_ok(status) && !p_docu->flags.has_data && !allow_no_data)
            status = create_error(ERR_NO_DATA_LOADED);

        if(status_ok(status))
            last_docno_loaded = *p_docno; /* yet again, to allow for dependent documents having been loaded */
        else
            docno_close(p_docno);
    }

    return(status);
}

_Check_return_
extern /*for ff_load*/ STATUS
new_docno_using(
    _OutRef_    P_DOCNO p_docno,
    _In_z_      PCTSTR leafname, /* might not exist */
    _InRef_     PC_DOCU_NAME p_docu_name /*copied*/,
    _InVal_     BOOL fReadOnly)
{
    PTSTR filename = NULL;
    STATUS status;

    *p_docno = DOCNO_NONE;

    {
    TCHARZ filename_buffer[BUF_MAX_PATHSTRING];
    status_return(status = file_find_on_path(filename_buffer, elemof32(filename_buffer), file_get_search_path(), leafname));
    if(status == 0)
        return(create_error(FILE_ERR_NOTFOUND));
    status_return(tstr_set(&filename, filename_buffer));
    } /*block*/

    status = new_docno_using_existing_file(p_docno, filename, p_docu_name, fReadOnly, FALSE);

    if(status_ok(status))
    {
        P_DOCU p_docu = p_docu_from_docno(*p_docno);

        if(!p_docu->flags.read_only)
            p_docu->flags.allow_modified_change = 1;
    }

    tstr_clr(&filename);

    return(status);
}

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
static STATUS
load_into_thunk_issue_init(
    _DocuRef_   P_DOCU p_docu)
{
    MSG_INITCLOSE msg_initclose;

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT1;
    status_return(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT2;
           return(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
}

_Check_return_
extern STATUS
load_into_thunk_using_array_handle(
    _InVal_     DOCNO docno,
    _InRef_     PC_ARRAY_HANDLE p_array_handle)
{
    STATUS status;
    OF_IP_FORMAT of_ip_format = OF_IP_FORMAT_INIT;
    POSITION position;
    const P_DOCU p_docu = p_docu_from_docno(docno);

    last_docno_loaded = docno;

    position_init_from_col_row(&position, 0, 0);

    of_ip_format.process_status.flags.foreground = 1;

    if(status_ok(status = ownform_initialise_load(p_docu, &position, &of_ip_format, NULL, p_array_handle)))
    {
        /* initialise skeleton services for this document */
        p_docu->flags.document_active = 1;

        status = load_into_thunk_issue_init(p_docu);

        if(status_fail(status))
            p_docu->flags.document_active = 0;
        else
        {
            p_docu->flags.init_close = 1;

            /*&p_docu->file_date >>>  obtained already from IStorage */

            status = load_ownform_file(&of_ip_format);
        }

        status_accumulate(status, ownform_finalise_load(p_docu, &of_ip_format));
    }

    return(status);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* Look up object identifier and obtain object id
* Must consider those objects not yet loaded!
*
******************************************************************************/

_Check_return_
extern STATUS
object_id_from_construct_id(
    _InVal_     U8 construct_id,
    _OutRef_    P_OBJECT_ID p_object_id)
{
    OBJECT_ID object_id;
    STATUS status;

    for(object_id = OBJECT_ID_FIRST; object_id < (OBJECT_ID) elemof32(of_load_statics.registered_construct_table); OBJECT_ID_INCR(object_id))
    {
        PC_REGISTERED_CONSTRUCT_TABLE p = &of_load_statics.registered_construct_table[object_id];

        if(construct_id != p->construct_id)
            continue;

        *p_object_id = object_id;

        if(p->p_construct_table)
            return(STATUS_OK);

        status = object_load(object_id);

        if(status == STATUS_MODULE_NOT_FOUND)
            status = STATUS_FAIL;

        return(status);
    }

    *p_object_id = OBJECT_ID_NONE;
    return(STATUS_FAIL);
}

static const U8
of_load_obtain_and_process_construct_special_handling[256] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 00 */ /*C0*/
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 10 */ /*C0*/
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 20 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 30 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 40 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, /* 50 */ /*5C:CH_BACKWARDS_SLASH*/ /*OF_ESCAPE_CHAR*/
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 60 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, /* 70 */ /*7B:CH_LEFT_CURLY_BRACKET,7D:CH_RIGHT_CURLY_BRACKET*/
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 80 */ /*C1*/
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 90 */ /*C1*/
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* B0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* C0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* D0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* E0 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* F0 */
};

_Check_return_
static STATUS
of_load_obtain_and_process_construct(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_patch,
    _InVal_     BOOL top_level_command_file)
{
    STATUS status;
    U8 byte_read;
    QUICK_BLOCK_WITH_BUFFER(con_quick_block, LOAD_QB_SIZE); /* construct line built up in here */
    quick_block_with_buffer_setup(con_quick_block);

    /* read chars until OF_CONSTRUCT_END read for this construct, then process */
    for(;;)
    {
        byte_read = binary_read_byte(&p_of_ip_format->input, &status);

        if(of_load_obtain_and_process_construct_special_handling[byte_read])
        {
            /* status_fail() and EOF_READ dealt with later for speed */

            if(OF_CONSTRUCT_END == byte_read)
            {
                if(!quick_block_byte_add_fast(&con_quick_block, CH_NULL))
                    status_break(status = quick_block_nullch_add(&con_quick_block));

                status_assert(status = of_load_process_construct(p_of_ip_format, p_quick_block_patch, &con_quick_block));

                break;
            }

            if(OF_CONSTRUCT_START == byte_read)
            {
                /* recursively process and patch on the end replacement data from constructs found whilst loading this construct */
                status_assert(status = of_load_obtain_and_process_construct(p_of_ip_format, &con_quick_block, 0));
                status_break(status);

                continue;
            }

            /* Any ASCII control characters (C0 [0x00..0x1F]) that we find at
             * this level (including LF/CR) are noise and shall be discarded.
             * The upper range (C1 [0x80..0x9F]) is used to encode C0 (e.g. embedded Draw files have a lot of these).
             * However still allow 0x7F thru for backwards compatibility of very old files.
             * EOF_READ should pass this test for speed.
             */
            if((byte_read & 0x7F) <= 0x1F)
            {
                assert_EQ(TRUE, (EOF_READ & 0x7F) <= 0x1F);

                if(status_fail(status))
                    break;

                if(EOF_READ == status)
                {
                    if(top_level_command_file)
                    {
                        status = STATUS_OK;
                        if(0 != quick_block_bytes(&con_quick_block))
                            if(status_ok(status = quick_block_nullch_add(&con_quick_block)))
                                status_assert(status = of_load_process_construct(p_of_ip_format, p_quick_block_patch, &con_quick_block));
                    }
                    else
                        status = create_error(ERR_EOF_BEFORE_FINISHED);

                    break;
                }

                /* C0 is noise */
                if(byte_read < 0x80)
                    continue;

                /* map C1 from file to C0 here */
                byte_read -= 0x80;
            }
            else if(OF_ESCAPE_CHAR == byte_read)
            {
                byte_read = binary_read_byte(&p_of_ip_format->input, &status);

                if(status_fail(status))
                    break;

                if(EOF_READ == status)
                {
                    status = create_error(ERR_EOF_BEFORE_FINISHED);
                    break;
                }

                /* escaped bytes yield that byte: EXCEPT escaped 0xFF which yields 0x7F */
                if(byte_read == 0xFF)
                    byte_read = 0x7F;
            }
        }

        if(quick_block_byte_add_fast(&con_quick_block, byte_read))
            continue;

        status_break(status = quick_block_byte_add(&con_quick_block, byte_read));
    }

    /* caller isn't interested in this; he wants converted data */
    quick_block_dispose(&con_quick_block);

    return(status);
}

_Check_return_
static STATUS
of_load_process_construct(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_patch,
    _InRef_     P_QUICK_BLOCK p_quick_block_construct_line /*in, poked*/)
{
    OBJECT_ID object_id = OBJECT_ID_SKEL;
    OBJECT_ID omit_object_id = OBJECT_ID_NONE;
    P_A7STR a7str_construct_name;
    U32 construct_name_length = 0;
    P_U8 p_construct_body;
    CONSTRUCT_TABLE_SEARCH construct_table_search;
    PC_CONSTRUCT_TABLE p_construct;

    if(NULL == (a7str_construct_name = (P_A7STR) quick_block_ptr_wr(p_quick_block_construct_line)))
        return(STATUS_OK);

    /*trace_1(TRACE_APP_LOAD, TEXT("Processing construct : %s"), a7str_construct_name);*/

    /* skip over construct name to delimiter */
    for(p_construct_body = a7str_construct_name; ; ++p_construct_body)
    {
        if(CH_NULL == *p_construct_body)
        {   /* leave pointing to null args */
            construct_name_length = PtrDiffBytesU32(p_construct_body, a7str_construct_name);
            break;
        }

        if(OF_CONSTRUCT_ARGLIST_SEPARATOR == *p_construct_body)
        {   /* leave construct name CH_NULL-terminated, p_construct_body pointing at args (null or otherwise) */
            construct_name_length = PtrDiffBytesU32(p_construct_body, a7str_construct_name);
            *p_construct_body++ = CH_NULL;
            break;
        }

        if(OF_CONSTRUCT_OBJECT_SEPARATOR == *p_construct_body)
        {   /* leave construct name CH_NULL-terminated, p_construct_body pointing at object id letter */
            construct_name_length = PtrDiffBytesU32(p_construct_body, a7str_construct_name);
            *p_construct_body++ = CH_NULL;

            /* lookup, skipping over object id letter to possible construct args */

            if(CH_NULL == *p_construct_body)
            {
                /* leave p_construct_body pointing to null args */
                assert0();
                break;
            }

            assert(OF_CONSTRUCT_ARGLIST_SEPARATOR != *p_construct_body);
            if(OF_CONSTRUCT_ARGLIST_SEPARATOR != *p_construct_body)
            {
                const U8 construct_id = *p_construct_body++; /* over the letter */
                STATUS status = object_id_from_construct_id(construct_id, &object_id);

                if(status_fail(status)) /* SKS after 1.09 25aug94 */
                {
                    if(status != STATUS_FAIL)
                        return(status);

                    /* Skip the construct - not fail, since we wish to continue in file */
                    return(STATUS_OK);
                }
            }

            /* always skip delimiter too, p_construct_body pointing at args */
            assert((OF_CONSTRUCT_ARGLIST_SEPARATOR == *p_construct_body) || (CH_NULL == *p_construct_body));
            if(CH_NULL != *p_construct_body)
                ++p_construct_body; /* SKS 12apr93 */

            break;
        }
    }

    construct_table_search.a7str_construct_name  = a7str_construct_name;
    construct_table_search.construct_name_length = construct_name_length;

    p_construct = NULL;

    if(OBJECT_ID_NONE != object_id)
    {
        const PC_REGISTERED_CONSTRUCT_TABLE p = &of_load_statics.registered_construct_table[object_id];
        const U32 table_entries = (construct_name_length <= SHORT_CONSTRUCT_NAME_LENGTH) ? p->short_construct_table_entries : p->construct_table_entries;

        /* don't scan this one again */
        omit_object_id = object_id;

        if(0 != table_entries)
        {
            p_construct = (PC_CONSTRUCT_TABLE)
                bsearch(&construct_table_search, p->p_construct_table, table_entries, sizeof(*p->p_construct_table), construct_table_compare_bsearch);
        }
    }

    if(NULL == p_construct)
    {
        /* construct name not in indicated object's construct table, so loop through all others that are currently loaded */
        object_id = OBJECT_ID_ENUM_START;
        while(status_ok(object_next(&object_id)))
        {
            const PC_REGISTERED_CONSTRUCT_TABLE p = &of_load_statics.registered_construct_table[object_id];
            const U32 table_entries = (construct_name_length <= SHORT_CONSTRUCT_NAME_LENGTH) ? p->short_construct_table_entries : p->construct_table_entries;

            if((0 != table_entries) && (object_id != omit_object_id))
            {
                if(NULL != (p_construct = (PC_CONSTRUCT_TABLE)
                        bsearch(&construct_table_search, p->p_construct_table, p->construct_table_entries, sizeof(*p->p_construct_table), construct_table_compare_bsearch)))
                    break;
            }
        }
    }

    if(NULL == p_construct)
    {
        /* construct name not in any table, so give up */
        myassert1(TEXT("of_load_process_construct failed to find construct %s in any table"), report_sbstr(a7str_construct_name));
        p_of_ip_format->flags.unknown_construct = 1;
        return(STATUS_OK);     /* Not fail, since we wish to continue in file */
    }

    /* Some constructs may not be loaded under certain conditions */
    /* Test for this, and load the construct if needed, otherwise discard it */

    if(p_construct->bits.reject_if_file_insertion)
    {
        BOOL reject = p_of_ip_format->flags.insert && !p_of_ip_format->flags.is_template;

        if(reject)
            /* Skip the construct - not fail, since we wish to continue in file */
            return(STATUS_OK);
    }

    if(p_construct->bits.reject_if_template_insertion)
    {
        BOOL reject = p_of_ip_format->flags.insert && p_of_ip_format->flags.is_template;

        if(reject)
            /* Skip the construct - not fail, since we wish to continue in file */
            return(STATUS_OK);
    }

    return(of_load_process_construct_end(p_of_ip_format, p_quick_block_patch, p_construct, object_id, p_construct_body));
}

static void
of_load_process_contruct_read_arg_type_f64(
    _InoutRef_  P_P_U8 p_p_construct_body,
    _InoutRef_  P_ARGLIST_ARG p_arg)
{
    P_U8 p_next;
    p_arg->val.f64 = strtod(*p_p_construct_body, &p_next); /* NB NOT ui_strtod, Fireworkz files contain 'C' locale format numbers */
    *p_p_construct_body = p_next;
}

#if !USTR_IS_SBSTR

/* Convert from Latin-1 + Unicode constructs XDR to UTF-8, also with inlines */

_Check_return_
static STATUS
of_load_process_construct_read_arg_type_ustr(
    _InoutRef_  P_P_U8 p_p_construct_body,
    _InoutRef_  P_ARGLIST_ARG p_arg)
{
    P_U8 p_construct_body = *p_p_construct_body;
    P_U8 p_data = p_construct_body;
    SBCHAR c;
    STATUS status = STATUS_OK;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
    quick_ublock_with_buffer_setup(quick_ublock);

    /* patch up strings in place wherever possible - avoids allocation */
    p_arg->val.ustr_inline = (P_USTR_INLINE) p_data;

    while(status_ok(status))
    {
        if(is_inline(p_construct_body))
        {
            const U32 size = inline_bytecount((PC_UCHARS_INLINE) p_construct_body);

            if(IL_UTF8 == inline_code(p_construct_body))
            {   /* Convert from Unicode inline (already UTF-8) */
                PC_UCHARS uchars = inline_data_ptr(PC_UCHARS, p_construct_body);
                const U32 il_data_size = inline_data_size(p_construct_body);

                if(NULL != p_data)
                {   /* move all data read so far over to quick_ublock as we will expand when adding this character */
                    const U32 n_so_far = PtrDiffBytesU32(p_data, p_arg->val.ustr_inline);
                    if(0 != n_so_far)
                        status_break(status = quick_ublock_uchars_add(&quick_ublock, (PC_UCHARS) p_arg->val.ustr_inline, n_so_far));
                    p_data = NULL;
                }

                status = quick_ublock_uchars_add(&quick_ublock, uchars, il_data_size);
            }
            else
            {   /* Copy other inlines in USTR args */
                if(NULL != p_data)
                {
                    if(p_data != p_construct_body)
                        memmove32(p_data, p_construct_body, size);
                    p_data += size;
                }
                else
                {
                    status = quick_ublock_uchars_add(&quick_ublock, (PC_UCHARS) p_construct_body, size);
                }
            }

            p_construct_body += size;
            continue;
        }

        c = *p_construct_body++;

        if(OF_CONSTRUCT_ARG_SEPARATOR == c)
        {
            break;
        }
        else if(CH_NULL == c)
        {
            --p_construct_body; /* leave pointing at terminator */
            break;
        }
        else if(OF_ESCAPE_CHAR == c)
        {
            c = *p_construct_body++; /* add character following the escape */
        }

        if(NULL != p_data)
        {
            if(u8_is_ascii7(c))
            {
                *p_data++ = c;
                continue;
            }

            /* move all data read so far over to quick_ublock as we will expand when adding this character */
            status_break(status = quick_ublock_uchars_add(&quick_ublock, (PC_UCHARS) p_arg->val.ustr_inline, PtrDiffBytesU32(p_data, p_arg->val.ustr_inline)));
            p_data = NULL;
        }

#if !USTR_IS_SBSTR
        /* Simple conversion from Latin-1 character of XDR to UTF-8 */
        if((c >= 0x80) && (c <= 0x9F))
        {   /* Watch out for 0080..009F differences !!! */
            UCS4 ucs4 = ucs4_from_sbchar_with_codepage(c, get_system_codepage());
            status = quick_ublock_ucs4_add(&quick_ublock, ucs4);
        }
        else
#endif
        {
            status = quick_ublock_ucs4_add(&quick_ublock, c);
        }
    }

    if(NULL != p_data)
    {
        *p_data++ = CH_NULL; /* all still in place - avoids allocation */
    }
    else
    {
        if(status_ok(status))
            status = ustr_set_n(&p_arg->val.ustr_wr, quick_ublock_ustr(&quick_ublock), quick_ublock_bytes(&quick_ublock));

        if(status_ok(status))
            p_arg->type = (ARG_TYPE) (ARG_TYPE_USTR | ARG_ALLOC);
    }

    quick_ublock_dispose(&quick_ublock);

    *p_p_construct_body = p_construct_body;

    return(status);
}

#endif /* USTR_IS_SBSTR */

#if defined(ARG_TYPE_TSTR_DISTINCT)

/* Convert from Latin-1 + Unicode constructs XDR to TSTR */

_Check_return_
static STATUS
of_load_process_construct_read_arg_type_tstr(
    _InoutRef_  P_P_U8 p_p_construct_body,
    _InoutRef_  P_ARGLIST_ARG p_arg)
{
    P_U8 p_construct_body = *p_p_construct_body;
    SBCHAR c;
    STATUS status = STATUS_OK;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 256);
    quick_tblock_with_buffer_setup(quick_tblock);

    while(status_ok(status))
    {
        if(is_inline(p_construct_body))
        {
            const U32 size = inline_bytecount((PC_UCHARS_INLINE) p_construct_body);

            if(IL_UTF8 == inline_code(p_construct_body))
            {   /* Convert from Unicode inline (UTF-8) to UTF-16 */
#if !TSTR_IS_SBSTR
                PC_UCHARS uchars = inline_data_ptr(PC_UCHARS, p_construct_body);
                const U32 il_data_size = inline_data_size(p_construct_body);

                status = quick_tblock_uchars_add(&quick_tblock, uchars, il_data_size);
#else
                assert0();
#endif /* TSTR_IS_SBSTR */
            }
            else
            {   /* Skip other inlines in TSTR args */
                assert0();
            }

            p_construct_body += size;
            continue;
        }

        c = *p_construct_body++;

        if(OF_CONSTRUCT_ARG_SEPARATOR == c)
        {
            break;
        }
        else if(CH_NULL == c)
        {
            --p_construct_body; /* leave pointing at terminator */
            break;
        }
        else if(OF_ESCAPE_CHAR == c)
        {
            c = *p_construct_body++; /* add character following the escape */
        }

#if !TSTR_IS_SBSTR
        /* Simple conversion from Latin-1 character of XDR to UTF-16 */
        if((c >= 0x80) && (c <= 0x9F))
        {   /* Watch out for 0080..009F differences !!! */
            UCS4 ucs4 = ucs4_from_sbchar_with_codepage(c, get_system_codepage());
            status = quick_tblock_ucs4_add(&quick_tblock, ucs4);
        }
        else
#endif
        {
            status = quick_tblock_tchar_add(&quick_tblock, c);
        }
    }

    if(status_ok(status))
        status = tstr_set_n(&p_arg->val.tstr_wr, quick_tblock_tstr(&quick_tblock), quick_tblock_chars(&quick_tblock));

    if(status_ok(status))
        p_arg->type = (ARG_TYPE) (ARG_TYPE_TSTR | ARG_ALLOC);

    quick_tblock_dispose(&quick_tblock);

    *p_p_construct_body = p_construct_body;

    return(status);
}

#endif /* ARG_TYPE_TSTR_DISTINCT */

_Check_return_
static STATUS
of_load_process_construct_end(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_QUICK_BLOCK p_quick_block_patch,
    _InRef_     PC_CONSTRUCT_TABLE p_construct,
    _In_        OBJECT_ID object_id,
    P_U8 p_construct_body_in /*in, poked*/)
{
    P_U8 p_construct_body = p_construct_body_in;
    ARGLIST_HANDLE arglist_handle;
    ARRAY_INDEX arg_idx, n_args, n_args_missing;
    STATUS status;

    status_return(arglist_prepare(&arglist_handle, p_construct->args));

    n_args = n_arglist_args(&arglist_handle);
    n_args_missing = 0;

    for(arg_idx = 0; arg_idx < n_args; ++arg_idx)
    {
        const P_ARGLIST_ARG p_arg = p_arglist_arg(&arglist_handle, arg_idx);
        const ARG_TYPE arg_type = (ARG_TYPE) (p_arg->type & ARG_TYPE_MASK);
        SBCHAR c = *p_construct_body;
        U32 u32, u32_limit;

        /* this arg missing? */
        if(OF_CONSTRUCT_ARG_SEPARATOR == c)
        {
            c = CH_NULL;
            ++p_construct_body;
        }

        /* arg(s) missing? */
        if(CH_NULL == c)
        {
            if(p_arg->type & ARG_MANDATORY)
            {
                BOOL interactive = command_query_interactive();

                /* SKS 05may94 allows mandatory args to be omitted when interactive */
                if(!interactive)
                {
                    /* SKS 19aug94 realises old test never reported errors */
                    if((p_arg->type & ARG_MANDATORY_OR_BLANK) == ARG_MANDATORY_OR_BLANK)
                    {
                        if(arg_type == ARG_TYPE_USTR)
                        {
                            p_arg->val.ustr = ustr_empty_string;
                            continue;
                        }
#if defined(ARG_TYPE_TSTR_DISTINCT)
                        if(arg_type == ARG_TYPE_TSTR)
                        {
                            p_arg->val.tstr = tstr_empty_string;
                            continue;
                        }
#endif
                        assert((arg_type == ARG_TYPE_USTR) || (arg_type == ARG_TYPE_TSTR));
                    }

                    reperr(ERR_MANDATORY_ARG_MISSING, _tstr_from_sbstr(p_construct->a7str_construct_name), (S32) arg_idx, (S32) arg_type);

                    /* ignore the entire command! */
                    arglist_dispose(&arglist_handle);

                    /* SKS 05may94 used to return STATUS_FAIL but that freaked people's files */
                    return(STATUS_OK);
                }
            }

            arg_dispose(&arglist_handle, arg_idx);
            ++n_args_missing;

            continue;
        }

        switch(arg_type)
        {
        default:
#if CHECKING
            myassert1(TEXT("unknown ARG_TYPE ") U32_TFMT TEXT(" for arg processing"), (U32) arg_type);

            /*FALLTHRU*/

        case ARG_TYPE_U8C:
#endif
            p_arg->val.u8c = *p_construct_body++;
            break;

        case ARG_TYPE_U8N:
            u32 = fast_strtoul(p_construct_body, &p_construct_body);
            u32_limit = 0xFF;
            if(u32 > u32_limit)
            {
                reperr(ERR_ARG_FAILURE, _tstr_from_sbstr(p_construct->a7str_construct_name), arg_idx, u32, u32_limit);
                status = STATUS_OK;
                goto dispose_args;
            }
            p_arg->val.u8n = (U8) u32;
            break;

        case ARG_TYPE_BOOL:
            u32 = fast_strtoul(p_construct_body, &p_construct_body);
            p_arg->val.fBool = (BOOL) (u32 != 0);
            break;

        case ARG_TYPE_S32:
            p_arg->val.s32 = fast_strtol(p_construct_body, &p_construct_body);
            break;

        case ARG_TYPE_X32:
            p_arg->val.x32 = (U32) strtoul(p_construct_body, &p_construct_body, 0); /* expect 0xXYZ */
            break;

        case ARG_TYPE_COL:
            u32 = fast_strtoul(p_construct_body, &p_construct_body);
            u32_limit = MAX_COL;
            if(u32 > u32_limit)
            {
                reperr(ERR_ARG_FAILURE, _tstr_from_sbstr(p_construct->a7str_construct_name), arg_idx, u32, u32_limit);
                status = STATUS_OK;
                goto dispose_args;
            }
            p_arg->val.col = (COL) u32;

            /* offset the COL ref at load time */
            p_arg->val.col += (p_of_ip_format->insert_position.slr.col - p_of_ip_format->original_docu_area.tl.slr.col);
            break;

        case ARG_TYPE_ROW:
            u32 = fast_strtoul(p_construct_body, &p_construct_body);
            u32_limit = MAX_ROW;
            if(u32 > u32_limit)
            {
                reperr(ERR_ARG_FAILURE, _tstr_from_sbstr(p_construct->a7str_construct_name), arg_idx, u32, u32_limit);
                status = STATUS_OK;
                goto dispose_args;
            }
            p_arg->val.row = (ROW) u32;

            /* offset the ROW ref at load time */
            p_arg->val.row += (p_of_ip_format->insert_position.slr.row - p_of_ip_format->original_docu_area.tl.slr.row);
            break;

        case ARG_TYPE_F64:
            of_load_process_contruct_read_arg_type_f64(&p_construct_body, p_arg);
            break;

        case ARG_TYPE_RAW:
        case ARG_TYPE_RAW_DS:
            { /* raw data immediately preceded by 32-bit length and comma */
            static /*poked*/ ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(U8), FALSE);
            S32 size;
            P_U8 p_data;

            u32 = fast_strtoul(p_construct_body, &p_construct_body);

            size = (S32) u32;

            /* test for malformed length arg - can't process further args if this has gone wrong */
            if((size <= 0) || (NULL == p_construct_body)  || (*p_construct_body != OF_CONSTRUCT_ARG_SEPARATOR))
            {
                reperr(ERR_RAW_ARG_FAILURE, _tstr_from_sbstr(p_construct->a7str_construct_name), arg_idx, size, (NULL != p_construct_body) ? *p_construct_body : CH_QUESTION_MARK);
                status = STATUS_OK;
                goto dispose_args;
            }

            ++p_construct_body;

            /* obviously anyone wanting to use the raw data in a fashion other than a byte stream
             * must recast (via an as yet non-existent mechanism) the array handle with an
             * array_init_block of their own to get correct number of elements, element size etc.
            */
#if WINDOWS
            /* use appropriate allocator - may be needed for Dial Solutions Draw file manipulation routines */
            array_init_block.use_alloc = (ARG_TYPE_RAW_DS == arg_type) ? ALLOC_USE_DS_ALLOC : ALLOC_USE_ALLOC;
#endif
            p_data = al_array_alloc_U8(&p_arg->val.raw, size, &array_init_block, &status);

            if(NULL == p_data)
            {
                myassert3x(p_data != NULL,
                           TEXT("of_load_process_construct failed to allocate ARG_TYPE_RAW arg ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" bytes in %s(UGH!!!RAW)"), arg_idx, size, report_sbstr(p_construct->a7str_construct_name));
                goto dispose_args;
            }

            p_arg->type |= ARG_ALLOC;

            memcpy32(p_data, p_construct_body, size);

            p_construct_body = (P_U8) p_construct_body + size; /* pragmatic, limited fix - assumes that the only BIG things happen as single things and don't add up */
            break;
            }

        case ARG_TYPE_USTR:
            {
#if !USTR_IS_SBSTR
            if(status_fail(status = of_load_process_construct_read_arg_type_ustr(&p_construct_body, p_arg)))
                goto dispose_args;
#else
            P_U8 p_data = p_construct_body;

            /* patch up strings in place - avoids allocation */
            p_arg->val.ustr_inline = (P_USTR_INLINE) p_data;

            for(;;)
            {
                if(is_inline(p_construct_body))
                {   /* including any IL_UTF8, which we must simply preserve here */
                    const S32 size = inline_bytecount(p_construct_body);
                    if(p_data != p_construct_body)
                        memmove32(p_data, p_construct_body, size);
                    p_construct_body += size;
                    p_data           += size;
                    continue;
                }

                c = *p_construct_body++;

                if(OF_CONSTRUCT_ARG_SEPARATOR == c)
                {
                    break;
                }
                else if(CH_NULL == c)
                {
                    --p_construct_body; /* leave pointing at terminator */
                    break;
                }
                else if(OF_ESCAPE_CHAR == c)
                {
                    c = *p_construct_body++; /* add character following the escape */
                }

                *p_data++ = c;
            }

            *p_data++ = CH_NULL;
#endif /* USTR_IS_SBSTR */

            continue; /* not break: we're now either pointing at the start of the next arg or at the terminator */
            }

#if defined(ARG_TYPE_TSTR_DISTINCT)
        case ARG_TYPE_TSTR:
            if(status_fail(status = of_load_process_construct_read_arg_type_tstr(&p_construct_body, p_arg)))
                goto dispose_args;

            continue; /* not break: we're now either pointing at the start of the next arg or at the terminator */
#endif /* ARG_TYPE_TSTR_DISTINCT */
        }

        /* skip junk following arg till separator or terminator found */
        for(;;)
        {
            c = *p_construct_body++;

            if(OF_CONSTRUCT_ARG_SEPARATOR == c)
            {
                break;
            }
            else if(CH_NULL == c)
            {
                /* leave it pointing at the terminator */
                --p_construct_body;
                break;
            }

            myassert4(TEXT("junk char 0x%.2X(%c) after arg ") S32_TFMT TEXT(" in %s"), c, c, arg_idx, report_sbstr(p_construct->a7str_construct_name));
        }
    }

    if((n_args_missing == n_args) && n_args)
        arglist_dispose(&arglist_handle);

#if TRACE_ALLOWED
    trace_2(TRACE_APP_ARGLIST, TEXT("loading construct '%s' object id ") S32_TFMT, report_sbstr(p_construct->a7str_construct_name), (S32) object_id);
    arglist_trace(&arglist_handle, TEXT("loaded args "));
#endif

    {
    const P_DOCU p_docu = (DOCNO_NONE != p_of_ip_format->docno) ? p_docu_from_docno(p_of_ip_format->docno) : P_DOCU_NONE;
    CONSTRUCT_CONVERT construct_convert;

    construct_convert.arglist_handle = arglist_handle;
    construct_convert.p_of_ip_format = p_of_ip_format;
    construct_convert.p_construct = p_construct;
    construct_convert.object_array_handle_uchars_inline = 0;

    if(of_load_statics.registered_construct_table[object_id].needs_help)
    {
        U32 n_bytes;

        status = object_call_id(object_id, p_docu, T5_MSG_LOAD_CONSTRUCT_OWNFORM, &construct_convert);

        /* patch in converted construct's result into our string - NB this can only be one or more inlines! */
        n_bytes = array_elements32(&construct_convert.object_array_handle_uchars_inline);

        if(0 != n_bytes)
            status = quick_block_bytes_add(p_quick_block_patch, array_rangec(&construct_convert.object_array_handle_uchars_inline, BYTE, 0, n_bytes), n_bytes);

        al_array_dispose(&construct_convert.object_array_handle_uchars_inline);
    }
    else
        status = execute_loaded_command(p_docu, &construct_convert, object_id);
    } /*block*/

dispose_args:;

    arglist_dispose(&arglist_handle);

    return(status);
}

extern void
of_load_S32_arg_offset_as_COL(
    _InRef_     P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_ARGLIST_ARG p_arg)
{
    /* offset a S32 arg as an unprocessed COL ref arg at load time */
    assert(p_arg->type == ARG_TYPE_S32);
    p_arg->type = ARG_TYPE_COL;
    p_arg->val.col += (p_of_ip_format->insert_position.slr.col - p_of_ip_format->original_docu_area.tl.slr.col);
}

extern void
of_load_S32_arg_offset_as_ROW(
    _InRef_     P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_ARGLIST_ARG p_arg)
{
    /* offset a S32 arg as an unprocessed ROW ref arg at load time */
    assert(p_arg->type == ARG_TYPE_S32);
    p_arg->type = ARG_TYPE_ROW;
    p_arg->val.row += (p_of_ip_format->insert_position.slr.row - p_of_ip_format->original_docu_area.tl.slr.row);
}

_Check_return_
extern STATUS
register_object_construct_table(
    _InVal_     OBJECT_ID object_id,
    _InRef_     P_CONSTRUCT_TABLE p_construct_table /*data sorted*/,
    _InVal_     BOOL needs_help)
{
    P_REGISTERED_CONSTRUCT_TABLE p;
    U32 construct_table_entries;
    U32 short_construct_table_entries = 0;

    for(construct_table_entries = 0; NULL != p_construct_table[construct_table_entries].a7str_construct_name; ++construct_table_entries)
    {
        const U32 construct_name_length = strlen32(p_construct_table[construct_table_entries].a7str_construct_name);
        assert(construct_name_length < U8_MAX);
        p_construct_table[construct_table_entries].bits.construct_name_length = (U8) construct_name_length;
        if(construct_name_length <= SHORT_CONSTRUCT_NAME_LENGTH)
            ++short_construct_table_entries;
    }

    qsort(p_construct_table, construct_table_entries, sizeof(*p_construct_table), construct_table_compare_qsort);

    p = &of_load_statics.registered_construct_table[object_id];
    assert(!p->p_construct_table); /* check for objects getting it wrong */

    p->p_construct_table = p_construct_table;
    p->construct_table_entries = construct_table_entries;
    p->short_construct_table_entries = short_construct_table_entries;
    /* do not set p->construct_id = CH_NULL; 'cos construct_id binding comes out of config file */
    p->needs_help = (U8) needs_help;

    return(STATUS_OK); /* simply for most object handlers switch() efficiency on T5_MSG_IC__STARTUP */
}

typedef struct TEMPLATE_LIST_ENTRY
{
    QUICK_TBLOCK_WITH_BUFFER(fullname_quick_tblock, 64); /* NB buffer adjacent for fixup */

    QUICK_TBLOCK_WITH_BUFFER(leafname_quick_tblock, elemof32("doseight.fwk")); /* NB buffer adjacent for fixup */

    int sort_order; /* 0 by default */
}
TEMPLATE_LIST_ENTRY, * P_TEMPLATE_LIST_ENTRY;

enum SELECT_TEMPLATE_CONTROL_IDS
{
    SELECT_TEMPLATE_ID_LIST = 333
};

static /*poked*/ DIALOG_CONTROL
select_template_list =
{
    SELECT_TEMPLATE_ID_LIST, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0 },
    { DRT(LTLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CTL_CREATE
select_template_ctl_create[] =
{
    { &dialog_main_group },

    { &select_template_list, &stdlisttext_data },

    { &defbutton_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static S32 g_selected_template = 0;

static UI_SOURCE
select_template_list_source;

_Check_return_
static STATUS
dialog_select_template_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case SELECT_TEMPLATE_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &select_template_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_select_template_ctl_create_state(P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case SELECT_TEMPLATE_ID_LIST:
        {
        const PC_S32 p_selected_template = (PC_S32) p_dialog_msg_ctl_create_state->client_handle;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_ALTERNATE;
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = *p_selected_template;
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_select_template_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    const P_S32 p_selected_template = (P_S32) p_dialog_msg_process_end->client_handle;
    *p_selected_template = ui_dlg_get_list_idx(p_dialog_msg_process_end->h_dialog, SELECT_TEMPLATE_ID_LIST);

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_select_template)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_select_template_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_select_template_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_select_template_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* oh, the joys of sorting ARRAY_QUICK_BLOCKs...
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, template_list_sort, TEMPLATE_LIST_ENTRY)
{
    QSORT_ARG1_VAR_DECL(P_TEMPLATE_LIST_ENTRY, p_template_list_entry_1);
    QSORT_ARG2_VAR_DECL(P_TEMPLATE_LIST_ENTRY, p_template_list_entry_2);

    const S32 res_so = p_template_list_entry_1->sort_order - p_template_list_entry_2->sort_order;

    if(0 == res_so)
    {
        P_QUICK_TBLOCK p_quick_tblock_1 = &p_template_list_entry_1->leafname_quick_tblock;
        P_QUICK_TBLOCK p_quick_tblock_2 = &p_template_list_entry_2->leafname_quick_tblock;

        PCTSTR tstr_1 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_1)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_1)) : (PCTSTR) (p_quick_tblock_1 + 1);
        PCTSTR tstr_2 = (0 != quick_tblock_array_handle_ref(p_quick_tblock_2)) ? array_tstr(&quick_tblock_array_handle_ref(p_quick_tblock_2)) : (PCTSTR) (p_quick_tblock_2 + 1);

        int res_cmp = tstr_compare_nocase(tstr_1, tstr_2);

        return(res_cmp);
    }

    return((res_so > 0) ? 1 : -1);
}

_Check_return_
static int
template_sort_order(
    _In_z_      PCTSTR leafname)
{
    const PC_DOCU p_docu_config = p_docu_from_config();
    const PC_ARRAY_HANDLE p_ui_numform_handle = &p_docu_config->numforms;
    const ARRAY_INDEX n_elements = array_elements(p_ui_numform_handle);
    ARRAY_INDEX i;
    const UI_NUMFORM_CLASS ui_numform_class = UI_NUMFORM_CLASS_LOAD_TEMPLATE;

    for(i = 0; i < n_elements; ++i)
    {
        const PC_UI_NUMFORM p_ui_numform = array_ptrc(p_ui_numform_handle, UI_NUMFORM, i);
        PC_USTR numform_leafname;
        PC_A7STR numform_sort_order;

        if(p_ui_numform->numform_class != ui_numform_class)
            continue;

        numform_leafname = p_ui_numform->ustr_numform;
        PTR_ASSERT(numform_leafname);

        if(0 != tstr_compare_nocase(leafname, _tstr_from_ustr(numform_leafname)))
            continue;

        numform_sort_order = (PC_A7STR) p_ui_numform->ustr_opt;

        return(atoi(numform_sort_order));
    }

    return(0);
}

_Check_return_
static STATUS
list_templates_from(
    P_ARRAY_HANDLE p_template_list_handle,
    /*inout*/ P_P_FILE_OBJENUM p_p_file_objenum,
    P_FILE_OBJINFO p_file_objinfo,
    _InVal_     BOOL allow_dirs,
    _InoutRef_  P_PIXIT p_max_width)
{
    P_TEMPLATE_LIST_ENTRY p_template_list_entry;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_template_list_entry), 1);
    STATUS status = STATUS_OK;

    for(; p_file_objinfo; p_file_objinfo = file_find_next(p_p_file_objenum))
    {
        BOOL is_file = (file_objinfo_type(p_file_objinfo) == FILE_OBJECT_FILE);

        if(is_file || allow_dirs)
        {
            if(NULL != (p_template_list_entry = al_array_extend_by(p_template_list_handle, TEMPLATE_LIST_ENTRY, 1, &array_init_block, &status)))
            {
                quick_tblock_with_buffer_setup(p_template_list_entry->fullname_quick_tblock);
                quick_tblock_with_buffer_setup(p_template_list_entry->leafname_quick_tblock);

                if(status_ok(status = file_objinfo_name(p_file_objinfo, &p_template_list_entry->leafname_quick_tblock)))
                if(status_ok(status = file_objenum_fullname(p_p_file_objenum, p_file_objinfo, &p_template_list_entry->fullname_quick_tblock)))
                {
                    PCTSTR tstr = quick_tblock_tstr(&p_template_list_entry->leafname_quick_tblock);
                    const PIXIT width = ui_width_from_tstr(tstr);
                    *p_max_width = MAX(*p_max_width, width);
                    p_template_list_entry->sort_order = template_sort_order(tstr);
                }
            }
        }

        status_break(status);
    }

    return(status);
}

_Check_return_
extern /*for ff_load*/ STATUS
select_a_template(
    _DocuRef_   P_DOCU cur_p_docu,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/,
    /*out*/ P_BOOL p_just_the_one,
    _InVal_     BOOL allow_dirs,
    _InRef_     PC_UI_TEXT p_ui_text_caption)
{
    ARRAY_HANDLE template_list_handle = 0;
    PCTSTR p_template_name;
    PIXIT max_width = DIALOG_SYSCHARSL_H(8); /* arbitrary minimum */
    STATUS status = STATUS_OK;

    select_template_list_source.type = UI_SOURCE_TYPE_NONE;

    {
    P_FILE_OBJENUM p_file_objenum;
    P_FILE_OBJINFO p_file_objinfo;
    TCHARZ wildcard_buffer[8];

#if WINDOWS

    /* Start with All Users Application Data / Templates directory */
    {
    TCHARZ templates_directory[BUF_MAX_PATHSTRING];

    if(GetCommonAppDataDirectoryName(templates_directory, elemof32(templates_directory)))
    {
        tstr_xstrkat(templates_directory, elemof32(templates_directory),
                       FILE_DIR_SEP_TSTR TEXT("Colton Software")
                       FILE_DIR_SEP_TSTR TEXT("Fireworkz"));

        tstr_xstrkpy(wildcard_buffer, elemof32(wildcard_buffer), FILE_WILD_MULTIPLE_ALL_TSTR);

        p_file_objinfo = file_find_first_subdir(&p_file_objenum, templates_directory, wildcard_buffer, TEMPLATES_SUBDIR);

        if(NULL != p_file_objenum)
        {
            status = list_templates_from(&template_list_handle, &p_file_objenum, p_file_objinfo, allow_dirs, &max_width);

            file_find_close(&p_file_objenum);
        }
    }
    } /*block*/

#if 0
    { /* Start with All Users Templates directory */
    TCHARZ templates_directory[BUF_MAX_PATHSTRING];

    if(GetCommonTemplatesDirectoryName(templates_directory))
    {
        /* Only enumerate the Fireworkz templates as there are loads of others in there */
        tstr_xstrkpy(wildcard_buffer, elemof32(wildcard_buffer), TEXT("*.fwt"));

        p_file_objinfo = file_find_first(&p_file_objenum, templates_directory, wildcard_buffer);

        if(NULL != p_file_objenum)
        {
            status = list_templates_from(&template_list_handle, &p_file_objenum, p_file_objinfo, allow_dirs, &max_width);

            file_find_close(&p_file_objenum);
        }
    }
    } /*block*/

    { /* Then add from User's Templates directory */
    TCHARZ templates_directory[BUF_MAX_PATHSTRING];

    if(GetUserTemplatesDirectoryName(templates_directory))
    {
        /* Only enumerate the Fireworkz templates as there are loads of others in there */
        tstr_xstrkpy(wildcard_buffer, elemof32(wildcard_buffer), TEXT("*.fwt"));

        p_file_objinfo = file_find_first(&p_file_objenum, templates_directory, wildcard_buffer);

        if(NULL != p_file_objenum)
        {
            status = list_templates_from(&template_list_handle, &p_file_objenum, p_file_objinfo, allow_dirs, &max_width);

            file_find_close(&p_file_objenum);
        }
    }
    } /*block*/
#endif

#endif /* WINDOWS */

    /* Add all from Fireworkz User & System directories, Templates subdir */
    tstr_xstrkpy(wildcard_buffer, elemof32(wildcard_buffer), FILE_WILD_MULTIPLE_ALL_TSTR);

    p_file_objinfo = file_find_first_subdir(&p_file_objenum, file_get_search_path(), wildcard_buffer, TEMPLATES_SUBDIR);

    if(NULL != p_file_objenum)
    {
        status = list_templates_from(&template_list_handle, &p_file_objenum, p_file_objinfo, allow_dirs, &max_width);

        file_find_close(&p_file_objenum);
    }
    } /*block*/

    /* sort the array we built */
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, fullname_quick_tblock)); /* fixup both the sets of quick blocks prior to sort */
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));
    al_array_qsort(&template_list_handle, template_list_sort);
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, fullname_quick_tblock)); /* fixup both the sets of quick blocks after the sort */
    ui_source_list_fixup_tb(&template_list_handle, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));

    p_template_name = NULL;

    *p_just_the_one = (array_elements(&template_list_handle) == 1);

    if(*p_just_the_one)
        p_template_name = quick_tblock_tstr(&(array_ptr(&template_list_handle, TEMPLATE_LIST_ENTRY, 0))->fullname_quick_tblock);

    if(status_ok(status) && !p_template_name)
        /* make a source of text pointers to these elements for list box processing */
        status = ui_source_create_tb(&template_list_handle, &select_template_list_source, UI_TEXT_TYPE_TSTR_PERM, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));

    if(status_ok(status) && !p_template_name)
    {
#if WINDOWS && 0
        splash_window_remove();
#endif

        { /* make appropriate size box */
        const PIXIT buttons_width = DIALOG_DEFOK_H + DIALOG_STDSPACING_H + DIALOG_STDCANCEL_H;
        const PIXIT caption_width = ui_width_from_p_ui_text(p_ui_text_caption) + DIALOG_CAPTIONOVH_H;
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &select_template_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(array_elements(&template_list_handle), &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, buttons_width);
        dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, caption_width);
        select_template_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        select_template_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
        } /*block*/

        if(first_time_through) /* just popup the first one we think of from the sorted list */
        {
            first_time_through = 0;

            g_selected_template = 0;
        }
        else
        {
            DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
            dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, select_template_ctl_create, elemof32(select_template_ctl_create), MSG_DIALOG_SELECT_TEMPLATE_HELP_TOPIC);
            dialog_cmd_process_dbox.caption = *p_ui_text_caption;
            dialog_cmd_process_dbox.p_proc_client = dialog_event_select_template;
            dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) (&g_selected_template);
            status = object_call_DIALOG_with_docu(cur_p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        }

        if(status_ok(status))
        {   /* selected template to be copied back to caller */
            const S32 selected_template = g_selected_template;

            if(array_index_is_valid(&template_list_handle, selected_template))
                p_template_name = quick_tblock_tstr(&(array_ptr(&template_list_handle, TEMPLATE_LIST_ENTRY, selected_template))->fullname_quick_tblock);
        }
    }

    if(status_ok(status) && (NULL != p_template_name))
        /* selected template gets copied back to caller */
        status = quick_tblock_tchars_add(p_quick_tblock, p_template_name, tstrlen32p1(p_template_name) /*CH_NULL*/);

    {
    ARRAY_INDEX i = array_elements(&template_list_handle);
    while(--i >= 0)
    {
        P_TEMPLATE_LIST_ENTRY p_template_list_entry = array_ptr(&template_list_handle, TEMPLATE_LIST_ENTRY, i);
        quick_tblock_dispose(&p_template_list_entry->fullname_quick_tblock);
    }
    } /*block*/

    ui_lists_dispose_tb(&template_list_handle, &select_template_list_source, offsetof32(TEMPLATE_LIST_ENTRY, leafname_quick_tblock));

    return(status);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_of_load);

T5_MSG_PROTO(static, maeve_services_of_load_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__EXIT2:
        ff_load_msg_exit2();
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_of_load)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_of_load_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_object_bind_construct)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    const OBJECT_ID object_id = p_args[0].val.object_id;
    const U8 construct_id = p_args[1].val.u8c;
    P_REGISTERED_CONSTRUCT_TABLE p = &of_load_statics.registered_construct_table[object_id];

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p->construct_id = construct_id;
    return(STATUS_OK);
}

static void
load_one_found_supporting_document(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR wholename)
{
    STATUS status = STATUS_OK;
    T5_FILETYPE t5_filetype = t5_filetype_from_filename(wholename);
    OBJECT_ID object_id = object_id_from_t5_filetype(t5_filetype);
    DOCNO docno;

    if(OBJECT_ID_NONE == object_id)
    {   /* no idea how to load this! */
        return;
    }

    if(OBJECT_ID_SKEL == object_id)
    {   /* it's a Fireworkz format file - should just work! */
        const BOOL fReadOnly = FALSE;

        if(status_fail(status = new_docno_using(&docno, wholename, &p_docu->docu_name, fReadOnly)))
        {
            reperr(status, wholename);
            status = STATUS_OK;
        }

        return;
    }

    { /* it's a foreign format file but give it a try */
    PTSTR template_name = TEMPLATES_SUBDIR FILE_DIR_SEP_TSTR TEXT("Sheet");

    if(status_fail(status = new_docno_using(&docno, template_name, &p_docu->docu_name, FALSE /*fReadOnly*/)))
    {
        reperr(status, template_name);
        status = STATUS_OK;
    }
    else
    {
        MSG_INSERT_FOREIGN msg_insert_foreign;
        zero_struct(msg_insert_foreign);

        p_docu->flags.allow_modified_change = 0;

        cur_change_before(p_docu);

        msg_insert_foreign.filename = wholename;
        msg_insert_foreign.t5_filetype = t5_filetype;
        msg_insert_foreign.ctrl_pressed = FALSE;
        position_init(&msg_insert_foreign.position);
        status = object_call_id_load(p_docu, T5_MSG_INSERT_FOREIGN, &msg_insert_foreign, object_id);

        p_docu = p_docu_from_docno(docno);

        cur_change_after(p_docu);

        if(status_fail(status))
            reperr(status, wholename);

        p_docu->flags.allow_modified_change = 1;
    }
    } /*block */
}

static void
load_one_supporting_document(
    _InVal_     DOCNO docno)
{
    const P_DOCU p_docu = p_docu_from_docno(docno);
    STATUS status = STATUS_OK;
    BOOL found = FALSE;
    PCTSTR wholename = p_docu->docu_name.leaf_name; /* keep dataflower happy */ /* and helps paranoid check */
    int pass;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 4); /* force it to go to array immediately */
    quick_tblock_with_buffer_setup(quick_tblock);

    assert(!p_docu->flags.has_data);
    assert(wholename);

    for(;;) /* loop for structure */
    {
        status_break(status = name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));

        wholename = quick_tblock_tstr(&quick_tblock);

        found = file_is_file(wholename);
        reportf(TEXT("load_one_supporting_document.1: docno: %d name(%s) found=%s"), docno, wholename, report_boolstring(found));
        if(found)
            break;

        {
        PTSTR tstr_extension = file_extension(wholename);

        if(NULL != tstr_extension)
        {   /* rip the extension off and try again */
            *--tstr_extension = CH_NULL;
        }
        else
        {
            /* try extension 'cos we may be on PC floppy or have transfered off one or be on PC anyway */
            quick_tblock_nullch_strip(&quick_tblock);
            if( status_ok(status = quick_tblock_tchar_add(&quick_tblock, FILE_EXT_SEP_CH)) &&
                status_ok(status = quick_tblock_tstr_add_n(&quick_tblock, extension_document_tstr, strlen_with_NULLCH)) )
            {
                wholename = quick_tblock_tstr(&quick_tblock);
            }
            else
            {
                break;
            }
        }

        found = file_is_file(wholename);
        reportf(TEXT("load_one_supporting_document.2: docno: %d name(%s) found=%s"), docno, wholename, report_boolstring(found));
        if(found)
        {   /* note the extension we used to find it */
            status = tstr_set(&p_docu->docu_name.extension, (NULL == tstr_extension) ? extension_document_tstr : NULL);
            break;
        }
        } /*block*/

        /* can we try library load ? */
        if(p_docu->docu_name.flags.path_name_supplied)
            break;

        for(pass = 1; pass <= 2; ++pass)
        {
            STATUS status_ffop;
            TCHARZ searchname[BUF_MAX_PATHSTRING];
            TCHARZ tempname[BUF_MAX_PATHSTRING];
            resource_lookup_tstr_buffer(searchname, elemof32(searchname), MSG_CUSTOM_LIBRARY);

            tstr_xstrkat(searchname, elemof32(searchname), FILE_DIR_SEP_TSTR);
            tstr_xstrkat(searchname, elemof32(searchname), p_docu->docu_name.leaf_name);

            if((pass & 1) != RISCOS)
            {   /* Scratching your head at this logic?
                 * RISCOS==0: Look first with an extension, then without one
                 * RISCOS==1: Look first without an extension, then with one
                 */
                tstr_xstrkat(searchname, elemof32(searchname), FILE_EXT_SEP_TSTR);
                tstr_xstrkat(searchname, elemof32(searchname), extension_document_tstr);
            }

            reportf(TEXT("load_one_supporting_document.2(%d): searching on path - name(%s)"), pass, searchname);
            if((status_ffop = file_find_on_path(tempname, elemof32(tempname), file_get_search_path(), searchname)) > 0)
            {
                reportf(TEXT("found on path: name(%s)"), tempname);
                name_dispose(&p_docu->docu_name);
                status_break(status = name_read_tstr(&p_docu->docu_name, tempname));
                p_docu->docu_name.flags.path_name_supplied = 0;
                reportf(TEXT("load_one_supporting_document.3: docno: ") S32_TFMT TEXT(" path(%s) leaf(%s) extension(%s)"),
                        (S32) docno,
                        report_tstr(p_docu->docu_name.path_name),
                        report_tstr(p_docu->docu_name.leaf_name),
                        report_tstr(p_docu->docu_name.extension));

                quick_tblock_dispose(&quick_tblock);
                status_break(status = name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));

                wholename = quick_tblock_tstr(&quick_tblock);
                reportf(TEXT("using name(%s)"), wholename);
                found = TRUE;
                break;
            }
        } /*block*/

        break; /* end of loop for structure */
    }

    if(!found && status_ok(status))
        status = create_error(ERR_SUPPORTER_NOT_FOUND);

    if(status_fail(status))
        reperr(found, p_docu->docu_name.leaf_name);
    else
        load_one_found_supporting_document(p_docu, wholename);

    quick_tblock_dispose(&quick_tblock);
}

extern void
load_supporting_documents(
    _DocuRef_   P_DOCU cur_p_docu)
{
    ARRAY_HANDLE h_supporters = 0;
    S32 n_doc = docno_get_supporting_docs(cur_p_docu, &h_supporters);
    ARRAY_INDEX i;

    for(i = 0; i < n_doc; ++i)
    {
        P_DOCNO p_docno = array_ptr(&h_supporters, DOCNO, i);
        const DOCNO docno = *p_docno;
        const P_DOCU sup_p_docu = p_docu_from_docno(docno);

        /* supporting document already populated? */
        if(sup_p_docu->flags.has_data)
        {
            reportf(TEXT("load_supporting_documents: docno: ") S32_TFMT TEXT(" path(%s) leaf(%s) extension(%s) has_data=%s"),
                    (S32) docno,
                    report_tstr(sup_p_docu->docu_name.path_name),
                    report_tstr(sup_p_docu->docu_name.leaf_name),
                    report_tstr(sup_p_docu->docu_name.extension),
                    report_boolstring(sup_p_docu->flags.has_data));

            continue;
        }

        load_one_supporting_document(docno);
    }

    al_array_dispose(&h_supporters);
}

_Check_return_
static STATUS
load_all_files_from_dir(
    _In_z_      PCTSTR dirname)
{
    STATUS status = STATUS_OK;
    P_FILE_OBJENUM p_file_objenum;
    P_FILE_OBJINFO p_file_objinfo;
    TCHARZ wildcard_buffer[8];
    S32 files_loaded = 0;
    DOCNO first_docno = DOCNO_NONE;

    tstr_xstrkpy(wildcard_buffer, elemof32(wildcard_buffer), FILE_WILD_MULTIPLE_ALL_TSTR);

    for(p_file_objinfo = file_find_first(&p_file_objenum, dirname, wildcard_buffer);
        p_file_objinfo;
        p_file_objinfo = file_find_next(&p_file_objenum))
    {
        T5_FILETYPE t5_filetype = file_objinfo_filetype(p_file_objinfo);

        switch(t5_filetype)
        {
        case FILETYPE_T5_FIREWORKZ:
        case FILETYPE_T5_WORDZ:
        case FILETYPE_T5_RESULTZ:
        case FILETYPE_T5_RECORDZ:
            {
            QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
            quick_tblock_with_buffer_setup(quick_tblock);

            if(status_ok(status = file_objenum_fullname(&p_file_objenum, p_file_objinfo, &quick_tblock)))
            {
                PCTSTR fullname = quick_tblock_tstr(&quick_tblock);
                const BOOL fReadOnly = FALSE;
                DOCU_NAME docu_name;

                name_init(&docu_name);

                status = name_read_tstr(&docu_name, fullname);

                if(status_ok(status))
                {
                    DOCNO docno;

                    status = load_is_file_loaded(&docno, &docu_name);

                    if(ERR_DUPLICATE_FILE == status) /* ok if already loaded as supporter of one that's gone before */
                        status = STATUS_OK;
                    else
                    {
                        if(STATUS_DONE == status)
                        {   /* SKS 15jun95 we've found the corresponding thunk for this document, which is probably not correctly formed name-wise */
                            reportf(
                                TEXT("load_all_files_from_dir: naming thunk docno: ") S32_TFMT TEXT(" path(%s) leaf(%s) extension(%s)"),
                                (S32) docno,
                                report_tstr(docu_name.path_name),
                                report_tstr(docu_name.leaf_name),
                                report_tstr(docu_name.extension));
                            status = maeve_event(p_docu_from_docno(docno), T5_MSG_DOCU_RENAME, (P_ANY) &docu_name);
                        }

                        if(status_ok(status))
                            status = new_docno_using(&docno, fullname, &docu_name, fReadOnly);

                        if(status_ok(status))
                        {
                            ++files_loaded;

                            if(first_docno == DOCNO_NONE)
                                first_docno = docno;
                        }
                    }
                }

                name_dispose(&docu_name);
            }

            quick_tblock_dispose(&quick_tblock);

            break;
            }

        default:
            break;
        }

        status_break(status);
    }

    file_find_close(&p_file_objenum);

    status_return(status);

    if(first_docno != DOCNO_NONE)
        status_return(command_set_current_docu(p_docu_from_docno(first_docno)));

    return(files_loaded ? STATUS_DONE : STATUS_OK);
}

_Check_return_
extern STATUS /*reported*/
load_this_dir_rl(
    _In_z_      PCTSTR dirname)
{
    STATUS status;

    if(status_ok(status = ensure_memory_froth()))
        status = load_all_files_from_dir(dirname);

    if(status_fail(status))
    {
        if(status != STATUS_FAIL)
            reperr_null(status);

        return(STATUS_FAIL);
    }

    return(STATUS_OK);
}

/* Load up configuration file as first document */

_Check_return_
extern STATUS
load_ownform_config_file(void)
{
    const BOOL fReadOnly = FALSE; /* we may need to modify Choices etc. */
    DOCU_NAME docu_name;
    TCHARZ filename[BUF_MAX_PATHSTRING];
    STATUS status;

    status_return(status = name_read_tstr(&docu_name, TEXT("$$$Config")));

    if(STATUS_OK == (status = file_find_on_path(filename, elemof32(filename), file_get_search_path(), CONFIG_FILE_NAME)))
    {
        reperr(ERR_NO_CONFIG, product_ui_id());
        status = STATUS_FAIL;
    }

    if(status_done(status))
    {
        DOCNO docno;

        status = new_docno_using_existing_file(&docno, filename, &docu_name, fReadOnly, TRUE); /* really needs to be a full live document (but it has no views and no data (yet)) */

        if(status_ok(status))
        {   /* try overwriting this bare document with one containing UI styles and a trivial amount of data */
            status = file_find_on_path(filename, elemof32(filename), file_get_search_path(), CHOICES_DOCU_FILE_NAME);

            if(status_done(status))
            {
                const P_DOCU p_docu_config = p_docu_from_config_wr();

                assert(p_docu_from_docno(docno) == p_docu_config);

                status = new_docno_using_core(p_docu_config, filename, fReadOnly);

                assert(FALSE == p_docu_config->flags.allow_modified_change);
            }

            if(status_ok(status))
            {   /* always flag document has data for closedown order */
                const P_DOCU p_docu_config = p_docu_from_config_wr();

                assert(p_docu_config->flags.has_data);
                if(!p_docu_config->flags.has_data)
                    p_docu_config->flags.has_data = 1;
            }
        }
    }

    if(status_fail(status))
    {
        TCHARZ errstring[BUF_MAX_ERRORSTRING];
        resource_lookup_tstr_buffer(errstring, elemof32(errstring), status);
        reperr(ERR_LOADING_CONFIG, product_ui_id(), errstring);
        status = STATUS_FAIL;
    }

    name_dispose(&docu_name);

    return(status);
}

/******************************************************************************
*
* load a Fireworkz format file
*
******************************************************************************/

_Check_return_
static STATUS
load_fireworkz_file_core(
    _OutRef_    P_DOCNO p_docno,
    _In_z_      PCTSTR filename,
    _InVal_     BOOL fReadOnly)
{
    DOCU_NAME docu_name;
    DOCNO docno = DOCNO_NONE;
    STATUS status;

    *p_docno = DOCNO_NONE;

    name_init(&docu_name);

    if(!host_xfer_loaded_file_is_safe())
    {
        status_return(status = name_set_untitled(&docu_name));
    }
    else
    {
        status_return(status = name_read_tstr(&docu_name, filename));

        status = load_is_file_loaded(&docno, &docu_name);

        if(STATUS_DONE == status)
        {   /* SKS 15jun95 we've found the corresponding thunk for this document, which is probably not correctly formed name-wise */
            DOCU_NAME docu_name_copy;
            reportf(
                TEXT("load_fireworkz_file_core: naming thunk docno: ") S32_TFMT TEXT(", path(%s), leaf(%s), extension(%s)"),
                (S32) docno,
                report_tstr(docu_name.path_name),
                report_tstr(docu_name.leaf_name),
                report_tstr(docu_name.extension));
            if(status_ok(status = name_dup(&docu_name_copy, &docu_name)))
                name_donate(&p_docu_from_docno(docno)->docu_name, &docu_name_copy);
        }
    }

    if(status_ok(status))
        status = new_docno_using(&docno, filename, &docu_name, fReadOnly);

    if(status_ok(status))
        status_assert(maeve_event(p_docu_from_docno(docno), T5_MSG_SUPPORTER_LOADED, P_DATA_NONE));

    if(status_ok(status))
        status = command_set_current_docu(p_docu_from_docno(docno));

    name_dispose(&docu_name);

    *p_docno = docno;

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_load)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    PCTSTR filename = p_args[0].val.tstr;
    P_ARGLIST_ARG p_arg;
    BOOL fReadOnly = FALSE;
    DOCNO docno;
    STATUS status;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_present(&p_t5_cmd->arglist_handle, 1, &p_arg))
        fReadOnly = p_arg->val.fBool;

    if(status_fail(status = load_fireworkz_file_core(&docno, filename, fReadOnly)))
    {
        reperr(status, filename);
        status = STATUS_FAIL;
    }

    return(status);
}

/* Load the specified file, maybe using a template */

_Check_return_
static STATUS
load_this_file_core(
    _DocuRef_   P_DOCU cur_p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _In_opt_z_  PCTSTR filename,
    _InVal_     BOOL fReadOnly)
{
    STATUS status = ensure_memory_froth();

    if(status_ok(status))
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
            P_ARGLIST_ARG p_arg;
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

            if(fReadOnly && arg_present(&arglist_handle, 1, &p_arg))
            {
                assert(T5_CMD_LOAD == t5_message);
                p_arg->val.fBool = fReadOnly;
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
static STATUS /*reported*/
load_this_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _In_opt_z_  PCTSTR filename,
    _InVal_     BOOL fReadOnly)
{
    STATUS status = load_this_file_core(cur_p_docu, t5_message, filename, fReadOnly);

    if(status_fail(status))
    {
        reperr(status, filename);
        status = STATUS_FAIL;
    }

    return(status);
}

_Check_return_
extern STATUS /*reported*/
load_this_fireworkz_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     BOOL fReadOnly)
{
    return(load_this_file_rl(cur_p_docu, T5_CMD_LOAD, filename, fReadOnly ? TRUE : file_is_read_only(filename)));
}

_Check_return_
extern STATUS /*reported*/
load_this_template_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_opt_z_  PCTSTR filename)
{
    return(load_this_file_rl(cur_p_docu, T5_CMD_LOAD_TEMPLATE, filename, FALSE /*fReadOnly*/));
}

_Check_return_
extern STATUS /*reported*/
load_this_command_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename)
{
    return(load_this_file_rl(cur_p_docu, T5_CMD_EXECUTE, filename, FALSE /*fReadOnly*/));
}

#if WINDOWS

_Check_return_
extern STATUS /*reported*/
load_file_for_windows_startup_rl(
    _In_z_      PCTSTR filename)
{
    T5_FILETYPE t5_filetype = t5_filetype_from_filename(filename);

    switch(t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ:
        status_return(load_this_fireworkz_file_rl(P_DOCU_NONE, filename, FALSE /*fReadOnly*/));
        break;

    case FILETYPE_T5_TEMPLATE:
        status_return(load_this_template_file_rl(P_DOCU_NONE, filename));
        break;

    case FILETYPE_T5_COMMAND:
        status_return(load_this_command_file_rl(P_DOCU_NONE, filename));
        break;

    default:
        status_return(load_foreign_file_rl(P_DOCU_NONE, filename, t5_filetype));
        break;
    }

    return(STATUS_OK);
}

#endif /* OS */

/* Load and print the specified document on the given printer */

_Check_return_
static STATUS
load_and_print_this_file_core(
    _In_z_      PCTSTR filename,
    _In_opt_z_  PCTSTR printername)
{
    STATUS status;
    BOOL need_to_load = TRUE;
    BOOL do_dispose = TRUE;
    DOCU_NAME docu_name;
    DOCNO docno = DOCNO_NONE;

    status_return(ensure_memory_froth());

    name_init(&docu_name);

    if(!host_xfer_loaded_file_is_safe())
    {
        status_return(status = name_set_untitled(&docu_name));

        do_dispose = TRUE;
    }
    else
    {
        status_return(status = name_read_tstr(&docu_name, filename));

        status = load_is_file_loaded(&docno, &docu_name);

        if(ERR_DUPLICATE_FILE == status)
        {   /* already loaded this file */
            need_to_load = FALSE;
            do_dispose = FALSE;
        }
        else if(STATUS_DONE == status)
        {   /* SKS 15jun95 we've found the corresponding thunk for this document, which is probably not correctly formed name-wise */
            DOCU_NAME docu_name_copy;
            reportf(
                TEXT("load_and_print_this_file_core: naming thunk docno: ") S32_TFMT TEXT(", path(%s), leaf(%s), extension(%s)"),
                (S32) docno,
                report_tstr(docu_name.path_name),
                report_tstr(docu_name.leaf_name),
                report_tstr(docu_name.extension));
            if(status_ok(status = name_dup(&docu_name_copy, &docu_name)))
            {
                name_donate(&p_docu_from_docno(docno)->docu_name, &docu_name_copy);
                do_dispose = FALSE; /* can't really unload this file now */
            }
        }
        else
            do_dispose = TRUE;
    }

    if(need_to_load)
    {
        const BOOL fReadOnly = FALSE; /* don't care */

        if(status_ok(status))
            status = new_docno_using(&docno, filename, &docu_name, fReadOnly);

        if(status_ok(status))
            status_assert(maeve_event(p_docu_from_docno(docno), T5_MSG_SUPPORTER_LOADED, P_DATA_NONE));
    }

    if(status_ok(status))
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;

#if WINDOWS
        if(NULL != printername) status = host_printer_set(printername); /* set up for specific printer, not default */
#else
        UNREFERENCED_PARAMETER(printername);
#endif

        if(status_ok(status))
            status = execute_command(p_docu_from_docno(docno), T5_CMD_PRINT, _P_DATA_NONE(P_ARGLIST_HANDLE), object_id);

#if WINDOWS
        if(NULL != printername) status_consume(host_printer_set(NULL)); /* revert to default */
#endif
    }

    name_dispose(&docu_name);

    if(do_dispose)
        docno_close(&docno);

    return(status);
}

/* Errors reported locally */

_Check_return_
extern STATUS /*reported*/
load_and_print_this_file_rl(
    _In_z_      PCTSTR filename,
    _In_opt_z_  PCTSTR printername)
{
    STATUS status = load_and_print_this_file_core(filename, printername);

    if(status_fail(status))
    {
        reperr(status, filename);
        status = STATUS_FAIL;
    }

    return(status);
}

/******************************************************************************
*
* load a Fireworkz template file
*
******************************************************************************/

static void
load_template_set_date(
    _InVal_     DOCNO docno)
{
    /* SKS 24jul06 needed for READER, sensible for normal */
    const P_DOCU p_docu_reload = p_docu_from_docno(docno);
    ss_local_time_as_ev_date(&p_docu_reload->file_date);
}

_Check_return_
static STATUS
load_this_template(
    _In_z_      PCTSTR filename_template)
{
    DOCNO docno;
    STATUS status;

    status_return(status = load_fireworkz_file_core(&docno, filename_template, FALSE));

    load_template_set_date(docno);

    { /* rename document AFTER loading, so double-clicked template can load any supporting documents */
    DOCU_NAME docu_name;

    name_init(&docu_name);

    if(status_ok(status = name_set_untitled_with(&docu_name, file_leafname(filename_template))))
    {
        status = maeve_event(p_docu_from_docno(docno), T5_MSG_DOCU_RENAME, (P_ANY) &docu_name);

        name_dispose(&docu_name);
    }
    } /*block*/

    return(status);
}

T5_CMD_PROTO(extern, t5_cmd_load_template)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    PCTSTR filename_template;
    DOCNO docno = DOCNO_NONE;
    STATUS status = STATUS_OK;
    BOOL just_the_one = FALSE;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 100);
    quick_tblock_with_buffer_setup(quick_tblock);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, 0))
    {
        filename_template = p_args[0].val.tstr;

        if(file_is_rooted(filename_template))
        {
            if(file_is_file(filename_template))
            {
                /*quick_tblock_dispose(&quick_tblock);*/
                return(load_this_template(filename_template));
            }
            else
                status = create_error(ERR_TEMPLATE_NOT_FOUND);
        }
        else
        {
            if(status_ok(status = quick_tblock_tstr_add(&quick_tblock, TEMPLATES_SUBDIR FILE_DIR_SEP_TSTR)))
                status = quick_tblock_tstr_add_n(&quick_tblock, filename_template, strlen_with_NULLCH);

            filename_template = status_ok(status) ? quick_tblock_tstr(&quick_tblock) : NULL;
        }
    }
    else
    {
        static const UI_TEXT caption = UI_TEXT_INIT_RESID(MSG_DIALOG_NEW_DOCUMENT_CAPTION);

        status = select_a_template(p_docu, &quick_tblock, &just_the_one, RISCOS ? TRUE : FALSE, &caption);

        filename_template = status_ok(status) ? quick_tblock_tstr(&quick_tblock) : NULL;
    }

    if(NULL != filename_template)
    {
        DOCU_NAME docu_name;

        name_init(&docu_name);

        /* if it's a directory, we need to make a copy of the stuff */
        if(file_is_dir(filename_template))
        {
            TCHARZ dirname_buffer[BUF_MAX_PATHSTRING];

            if(status_ok(status = locate_copy_of_dir_template(p_docu, filename_template, dirname_buffer, elemof32(dirname_buffer))))
                status = load_all_files_from_dir(dirname_buffer);
        }
        else
        {
            if(status_ok(status = name_set_untitled_with(&docu_name, file_leafname(filename_template))))
                if(status_ok(status = new_docno_using(&docno, filename_template, &docu_name, FALSE /*fReadOnly*/)))
                    load_template_set_date(docno);

            if(FILE_ERR_NOTFOUND == status)
                status = create_error(ERR_TEMPLATE_NOT_FOUND);
        }

        name_dispose(&docu_name);
    }

    quick_tblock_dispose(&quick_tblock);

    status_return(status);

    if(docno != DOCNO_NONE)
        status_return(command_set_current_docu(p_docu_from_docno(docno)));

    return(status);
}

/* end of of_load.c */
