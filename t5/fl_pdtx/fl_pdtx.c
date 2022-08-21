/* fl_pdtx.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* PipeDream text load object module for Fireworkz */

/* MRJC January 1992 */

#include "common/gflags.h"

#include "fl_pdtx/fl_pdtx.h"

#if RISCOS
#define MSG_WEAK &rb_fl_pdtx_msg_weak
extern PC_U8 rb_fl_pdtx_msg_weak;
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_PDTX DONT_LOAD_RESOURCES

/*
PD format codes
*/

enum PD_FORMAT_CODES
{
    PD_FORMAT_START = CR,
    PD_FORMAT_LEFT,
    PD_FORMAT_CENTRE,
    PD_FORMAT_RIGHT,
    PD_FORMAT_BOTH,
    PD_FORMAT_UNDERLINE,
    PD_FORMAT_BOLD,
    PD_FORMAT_ITALIC,
    PD_FORMAT_SUB,
    PD_FORMAT_SUPER,
    PD_FORMAT_SOFT,
    PD_FORMAT_DATE,
    PD_FORMAT_OFF,
    PD_FORMAT_PAGE,
    PD_FORMAT_END
};

#define PDTX_FORMAT_COUNT (PD_FORMAT_END - PD_FORMAT_START - 1)

/*
local structures
*/

typedef struct PDTX_COL
{
    ARRAY_INDEX start;
    ARRAY_INDEX end;
    ARRAY_INDEX pos;
    S32 width;
    S32 wrap_width;
}
PDTX_COL, * P_PDTX_COL;

typedef struct PDTX_FORMAT
{
    S32 pos;
    U8 format;
}
PDTX_FORMAT, * P_PDTX_FORMAT; typedef const PDTX_FORMAT * PC_PDTX_FORMAT;

typedef void (* P_PROC_PDTX_CONSTRUCT) (
    P_U8 p_u8_start,
    P_U8 p_construct,
    _InVal_     S32 len);

#define PROC_PDTX_CONSTRUCT_PROTO(_proc_name) \
static void \
_proc_name( \
    P_U8 p_u8_start, \
    P_U8 p_construct, \
    S32 len)

/*
internal routines
*/

_Check_return_
static ARRAY_INDEX /* <0 == not found */
pdtx_construct_search(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InVal_     ARRAY_INDEX start,
    _InVal_     ARRAY_INDEX end,
    _In_z_      PC_A7STR p_construct_id);

static void
pdtx_file_strip(
    _InRef_     PC_ARRAY_HANDLE p_h_data /*data modified*/,
    _InRef_     PC_ARRAY_HANDLE p_h_cols /*data modified*/);

_Check_return_
static S32
pdtx_para_count(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InRef_     PC_ARRAY_HANDLE p_h_cols /*data modified*/);

_Check_return_
static STATUS
pdtx_para_make(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _OutRef_    P_ARRAY_HANDLE p_h_pd_format,
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InRef_     PC_ARRAY_HANDLE p_h_cols);

_Check_return_
static STATUS
pdtx_regions_make(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_pd_format,
    _InRef_     PC_SLR p_slr);

static void
pdtx_reset_col_positions(
    _InRef_     PC_ARRAY_HANDLE p_h_cols /*data modified*/);

static void
pdtx_text_at_char_determine(
    _InRef_     PC_ARRAY_HANDLE p_h_data);

_Check_return_
static S32
pdtx_slot_construct_strip(
    _InRef_     PC_ARRAY_HANDLE p_h_data /*data modified*/,
    ARRAY_INDEX slot_pos);

PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_both);
PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_centre);
PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_hilite);
PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_left);
PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_percent);
PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_right);

/*
construct table
*/

typedef struct PDTX_CONSTRUCT
{
    U8Z uc_construct_id[7];
    U8 len_id;
    P_PROC_PDTX_CONSTRUCT p_proc_construct;
}
PDTX_CONSTRUCT; typedef const PDTX_CONSTRUCT * PC_PDTX_CONSTRUCT;

static const PDTX_CONSTRUCT
table_constructs[] =                    /* NB longer ones of each leading sequence first and sorted in reverse alphabetical order */
{
    { "V",   1, NULL },                 /* number cell */
    { "TC",  2, NULL },                 /* trailing characters */
    { "R",   1, proc_pdtx_right },      /* right align */
    { "PC",  2, proc_pdtx_percent },    /* percent character */
    { "P",   1, NULL },                 /* page break */
    { "LF",  2, NULL },                 /* linefeed */
    { "LCR", 3, NULL },                 /* LCR align */
    { "LC",  2, NULL },                 /* leading characters */
    { "L",   1, proc_pdtx_left },       /* left align */
    { "JR",  2, proc_pdtx_both },       /* justify right */
    { "JL",  2, proc_pdtx_both },       /* justify left */
    { "H",   1, proc_pdtx_hilite },     /* highlight */
    { "F",   1, NULL },                 /* free align */
    { "DF",  2, NULL },                 /* floating format */
    { "D",   1, NULL },                 /* decimal places */
    { "C",   1, proc_pdtx_centre },     /* centre */
    { "B",   1, NULL }                  /* brackets */
};

