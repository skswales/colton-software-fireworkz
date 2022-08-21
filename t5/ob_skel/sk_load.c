/* sk_load.c */

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

/*
internal routines
*/

_Check_return_
static STATUS
t5_cmd_of_base_single_col(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_block(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_flow(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_implied_region(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_region(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_rowtable(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_cell(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_style(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static STATUS
t5_cmd_of_style_specific(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
static TAB_TYPE
tab_type_from_ownform(
    _InVal_     U8 ch);

#define STYLE_AUTONAME_PREFIX_CH CH_FULL_STOP

/******************************************************************************
*
* Calls object to load cell contents, and will try other objects if owner does not reply
*
******************************************************************************/

_Check_return_
extern STATUS
insert_cell_contents_ownform(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_LOAD_CELL_OWNFORM p_load_cell_ownform,
    _InRef_     PC_POSITION p_position)
{
    consume_bool(cell_data_from_position(p_docu, &p_load_cell_ownform->object_data, p_position, NULL));

    p_load_cell_ownform->processed = 0;

    if((OBJECT_ID_NONE != object_id) && object_present(object_id))
    {
        status_return(object_call_id(object_id, p_docu, t5_message, p_load_cell_ownform));

        if(p_load_cell_ownform->processed)
            return(STATUS_OK);
    }

    /* Loop over all other objects to attempt to load this data we have in our hand */
    /* Unless type is Owner in which case nobody else can attempt to load it */
    if(p_load_cell_ownform->data_type == OWNFORM_DATA_TYPE_OWNER)
    {
        assert(OBJECT_ID_NONE != object_id);
        assert0();

        /* Object specific data found, but object not present! Return without aborting the load */
        return(STATUS_OK);
    }

    {
    OBJECT_ID object_id_t = OBJECT_ID_ENUM_START;

    while(status_ok(object_next(&object_id_t)))
    {
        if(object_id_t != object_id)
        {
            status_return(object_call_id(object_id_t, p_docu, t5_message, p_load_cell_ownform));

            if(p_load_cell_ownform->processed)
                return(STATUS_OK);
        }
    }

    /* If we get here, data is not loaded */
    /* So we can either abort load, or continue - currently, we ignore and continue */
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* Calls object to load cell contents, and will try other objects if owner does not reply
*
******************************************************************************/

_Check_return_
extern STATUS
insert_cell_contents_foreign(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_LOAD_CELL_FOREIGN p_load_cell_foreign)
{
    p_load_cell_foreign->processed = 0;

    if((OBJECT_ID_NONE != object_id) && object_present(object_id))
    {
        status_return(object_call_id(object_id, p_docu, t5_message, p_load_cell_foreign));

        if(p_load_cell_foreign->processed)
            goto processed;
    }

    /* Loop over all other objects to attempt to load this data we have in our hand */
    /* Unless type is Owner in which case nobody else can attempt to load it */
    if(p_load_cell_foreign->data_type == OWNFORM_DATA_TYPE_OWNER)
    {
        assert(OBJECT_ID_NONE != object_id);
        assert0();

        /* Object specific data found, but object not present! Return without aborting the load */
        return(STATUS_OK);
    }

    {
    OBJECT_ID object_id_t = OBJECT_ID_ENUM_START;

    while(status_ok(object_next(&object_id_t)))
    {
        if(object_id_t != object_id)
        {
            status_return(object_call_id(object_id_t, p_docu, t5_message, p_load_cell_foreign));

            if(p_load_cell_foreign->processed)
                goto processed;
        }
    }

    /* If we get here, data is not loaded */
    /* So we can either abort load, or continue - currently, we ignore and continue */
    } /*block*/

    return(STATUS_OK);

processed:

    {
    STATUS status = STATUS_OK;

    if(STYLE_HANDLE_NONE != p_load_cell_foreign->style_handle)
    {
        DOCU_AREA docu_area;
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

        assert(DATA_SLOT == p_load_cell_foreign->object_data.data_ref.data_space);
        docu_area_from_slr(&docu_area, &p_load_cell_foreign->object_data.data_ref.arg.slr);

        STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, p_load_cell_foreign->style_handle);
        status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);
    }

    if(status_ok(status) && style_selector_any(&p_load_cell_foreign->style.selector))
    {
        DOCU_AREA docu_area;
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

        assert(DATA_SLOT == p_load_cell_foreign->object_data.data_ref.data_space);
        docu_area_from_slr(&docu_area, &p_load_cell_foreign->object_data.data_ref.arg.slr);

        STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &p_load_cell_foreign->style);
        status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);

        /* NB any handles have been donated to style system */
    }

    return(status);
    } /*block*/
}

static void
t5_cmd_version_encoding(
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert,
    _In_z_      PCTSTR tstr_encoding)
{
    if(0 == tstrcmp(tstr_encoding, TEXT("Windows-1252")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_1252;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Windows-1250")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_1250;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Windows-28591")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_28591;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Windows-28592")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_28592;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Windows-28593")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_28593;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Windows-28594")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_28594;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Windows-28605")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_28605;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Alphabet-Latin1")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN1;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Alphabet-Latin2")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN2;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Alphabet-Latin3")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN3;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Alphabet-Latin4")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN4;
    }
    else if(0 == tstrcmp(tstr_encoding, TEXT("Alphabet-Latin9")))
    {
        p_construct_convert->p_of_ip_format->input.sbchar_codepage = SBCHAR_CODEPAGE_ALPHABET_LATIN9;
    }
    else
    {
        reportf(TEXT("Version: Unhandled encoding(%s)"), tstr_encoding);
    }
}

_Check_return_
static STATUS
t5_cmd_version(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const PC_ARGLIST_HANDLE p_arglist_handle = &p_construct_convert->arglist_handle;
    PC_ARGLIST_ARG p_arg;

    IGNOREPARM_DocuRef_(p_docu);

    /* ignore most of the supplied information */

    /* however the encoding parameter may be of interest... */
    if(arg_present(p_arglist_handle, 5, &p_arg))
    {
        PCTSTR tstr_encoding = p_arg->val.tstr;

        if(NULL != tstr_encoding)
            t5_cmd_version_encoding(p_construct_convert, tstr_encoding);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
t5_cmd_of_start_of_data(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    /* inform everyone out there of these events */
    const T5_MESSAGE t5_message = p_construct_convert->p_construct->t5_message;
    status_assert(maeve_event(p_docu, t5_message, P_DATA_NONE));
    return(STATUS_OK);
}

/* SKS 05jun96 moved all this stuff into a routine */

_Check_return_
extern STATUS
t5_cmd_of_end_of_data(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format)
{
    STATUS status = STATUS_OK;

    if(!p_of_ip_format->flags.had_end_of_data)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_of_ip_format->docno);

        if(p_of_ip_format->flags.table_insert)
        {
            COL cols_add = (p_of_ip_format->original_docu_area.br.slr.col - p_of_ip_format->original_docu_area.tl.slr.col);
            ROW rows_add = (p_of_ip_format->original_docu_area.br.slr.row - p_of_ip_format->original_docu_area.tl.slr.row);
            DOCU_AREA docu_area_table;

            docu_area_init(&docu_area_table);
            docu_area_table.tl = p_of_ip_format->insert_position;
            docu_area_table.br.slr.col = docu_area_table.tl.slr.col + cols_add;
            docu_area_table.br.slr.row = docu_area_table.tl.slr.row + rows_add;

            docu_area_table.tl.slr.col -= 1; /* add style to the column to the left too */

            status_assert(status = object_call_id_load(p_docu, T5_MSG_TABLE_STYLE_ADD, &docu_area_table, OBJECT_ID_SKEL_SPLIT));
        }

        /* SKS 03feb96 deferred execution to here */
        status_accumulate(status, virtual_row_table_set(p_docu, p_of_ip_format->flags.virtual_row_table));

        /* inform everyone out there of these events */
        status_assert(maeve_event(p_docu, T5_CMD_OF_END_OF_DATA, P_DATA_NONE));

        p_of_ip_format->flags.had_end_of_data = TRUE;
    }

    return(status);
}

/*
decode construct held at p_construct_convert to cell contents / inlines
*/

_Check_return_
static STATUS
skeleton_load_convert_IL_STYLE_HANDLE(
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    STATUS status = STATUS_OK;
    const IL_CODE il_code = IL_STYLE_HANDLE;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
    PCTSTR tstr_style_name = p_args[0].val.tstr;
    const P_DOCU p_docu = p_docu_from_docno(p_construct_convert->p_of_ip_format->docno);
    STYLE_HANDLE style_handle;

    /* if it's an autoname prefix, see if it's on the list of renamed styles */
    if(STYLE_AUTONAME_PREFIX_CH == *tstr_style_name)
    {
        ARRAY_INDEX i;

        for(i = 0; i < array_elements(&p_construct_convert->p_of_ip_format->renamed_styles); ++i)
        {
            P_STYLE_LOAD_MAP p_style_load_map = array_ptr(&p_construct_convert->p_of_ip_format->renamed_styles, STYLE_LOAD_MAP, i);
            PCTSTR tstr_style_name_map = array_tstr(&p_style_load_map->h_old_name_tstr);

            if(tstr_compare_equals_nocase(tstr_style_name_map, tstr_style_name))
            {
                tstr_style_name = array_tstr(&p_style_load_map->h_new_name_tstr);
                break;
            }
        }
    }

    if(0 == (style_handle = style_handle_from_name(p_docu, tstr_style_name)))
        return(create_error(ERR_NAMED_STYLE_NOT_FOUND));

    status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_S32, &style_handle, sizeof32(style_handle));

    return(status);
}

_Check_return_
static STATUS
skeleton_load_convert_IL_STYLE_PS_TAB_LIST(
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    STATUS status = STATUS_OK;
    const IL_CODE il_code = IL_STYLE_PS_TAB_LIST;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
    ARRAY_HANDLE h_tab_info = 0;

    if(p_args && (NULL != p_args->val.ustr_wr))
    {
        P_A7STR a7str = (P_A7STR) p_args->val.ustr_wr; /* TAB lists are ASCII-only */

        for(;;)
        {
            TAB_INFO tab_info;
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(tab_info), FALSE);

            StrSkipSpaces(a7str);

            if(CH_NULL == PtrGetByte(a7str))
                break;

            tab_info.type = tab_type_from_ownform(*a7str++);

            tab_info.offset = (PIXIT) fast_strtol(a7str, &a7str);

            status_break(status = al_array_add(&h_tab_info, TAB_INFO, 1, &array_init_block, &tab_info));
        }
    }

    if(status_ok(status))
    {
        const U32 n_elements = array_elements32(&h_tab_info);
        const U32 n_bytes = n_elements * array_element_size32(&h_tab_info);
        status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_ANY, array_rangec(&h_tab_info, TAB_INFO, 0, n_elements), n_bytes);
    }

    al_array_dispose(&h_tab_info);

    return(status);
}

