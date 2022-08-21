/* fl_csv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* CSV load object module for Fireworkz */

/* SKS January 1993 */

#include "common/gflags.h"

#include "fl_csv/fl_csv.h"

#include "ob_skel/ff_io.h"

#include "cmodules/unicode/u2000.h" /* 2000..206F General Punctuation */

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#define MSG_WEAK &rb_fl_csv_msg_weak
extern PC_U8 rb_fl_csv_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_FL_CSV NULL

/*
internal routines
*/

_Check_return_
static STATUS
csv_load_file_apply_style_label(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area_table,
    _InVal_     COL labels_across);

#define CSV_FIELD_SEP_CH  CH_COMMA
#define CSV_FIELD_SEP_STR ","

static U8
g_field_sep_ch = CH_NULL; /* auto-detect */

#define QUOTE_STR     "\""

#define CSV_LOAD_ROWS_INCREMENT     8
#define CSV_LOAD_ROWS_INCREMENT_MAX 8192

enum CSV_LOAD_QUERY_IDS
{
    CSV_LOAD_QUERY_ID_INSERT_AS_TABLE = 666,
    CSV_LOAD_QUERY_ID_OVERWRITE_BLANK_CELLS,
    CSV_LOAD_QUERY_ID_LABELS,
    CSV_LOAD_QUERY_ID_ACROSS_TEXT,
    CSV_LOAD_QUERY_ID_ACROSS,
    CSV_LOAD_QUERY_ID_DATABASE
};

/* -------------------------------------------------------------------------------------------- */

static inline void
csv_quick_ublock_setup(
    _OutRef_    P_QUICK_UBLOCK p_quick_ublock /*set up*/,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    quick_ublock_setup_without_clearing_ubuf(p_quick_ublock, array_base(p_array_handle, _UCHARS), array_size32(p_array_handle));
}

/*
if the quick block now contains an array_handle,
this will be bigger than the previous handle so
deallocate the previous one and setup with this
new bigger one
*/

static void
csv_quick_ublock_dispose(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*disposed,reset*/,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    quick_ublock_empty(p_quick_ublock);

    if(0 != quick_ublock_array_handle_ref(p_quick_ublock))
    {
        al_array_dispose(p_array_handle);

        *p_array_handle = quick_ublock_array_handle_ref(p_quick_ublock);

        csv_quick_ublock_setup(p_quick_ublock, p_array_handle);
    }
}