static U8 pdtx_text_at_char;

#define PD_DATE_STRING USTR_TEXT("d\\ Mmmm\\ yyyy")

static void
pdtx_blat_with_format_ch(
    _Out_writes_all_(n) P_U8 p_u8,
    _InVal_     U32 n,
    _InVal_     U8 format_ch)
{
    assert(n != 0);

    *p_u8++ = format_ch;

    memset32(p_u8, PD_FORMAT_SOFT, n - 1);
}

/******************************************************************************
*
* search loaded PipeDream file for column constructs
* and note their positions in the array
*
******************************************************************************/

_Check_return_
static STATUS
pdtx_column_found(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InoutRef_  P_ARRAY_HANDLE p_h_cols /*created|extended*/,
    _InoutRef_  P_ARRAY_INDEX p_col_offset,
    _InoutRef_  P_S32 p_col_last)
{
    const ARRAY_INDEX n_bytes = array_elements(p_h_data);
    const PC_U8 p_file_start = array_rangec(p_h_data, U8, 0, n_bytes);
    U8 buffer[16];
    U32 x;
    int i;
    S32 col, len_1, len_2;
    S32 width_col = 0; /* keep dataflower happy */
    S32 width_wrap = 0;
    ARRAY_INDEX col_offset_last_end = *p_col_offset - 4;
    STATUS status = STATUS_OK;

    /* copy construct args to buffer without falling off EOF */
    for(x = 0; x < sizeof32(buffer); ++x)
    {
        if(*p_col_offset + (ARRAY_INDEX) x >= n_bytes)
        {
            buffer[x] = CH_NULL;
            break;
        }
        buffer[x] = PtrGetByteOff(p_file_start, *p_col_offset + (ARRAY_INDEX) x);
    }
    buffer[sizeof32(buffer)-1] = CH_NULL; /* ensure CH_NULL-terminated */

    len_1 = stox(ustr_bptr(buffer), &col);

    i = 0;
    len_2 = 0;

    if(0 != len_1)
        i = sscanf_s(buffer + len_1, "," S32_FMT "," S32_FMT "%n", &width_col, &width_wrap, &len_2);

    if((i == 2) && (PtrGetByteOff(p_file_start, *p_col_offset + len_1 + len_2) == CH_PERCENT_SIGN))
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(PDTX_COL), TRUE);
        P_PDTX_COL p_pd_col;

        if(NULL == (p_pd_col = al_array_extend_by(p_h_cols, PDTX_COL, (col - array_elements(p_h_cols)) + 1, &array_init_block, &status)))
            return(status);

        *p_col_offset += (len_1 + len_2 + 1 /*%*/);

        p_pd_col->start = *p_col_offset;
        /*p_pd_col->end = 0;*/
        p_pd_col->pos = p_pd_col->start;
        p_pd_col->width = width_col;
        p_pd_col->wrap_width = width_wrap ? width_wrap : width_col;

        if(*p_col_last >= 0)
            array_ptr(p_h_cols, PDTX_COL, *p_col_last)->end = col_offset_last_end;

        *p_col_last = col;
    }

    return(status);
}

_Check_return_
static STATUS
pdtx_columns_find(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _OutRef_    P_ARRAY_HANDLE p_h_cols)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX col_offset = 0;
    S32 col_last = -1;

    *p_h_cols = 0;

    while((col_offset = pdtx_construct_search(p_h_data, col_offset, array_elements(p_h_data), "CO:")) > 0)
    {
        status_break(status = pdtx_column_found(p_h_data, p_h_cols, &col_offset, &col_last));
    }

    if(col_last >= 0)
        array_ptr(p_h_cols, PDTX_COL, col_last)->end = array_elements(p_h_data);

    return(status);
}

/******************************************************************************
*
* search PipeDream file for construct
*
******************************************************************************/

_Check_return_
static ARRAY_INDEX /* <0 == not found */
pdtx_construct_search(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InVal_     ARRAY_INDEX start,
    _InVal_     ARRAY_INDEX end,
    _In_z_      PC_A7STR p_uc_construct_id)
{
    const PC_U8 p_u8_start = array_range(p_h_data, U8, 0, (U32) end);
    const PC_U8 p_u8_end = p_u8_start + (U32) end;
    PC_U8 p_u8 = p_u8_start + start;
    const U32 len_construct = strlen32(p_uc_construct_id);
    ARRAY_INDEX construct_offset = -1;

    while(p_u8 < p_u8_end)
    {
        if(CH_PERCENT_SIGN == *p_u8++)
        {
            U32 len = len_construct;
            PC_U8Z p_a7_u8_c = p_uc_construct_id;
            PC_U8 p_a7_u8_f = p_u8;

            do  {
                if(/*"C"*/toupper(*p_a7_u8_f) != /*"C"toupper*/(*p_a7_u8_c)) /* ASCII, no remapping */
                    break;
                ++p_a7_u8_f;
                ++p_a7_u8_c;
            }
            while(0 != --len);

            if(0 == len)
            {
                construct_offset = PtrDiffBytesS32(p_a7_u8_f, p_u8_start);
                break;
            }
        }
    }

    return(construct_offset);
}