_Check_return_
static STATUS
skeleton_load_convert_IL_UTF8(
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    STATUS status = STATUS_OK;
    const IL_CODE il_code = IL_UTF8;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
    QUICK_BLOCK_WITH_BUFFER(quick_block, 32);
    quick_block_with_buffer_setup(quick_block);

    if(p_args && (NULL != p_args->val.ustr_wr))
    {
        P_A7STR a7str = (P_A7STR) p_args->val.ustr_wr; /* Unicode code point lists in XDR are ASCII-only */

        for(;;)
        {
            UCS4 ucs4;
            UTF8B utf8_buffer[8];
            U32 bytes_of_char;

            StrSkipSpaces(a7str);

            if(CH_NULL == PtrGetByte(a7str))
                break;

            ucs4 = (UCS4) /*"C"*/strtoul(a7str, &a7str, 16);

            /*zero_struct(utf8_buffer);*/
            bytes_of_char = utf8_char_encode(utf8_bptr(utf8_buffer), elemof32(utf8_buffer), ucs4);
            /*reportf(TEXT("skeleton_load_convert_IL_UTF8: ucs4=U+%04X -> bytes=%u %02X %02X %02X %02X"), ucs4, bytes_of_char, utf8_buffer[0], utf8_buffer[1], utf8_buffer[2], utf8_buffer[3]);*/

            status_break(status = quick_block_bytes_add(&quick_block, utf8_buffer, bytes_of_char));
        }
    }

    if(status_ok(status))
    {
        const PC_BYTE il_data = quick_block_ptr(&quick_block);
        const U32 il_data_size = quick_block_bytes(&quick_block);
        status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_ANY, il_data, il_data_size);
    }

    quick_block_dispose(&quick_block);

    return(status);
}