_Check_return_
static STATUS
csv_load_decode_line(
    _DocuRef_maybenone_ P_DOCU p_docu,
    _In_z_      PC_USTR_INLINE ustr_inline_line_contents,
    _InoutRef_  P_SLR p_slr,
    _InVal_     S32 labels_across,
    _InVal_     BOOL sizing_up,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle /*appended*/ /*maybe NULL*/,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_temp_decode /*temp use*/,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle_temp_decode /*temp use*/)
{
    PC_U8Z p_u8 = (PC_U8Z) ustr_inline_line_contents;
    STATUS status = STATUS_OK;
    COL col = 0;
    BOOL eol = FALSE;

#if CHECKING
    if(sizing_up)
        assert(IS_DOCU_NONE(p_docu));
    else
        DOCU_ASSERT(p_docu);
#endif

    while(!eol)
    {
        U8 ch;
        BOOL is_a_string = FALSE;
        BOOL in_quotes = FALSE;
        BOOL absorbing = FALSE;
        S32 chars_this_field = 0;
        S32 n_spaces = 0;

        StrSkipSpaces(p_u8); /* strip leading spaces from field */

        if(*p_u8 == CH_QUOTATION_MARK)
        {
            in_quotes = TRUE;
            is_a_string = TRUE;
            p_u8++;
        }

        /* now copy over till we find end of field */
        for(;;)
        {
            ch = *p_u8++;

            if(CH_NULL == ch)
            {
                eol = TRUE;

                if(!sizing_up)
                    status = quick_ublock_nullch_add(p_quick_ublock_temp_decode);

                break;
            }

            if((CH_NULL == g_field_sep_ch) && !in_quotes && ((ch == CSV_FIELD_SEP_CH) || (ch == CH_TAB))) /* SKS 12dec94 let them stamp TSV as CSV to get this */
                g_field_sep_ch = ch;

            if((ch == g_field_sep_ch) && !in_quotes)
            {
                if(labels_across)
                {
                    if(chars_this_field)
                        if(!sizing_up)
                            status = inline_quick_ublock_from_code(p_quick_ublock_temp_decode, IL_RETURN);

                    break;
                }

                ch = CH_NULL;

                if(!sizing_up)
                    status = quick_ublock_nullch_add(p_quick_ublock_temp_decode);

                break;
            }

            if((ch == CH_QUOTATION_MARK) && in_quotes)
            {
                if(*p_u8 == CH_QUOTATION_MARK)
                    p_u8++;
                else
                {
                    in_quotes = FALSE;
                    absorbing = TRUE;
                }
            }

            if(absorbing)
                continue;

            if(ch == CH_SPACE) /* SKS 09nov94 adds trailing space suppression too, to aid constant recogniser below */
            {
                n_spaces += 1;
                continue;
            }

            if(n_spaces)
            {
                while(--n_spaces >= 0) /* flush spaces so far */
                {
                    if(!sizing_up)
                        status_break(status = quick_ublock_a7char_add(p_quick_ublock_temp_decode, CH_SPACE));

                    chars_this_field++;
                }

                n_spaces = 0;

                status_break(status);
            }

            if(CH_TAB == ch)
            {
                is_a_string = TRUE; /* can't parse this! */

                if(!sizing_up)
                    status_break(status = inline_quick_ublock_IL_TAB(p_quick_ublock_temp_decode));

                chars_this_field++;
                continue;
            }

#if USTR_IS_SBSTR /* might contain IL_UTF8 */
            if(CH_INLINE == ch)
            {
                PC_UCHARS_INLINE uchars_inline = PtrSubBytes(PC_UCHARS_INLINE, p_u8, 1);
                const U32 n_bytes = inline_bytecount(uchars_inline);

                is_a_string = TRUE; /* can't parse this! */

                if(!sizing_up)
                    status_break(status = quick_ublock_uchars_add(p_quick_ublock_temp_decode, (PC_UCHARS) uchars_inline, n_bytes)); /* copy blindly */

                chars_this_field += n_bytes;
                continue;
            }
#endif

            if(!sizing_up)
                status_break(status = quick_ublock_ucs4_add(p_quick_ublock_temp_decode, ch));

            chars_this_field++;
            continue;
        }

        status_break(status);

        if(eol || !labels_across)
        {
            if(!sizing_up)
            {
                PC_USTR_INLINE ustr_inline_elem = quick_ublock_ustr_inline(p_quick_ublock_temp_decode);  /* temp hack for U inlines */
                EV_DATA ev_data;

                ev_data_set_blank(&ev_data);

                if(!is_a_string && !labels_across)
                {
#if 1
                    assert(ustr_inline_elem);
                    status_assert(ss_recog_constant(&ev_data, (PC_USTR) ustr_inline_elem /*, FALSE*/));
#else
                    ARRAY_HANDLE h_mrofmun;
                    STYLE_HANDLE style_handle_autoformat;
                    if(status_ok(mrofmun_get_list(p_docu, &h_mrofmun)))
                        status_assert(autoformat(&ev_data, &style_handle_autoformat, ustr_elem, &h_mrofmun));
#endif
                }

                if(NULL != p_array_handle)
                {
                    EV_DATA ev_data_copy;

                    switch(ev_data.did_num)
                    {
                    case RPN_DAT_REAL:
                    case RPN_DAT_BOOL8:
                    case RPN_DAT_WORD8:
                    case RPN_DAT_WORD16:
                    case RPN_DAT_WORD32:
                    case RPN_DAT_DATE:
                        break;

                    default:
                        ev_data.did_num = RPN_DAT_STRING;
                        ev_data.local_data = FALSE;
                        ev_data.arg.string.uchars = (PC_UCHARS) ustr_inline_elem;
                        ev_data.arg.string.size = ustr_inline_strlen32(ustr_inline_elem);
                        assert(!contains_inline(ev_data.arg.string.uchars, ev_data.arg.string.size));
                        break;
                    }

                    if(status_ok(status = ss_data_resource_copy(&ev_data_copy, &ev_data)))
                    {
                        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(EV_DATA), TRUE);
                        status = al_array_add(p_array_handle, EV_DATA, 1, &array_init_block, &ev_data_copy);
                    }
                }
                else
                {
                    LOAD_CELL_FOREIGN load_cell_foreign;
                    SS_RECOG_CONTEXT ss_recog_context;

                    ss_recog_context_push(&ss_recog_context); /* recognise numbers from file as if typed by user with current UI settings */

                    zero_struct(load_cell_foreign);

                    switch(ev_data.did_num)
                    {
                    case RPN_DAT_REAL:
                    case RPN_DAT_BOOL8:
                    case RPN_DAT_WORD8:
                    case RPN_DAT_WORD16:
                    case RPN_DAT_WORD32:
                        load_cell_foreign.data_type = OWNFORM_DATA_TYPE_CONSTANT;
                        break;

                    case RPN_DAT_DATE:
                        load_cell_foreign.data_type = OWNFORM_DATA_TYPE_DATE;
                        break;

                    default:
                        load_cell_foreign.data_type = OWNFORM_DATA_TYPE_TEXT;
                        break;
                    }

                    {
                    const OBJECT_ID object_id = (load_cell_foreign.data_type == OWNFORM_DATA_TYPE_TEXT) ? OBJECT_ID_TEXT : OBJECT_ID_SS;
                    SLR slr = *p_slr;
                    slr.col += col;
                    consume_bool(object_data_from_slr(p_docu, &load_cell_foreign.object_data, &slr));
                    load_cell_foreign.original_slr = slr;

                    load_cell_foreign.ustr_inline_contents = ustr_inline_elem;
                  /*load_cell_foreign.ustr_formula = NULL;*/

                    if(status_ok(status))
                        status = insert_cell_contents_foreign(p_docu, object_id, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);
                    } /*block*/

                    ss_recog_context_pull(&ss_recog_context);
                }

                ss_data_free_resources(&ev_data);

                csv_quick_ublock_dispose(p_quick_ublock_temp_decode, p_array_handle_temp_decode);
            }
            else
            {
                /* record maximum field attained */
                if( p_slr->col < col + 1)
                    p_slr->col = col + 1;
            }
        }

        status_break(status);

        if(!labels_across)
            col++;
    }

    csv_quick_ublock_dispose(p_quick_ublock_temp_decode, p_array_handle_temp_decode);

    return(status);
}