_Check_return_
static STATUS
pdtx_para_process(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InRef_     PC_ARRAY_HANDLE p_h_cols,
    _InRef_     PC_POSITION p_position)
{
    STATUS status;
    ARRAY_HANDLE h_pd_format;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_para, 500);
    quick_ublock_with_buffer_setup(quick_ublock_para);

    if(status_ok(status = pdtx_para_make(&quick_ublock_para, &h_pd_format, p_h_data, p_h_cols)))
    {
        if((0 != quick_ublock_bytes(&quick_ublock_para)) && status_ok(status = quick_ublock_nullch_add(&quick_ublock_para)))
        {
            LOAD_CELL_FOREIGN load_cell_foreign;
            zero_struct(load_cell_foreign);
            status_consume(object_data_from_position(p_docu, &load_cell_foreign.object_data, p_position, P_OBJECT_POSITION_NONE));
            load_cell_foreign.original_slr = p_position->slr;

            load_cell_foreign.data_type = OWNFORM_DATA_TYPE_TEXT;
            load_cell_foreign.ustr_inline_contents = quick_ublock_ustr_inline(&quick_ublock_para);
          /*load_cell_foreign.ustr_formula = NULL;*/
            status = object_call_id(OBJECT_ID_TEXT, p_docu, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);

            status_assert(pdtx_regions_make(p_docu, &h_pd_format, &p_position->slr));
        }

        al_array_dispose(&h_pd_format);
    }

    quick_ublock_dispose(&quick_ublock_para);

    return(status);
}

/******************************************************************************
*
* main routine to load a PipeDream file as a text document
*
******************************************************************************/

T5_MSG_PROTO(static, pdtx_msg_insert_foreign, _InoutRef_ P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    P_POSITION p_position = &p_msg_insert_foreign->position;
    STATUS status = STATUS_OK;
    PROCESS_STATUS process_status;
    ARRAY_HANDLE h_data = 0;

    if(!p_docu->flags.base_single_col)
        return(object_call_id_load(p_docu, t5_message, (P_ANY) p_msg_insert_foreign, OBJECT_ID_FL_PDSS));

    if(status_ok(status = file_memory_load(p_docu, &h_data, p_msg_insert_foreign->filename, &process_status, 16)))
    {
        P_ARRAY_HANDLE p_h_data = &h_data;
        ARRAY_HANDLE h_cols;

        pdtx_text_at_char_determine(p_h_data);

        if(status_ok(status = pdtx_columns_find(p_h_data, &h_cols)) && (0 != array_elements(&h_cols)))
        {
            S32 para_count;

            pdtx_file_strip(p_h_data, &h_cols);

            if((para_count = pdtx_para_count(p_h_data, &h_cols)) != 0)
            {
                DOCU_AREA docu_area;
                POSITION position_actual;

                docu_area_init(&docu_area);
                docu_area.br.slr.col = 1;
                docu_area.br.slr.row = para_count;

                if(status_ok(status = cells_docu_area_insert(p_docu, p_position, &docu_area, &position_actual)))
                {
                    S32 row_count = para_count;
                    POSITION position = position_actual;

                    pdtx_reset_col_positions(&h_cols);

                    while(row_count--)
                    {
                        status = pdtx_para_process(p_docu, p_h_data, &h_cols, &position);

                        position.slr.row += 1;

                        process_status.data.percent.current = (100 * (para_count - row_count)) / para_count;
                        process_status_reflect(&process_status);

                        status_break(status);
                    }
                }
            }
        }

        al_array_dispose(&h_cols);
    }

    process_status_end(&process_status);

    al_array_dispose(&h_data);

    return(status);
}

/******************************************************************************
*
* furtle about in PipeDream file and remove junk
*
******************************************************************************/

static void
pdtx_file_strip(
    _InRef_     PC_ARRAY_HANDLE p_h_data /*data modified*/,
    _InRef_     PC_ARRAY_HANDLE p_h_cols /*data modified*/)
{
    const ARRAY_INDEX n_cols = array_elements(p_h_cols);
    ARRAY_INDEX col;

    pdtx_reset_col_positions(p_h_cols);

    for(col = 0; col < n_cols; ++col)
    {
        const P_PDTX_COL p_pd_col = array_ptr(p_h_cols, PDTX_COL, col);

        /* consume in-slot constructs in this column */
        while(p_pd_col->pos < p_pd_col->end)
            p_pd_col->pos += pdtx_slot_construct_strip(p_h_data, p_pd_col->pos);
    }
}