_Check_return_
static STATUS
skeleton_load_inline(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    STATUS status = STATUS_OK;
    const IL_CODE il_code = (IL_CODE) p_construct_convert->p_construct->t5_message;

    IGNOREPARM_DocuRef_(p_docu);

    p_construct_convert->object_array_handle_uchars_inline = 0;

    if(p_construct_convert->p_construct->bits.exceptional)
    {
        switch(il_code)
        {
        case IL_STYLE_HANDLE:
            status = skeleton_load_convert_IL_STYLE_HANDLE(p_construct_convert);
            break;

        case IL_STYLE_KEY:
            {
            const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
            KMAP_CODE kmap_code = key_code_from_key_of_name(p_args[0].val.tstr);

            /* SKS 16aug93 kill off any old/unknown style key name definitions; punter must start afresh */
            if(!kmap_code)
                return(STATUS_OK);

            status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_S32, &kmap_code, sizeof32(kmap_code));

            break;
            }

        case IL_STYLE_PS_TAB_LIST:
            status = skeleton_load_convert_IL_STYLE_PS_TAB_LIST(p_construct_convert);
            break;

        case IL_STYLE_PS_LINE_SPACE:
            {
            const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 2);
            LINE_SPACE line_space;

            line_space.type    = p_args[0].val.s32;
            line_space.leading = p_args[1].val.s32;

            status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_ANY, &line_space, sizeof32(line_space));

            break;
            }

        case IL_UTF8:
            status = skeleton_load_convert_IL_UTF8(p_construct_convert);
            break;

        default:
#if CHECKING
            myassert2(TEXT("construct %s: inline_data_from_ownform didn't cope, inline ") S32_TFMT, report_sbstr(p_construct_convert->p_construct->a7str_construct_name), (S32) il_code);
            return(STATUS_OK); /* SKS: only during debug, saves big jump table on release */

        case IL_STYLE_FS_COLOUR:
        case IL_STYLE_PS_RGB_BACK:
        case IL_STYLE_PS_RGB_BORDER:
        case IL_STYLE_PS_RGB_GRID_LEFT:
        case IL_STYLE_PS_RGB_GRID_TOP:
        case IL_STYLE_PS_RGB_GRID_RIGHT:
        case IL_STYLE_PS_RGB_GRID_BOTTOM:
#endif
            {
            const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 4);
            RGB rgb;

            rgb_set(&rgb, (U8) p_args[0].val.s32 /*r*/, (U8) p_args[1].val.s32 /*g*/, (U8) p_args[2].val.s32 /*b*/);

            if(arg_is_present(p_args, 3))
            {
                assert_EQ(p_args[3].type, ARG_TYPE_S32);
                rgb.transparent = (U8) p_args[3].val.s32;
            }

            status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, IL_TYPE_ANY, &rgb, sizeof32(rgb));

            break;
            }
        }
    }
    else
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
        const ARG_TYPE arg_type = (ARG_TYPE) (p_args[0].type & ARG_TYPE_MASK);

        static const IL_TYPE
        il_types[] =
        {
            IL_TYPE_NONE,   /* ARG_TYPE_NONE */
            IL_TYPE_U8,     /* ARG_TYPE_U8C */
            IL_TYPE_U8,     /* ARG_TYPE_U8N */
            IL_TYPE_S32,    /* ARG_TYPE_BOOL */
            IL_TYPE_S32,    /* ARG_TYPE_S32 */
            IL_TYPE_S32,    /* ARG_TYPE_X32 */
            IL_TYPE_S32,    /* ARG_TYPE_COL */
            IL_TYPE_S32,    /* ARG_TYPE_ROW */
            IL_TYPE_F64,    /* ARG_TYPE_F64 */
            IL_TYPE_NONE,   /* ARG_TYPE_RAW */
            IL_TYPE_NONE,   /* ARG_TYPE_RAW_DS */
            IL_TYPE_USTR    /* ARG_TYPE_USTR */
#if defined(ARG_TYPE_TSTR_DISTINCT)
,           IL_TYPE_NONE    /* ARG_TYPE_TSTR */
#endif /* ARG_TYPE_TSTR_DISTINCT */
        };

#if CHECKING
        static BOOL done = FALSE;

        if(!done)
        {
            assert_EQ(il_types[ARG_TYPE_U8C],   IL_TYPE_U8);
            assert_EQ(il_types[ARG_TYPE_U8N],   IL_TYPE_U8);
            assert_EQ(il_types[ARG_TYPE_BOOL],  IL_TYPE_S32);
            assert_EQ(il_types[ARG_TYPE_S32],   IL_TYPE_S32);
            assert_EQ(il_types[ARG_TYPE_X32],   IL_TYPE_S32);
            assert_EQ(il_types[ARG_TYPE_COL],   IL_TYPE_S32);
            assert_EQ(il_types[ARG_TYPE_ROW],   IL_TYPE_S32);
            assert_EQ(il_types[ARG_TYPE_F64],   IL_TYPE_F64);
            assert_EQ(il_types[ARG_TYPE_USTR],  IL_TYPE_USTR);
            done = TRUE;
        }

        if(!IS_ARRAY_INDEX_VALID(arg_type, elemof32(il_types)) || (IL_TYPE_NONE == il_types[arg_type]))
        {
            myassert2(TEXT("construct %s: unhandled ARG_TYPE ") U32_TFMT TEXT(" at decode time"), report_sbstr(p_construct_convert->p_construct->a7str_construct_name), (U32) arg_type);
            return(STATUS_OK);
        }
#endif /* CHECKING */

        if(IS_ARRAY_INDEX_VALID(arg_type, elemof32(il_types)))
        {
            PC_ANY p_data = &p_args[0].val; /* all values are at the same offset */

            if(ARG_TYPE_USTR == arg_type)
                p_data = p_args[0].val.ustr; /* but dereference this one */

            status = inline_array_from_data(&p_construct_convert->object_array_handle_uchars_inline, il_code, il_types[arg_type], p_data, 0);
        }
    }

    if(status_fail(status))
        al_array_dispose(&p_construct_convert->object_array_handle_uchars_inline);

    return(status);
}