/******************************************************************************
*
* read a line from a file, returning error or terminator
*
******************************************************************************/

_Check_return_
static STATUS
csv_load_line(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_contents /*CH_NULL-terminated*/)
{
    for(;;)
    {
        UCS4 char_read;
        
        status_return(plain_read_ucs4_character(p_ff_ip_format, &char_read));

        if(char_read <= 0x1F)
        {
            if((LF != char_read) && (CH_TAB != char_read))
                /* ignore other C0 CtrlChars */
                continue;
        }

        if((LF == char_read) || (UCH_LINE_SEPARATOR == char_read) || (EOF_READ == char_read))
        {
            status_return(quick_ublock_nullch_add(p_quick_ublock_contents));
            return(char_read);
        }

        status_return(quick_ublock_ucs4_add_aiu(p_quick_ublock_contents, char_read));
    }
}

_Check_return_
static STATUS
csv_msg_insert_foreign_as_db(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    STATUS status;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
    quick_tblock_with_buffer_setup(quick_tblock);

    if(NULL == p_docu->docu_name.path_name)
        return(create_error(CSV_ERR_DOC_NOT_SAVED));

    status = file_derive_name(p_docu->docu_name.path_name, p_docu->docu_name.leaf_name, TEXT("_b"), &quick_tblock, 0 /*CSV_ERR_CANT_CREATE_DB_ISAFILE*/);
    if(FILE_ERR_ISAFILE == status)
    { /* SKS 28jul95 have another go! */
        quick_tblock_dispose(&quick_tblock);
        status = file_tempname(p_docu->docu_name.path_name, p_docu->docu_name.leaf_name, NULL, FILE_TEMPNAME_INITIAL_TRY, &quick_tblock);
    }
    if(status_ok(status))
    {
        RECORDZ_IMPORT recordz_import;
        recordz_import.msg_insert_foreign = *p_msg_insert_foreign;
        recordz_import.input_filename = p_msg_insert_foreign->filename;
        recordz_import.output_filename = quick_tblock_tstr(&quick_tblock);
        status = object_call_id_load(p_docu, T5_MSG_RECORDZ_IMPORT, &recordz_import, OBJECT_ID_REC);
    }
    quick_tblock_dispose(&quick_tblock);

    return(status);
}

/* find the CSV file size (cols, rows) */