/******************************************************************************
*
* count the paragraphs in a furtled file
*
******************************************************************************/

static void
pdtx_para_break(
    P_U8 p_data,
    P_U8 p_data_end,
    P_U8 p_break_test,
    P_U8 p_format_break,
    P_S32 p_col_count)
{
    U8 stop_test = 0, had_space = 0;

    /* look down column onto next row to determine whether the next row
     * can be considered part of this paragraph; leading blank columns
     * are ignored; if there is more than one column on the next row, then
     * we don't include the next row as part of this paragraph
     */

    while((p_data < p_data_end) && *p_break_test)
    {
        switch(*p_data)
        {
        case CR:
            stop_test = 1;
            break;

        case CH_SPACE:
            had_space = 1;
            ++p_data;
            break;

        case PD_FORMAT_LEFT:
        case PD_FORMAT_CENTRE:
        case PD_FORMAT_RIGHT:
            *p_format_break = 1;
            *p_break_test = 0;
            break;

        case PD_FORMAT_UNDERLINE:
        case PD_FORMAT_BOLD:
        case PD_FORMAT_ITALIC:
        case PD_FORMAT_SUB:
        case PD_FORMAT_SUPER:
        case PD_FORMAT_BOTH:
        case PD_FORMAT_SOFT:
            ++p_data;
            break;

        /* a normal character */
#if CHECKING
        case PD_FORMAT_DATE:
        case PD_FORMAT_PAGE:
#endif
        default:
            if(had_space)
            {
                *p_format_break = 1;
                *p_break_test = 0;
            }
            else
            {
                *p_col_count += 1;
                if(*p_col_count > 1)
                {
                    *p_format_break = 1;
                    *p_break_test = 0;
                }
                else
                {
                    *p_format_break = 0;
                    stop_test = 1;
                }
            }
            break;
        }

        if(stop_test)
            break;
    }
}

_Check_return_
static S32
pdtx_para_count(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InRef_     PC_ARRAY_HANDLE p_h_cols /*data modified*/)
{
    S32 para_count = 0;
    const S32 n_cols = array_elements(p_h_cols);
    S32 n_at_end;
    const U32 n_file_bytes = array_elements32(p_h_data);
    const P_U8 p_u8_start = array_range(p_h_data, U8, 0, n_file_bytes);
    const P_U8 p_u8_end = p_u8_start + n_file_bytes;

    pdtx_reset_col_positions(p_h_cols);

    do  {
        ARRAY_INDEX col;
        U8 format_break = 1, break_test = 1, half_width_line = 0;
        S32 n_cols_on_next_row = 0, n_cols_on_this_row = 0;

        n_at_end = 0;

        for(col = 0; col < n_cols; ++col)
        {
            P_PDTX_COL p_pd_col = array_ptr(p_h_cols, PDTX_COL, col);

            if(p_pd_col->pos >= p_pd_col->end)
            {
                ++n_at_end;
            }
            else
            {
                P_U8 p_data = p_u8_start + p_pd_col->pos;
                U8 hit_cr = 0;
                S32 n_chars = 0;

                while(p_data < p_u8_end && !hit_cr)
                {
                    switch(*p_data)
                    {
                    case CR:
                        hit_cr = 1;
                        ++p_data;
                        if(n_chars && (2 * n_chars < p_pd_col->wrap_width))
                            half_width_line = 1;
                        pdtx_para_break(p_data, p_u8_end, &break_test, &format_break, &n_cols_on_next_row);
                        break;

                    default:
                        if(!n_chars)
                            ++n_cols_on_this_row;
                        ++n_chars;
                        ++p_data;
                        break;
                    }
                }

                p_pd_col->pos = PtrDiffBytesS32(p_data, p_u8_start);
            }
        }

        if(half_width_line && n_cols_on_this_row == 1)
            format_break = 1;

        if(format_break)
            ++para_count;
    }
    while(n_at_end < n_cols);

    return(para_count);
}

/******************************************************************************
*
* bundle the stripped data into data for input to the text object
*
******************************************************************************/

_Check_return_
static STATUS
pdtx_para_make_char_out(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     U8 ch)
{
    switch(ch)
    {
    case PD_FORMAT_DATE:
        return(inline_quick_ublock_from_ustr(p_quick_ublock, IL_DATE, PD_DATE_STRING));

    case PD_FORMAT_PAGE:
        return(inline_quick_ublock_from_ustr(p_quick_ublock, IL_PAGE_Y, USTR_TEXT("#")));

    default:
        return(quick_ublock_ucs4_add(p_quick_ublock, ch)); /* Probably Latin-1 */
    }
}