_Check_return_
extern STATUS
skeleton_load_construct(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const T5_MESSAGE t5_message = p_construct_convert->p_construct->t5_message;

    switch(t5_message)
    {
    case T5_CMD_OF_VERSION:
        return(t5_cmd_version(p_docu, p_construct_convert));

    case T5_CMD_OF_GROUP: /* nothing at all - only purpose is to string together associated groups of command constructs for atomic processing during load */
        return(STATUS_OK);

    case T5_CMD_OF_START_OF_DATA:
        return(t5_cmd_of_start_of_data(p_docu, p_construct_convert));

    case T5_CMD_OF_END_OF_DATA:
        return(t5_cmd_of_end_of_data(p_construct_convert->p_of_ip_format));

    case T5_CMD_OF_BLOCK:
        return(t5_cmd_of_block(p_docu, p_construct_convert));

    case T5_CMD_OF_CELL:
        return(t5_cmd_of_cell(p_docu, p_construct_convert));

    case T5_CMD_OF_STYLE:
        return(t5_cmd_of_style(p_docu, p_construct_convert));

    case T5_CMD_OF_BASE_REGION:
    case T5_CMD_OF_REGION:
        return(t5_cmd_of_region(p_docu, p_construct_convert));

    case T5_CMD_OF_BASE_IMPLIED_REGION:
    case T5_CMD_OF_IMPLIED_REGION:
        return(t5_cmd_of_implied_region(p_docu, p_construct_convert));

    case T5_CMD_OF_STYLE_BASE:
    case T5_CMD_OF_STYLE_HEFO:
    case T5_CMD_OF_STYLE_CURRENT:
    case T5_CMD_OF_STYLE_TEXT:
        return(t5_cmd_of_style_specific(p_docu, p_construct_convert));

    case T5_CMD_OF_FLOW:
        return(t5_cmd_of_flow(p_docu, p_construct_convert));

    case T5_CMD_OF_ROWTABLE:
        return(t5_cmd_of_rowtable(p_docu, p_construct_convert));

    case T5_CMD_OF_BASE_SINGLE_COL:
        return(t5_cmd_of_base_single_col(p_docu, p_construct_convert));

    default:
        if(t5_message_is_inline(t5_message))
            return(skeleton_load_inline(p_docu, p_construct_convert));

        return(execute_loaded_command(p_docu, p_construct_convert, OBJECT_ID_SKEL));
    }
}

_Check_return_
static STATUS
t5_cmd_of_implied_region(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    const T5_MESSAGE io_t5_message = p_construct_convert->p_construct->t5_message;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, ARG_IMPLIED_N_ARGS);
    const OBJECT_ID object_id = p_args[ARG_IMPLIED_OBJECT_ID].val.object_id;
    const T5_MESSAGE implied_t5_message = p_args[ARG_IMPLIED_MESSAGE].val.t5_message;
    DOCU_AREA docu_area, docu_area_incoming_offset, docu_area_saved_offset, docu_area_clipped;
    STATUS status = STATUS_OK;

    /* Find the region over which we are to apply the style */
    docu_area.tl.slr.col                   = p_args[ARG_IMPLIED_TL_COL].val.col;
    docu_area.tl.slr.row                   = p_args[ARG_IMPLIED_TL_ROW].val.row;
    docu_area.tl.object_position.data      = p_args[ARG_IMPLIED_TL_DATA].val.s32;
    docu_area.tl.object_position.object_id = p_args[ARG_IMPLIED_TL_OBJECT_ID].val.object_id;

    docu_area.br.slr.col                   = p_args[ARG_IMPLIED_BR_COL].val.col;
    docu_area.br.slr.row                   = p_args[ARG_IMPLIED_BR_ROW].val.row;
    docu_area.br.object_position.data      = p_args[ARG_IMPLIED_BR_DATA].val.s32;
    docu_area.br.object_position.object_id = p_args[ARG_IMPLIED_BR_OBJECT_ID].val.object_id;

    docu_area.whole_col                    = p_args[ARG_IMPLIED_WHOLE_COL].val.fBool;
    docu_area.whole_row                    = p_args[ARG_IMPLIED_WHOLE_ROW].val.fBool;

    trace_5(TRACE_APP_LOAD,
            TEXT("load_implied_region tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(" br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(", insert: %s"),
            docu_area.tl.slr.col, docu_area.tl.slr.row, docu_area.br.slr.col, docu_area.br.slr.row,
            report_boolstring(p_of_ip_format->flags.insert));

    /* SKS 05nov95 all this was missing from implied region load before */
    docu_area_clean_up(&docu_area);

    /* adjust incoming docu area for load offset */
    docu_area_offset_to_position(&docu_area_incoming_offset, &docu_area, &p_of_ip_format->insert_position, &p_of_ip_format->original_docu_area.tl);

    /* adjust saved docu area for load offset */
    docu_area_offset_to_position(&docu_area_saved_offset, &p_of_ip_format->original_docu_area, &p_of_ip_format->insert_position, &p_of_ip_format->original_docu_area.tl);

    /* when inserting, clip incoming regions to inserted area */
    if(p_of_ip_format->flags.insert && !p_of_ip_format->flags.is_template)
        docu_area_intersect_docu_area_out(&docu_area_clipped, &docu_area_incoming_offset, &docu_area_saved_offset);
    else
        docu_area_clipped = docu_area_incoming_offset;

    /* List of inlines - if empty there is no sense in adding it! */
    if(!docu_area_is_empty(&docu_area_clipped))
    {
        if(arg_is_present(p_args, ARG_IMPLIED_STYLE))
        {
            STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
            STYLE_DOCU_AREA_ADD_IMPLIED(&style_docu_area_add_parm, p_args[ARG_IMPLIED_STYLE].val.ustr_inline, object_id, implied_t5_message, p_args[ARG_IMPLIED_ARG].val.s32, p_args[ARG_IMPLIED_REGION_CLASS].val.u8n);

            if((T5_CMD_OF_BASE_IMPLIED_REGION == io_t5_message) && !p_of_ip_format->flags.insert)
                style_docu_area_add_parm.base = 1; /* SKS 05nov95 add base implied regions so punters don't clear them */

            status = style_docu_area_add(p_docu, &docu_area_clipped, &style_docu_area_add_parm);
        }
    }

    return(status);
}

/******************************************************************************
*
* Create DOCU_AREA in document from ownform region constructs
*
******************************************************************************/