_Check_return_
static STATUS
csv_load_file_sizeup(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    P_ARRAY_HANDLE p_array_handle_contents,
    P_ARRAY_HANDLE p_array_handle_temp_decode,
    _OutRef_    P_SLR p_size,
    _InVal_     COL labels_across,
    _InVal_     BOOL is_sid)
{
    STATUS status;
    COL label_idx = 0;

    p_size->col = 0;
    p_size->row = 0;

    { /* SKS 11oct95 make quick_block for contents and temp decode grow to size of max line used for quick reallocs */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1024, sizeof32(U8), FALSE);
    if(status_ok(status = al_array_preallocate_zero(p_array_handle_contents, &array_init_block)))
        status = al_array_preallocate_zero(p_array_handle_temp_decode, &array_init_block);
    } /*block*/

    while(status_ok(status))
    {
        STATUS load_line_status;
        QUICK_UBLOCK contents;

        /* each time round, use the grown dynamic buffer as the static buffer */
        csv_quick_ublock_setup(&contents, p_array_handle_contents);

        status = csv_load_line(p_ff_ip_format, &contents);

        load_reflect_status(&p_ff_ip_format->of_ip_format);

        if(status_fail(status))
        {
            csv_quick_ublock_dispose(&contents, p_array_handle_contents);
            break;
        }

        load_line_status = status;

        if(is_sid)
        {
            if(quick_ublock_bytes(&contents) >= 2)
            {
                PC_USTR ustr = quick_ublock_ustr(&contents);

                /* SID directive? */
                if((PtrGetByte(ustr) == CH_PERCENT_SIGN) && (PtrGetByteOff(ustr, 1) == CH_PERCENT_SIGN))
                {
                    csv_quick_ublock_dispose(&contents, p_array_handle_contents);
                    continue;
                }
            }
        }

        if(quick_ublock_bytes(&contents) >= 2)
        {
            QUICK_UBLOCK temp_decode;

            /* each time round, use the grown dynamic buffer as the static buffer */
            csv_quick_ublock_setup(&temp_decode, p_array_handle_temp_decode);

            status = csv_load_decode_line(P_DOCU_NONE, quick_ublock_ustr_inline(&contents),
                                          p_size, labels_across, TRUE,
                                          NULL, &temp_decode, p_array_handle_temp_decode);
        }

        csv_quick_ublock_dispose(&contents, p_array_handle_contents);

        status_break(status);

        if(labels_across)
        {
            if(++label_idx >= labels_across)
            {
                label_idx = 0;
                p_size->row++;
            }
        }
        else
        {
            p_size->row++;
        }

        if(EOF_READ == load_line_status)
        {
            status = STATUS_OK;
            break;
        }
    }

    if(status_ok(status))
    {
        if(labels_across)
        {
            /* partially filled rows of labels? */
            if(label_idx != 0)
            {
                p_size->row++;
            }

            p_size->col = labels_across;
        }

        status = input_rewind(&p_ff_ip_format->of_ip_format.input);

        p_ff_ip_format->of_ip_format.input.file.file_offset_in_bytes = 0; /* manual reset */
        p_ff_ip_format->of_ip_format.process_status_threshold = 0; /* manual reset */

        load_reflect_status(&p_ff_ip_format->of_ip_format);
    }

    return(status);
}