_Check_return_
static STATUS
pdtx_para_make(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _OutRef_    P_ARRAY_HANDLE p_h_pd_format,
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InRef_     PC_ARRAY_HANDLE p_h_cols)
{
    STATUS status = STATUS_OK;
    const U32 n_file_bytes = array_elements32(p_h_data);
    const P_U8 p_u8_start = array_range(p_h_data, U8, 0, n_file_bytes);
    const P_U8 p_u8_end = p_u8_start + n_file_bytes;
    U8 format_break, had_text_in_para = 0;
    S32 space_n = 0, space_gap = 0;

    *p_h_pd_format = 0;

    do  {
        U8 break_test = 1, had_text_in_line = 0, half_width_line = 0;
        const COL n_cols = array_elements(p_h_cols);
        S32 n_cols_on_next_row = 0, n_cols_on_this_row = 0, tab_count = 0;
        ARRAY_INDEX col;

        format_break = 1;

        for(col = 0; col < n_cols; ++col)
        {
            const P_PDTX_COL p_pd_col = array_ptr(p_h_cols, PDTX_COL, col);

            if(col > 0)
            {
                ++tab_count;
                space_n = 1;
            }

            if(p_pd_col->pos < p_pd_col->end)
            {
                P_U8 p_data = p_u8_start + p_pd_col->pos;
                U8 hit_cr = 0;
                S32 n_chars = 0;

                while(!hit_cr && (p_data < p_u8_end))
                {
                    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(PDTX_FORMAT), TRUE);

                    switch(*p_data)
                    {
                    case CR:
                        {
                        PDTX_FORMAT pd_format;

                        hit_cr = 1;
                        ++p_data;
                        if(n_chars && n_chars < p_pd_col->wrap_width / 2)
                            half_width_line = 1;

                        pdtx_para_break(p_data, p_u8_end, &break_test, &format_break, &n_cols_on_next_row);

                        pd_format.pos = quick_ublock_bytes(p_quick_ublock);
                        pd_format.format = PD_FORMAT_OFF;

                        status = al_array_add(p_h_pd_format, PDTX_FORMAT, 1, &array_init_block, &pd_format);

                        if(had_text_in_para && !format_break)
                            space_gap = 1;

                        break;
                        }

                    case PD_FORMAT_LEFT:
                    case PD_FORMAT_CENTRE:
                    case PD_FORMAT_RIGHT:
                    case PD_FORMAT_BOTH:
                        /* these only count in first column */
                        if(col)
                        {
                            ++p_data;
                            break;
                        }

                        /*FALLTHRU*/

                    case PD_FORMAT_UNDERLINE:
                    case PD_FORMAT_BOLD:
                    case PD_FORMAT_ITALIC:
                    case PD_FORMAT_SUB:
                    case PD_FORMAT_SUPER:
                        {
                        PDTX_FORMAT pd_format;

                        pd_format.pos = quick_ublock_bytes(p_quick_ublock);
                        pd_format.format = *p_data++;

                        status = al_array_add(p_h_pd_format, PDTX_FORMAT, 1, &array_init_block, &pd_format);

                        break;
                        }

                    case PD_FORMAT_SOFT:
                        space_gap = 1;
                        ++p_data;
                        break;

                    case CH_SPACE:
                        if(had_text_in_line)
                            ++space_n;
                        ++p_data;
                        break;

#if CHECKING
                    case PD_FORMAT_DATE:
                    case PD_FORMAT_PAGE:
#endif
                    default:
                        if(!n_chars)
                            ++n_cols_on_this_row;

                        if(space_gap && !space_n)
                            space_n = 1;

                        if(tab_count && had_text_in_line)
                        {
                            while(tab_count--)
                                status_break(status = inline_quick_ublock_IL_TAB(p_quick_ublock));

                            status_break(status);
                        }
                        else if(had_text_in_para)
                        {
                            while(space_n--)
                            {
                                status_break(status = pdtx_para_make_char_out(p_quick_ublock, CH_SPACE));
                                ++n_chars;
                            }

                            status_break(status);
                        }

                        status_break(status = pdtx_para_make_char_out(p_quick_ublock, *p_data++));
                        ++n_chars;
                        had_text_in_line = had_text_in_para = 1;
                        space_n = space_gap = 0;
                        tab_count = 0;
                        break;
                    }
                }

                p_pd_col->pos = PtrDiffBytesS32(p_data, p_u8_start);
            }

            status_break(status);
        }

        if(half_width_line && n_cols_on_this_row == 1)
            format_break = 1;
    }
    while(!format_break);

    return(status);
}

/******************************************************************************
*
* make up regions from the hilite list; this
* is done in an elementary fashion, relying on
* the region adding and subsuming to do any
* sensible minimising of regions
*
******************************************************************************/