_Check_return_
static STATUS
t5_cmd_of_region(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    const T5_MESSAGE t5_message = p_construct_convert->p_construct->t5_message;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, ARG_DOCU_AREA_N_ARGS);
    DOCU_AREA docu_area, docu_area_incoming_offset, docu_area_saved_offset, docu_area_clipped;
    STATUS status = STATUS_OK;

    /* Find the region over which we are to apply the style */
    docu_area.tl.slr.col                   = p_args[ARG_DOCU_AREA_TL_COL].val.col;
    docu_area.tl.slr.row                   = p_args[ARG_DOCU_AREA_TL_ROW].val.row;
    docu_area.tl.object_position.data      = p_args[ARG_DOCU_AREA_TL_DATA].val.s32;
    docu_area.tl.object_position.object_id = p_args[ARG_DOCU_AREA_TL_OBJECT_ID].val.object_id;

    docu_area.br.slr.col                   = p_args[ARG_DOCU_AREA_BR_COL].val.col;
    docu_area.br.slr.row                   = p_args[ARG_DOCU_AREA_BR_ROW].val.row;
    docu_area.br.object_position.data      = p_args[ARG_DOCU_AREA_BR_DATA].val.s32;
    docu_area.br.object_position.object_id = p_args[ARG_DOCU_AREA_BR_OBJECT_ID].val.object_id;

    docu_area.whole_col                    = p_args[ARG_DOCU_AREA_WHOLE_COL].val.fBool;
    docu_area.whole_row                    = p_args[ARG_DOCU_AREA_WHOLE_ROW].val.fBool;

    trace_5(TRACE_APP_LOAD,
            TEXT("load_region tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(" br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(", insert: %s"),
            docu_area.tl.slr.col, docu_area.tl.slr.row, docu_area.br.slr.col, docu_area.br.slr.row,
            report_boolstring(p_of_ip_format->flags.insert));

    docu_area_clean_up(&docu_area);

    /* adjust incoming docu area for load offset */
    docu_area_offset_to_position(&docu_area_incoming_offset, &docu_area, &p_of_ip_format->insert_position, &p_of_ip_format->original_docu_area.tl);

    /* adjust saved docu area for load offset */
    docu_area_offset_to_position(&docu_area_saved_offset, &p_of_ip_format->original_docu_area, &p_of_ip_format->insert_position, &p_of_ip_format->original_docu_area.tl);

    /* when inserting, clip incoming regions to inserted area */
    if(p_of_ip_format->flags.insert && !p_of_ip_format->flags.is_template)
        docu_area_intersect_docu_area_out(&docu_area_clipped, &docu_area_incoming_offset, &docu_area_saved_offset);
    else
        docu_area_clipped = docu_area_incoming_offset;

    /* List of inlines - if empty there is no sense in adding it! */
    if(!docu_area_is_empty(&docu_area_clipped))
    {
        if(arg_is_present(p_args, ARG_DOCU_AREA_DEFN))
        {
            /* base regions are ignored on insertion when base styles are the same */
            if((t5_message != T5_CMD_OF_BASE_REGION) || !p_of_ip_format->flags.base_style_same)
            {
                STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
                STYLE_DOCU_AREA_ADD_INLINE(&style_docu_area_add_parm, p_args[ARG_DOCU_AREA_DEFN].val.ustr_inline);

                if((t5_message == T5_CMD_OF_BASE_REGION) && !p_of_ip_format->flags.insert)
                {
                    style_docu_area_add_parm.base = 1;
                    style_docu_area_add_parm.region_class = REGION_BASE;
                }

                if(p_of_ip_format->flags.full_file_load) /* SKS 31jan97 */
                    style_docu_area_add_parm.add_without_subsume = TRUE;

                status = style_docu_area_add(p_docu, &docu_area_clipped, &style_docu_area_add_parm);
            }
        }
    }

    return(status);
}

/******************************************************************************
*
* Decode construct information for ownform cell construct, call relevant object
*
******************************************************************************/

_Check_return_
static STATUS
t5_cmd_of_cell(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, ARG_CELL_N_ARGS);
    LOAD_CELL_OWNFORM load_cell_ownform;
    STATUS status;
    OBJECT_ID object_id;
    POSITION position;

    zero_struct(load_cell_ownform);

    if(status_fail(status = object_id_from_construct_id(p_args[ARG_CELL_OWNER].val.u8c, &object_id)))
    {
        if(status != STATUS_FAIL)
            return(status);
        status = STATUS_OK;
    }

    load_cell_ownform.data_type = OWNFORM_DATA_TYPE_TEXT;

    {
    U8 owner = p_args[ARG_CELL_DATA_TYPE].val.u8c;

    if(owner && status_fail(data_type_from_construct_id(owner, &load_cell_ownform.data_type)))
    {
        p_of_ip_format->flags.unknown_data_type = 1;
        return(STATUS_OK);
    }
    } /*block*/

#if 1
    /* already offset using COL,ROW arg processing */
    position.slr.col = p_args[ARG_CELL_COL].val.col;
    position.slr.row = p_args[ARG_CELL_ROW].val.row;
    position.object_position.object_id = OBJECT_ID_NONE;

    load_cell_ownform.original_slr.col = position.slr.col + (p_of_ip_format->original_docu_area.tl.slr.col - p_of_ip_format->insert_position.slr.col);
    load_cell_ownform.original_slr.row = position.slr.row + (p_of_ip_format->original_docu_area.tl.slr.row - p_of_ip_format->insert_position.slr.row);
#else
    load_cell_ownform.original_slr.col = p_args[ARG_CELL_COL].val.col;
    load_cell_ownform.original_slr.row = p_args[ARG_CELL_ROW].val.row;

    position.slr.col = (load_cell_ownform.original_slr.col - p_of_ip_format->original_docu_area.tl.slr.col) + p_of_ip_format->insert_position.slr.col;
    position.slr.row = (load_cell_ownform.original_slr.row - p_of_ip_format->original_docu_area.tl.slr.row) + p_of_ip_format->insert_position.slr.row;
    position.object_position.object_id = OBJECT_ID_NONE;
#endif

    region_from_docu_area_max(&load_cell_ownform.region_saved, &p_of_ip_format->original_docu_area);

    load_cell_ownform.ustr_inline_contents = p_args[ARG_CELL_CONTENTS].val.ustr_inline;
    load_cell_ownform.ustr_formula         = p_args[ARG_CELL_FORMULA ].val.ustr;
    load_cell_ownform.tstr_mrofmun_style   = p_args[ARG_CELL_MROFMUN ].val.tstr;

    load_cell_ownform.clip_data_from_cut_operation = p_of_ip_format->clip_data_from_cut_operation;

    if(!load_cell_ownform.ustr_inline_contents && !load_cell_ownform.ustr_formula)
        return(STATUS_OK);

    return(insert_cell_contents_ownform(p_docu, object_id, T5_MSG_LOAD_CELL_OWNFORM, &load_cell_ownform, &position));
}

/******************************************************************************
*
* invent a new autoname based on incoming file's leafname
* and create a style load mapping entry to fixup incoming regions
*
******************************************************************************/