_Check_return_ _Success_(status_ok(return))
static STATUS
csv_load_file_core(
    _DocuRef_   P_DOCU p_docu,
    P_POSITION p_position,
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _InVal_     S32 load_type,
    _InVal_     COL labels_across,
    _InVal_     BOOL is_sid,
    _OutRef_    P_SLR p_i_slr)
{
    ARRAY_HANDLE array_handle_contents = 0;
    ARRAY_HANDLE array_handle_temp_decode = 0;
    COL lhs_extra_cols = 0;
    ROW top_extra_rows = 0;
    ROW bot_extra_rows = 0;
    SLR insert_size_slr;
    STATUS status = STATUS_OK;

    status = csv_load_file_sizeup(p_ff_ip_format, &array_handle_contents, &array_handle_temp_decode, &insert_size_slr, labels_across, is_sid);

    if(status_ok(status))
    {
        /* split the document at insertion point */
        DOCU_AREA docu_area;
        COL docu_cols;
        ROW docu_rows;
        SLR slr;

        docu_area_init(&docu_area);
        docu_area.br.slr = insert_size_slr;

        if(load_type == CSV_LOAD_QUERY_ID_INSERT_AS_TABLE)
        {
            /* we require an extra LHS column for positioning if inserting as a table at the first column */
            if(0 == p_position->slr.col)
                lhs_extra_cols = 1;

            top_extra_rows = 1;
            bot_extra_rows = 1;

            docu_area.br.slr.col += lhs_extra_cols;
            docu_area.br.slr.row += top_extra_rows + bot_extra_rows;
        }

        status_return(cells_docu_area_insert(p_docu, p_position /*updated*/, &docu_area, p_position));

        slr = p_position->slr;
        slr.col += lhs_extra_cols;
        slr.row += top_extra_rows;

        docu_cols = n_cols_logical(p_docu);
        docu_rows = n_rows(p_docu);

        status_return(format_col_row_extents_set(p_docu,
                                                 MAX(docu_cols, slr.col + insert_size_slr.col),
                                                 MAX(docu_rows, slr.row + insert_size_slr.row + bot_extra_rows)));

        /* remember address of first cell */
        *p_i_slr = slr;

        for(;;)
        {
            STATUS load_line_status;
            QUICK_UBLOCK contents_quick_ublock;

            /* each time round, use the grown dynamic buffer as the static buffer */
            csv_quick_ublock_setup(&contents_quick_ublock, &array_handle_contents);

            status = csv_load_line(p_ff_ip_format, &contents_quick_ublock);

            load_reflect_status(&p_ff_ip_format->of_ip_format);

            if(status_fail(status))
            {
                csv_quick_ublock_dispose(&contents_quick_ublock, &array_handle_contents);
                break;
            }

            load_line_status = status;

            if(is_sid)
            {
                if(quick_ublock_bytes(&contents_quick_ublock) >= 2)
                {
                    PC_USTR ustr = quick_ublock_ustr(&contents_quick_ublock);

                    /* SID directive? */
                    if((PtrGetByte(ustr) == CH_PERCENT_SIGN) && (PtrGetByteOff(ustr, 1) == CH_PERCENT_SIGN))
                    {
                        csv_quick_ublock_dispose(&contents_quick_ublock, &array_handle_contents);
                        continue;
                    }
                }
            }

            p_docu = p_docu_from_docno(p_ff_ip_format->of_ip_format.docno);

            if(quick_ublock_bytes(&contents_quick_ublock) >= 2)
            {
                QUICK_UBLOCK temp_decode;

                /* each time round, use the grown dynamic buffer as the static buffer */
                csv_quick_ublock_setup(&temp_decode, &array_handle_temp_decode);

                status = csv_load_decode_line(p_docu, quick_ublock_ustr_inline(&contents_quick_ublock),
                                              &slr, labels_across, FALSE,
                                              NULL, &temp_decode, &array_handle_temp_decode);
            }

            csv_quick_ublock_dispose(&contents_quick_ublock, &array_handle_contents);

            status_break(status);

            if(labels_across)
            {
                if(++slr.col >= p_i_slr->col + labels_across)
                {
                    slr.col = p_i_slr->col;
                    slr.row++;
                }
            }
            else
            {
                /* leave slr.row pointing past inserted rows */
                slr.row++;
            }

            if(EOF_READ == load_line_status)
            {
                status = STATUS_OK;
                break;
            }
        }

        /* leave slr.row pointing past all inserted rows */
        if(labels_across)
        {
            if(slr.col != p_i_slr->col)
            {
                slr.row++;
            }
        }
    }

    al_array_dispose(&array_handle_temp_decode);
    al_array_dispose(&array_handle_contents);

    status_return(status);

    if(CSV_LOAD_QUERY_ID_INSERT_AS_TABLE == load_type)
    {
        DOCU_AREA docu_area_table;

        docu_area_init(&docu_area_table);
        docu_area_table.tl.slr = p_position->slr;
        docu_area_table.br.slr = docu_area_table.tl.slr;
        docu_area_table.br.slr.col += insert_size_slr.col;
        docu_area_table.br.slr.row += insert_size_slr.row;

        /* table docu_area needs to be this much bigger */
        docu_area_table.br.slr.col += lhs_extra_cols;
        docu_area_table.br.slr.row += top_extra_rows + bot_extra_rows;

        status_return(foreign_load_file_apply_style_table(p_docu, &docu_area_table, lhs_extra_cols, top_extra_rows, bot_extra_rows));

        /* no widths available from the CSV file, so auto-width is most sensible option */
        status_return(foreign_load_file_auto_width_table(p_docu, &docu_area_table));
    }
    else if(CSV_LOAD_QUERY_ID_LABELS == load_type)
    {
        DOCU_AREA docu_area_labels;

        docu_area_init(&docu_area_labels);
        docu_area_labels.tl.slr = *p_i_slr;
        docu_area_labels.br.slr = docu_area_labels.tl.slr;
        docu_area_labels.br.slr.col += insert_size_slr.col;
        docu_area_labels.br.slr.row += insert_size_slr.row;

        status_return(csv_load_file_apply_style_label(p_docu, &docu_area_labels, labels_across));
    }

    return(status);
}