_Check_return_
static STATUS
pdtx_regions_add(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_PDTX_FORMAT p_pd_format_on,
    _InRef_     PC_PDTX_FORMAT p_pd_format_off,
    _InRef_     PC_SLR p_slr)
{
    DOCU_AREA docu_area;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE style;

    docu_area_init(&docu_area);
    docu_area.tl.slr = *p_slr;
    docu_area.br.slr.col = p_slr->col + 1;
    docu_area.br.slr.row = p_slr->row + 1;

    if(p_pd_format_on->pos >= 0)
    {
        docu_area.tl.object_position.object_id = OBJECT_ID_TEXT;
        docu_area.tl.object_position.data = p_pd_format_on->pos;
    }
    else
        object_position_init(&docu_area.tl.object_position);

    if(p_pd_format_off->pos >= 0)
    {
        docu_area.br.object_position.object_id = OBJECT_ID_TEXT;
        docu_area.br.object_position.data = p_pd_format_off->pos;
    }
    else
        object_position_init(&docu_area.br.object_position);

    style_init(&style);

    switch(p_pd_format_on->format)
    {
    case PD_FORMAT_LEFT:
        style_bit_set(&style, STYLE_SW_PS_JUSTIFY);
        style.para_style.justify = SF_JUSTIFY_LEFT;
        break;

    case PD_FORMAT_CENTRE:
        style_bit_set(&style, STYLE_SW_PS_JUSTIFY);
        style.para_style.justify = SF_JUSTIFY_CENTRE;
        break;

    case PD_FORMAT_RIGHT:
        style_bit_set(&style, STYLE_SW_PS_JUSTIFY);
        style.para_style.justify = SF_JUSTIFY_RIGHT;
        break;

    case PD_FORMAT_BOTH:
        style_bit_set(&style, STYLE_SW_PS_JUSTIFY);
        style.para_style.justify = SF_JUSTIFY_BOTH;
        break;

    case PD_FORMAT_UNDERLINE:
        style_bit_set(&style, STYLE_SW_FS_UNDERLINE);
        style.font_spec.underline = 1;
        break;

    case PD_FORMAT_BOLD:
        style_bit_set(&style, STYLE_SW_FS_BOLD);
        style.font_spec.bold = 1;
        break;

    case PD_FORMAT_ITALIC:
        style_bit_set(&style, STYLE_SW_FS_ITALIC);
        style.font_spec.italic = 1;
        break;

    case PD_FORMAT_SUB:
        style_bit_set(&style, STYLE_SW_FS_SUBSCRIPT);
        style.font_spec.subscript = 1;
        break;

    case PD_FORMAT_SUPER:
        style_bit_set(&style, STYLE_SW_FS_SUPERSCRIPT);
        style.font_spec.superscript = 1;
        break;

    default: default_unhandled(); break;
    }

    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
    return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
}

_Check_return_
static STATUS
pdtx_regions_off(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_PDTX_FORMAT p_pd_format_state /*[PDTX_FORMAT_COUNT]*/,
    _InVal_     S32 pd_format_off_pos,
    _InRef_     PC_SLR p_slr)
{
    STATUS status = STATUS_OK;
    PDTX_FORMAT pd_format_off;
    S32 x;

    pd_format_off.pos = pd_format_off_pos;

    for(x = 0; x < PDTX_FORMAT_COUNT; ++x)
    {
        if(p_pd_format_state[x].format)
        {
            pd_format_off.format = p_pd_format_state[x].format;
            status_break(status = pdtx_regions_add(p_docu, &p_pd_format_state[x], &pd_format_off, p_slr));
            p_pd_format_state[x].format = 0;
        }
    }

    return(status);
}

_Check_return_
static STATUS
pdtx_regions_make(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_pd_format,
    _InRef_     PC_SLR p_slr)
{
    const ARRAY_INDEX n_elements = array_elements(p_h_pd_format);
    STATUS status = STATUS_OK;

    if(0 != n_elements)
    {
        ARRAY_INDEX i;
        P_PDTX_FORMAT p_pd_format = array_range(p_h_pd_format, PDTX_FORMAT, 0, n_elements);
        PDTX_FORMAT pd_format_state[PDTX_FORMAT_COUNT];

        zero_array(pd_format_state);

        for(i = 0; i < n_elements; ++i, ++p_pd_format)
        {
            if(p_pd_format->format == PD_FORMAT_OFF)
            {
                status_break(status = pdtx_regions_off(p_docu, pd_format_state, p_pd_format->pos, p_slr));
            }
            else
            {
                S32 x;
                U8 done = 0;

                /* if the effect is switched on, switch it off */
                for(x = 0; x < PDTX_FORMAT_COUNT; ++x)
                {
                    if(pd_format_state[x].format == p_pd_format->format)
                    {
                        status_break(status = pdtx_regions_add(p_docu, &pd_format_state[x], p_pd_format, p_slr));
                        pd_format_state[x].format = 0;
                        done = 1;
                        break;
                    }
                }

                /* must be switching on effect */
                if(!done)
                    for(x = 0; x < PDTX_FORMAT_COUNT; ++x)
                    {
                        if(!pd_format_state[x].format)
                        {
                            pd_format_state[x] = *p_pd_format;
                            break;
                        }
                    }
            }
        }

        status = pdtx_regions_off(p_docu, pd_format_state, -1, p_slr);
    }

    return(status);
}