_Check_return_
static STATUS
style_autoname(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _In_z_      PCTSTR tstr_style_name,
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format)
{
    PCTSTR input_filename = p_of_ip_format->input_filename;
    PCTSTR input_leafname = input_filename ? file_leafname(input_filename) : TEXT("Untitled");
    S32 i = 0;
    TCHARZ tstr_buf[256];
    PTSTR p_buf;
    STATUS status;

    tstr_buf[0] = STYLE_AUTONAME_PREFIX_CH;
    p_buf = tstr_buf + 1;
    tstr_xstrkpy(p_buf, elemof32(tstr_buf) - PtrDiffElemU32(p_buf, tstr_buf), input_leafname); /* no extra guff first time through */
    p_buf += tstrlen32(p_buf);

    for(;;)
    {
        tstr_xstrkat(p_buf, elemof32(tstr_buf) - PtrDiffElemU32(p_buf, tstr_buf), tstr_style_name); /* still has prefix -> sepch */

        if(0 == style_handle_from_name(p_docu, tstr_buf))
        {
            status_return(al_tstr_set(p_array_handle_tstr, tstr_buf));

            {
            P_STYLE_LOAD_MAP p_style_load_map;
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_style_load_map), 1);

            if(NULL != (p_style_load_map = al_array_extend_by(&p_of_ip_format->renamed_styles, STYLE_LOAD_MAP, 1, &array_init_block, &status)))
            {
                if(status_ok(status = al_tstr_set(&p_style_load_map->h_new_name_tstr, tstr_buf)))
                             status = al_tstr_set(&p_style_load_map->h_old_name_tstr, tstr_style_name);

                if(status_fail(status))
                {
                    al_array_dispose(&p_style_load_map->h_new_name_tstr);
                    al_array_dispose(&p_style_load_map->h_old_name_tstr);

                    al_array_shrink_by(&p_of_ip_format->renamed_styles, -1);
                }
            }
            } /*block*/

            if(status_fail(status))
                al_array_dispose(p_array_handle_tstr);

            return(status);
        }

        consume_int(tstr_xsnprintf(p_buf, elemof32(tstr_buf) - PtrDiffElemU32(p_buf, tstr_buf),
                                   S32_TFMT,
                                   ++i));
    }
}