_Check_return_
static STATUS
csv_msg_insert_foreign_normal(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_MSG_INSERT_FOREIGN p_msg_insert_foreign,
    _InVal_     S32 load_type,
    _In_        COL labels_across,
    _InVal_     BOOL is_sid)
{
    FF_IP_FORMAT ff_ip_format = FF_IP_FORMAT_INIT;
    POSITION position = p_msg_insert_foreign->position;
    SLR first_slr = { 0, 0 }; /* keep dataflower happy */
    STATUS status = STATUS_OK;

    status_return(ensure_memory_froth());

    if(load_type != CSV_LOAD_QUERY_ID_LABELS)
        labels_across = 0;

    ff_ip_format.of_ip_format.flags.insert = p_msg_insert_foreign->insert;

    ff_ip_format.of_ip_format.process_status.flags.foreground = 1;

    status_return(foreign_load_initialise(p_docu, &p_msg_insert_foreign->position, &ff_ip_format, p_msg_insert_foreign->filename, &p_msg_insert_foreign->array_handle));

    status = foreign_load_determine_io_type(&ff_ip_format);

    if(status_ok(status))
    {
        /* temporarily hack this to remove loads of superfluous style processing during load */
        style_region_class_limit_set(p_docu, REGION_BASE);

        /* load the file into memory, size it up */
        status = csv_load_file_core(p_docu, &position, &ff_ip_format, load_type, labels_across, is_sid, &first_slr);

        /* restore style region limit */
        style_region_class_limit_set(p_docu, REGION_END);
    }

    status_accumulate(status, foreign_load_finalise(p_docu, &ff_ip_format));

    if(status_ok(status))
    {   /* reposition in first entry of table */
        p_docu->cur.slr = first_slr;
        p_docu->cur.object_position.object_id = OBJECT_ID_NONE;

        caret_show_claim(p_docu, OBJECT_ID_CELLS, FALSE);
    }

    return(status);
}

_Check_return_
static STATUS
csv_load_file_apply_style_label(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area_table,
    _InVal_     COL labels_across)
{
    STYLE style;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    DOCU_AREA docu_area = *p_docu_area_table;
    PIXIT col_width = p_docu->page_def.size_x / labels_across;

    p_docu->flags.faint_grid = 1;

    style_init(&style);

    style.para_style.margin_para = 0;
    style_bit_set(&style, STYLE_SW_PS_MARGIN_PARA);

    style.para_style.margin_left = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_LEFT);
    style_bit_set(&style, STYLE_SW_PS_MARGIN_LEFT);

    style.para_style.margin_right = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_RIGHT);
    style_bit_set(&style, STYLE_SW_PS_MARGIN_RIGHT);

    style.para_style.h_tab_list = 0;
    style_bit_set(&style, STYLE_SW_PS_TAB_LIST);

    /* middle columns all the same */
    style.col_style.width = (labels_across == 1) ? p_docu->page_def.cells_usable_x : col_width;
    style_bit_set(&style, STYLE_SW_CS_WIDTH);

    style.row_style.unbreakable = 1;
    style_bit_set(&style, STYLE_SW_RS_UNBREAKABLE);

    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);

    status_return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));

    if(labels_across >= 2)
    {
        style_init(&style);

        style_bit_set(&style, STYLE_SW_CS_WIDTH);

        /* left column has left margin taken away */
        style.col_style.width = col_width - p_docu->page_def.margin_left;

        docu_area.tl.slr.col = p_docu_area_table->tl.slr.col;
        docu_area.br.slr.col = docu_area.tl.slr.col + 1;

        status_return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));

        /* don't know (or want to know) grid subtraction calculation here but can derive the rhs grid width from: */
        style.col_style.width = col_width - (p_docu->page_def.size_x - p_docu->page_def.margin_left - p_docu->page_def.cells_usable_x);

        docu_area.tl.slr.col = p_docu_area_table->br.slr.col - 1;
        docu_area.br.slr.col = docu_area.tl.slr.col + 1;

        status_return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
    }

    return(STATUS_OK);
}

static S32
csv_load_query_current = CSV_LOAD_QUERY_ID_INSERT_AS_TABLE;