/******************************************************************************
*
* reset column positions to the start
*
******************************************************************************/

static void
pdtx_reset_col_positions(
    _InRef_     PC_ARRAY_HANDLE p_h_cols /*data modified*/)
{
    const COL n_cols = array_elements(p_h_cols);
    COL col;

    for(col = 0; col < n_cols; ++col)
    {
        P_PDTX_COL p_pd_col = array_ptr(p_h_cols, PDTX_COL, col);

        p_pd_col->pos = p_pd_col->start;
    }
}

/******************************************************************************
*
* strip in-slot constructs
*
******************************************************************************/

_Check_return_
static S32
pdtx_slot_construct_strip(
    _InRef_     PC_ARRAY_HANDLE p_h_data /*data modified*/,
    ARRAY_INDEX slot_pos)
{
    const U32 n_file_bytes = array_elements32(p_h_data);
    const P_U8 p_file_start = array_range(p_h_data, U8, 0, n_file_bytes);
    const P_U8 p_file_end = p_file_start + n_file_bytes;
    P_U8 p_u8_slot = p_file_start + slot_pos;
    P_U8 p_u8 = p_u8_slot;
    P_U8 p_u8_construct = NULL;

    while((p_u8 < p_file_end) && (*p_u8 != LF) && (*p_u8 != CR))
    {
        if(CH_PERCENT_SIGN == *p_u8)
        {
            if(NULL == p_u8_construct)
            {   /* mark start of construct */
                p_u8_construct = p_u8;
            }
            else
            {
                S32 len = PtrDiffBytesS32(p_u8, p_u8_construct) + 1;

                if(len < 25)
                {
                    PC_U8 p_u8_construct_letters = p_u8_construct + 1;
                    BOOL done = FALSE;
                    U32 i;

                    for(i = 0; i < elemof32(table_constructs); i++)
                    {
                        PC_PDTX_CONSTRUCT p_pd_construct = &table_constructs[i];
                        U8 uc_first_letter = (U8) /*"C"*/toupper(*p_u8_construct_letters);

                        if(uc_first_letter != p_pd_construct->uc_construct_id[0])
                        {
                            if(uc_first_letter < p_pd_construct->uc_construct_id[0])
                                break;

                            continue;
                        }

                        /* first letter matched - many are just one character */
                        if(p_pd_construct->len_id > 1)
                            if(0 != C_strnicmp(p_u8_construct_letters + 1, p_pd_construct->uc_construct_id, p_pd_construct->len_id - 1))
                                continue;

                        if(p_pd_construct->p_proc_construct)
                        {
                            (*p_pd_construct->p_proc_construct) (p_u8_slot, p_u8_construct, len);
                            done = TRUE;
                        }

                        break;
                    }

                    /* if we haven't already processed this, erase it */
                    if(!done)
                        pdtx_blat_with_format_ch(p_u8_construct, len, PD_FORMAT_SOFT);
                }

                p_u8_construct = NULL;
            }
        }
        else if(pdtx_text_at_char == *p_u8)
        {
            P_U8 p_u8_t = p_u8 + 1;

            if(p_u8_t < p_file_end)
            {
                const U8 first_char = (U8) /*"C"*/toupper(p_u8_t[0]); /* ASCII, no remapping */

                if(pdtx_text_at_char == first_char)
                {
                    /* multiple text-at chars: skip to the end and reduce the length of this sequence by 1 */
                    while(++p_u8_t < p_file_end)
                        if(pdtx_text_at_char != *p_u8_t)
                            break;

                    p_u8 = p_u8_t - 1; /* retract as pointer gets incremented */

                    *p_u8 = PD_FORMAT_SOFT; /* and lose one text-at char anyway */
                }
                else if(NULL != strchr("CDFGLNPT", first_char))
                {
                    U8 seen_trailing_at = 0, stop_it = 0;

                    ++p_u8_t; /* past the first char */

                    /* further validation will help import */
                    if(p_u8_t < p_file_end)
                    {
                        switch(first_char)
                        {
                            default:
#if CHECKING
                            case 'D':
                            case 'L':
                            case 'N':
                            case 'P':
                            case 'T':
#endif
                                if(pdtx_text_at_char != p_u8_t[0])
                                    stop_it = 1;
                                break;

                            case 'C':
                            case 'F':
                            case 'G':
                                if((pdtx_text_at_char != p_u8_t[0]) && (CH_COLON != p_u8_t[0]))
                                    stop_it = 1;
                                break;
                        }
                    }

                    /* find extent of text-at field */
                    while((p_u8_t < p_file_end) && !stop_it)
                    {
                        switch(p_u8_t[0])
                        {
                        case LF:
                        case CR:
                            stop_it = 1;
                            break;

                        default:
                            if(pdtx_text_at_char == p_u8_t[0])
                            {
                                seen_trailing_at = 1;
                                ++p_u8_t;
                                break;
                            }

                            if(seen_trailing_at)
                                stop_it = 1;
                            else
                                ++p_u8_t;
                            break;
                        }
                    }

                    if(seen_trailing_at)
                    {
                        P_U8 p_u8_e = p_u8_t;

                        p_u8_t = p_u8;

                        if(first_char == 'D')
                            *p_u8_t++ = PD_FORMAT_DATE;
                        else if(first_char == 'P')
                            *p_u8_t++ = PD_FORMAT_PAGE;
                        /* CFGLNT are ignored */

                        pdtx_blat_with_format_ch(p_u8_t, PtrDiffBytesU32(p_u8_e, p_u8_t), PD_FORMAT_SOFT);

                        p_u8 = p_u8_e - 1; /* retract as pointer gets incremented */
                    }
                    else
                        p_u8 = p_u8_t - 1; /* retract as pointer gets incremented */
                }
            }
        }

        ++p_u8;
    }

    /* standardise separators */
    if(p_u8 < p_file_end)
    {
        if((p_u8 + 1) < p_file_end)
        {
            if(((*p_u8 == LF) && (p_u8[1] == CR))
               ||
               ((*p_u8 == CR) && (p_u8[1] == LF)) )
            {
                *p_u8++ = PD_FORMAT_SOFT;
            }
        }

        *p_u8++ = CR;
    }

    return(PtrDiffBytesS32(p_u8, p_u8_slot));
}