_Check_return_
static STATUS
update_style_from_choices(
    _DocuRef_   P_DOCU p_docu_loaded,
    _InoutRef_  P_STYLE p_style_loaded)
{
    const PCTSTR tstr_style_name_loaded = array_tstr(&p_style_loaded->h_style_name_tstr);
    const PC_DOCU p_docu_config = p_docu_from_config();
    const PCTSTR tstr_update_prefix = TEXT("Update.");
    const U32 update_prefix_chars = tstrlen32(tstr_update_prefix);
    STYLE_HANDLE style_handle_config = -1;
    P_STYLE p_style_config = NULL;

    if(p_docu_loaded == p_docu_config)
        return(STATUS_OK);

    while(style_enum_styles(p_docu_config, &p_style_config, &style_handle_config) > 0)
    {
        PCTSTR tstr_style_name_config = array_tstr(&p_style_config->h_style_name_tstr);

        if(0 != tstrnicmp(tstr_style_name_config, tstr_update_prefix, update_prefix_chars))
            continue;

        /* this is an Update.StyleName style - skip the prefix */
        tstr_style_name_config += update_prefix_chars;

        if(0 != tstricmp(tstr_style_name_config, tstr_style_name_loaded))
            continue;

        { /* rest of the name matches - update */
        STATUS status;
        STYLE style_config;
        style_init(&style_config);
        if(status_ok(status = style_duplicate(&style_config, p_style_config, &style_selector_all)))
        {
            /* steal the loaded style's name */
            al_array_dispose(&style_config.h_style_name_tstr);
            style_config.h_style_name_tstr = p_style_loaded->h_style_name_tstr;
            style_selector_bit_clear(&p_style_loaded->selector, STYLE_SW_NAME);
            style_free_resources_all(p_style_loaded); /* dispose of the current definition */

            *p_style_loaded = style_config; /* donate */
        }
        return(status);
        } /*block*/
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
t5_cmd_of_style(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 2);
    PCTSTR tstr_style_name = p_args[0].val.tstr;
    PC_USTR_INLINE p_style_ustr_inline = p_args[1].val.ustr_inline;
    STYLE style;
    BOOL add;
    STYLE_HANDLE existing_style_handle;
    STATUS status = STATUS_OK;

    trace_0(TRACE_APP_LOAD, TEXT("IN: load_style"));

    style_init(&style);

    if(NULL != p_style_ustr_inline) /* NB. empty style definitions are allowed */
        status_return(style_struct_from_ustr_inline(p_docu, &style, p_style_ustr_inline, &style_selector_all));

    for(;;) /* loop for structure */
    {
        status_break(status = al_tstr_set(&style.h_style_name_tstr, tstr_style_name));
        style_bit_set(&style, STYLE_SW_NAME);

        /* always add if style doesn't exist */
        add = (0 == (existing_style_handle = style_handle_from_name(p_docu, tstr_style_name)));

        if(add && global_preferences.update_styles_from_choices && !p_construct_convert->p_of_ip_format->flags.insert)
            status_break(status = update_style_from_choices(p_docu, &style));

        if(!add)
        {   /* otherwise override existing style if it's a template (pedantically a template insert) */
            /* done AFTER considering update_styles_from_choices so that template inserts use the style you explictly wanted */
            add = p_construct_convert->p_of_ip_format->flags.is_template /* && p_construct_convert->p_of_ip_format->flags.insert */;
        }

        /* otherwise if it's an autoname style then add under an autoname if different */
        if(!add && (*tstr_style_name == STYLE_AUTONAME_PREFIX_CH))
        {
            STYLE existing_style;
            STYLE_SELECTOR style_selector_diff;

            style_init(&existing_style);
            style_struct_from_handle(p_docu, &existing_style, existing_style_handle, &style_selector_all);
            add = style_compare(&style_selector_diff, &style, &existing_style);

            if(add)
            {
                al_array_dispose(&style.h_style_name_tstr);

                if(status_fail(status = style_autoname(p_docu, &style.h_style_name_tstr, tstr_style_name, p_construct_convert->p_of_ip_format)))
                    add = 0;
            }
        }

        if(add)
            /* donated all resources to style list (or thrown away in case of error) */
            return(style_handle_add(p_docu, &style));

        break; /* out of loop for structure */
    }

    style_free_resources_all(&style);

    return(status);
}

/*
Takes a block construct and blasts a hole in the document, inserting fragments if present
*/

_Check_return_
static STATUS
t5_cmd_of_block(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, ARG_BLOCK_N_ARGS);
    PC_USTR_INLINE ustr_inline_frag_stt = NULL;
    PC_USTR_INLINE ustr_inline_frag_end = NULL;
    SLR doc_size;
    STATUS status;

    /* 8.2.94 MRJC initialise flow object for old documents */
    if(!p_docu->flags.flow_installed)
    {
        OBJECT_ID object_id = OBJECT_ID_CELLS;
        trace_0(TRACE_OUT | TRACE_ANY, TEXT("File has no flow construct - setting CELLS"));
        status_assert(maeve_event(p_docu, T5_MSG_INIT_FLOW, &object_id));
    }

    /* NB. these args aren't COL,ROW as COL,ROW arg processing during loading offset using the original_docu_area set up here! */

    /* Top left position */
    p_of_ip_format->original_docu_area.tl.slr.col                   = p_args[ARG_BLOCK_TL_COL].val.col;
    p_of_ip_format->original_docu_area.tl.slr.row                   = p_args[ARG_BLOCK_TL_ROW].val.row;
    p_of_ip_format->original_docu_area.tl.object_position.data      = p_args[ARG_BLOCK_TL_DATA].val.s32;
    p_of_ip_format->original_docu_area.tl.object_position.object_id = p_args[ARG_BLOCK_TL_OBJECT_ID].val.object_id;

    /* Bottom right position */
    p_of_ip_format->original_docu_area.br.slr.col                   = p_args[ARG_BLOCK_BR_COL].val.col;
    p_of_ip_format->original_docu_area.br.slr.row                   = p_args[ARG_BLOCK_BR_ROW].val.row;
    p_of_ip_format->original_docu_area.br.object_position.data      = p_args[ARG_BLOCK_BR_DATA].val.s32;
    p_of_ip_format->original_docu_area.br.object_position.object_id = p_args[ARG_BLOCK_BR_OBJECT_ID].val.object_id;

    /* set states of whole bits; these are used for clipping incoming regions */
    p_of_ip_format->original_docu_area.whole_col = 0;
    p_of_ip_format->original_docu_area.whole_row = 0;

    doc_size.col = p_args[ARG_BLOCK_DOC_COLS].val.col;
    doc_size.row = p_args[ARG_BLOCK_DOC_ROWS].val.row;

    if(arg_is_present(p_args, ARG_BLOCK_FA))
        ustr_inline_frag_stt = p_args[ARG_BLOCK_FA].val.ustr_inline;

    if(arg_is_present(p_args, ARG_BLOCK_FB))
        ustr_inline_frag_end = p_args[ARG_BLOCK_FB].val.ustr_inline;

    /* if object_position isn't set but we have a fragment, assume we are at start of a (text) cell */
    if(ustr_inline_frag_stt && (OBJECT_ID_NONE == p_of_ip_format->original_docu_area.tl.object_position.object_id))
    {
        p_of_ip_format->original_docu_area.tl.object_position.data      = 0;
        p_of_ip_format->original_docu_area.tl.object_position.object_id = OBJECT_ID_TEXT;
    }

    if(!ustr_inline_frag_stt && !ustr_inline_frag_end)
    {
        if( (p_of_ip_format->insert_position.slr.col == 0)
            &&
            ((p_of_ip_format->original_docu_area.br.slr.col - p_of_ip_format->original_docu_area.tl.slr.col) >= doc_size.col) /* whole_row in original */
            &&
            ((p_of_ip_format->original_docu_area.br.slr.col - p_of_ip_format->original_docu_area.tl.slr.col) >= n_cols_logical(p_docu)) /* becomes whole_row in this document */
            &&
            (0 != n_cols_logical(p_docu))
          )
            p_of_ip_format->original_docu_area.whole_row = 1;
    }

    if(!docu_area_is_frag(&p_of_ip_format->original_docu_area))
        cur_change_before(p_docu);

    /* insert document space with split as needed, updating insert_position */
    status_return(cells_docu_area_insert(p_docu, &p_of_ip_format->insert_position, &p_of_ip_format->original_docu_area, &p_of_ip_format->insert_position));

    { /* set new extents (table stuff MRJC 08.11.93, revised SKS 05.11.95 and 16.11.95) */
    COL cols_add = (p_of_ip_format->original_docu_area.br.slr.col - p_of_ip_format->original_docu_area.tl.slr.col);
    ROW rows_add = (p_of_ip_format->original_docu_area.br.slr.row - p_of_ip_format->original_docu_area.tl.slr.row);

    if( p_of_ip_format->flags.insert
    && !p_of_ip_format->flags.incoming_base_single_col
    && p_docu->flags.base_single_col
    && (p_of_ip_format->insert_position.slr.col == 0)
    && !cell_in_table(p_docu, &p_of_ip_format->insert_position.slr)
    && !docu_area_is_frag(&p_of_ip_format->original_docu_area))
        p_of_ip_format->flags.table_insert = 1;

    if(p_of_ip_format->flags.table_insert)
        p_of_ip_format->insert_position.slr.col += 1;

    {
    SLR block_slr  = p_of_ip_format->insert_position.slr;
    SLR extent_slr;
    block_slr.col += cols_add;
    block_slr.row += rows_add;
    extent_slr.col = n_cols_logical(p_docu);
    extent_slr.row = n_rows(p_docu);
    status_return(format_col_row_extents_set(p_docu, MAX(block_slr.col, extent_slr.col), MAX(block_slr.row, extent_slr.row)));
    } /*block*/

    } /*block*/

    /* Load both fragments to correct objects & positions */

    if(ustr_inline_frag_stt)
    {
        LOAD_CELL_OWNFORM load_cell_ownform_stt_frag;
        OBJECT_ID object_id;

        zero_struct(load_cell_ownform_stt_frag);
        load_cell_ownform_stt_frag.ustr_inline_contents = ustr_inline_frag_stt;
        load_cell_ownform_stt_frag.ustr_formula = NULL;

        { /* 19.12.94 send load fragment to object owning an existing cell */
        OBJECT_DATA object_data;
        if(cell_data_from_position(p_docu, &object_data, &p_of_ip_format->insert_position, NULL))
            object_id = object_data.object_id;
        else if(status_fail(status = object_id_from_construct_id(p_args[ARG_BLOCK_FA_CS1].val.u8c, &object_id)))
        {
            if(status != STATUS_FAIL)
                return(status);
            status = STATUS_OK;
        }
        } /*block*/

        {
        U8 owner = p_args[ARG_BLOCK_FA_CS2].val.u8c;

        if(status_fail(data_type_from_construct_id(owner, &load_cell_ownform_stt_frag.data_type)))
        {
            p_of_ip_format->flags.unknown_data_type = 1;
            return(STATUS_OK);
        }
        } /*block*/

        load_cell_ownform_stt_frag.original_slr = p_of_ip_format->original_docu_area.tl.slr;

        /* call object to load in start fragment */
        status_return(insert_cell_contents_ownform(p_docu, object_id, T5_MSG_LOAD_FRAG_OWNFORM, &load_cell_ownform_stt_frag, &p_of_ip_format->insert_position));
    }

    if(ustr_inline_frag_end)
    {
        LOAD_CELL_OWNFORM load_cell_ownform_end_frag;
        OBJECT_ID object_id;
        POSITION position_end;

        zero_struct(load_cell_ownform_end_frag);
        load_cell_ownform_end_frag.ustr_inline_contents = ustr_inline_frag_end;
        load_cell_ownform_end_frag.ustr_formula = NULL;

        { /* 19.12.94 send load fragment to object owning an existing cell */
        OBJECT_DATA object_data;
        if(cell_data_from_position(p_docu, &object_data, &p_of_ip_format->insert_position, NULL))
            object_id = object_data.object_id;
        else if(status_fail(status = object_id_from_construct_id(p_args[ARG_BLOCK_FB_CS1].val.u8c, &object_id)))
        {
            if(status != STATUS_FAIL)
                return(status);
            status = STATUS_OK;
        }
        } /*block*/

        {
        U8 owner = p_args[ARG_BLOCK_FB_CS2].val.u8c;

        if(status_fail(data_type_from_construct_id(owner, &load_cell_ownform_end_frag.data_type)))
        {
            p_of_ip_format->flags.unknown_data_type = 1;
            return(STATUS_OK);
        }
        } /*block*/

        /* compose slr for trailing fragment */
        position_end.slr = p_of_ip_format->original_docu_area.br.slr;
        position_end.slr.col += (p_of_ip_format->insert_position.slr.col - p_of_ip_format->original_docu_area.tl.slr.col) - 1;
        position_end.slr.row += (p_of_ip_format->insert_position.slr.row - p_of_ip_format->original_docu_area.tl.slr.row) - 1;

        {
        OBJECT_POSITION_SET object_position_set;

        position_end.object_position.object_id = OBJECT_ID_NONE;

        if(STATUS_DONE == object_data_from_position(p_docu, &object_position_set.object_data, &position_end, NULL))
        {
            object_position_set.action = OBJECT_POSITION_SET_START;
            status_consume(object_call_id(object_position_set.object_data.object_id, p_docu, T5_MSG_OBJECT_POSITION_SET, &object_position_set));
            position_end.object_position = object_position_set.object_data.object_position_start;
        }
        } /*block*/

        load_cell_ownform_end_frag.original_slr = position_end.slr;
        load_cell_ownform_end_frag.original_slr.col += (p_of_ip_format->original_docu_area.tl.slr.col - p_of_ip_format->insert_position.slr.col);
        load_cell_ownform_end_frag.original_slr.row += (p_of_ip_format->original_docu_area.tl.slr.row - p_of_ip_format->insert_position.slr.row);

        status_return(insert_cell_contents_ownform(p_docu, object_id, T5_MSG_LOAD_FRAG_OWNFORM, &load_cell_ownform_end_frag, &position_end));
    }

    /* flag document has data */
    p_docu->flags.has_data = 1;

    return(STATUS_OK);
}