static const DIALOG_CONTROL
csv_load_query_overwrite_blank_cells =
{
    CSV_LOAD_QUERY_ID_OVERWRITE_BLANK_CELLS, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
csv_load_query_overwrite_blank_cells_data = { { 0 }, CSV_LOAD_QUERY_ID_OVERWRITE_BLANK_CELLS, UI_TEXT_INIT_RESID(CSV_MSG_LOAD_QUERY_OVERWRITE_BLANK_CELLS) };

static const DIALOG_CONTROL
csv_load_query_insert_as_table =
{
    CSV_LOAD_QUERY_ID_INSERT_AS_TABLE, DIALOG_MAIN_GROUP,
    { CSV_LOAD_QUERY_ID_OVERWRITE_BLANK_CELLS, CSV_LOAD_QUERY_ID_OVERWRITE_BLANK_CELLS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
csv_load_query_insert_as_table_data = { { 0 }, CSV_LOAD_QUERY_ID_INSERT_AS_TABLE, UI_TEXT_INIT_RESID(CSV_MSG_LOAD_QUERY_INSERT_AS_TABLE) };

static const DIALOG_CONTROL
csv_load_query_labels =
{
    CSV_LOAD_QUERY_ID_LABELS, DIALOG_MAIN_GROUP,
    { CSV_LOAD_QUERY_ID_INSERT_AS_TABLE, CSV_LOAD_QUERY_ID_INSERT_AS_TABLE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
csv_load_query_labels_data = { { 0 }, CSV_LOAD_QUERY_ID_LABELS, UI_TEXT_INIT_RESID(CSV_MSG_LOAD_QUERY_LABELS) };

#define BUMP_FIELDS_H DIALOG_BUMP_H(3)

static const DIALOG_CONTROL
csv_load_query_across_text =
{
    CSV_LOAD_QUERY_ID_ACROSS_TEXT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, CSV_LOAD_QUERY_ID_ACROSS, DIALOG_CONTROL_SELF, CSV_LOAD_QUERY_ID_ACROSS },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
csv_load_query_across_text_data = { UI_TEXT_INIT_RESID(CSV_MSG_LOAD_QUERY_ACROSS), { 0 /*left_text*/ } };

static const DIALOG_CONTROL
csv_load_query_across =
{
    CSV_LOAD_QUERY_ID_ACROSS, DIALOG_MAIN_GROUP,
    { CSV_LOAD_QUERY_ID_ACROSS_TEXT, CSV_LOAD_QUERY_ID_LABELS },
    { DIALOG_STDSPACING_H, DIALOG_SMALLSPACING_V, BUMP_FIELDS_H, DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_S32), 1 /*tabstop*/ }
};

static const UI_CONTROL_S32
csv_load_query_across_control = { 1, 1000 };

static const DIALOG_CONTROL_DATA_BUMP_S32
csv_load_query_across_data = { { { { FRAMED_BOX_EDIT } }, &csv_load_query_across_control } };

static S32
csv_load_query_across_data_state = 3;

static const DIALOG_CONTROL
csv_load_query_database =
{
    CSV_LOAD_QUERY_ID_DATABASE, DIALOG_MAIN_GROUP,
    { CSV_LOAD_QUERY_ID_LABELS, CSV_LOAD_QUERY_ID_ACROSS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
csv_load_query_database_data = { { 0 }, CSV_LOAD_QUERY_ID_DATABASE, UI_TEXT_INIT_RESID(CSV_MSG_LOAD_QUERY_DATABASE) };

static const DIALOG_CTL_CREATE
csv_load_query_ctl_create[] =
{
    { &dialog_main_group },

    { &defbutton_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data },

    { &csv_load_query_overwrite_blank_cells,    &csv_load_query_overwrite_blank_cells_data },
    { &csv_load_query_insert_as_table,          &csv_load_query_insert_as_table_data },
    { &csv_load_query_labels,                   &csv_load_query_labels_data },
    { &csv_load_query_across_text,              &csv_load_query_across_text_data },
    { &csv_load_query_across,                   &csv_load_query_across_data },

    { &csv_load_query_database, &csv_load_query_database_data } /* optional ... */
};

_Check_return_
static STATUS
dialog_csv_load_query_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    status_return(ui_dlg_set_s32(p_dialog_msg_process_start->h_dialog, CSV_LOAD_QUERY_ID_ACROSS, csv_load_query_across_data_state));
    return(ui_dlg_set_radio_forcing(p_dialog_msg_process_start->h_dialog, DIALOG_MAIN_GROUP, csv_load_query_current));
}

_Check_return_
static STATUS
dialog_csv_load_query_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case DIALOG_MAIN_GROUP:
        {
        const BOOL enabled = (CSV_LOAD_QUERY_ID_LABELS == p_dialog_msg_ctl_state_change->new_state.radiobutton);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CSV_LOAD_QUERY_ID_ACROSS, enabled);
        ui_dlg_ctl_enable(p_dialog_msg_ctl_state_change->h_dialog, CSV_LOAD_QUERY_ID_ACROSS_TEXT, enabled);
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_csv_load_query_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case IDOK:
        {
        csv_load_query_across_data_state = ui_dlg_get_s32(p_dialog_msg_ctl_pushbutton->h_dialog, CSV_LOAD_QUERY_ID_ACROSS); /* remember for next time round as well */
        csv_load_query_current = ui_dlg_get_radio(p_dialog_msg_ctl_pushbutton->h_dialog, DIALOG_MAIN_GROUP); /* remember for next time round as well */

        p_dialog_msg_ctl_pushbutton->completion_code = csv_load_query_current;
        p_dialog_msg_ctl_pushbutton->processed = 1;
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_csv_load_query)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_csv_load_query_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_csv_load_query_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_csv_load_query_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
csv_load_query(
    _DocuRef_   P_DOCU p_docu)
{
    if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
        return(CSV_LOAD_QUERY_ID_DATABASE);

    /* If it's a Letter-derived document, makes little sense to overwrite blank cells */
    if(p_docu->flags.base_single_col)
    {
        if(CSV_LOAD_QUERY_ID_OVERWRITE_BLANK_CELLS == csv_load_query_current)
            csv_load_query_current = CSV_LOAD_QUERY_ID_INSERT_AS_TABLE;
    }
    else
    {
        csv_load_query_current = CSV_LOAD_QUERY_ID_OVERWRITE_BLANK_CELLS;
    }

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, csv_load_query_ctl_create, elemof32(csv_load_query_ctl_create), CSV_MSG_LOAD_QUERY_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = ((CH_COMMA == g_field_sep_ch) || (CH_NULL == g_field_sep_ch)) ? CSV_MSG_LOAD_QUERY_CAPTION : CSV_MSG_LOAD_QUERY_DELIMITED_TEXT_CAPTION;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_csv_load_query;
    if(!has_real_database)
        dialog_cmd_process_dbox.n_ctls -= 1; /* strip end radiobutton off */
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
    } /*block*/
}

T5_MSG_PROTO(static, csv_msg_insert_foreign, _InoutRef_ P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(FILETYPE_TEXT == p_msg_insert_foreign->t5_filetype)
        g_field_sep_ch = (CH_NULL != global_preferences.ascii_load_delimiter) ? (U8) global_preferences.ascii_load_delimiter : CH_TAB;
    else
        g_field_sep_ch = CH_NULL; /* auto-detect - keep this as some users will still stamp TSV as CSV */

    status = csv_load_query(p_docu);

    if(status_ok(status))
    {
        if(CSV_LOAD_QUERY_ID_DATABASE == status)
            status = csv_msg_insert_foreign_as_db(p_docu, p_msg_insert_foreign);
        else
            status = csv_msg_insert_foreign_normal(p_docu, p_msg_insert_foreign, status, (COL) csv_load_query_across_data_state, (p_msg_insert_foreign->t5_filetype == FILETYPE_SID));
    }

    return(status);
}

_Check_return_
static STATUS
csv_msg_csv_read_record_core(
    _InoutRef_ P_CSV_READ_RECORD p_csv_read_record)
{
    P_FF_IP_FORMAT p_ff_ip_format = p_csv_read_record->p_ff_ip_format;
    P_QUICK_UBLOCK p_quick_ublock_temp_contents = &p_csv_read_record->temp_contents_quick_ublock;
    P_ARRAY_HANDLE p_array_handle_temp_contents = &p_csv_read_record->temp_contents_array_handle;
    P_QUICK_UBLOCK p_quick_ublock_temp_decode = &p_csv_read_record->temp_decode_quick_ublock;
    P_ARRAY_HANDLE p_array_handle_temp_decode = &p_csv_read_record->temp_decode_array_handle;
    SLR dummy_slr = { 0, 0 };
    STATUS status;

    if(status_ok(status = csv_load_line(p_ff_ip_format, p_quick_ublock_temp_contents)))
    {
        STATUS load_line_status = status;

        load_reflect_status(&p_ff_ip_format->of_ip_format);

        if(quick_ublock_bytes(p_quick_ublock_temp_contents) >= 2)
        {
            const P_DOCU p_docu = p_docu_from_docno(p_ff_ip_format->of_ip_format.docno);

            if(status_ok(status = csv_load_decode_line(p_docu, quick_ublock_ustr_inline(p_quick_ublock_temp_contents),
                                                       &dummy_slr, 0, FALSE,
                                                       &p_csv_read_record->array_handle /*appended*/, p_quick_ublock_temp_decode, p_array_handle_temp_decode)))
                status = STATUS_DONE;
        }
        else if(EOF_READ != load_line_status)
            status = STATUS_DONE; /* blank record */
        else
            status = STATUS_OK; /* that's all folks */
    }

    csv_quick_ublock_dispose(p_quick_ublock_temp_contents, p_array_handle_temp_contents);

    return(status);
}

T5_MSG_PROTO(static, csv_msg_csv_read_record, _InoutRef_ P_CSV_READ_RECORD p_csv_read_record)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(csv_msg_csv_read_record_core(p_csv_read_record));
}

/******************************************************************************
*
* CSV file converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, csv_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FL_CSV, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FL_CSV));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FL_CSV));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fl_csv);
OBJECT_PROTO(extern, object_fl_csv)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(csv_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(csv_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    case T5_MSG_CSV_READ_RECORD:
        return(csv_msg_csv_read_record(p_docu, t5_message, (P_CSV_READ_RECORD) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fl_csv.c */