static void
pdtx_text_at_char_determine(
    _InRef_     PC_ARRAY_HANDLE p_h_data)
{
    ARRAY_INDEX construct_offset = 0;

    pdtx_text_at_char = CH_COMMERCIAL_AT;

    if((construct_offset = pdtx_construct_search(p_h_data, construct_offset, array_elements(p_h_data), "OP%TA")) > 0)
    {
        PC_U8Z p_u8 = array_ptr(p_h_data, U8, construct_offset);

        switch(*p_u8)
        {
        case LF:
        case CR:
            break;

        default:
            pdtx_text_at_char = *p_u8;
            break;
        }
    }
}

/*
construct handlers
*/

PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_both)
{
    pdtx_blat_with_format_ch(p_construct, len, (U8) ((p_construct == p_u8_start) ? PD_FORMAT_BOTH : PD_FORMAT_SOFT));
}

PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_centre)
{
    pdtx_blat_with_format_ch(p_construct, len, (U8) ((p_construct == p_u8_start) ? PD_FORMAT_CENTRE : PD_FORMAT_SOFT));
}

PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_hilite)
{
    S32 hilite = fast_strtol(p_construct + 2, NULL);
    U8 format_ch;

    UNREFERENCED_PARAMETER(p_u8_start);

    switch(hilite)
    {
    default:format_ch = PD_FORMAT_SOFT; break;
    case 1: format_ch = PD_FORMAT_UNDERLINE; break;
    case 2: format_ch = PD_FORMAT_BOLD; break;
    case 4: format_ch = PD_FORMAT_ITALIC; break;
    case 5: format_ch = PD_FORMAT_SUB; break;
    case 6: format_ch = PD_FORMAT_SUPER; break;
    }

    pdtx_blat_with_format_ch(p_construct, len, format_ch);
}

PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_left)
{
    pdtx_blat_with_format_ch(p_construct, len, (U8) ((p_construct == p_u8_start) ? PD_FORMAT_LEFT : PD_FORMAT_SOFT));
}

PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_percent)
{
    UNREFERENCED_PARAMETER(p_u8_start);

    pdtx_blat_with_format_ch(p_construct, len, CH_PERCENT_SIGN);
}

PROC_PDTX_CONSTRUCT_PROTO(proc_pdtx_right)
{
    pdtx_blat_with_format_ch(p_construct, len, (U8) ((p_construct == p_u8_start) ? PD_FORMAT_RIGHT : PD_FORMAT_SOFT));
}

/******************************************************************************
*
* PipeDream -> Text converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, pdtx_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FL_PDTX, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_PDTX));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FL_PDTX));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fl_pdtx);
OBJECT_PROTO(extern, object_fl_pdtx)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(pdtx_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(pdtx_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fl_pdtx.c */