_Check_return_
static STATUS
t5_cmd_of_style_specific(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    STATUS status = STATUS_OK;
    const P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    const T5_MESSAGE t5_message = p_construct_convert->p_construct->t5_message;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
    PCTSTR style_name = p_args[0].val.tstr;
    STYLE_HANDLE style_handle = style_handle_from_name(p_docu, style_name);

    if(0 == style_handle)
        return(create_error(ERR_NAMED_STYLE_NOT_FOUND));

    switch(t5_message)
    {
    default: default_unhandled();
#if CHECKING
    case T5_CMD_OF_STYLE_BASE:
#endif
        /* 13.9.94 this copes with old files containing base style bindings */
        if(p_of_ip_format->flags.insert && !p_of_ip_format->flags.is_template)
        {
            if(style_handle == style_handle_base(&p_docu->h_style_docu_area))
                p_of_ip_format->flags.base_style_same = 1;
        }
        break;

    case T5_CMD_OF_STYLE_CURRENT:
        { /* 12.9.94 this copes with old files containing references to current cell styles */
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        DOCU_AREA docu_area;

        STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle);
        style_docu_area_add_parm.object_id = OBJECT_ID_IMPLIED_STYLE;
        style_docu_area_add_parm.t5_message = T5_EXT_STYLE_CELL_CURRENT;
        style_docu_area_add_parm.region_class = REGION_UPPER;

        docu_area_init(&docu_area);
        docu_area.whole_col = 1;
        docu_area.whole_row = 1;

        status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);
        break;
        }

    case T5_CMD_OF_STYLE_HEFO:
        break;

    case T5_CMD_OF_STYLE_TEXT:
        { /* 12.9.94 this copes with old files containing references to text styles */
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        DOCU_AREA docu_area;

        STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle);
        style_docu_area_add_parm.object_id = OBJECT_ID_IMPLIED_STYLE;
        style_docu_area_add_parm.t5_message = T5_EXT_STYLE_CELL_TYPE;
        style_docu_area_add_parm.arg = OBJECT_ID_TEXT;
        style_docu_area_add_parm.region_class = REGION_LOWER;

        docu_area_init(&docu_area);
        docu_area.whole_col = 1;
        docu_area.whole_row = 1;

        status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);
        break;
        }
    }

    return(status);
}

_Check_return_
static STATUS
t5_cmd_of_flow(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);
    OBJECT_ID object_id = p_args[0].val.object_id;

    assert_EQ(p_args[0].type, ARG_TYPE_S32 | ARG_MANDATORY);

    /* flow object has now been specified */
    status_assert(maeve_event(p_docu, T5_MSG_INIT_FLOW, &object_id));

    return(STATUS_OK);
}

_Check_return_
static STATUS
t5_cmd_of_rowtable(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    /* SKS 03feb96 defer execution to end of data */
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);

    IGNOREPARM_DocuRef_(p_docu);

    p_construct_convert->p_of_ip_format->flags.virtual_row_table = p_args[0].val.fBool;

    return(STATUS_OK);
}

_Check_return_
static STATUS
t5_cmd_of_base_single_col(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 1);

    IGNOREPARM_DocuRef_(p_docu);

    if(p_args[0].val.fBool)
        p_construct_convert->p_of_ip_format->flags.incoming_base_single_col = 1;

    return(STATUS_OK);
}

_Check_return_
static TAB_TYPE
tab_type_from_ownform(
    _InVal_     U8 ch)
{
    static const U8 ownform_tab_types[] = { 'L', 'C', 'R', 'D' };

    TAB_TYPE tab_type = 0;

    for(tab_type = 0; tab_type < (TAB_TYPE) elemof32(ownform_tab_types); ++tab_type)
        if(ch == ownform_tab_types[tab_type])
            return(tab_type);

    assert0();
    return(TAB_LEFT);
}

/* end of sk_load.c */
