/* sk_styl.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Fireworkz style manager */

/* MRJC January 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
entry points
*/

PROC_UREF_EVENT_PROTO(static, proc_uref_event_style);

/*
internal routines
*/

static void
style_docu_area_expand(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE_DOCU_AREA p_style_docu_area);

_Check_return_
static BOOL
style_docu_area_subsume(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle /*data modified*/,
    P_STYLE_DOCU_AREA p_style_docu_area);

/*
bitmap constants (const-to-you)
*/

STYLE_SELECTOR style_selector_all;
STYLE_SELECTOR style_selector_font_spec;
STYLE_SELECTOR style_selector_para_leading;
STYLE_SELECTOR style_selector_para_text;

STYLE_SELECTOR style_selector_para_ruler;
STYLE_SELECTOR style_selector_para_format;
STYLE_SELECTOR style_selector_para_border;
STYLE_SELECTOR style_selector_para_grid;

STYLE_SELECTOR style_selector_col;
STYLE_SELECTOR style_selector_row;
STYLE_SELECTOR style_selector_numform;

static STYLE_SELECTOR style_selector_para;
static STYLE_SELECTOR style_selector_extent_x;

#define STYLE_LIST_DEP 0

#define SLR_CACHE_SIZE_MIN 32 /* SKS 03feb97 was 10 but make a power of two (and larger) */
#define SLR_CACHE_FACTOR   2  /* cache grows to array_elements(region_list) / CACHE_FACTOR_SLR */

#define CACHE_HIT  -1
#define CACHE_MISS -2

static S32 next_client_handle = STYLE_LIST_DEP + 1;

_Check_return_
extern STATUS
style_apply_struct_to_source(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE p_style)
{
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    const T5_MESSAGE t5_message = T5_CMD_STYLE_APPLY_SOURCE;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);

        if(status_ok(status = style_ustr_inline_from_struct(p_docu, &quick_ublock, p_style)))
        {
            p_args[0].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);
            status = execute_command(object_id, p_docu, t5_message, &arglist_handle);
            quick_ublock_dispose(&quick_ublock);
        }

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

/******************************************************************************
*
* return a default sort of measurement for the given style attribute
* in this document (accounting for ruler settings etc)
*
******************************************************************************/

#define COL_DEFAULT_WIDTH        ((PIXIT) (FP_PIXITS_PER_MM * 26.0))
#define COL_DEFAULT_MARGIN_LEFT  ((PIXIT) (FP_PIXITS_PER_MM *  2.0))
#define COL_DEFAULT_MARGIN_RIGHT ((PIXIT) (FP_PIXITS_PER_MM *  2.0))

#define DEFAULT_PARA_START       ((PIXIT) (FP_PIXITS_PER_MM *  1.2))
#define DEFAULT_PARA_END         ((PIXIT) (FP_PIXITS_PER_MM *  0.8))

_Check_return_
extern PIXIT
style_default_measurement(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     STYLE_BIT_NUMBER style_bit_number)
{
    SNAP_TO_CLICK_STOP_MODE snap_mode = SNAP_TO_CLICK_STOP_ROUND;
    PIXIT value;
    BOOL horizontal = TRUE;

    switch(style_bit_number)
    {
    case STYLE_SW_CS_WIDTH:
        value = COL_DEFAULT_WIDTH;
        break;

    case STYLE_SW_PS_MARGIN_LEFT:
        value = COL_DEFAULT_MARGIN_LEFT;
        break;

    case STYLE_SW_PS_MARGIN_PARA:
        return(0);

    case STYLE_SW_PS_MARGIN_RIGHT:
        value = COL_DEFAULT_MARGIN_RIGHT;
        break;

    case STYLE_SW_PS_PARA_START:
        return(DEFAULT_PARA_START);

    case STYLE_SW_PS_PARA_END:
        return(DEFAULT_PARA_END);

    default: default_unhandled(); return(0);
    }

    value = skel_ruler_snap_to_click_stop(p_docu, horizontal, value, snap_mode);

    return(value);
}

/******************************************************************************
*
* find a given client handle in the style region list
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_STYLE_DOCU_AREA
p_style_docu_area_from_client_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     CLIENT_HANDLE client_handle)
{
    P_STYLE_DOCU_AREA p_style_docu_area;
    ARRAY_INDEX n_regions = array_elements(&p_docu->h_style_docu_area);
    ARRAY_INDEX style_docu_area_ix;

    profile_ensure_frame();

    for(style_docu_area_ix = 0, p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, style_docu_area_ix);
        style_docu_area_ix < n_regions;
        ++style_docu_area_ix, ++p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(p_style_docu_area->client_handle == client_handle)
            return(p_style_docu_area);
    }

    return(NULL);
}

/******************************************************************************
*
* check for an entry existing in the style cache
*
******************************************************************************/

static S32 /* miss/hit/entry */
style_cache_slr_check(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_SLR p_slr)
{
    ARRAY_INDEX n_elements = array_elements(&p_docu->h_style_cache_slr);
    ARRAY_INDEX i;
    P_STYLE_CACHE_SLR_ENTRY p_style_cache_slr_entry;

    for(i = 0, p_style_cache_slr_entry = array_ptr(&p_docu->h_style_cache_slr, STYLE_CACHE_SLR_ENTRY, i);
        i < n_elements;
        i += 1, p_style_cache_slr_entry += 1)
    {
        STYLE_SELECTOR temp;

        if(!p_style_cache_slr_entry->used)
            continue;

        if(slr_equal(&p_style_cache_slr_entry->slr, p_slr))
        {
            if(!style_selector_bic(&temp, p_style_selector, &p_style_cache_slr_entry->style.selector))
            {
                style_copy(p_style, &p_style_cache_slr_entry->style, p_style_selector);
                return(CACHE_HIT);
            }
            return(i);
        }
    }

    return(CACHE_MISS);
}

/******************************************************************************
*
* dispose of info in the style cache
*
******************************************************************************/

static void
style_cache_slr_dispose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_opt_ PC_REGION p_region)
{
    ARRAY_INDEX n_elements = array_elements(&p_docu->h_style_cache_slr);
    ARRAY_INDEX i;
    P_STYLE_CACHE_SLR_ENTRY p_style_cache_slr_entry;

    for(i = 0, p_style_cache_slr_entry = array_ptr(&p_docu->h_style_cache_slr, STYLE_CACHE_SLR_ENTRY, i);
        i < n_elements;
        i += 1, p_style_cache_slr_entry += 1)
    {
        if(!p_style_cache_slr_entry->used)
            continue;

        if(!p_region || slr_in_region(p_region, &p_style_cache_slr_entry->slr))
        {
            style_dispose(&p_style_cache_slr_entry->style);
            p_style_cache_slr_entry->used = 0;
        }
    }
}

/******************************************************************************
*
* insert an entry into the style cache
*
******************************************************************************/

static void
style_cache_slr_insert(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_SLR p_slr,
    _InVal_     S32 entry)
{
    P_STYLE_CACHE_SLR_ENTRY p_style_cache_slr_entry;

    if(entry == CACHE_MISS)
    {
        ARRAY_INDEX i;
        S32 cache_size_slr = array_elements(&p_docu->h_style_docu_area) / SLR_CACHE_FACTOR;

        cache_size_slr = MAX(cache_size_slr, SLR_CACHE_SIZE_MIN);

        if(cache_size_slr > array_elements(&p_docu->h_style_cache_slr))
        {
            STATUS status;
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(STYLE_CACHE_SLR_ENTRY), TRUE);

            consume_ptr(al_array_extend_by(&p_docu->h_style_cache_slr, STYLE_CACHE_SLR_ENTRY, cache_size_slr - array_elements(&p_docu->h_style_cache_slr), &array_init_block, &status));

            status_assert(status);

            trace_1(TRACE_APP_MEMORY_USE,
                    TEXT("style_cache_slr_size is ") U32_TFMT TEXT(" bytes"),
                    array_size32(&p_docu->h_style_cache_slr) * sizeof32(STYLE_CACHE_SLR_ENTRY));
        }

        if(0 == array_elements(&p_docu->h_style_cache_slr))
            /* SKS 03feb97 so what if we fail to insert an entry? better to ignore it than to divide by zero... */
            return;

        i = rand() % array_elements(&p_docu->h_style_cache_slr);
        p_style_cache_slr_entry = array_ptr(&p_docu->h_style_cache_slr, STYLE_CACHE_SLR_ENTRY, i);

        if(p_style_cache_slr_entry->used)
        {
            style_dispose(&p_style_cache_slr_entry->style);
            p_style_cache_slr_entry->used = 0;
        }

        style_init(&p_style_cache_slr_entry->style);
        p_style_cache_slr_entry->slr = *p_slr;
        p_style_cache_slr_entry->used = 1;
    }
    else
        p_style_cache_slr_entry = array_ptr(&p_docu->h_style_cache_slr, STYLE_CACHE_SLR_ENTRY, entry);

    style_copy(&p_style_cache_slr_entry->style, p_style, p_style_selector);

    trace_3(TRACE_APP_STYLE, TEXT("style_cache_slr_insert: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(" (") S32_TFMT TEXT(" elements)"),
            p_style_cache_slr_entry->slr.col, p_style_cache_slr_entry->slr.row, array_elements(&p_docu->h_style_cache_slr));
}

/*
sub style cache entry
*/

typedef struct STYLE_CACHE_SUB_ENTRY
{
    DATA_REF data_ref;
    ARRAY_HANDLE h_sub_changes;
    STYLE_SELECTOR selector;
    U8 used;
    U8 _spares[3];
}
STYLE_CACHE_SUB_ENTRY, * P_STYLE_CACHE_SUB_ENTRY;

/******************************************************************************
*
* check for an entry existing in the style cache
*
******************************************************************************/

static ARRAY_HANDLE
style_cache_sub_check(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    ARRAY_INDEX n_elements = array_elements(&p_docu->h_style_cache_sub);
    ARRAY_INDEX i;
    P_STYLE_CACHE_SUB_ENTRY p_style_cache_sub_entry;

    for(i = 0, p_style_cache_sub_entry = array_ptr(&p_docu->h_style_cache_sub, STYLE_CACHE_SUB_ENTRY, i);
        i < n_elements;
        i += 1, p_style_cache_sub_entry += 1)
    {
        STYLE_SELECTOR temp;

        if(!p_style_cache_sub_entry->used)
            continue;

        if(!data_refs_equal(&p_style_cache_sub_entry->data_ref, p_data_ref))
            continue;

        if(0 != style_selector_bic(&temp, p_style_selector, &p_style_cache_sub_entry->selector))
            continue;

        /* matched all the required bits */
        trace_2(TRACE_APP_STYLE, TEXT("style_cache_sub_check found: ") COL_TFMT TEXT(",") ROW_TFMT, p_style_cache_sub_entry->data_ref.arg.slr.col, p_style_cache_sub_entry->data_ref.arg.slr.row);
        return(p_style_cache_sub_entry->h_sub_changes);
    }

    return(0);
}

/******************************************************************************
*
* dispose of info in the style cache
*
******************************************************************************/

static void
style_cache_sub_dispose(
    _DocuRef_   P_DOCU p_docu,
    _In_        DATA_SPACE data_space,
    _InRef_opt_ PC_REGION p_region)
{
    ARRAY_INDEX n_elements = array_elements(&p_docu->h_style_cache_sub);
    ARRAY_INDEX i;
    P_STYLE_CACHE_SUB_ENTRY p_style_cache_sub_entry;

    for(i = 0, p_style_cache_sub_entry = array_ptr(&p_docu->h_style_cache_sub, STYLE_CACHE_SUB_ENTRY, i);
        i < n_elements;
        i += 1, p_style_cache_sub_entry += 1)
    {
        if(!p_style_cache_sub_entry->used)
            continue;

        if(!p_region || data_ref_in_region(p_docu, data_space, p_region, &p_style_cache_sub_entry->data_ref))
        {
            al_array_dispose(&p_style_cache_sub_entry->h_sub_changes);
            p_style_cache_sub_entry->used = 0;
            trace_2(TRACE_APP_STYLE, TEXT("style_cache_sub_dispose: ") COL_TFMT TEXT(",") ROW_TFMT, p_style_cache_sub_entry->data_ref.arg.slr.col, p_style_cache_sub_entry->data_ref.arg.slr.row);
        }
    }
}

/******************************************************************************
*
* insert an entry into the style cache
*
******************************************************************************/

static void
style_cache_sub_insert(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_sub_changes,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    P_STYLE_CACHE_SUB_ENTRY p_style_cache_sub_entry;
    S32 cache_size_sub = array_elements(&p_docu->h_style_docu_area);

    cache_size_sub = cache_size_sub >> 2;

    if(cache_size_sub < 4)
        cache_size_sub = 4;

    if(cache_size_sub > array_elements(&p_docu->h_style_cache_sub))
    {   /* better allocate another entry */
        STATUS status;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(STYLE_CACHE_SUB_ENTRY), TRUE);

        if(NULL == (p_style_cache_sub_entry = al_array_extend_by(&p_docu->h_style_cache_sub, STYLE_CACHE_SUB_ENTRY, 1, &array_init_block, &status)))
        {
            status_assert(status);
            al_array_dispose(p_h_sub_changes); /* best lose this then ... poor client will get a big shock! */
            return;
        }

        trace_1(TRACE_APP_MEMORY_USE,
                TEXT("style_cache_sub_size is ") U32_TFMT TEXT(" bytes"),
                array_size32(&p_docu->h_style_cache_sub) * sizeof32(STYLE_CACHE_SUB_ENTRY));
    }
    else
    { /* usual random replacement */
        ARRAY_INDEX i = rand() % array_elements(&p_docu->h_style_cache_sub);

        p_style_cache_sub_entry = array_ptr(&p_docu->h_style_cache_sub, STYLE_CACHE_SUB_ENTRY, i);

        if(p_style_cache_sub_entry->used)
        {
            p_style_cache_sub_entry->used = 0;

            al_array_dispose(&p_style_cache_sub_entry->h_sub_changes);
        }
    }

    p_style_cache_sub_entry->data_ref = *p_data_ref;
    p_style_cache_sub_entry->h_sub_changes = *p_h_sub_changes;
    p_style_cache_sub_entry->used = 1;
    style_selector_copy(&p_style_cache_sub_entry->selector, p_style_selector);

    trace_2(TRACE_APP_STYLE, TEXT("style_cache_sub_insert: ") COL_TFMT TEXT(",") ROW_TFMT, p_style_cache_sub_entry->data_ref.arg.slr.col, p_style_cache_sub_entry->data_ref.arg.slr.row);
}

/******************************************************************************
*
* produce an array of col numbers which
* represent cols containing a change of the
* selected style in the given row
*
* --out--
* the array handle is owned by the caller and
* must be freed after use
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, proc_col_sort, COL)
{
    QSORT_ARG1_VAR_DECL(PC_COL, p_col_1);
    QSORT_ARG2_VAR_DECL(PC_COL, p_col_2);

    const COL col_1 = *p_col_1;
    const COL col_2 = *p_col_2;

    profile_ensure_frame();

    if(col_1 > col_2)
        return(1);

    if(col_1 < col_2)
        return(-1);

    return(0);
}

_Check_return_
static STATUS
col_add_to_list(
    _InoutRef_  P_ARRAY_HANDLE p_h_col_table,
    _InVal_     COL col_max,
    _InVal_     COL col_1,
    _InVal_     COL col_2)
{
    STATUS status;
    P_COL p_col;

    if(NULL == (p_col = al_array_extend_by(p_h_col_table, COL, 2, PC_ARRAY_INIT_BLOCK_NONE, &status)))
    {
        al_array_dispose(p_h_col_table);
        return(status);
    }

    *p_col++ = MIN(col_1, col_max);
    *p_col++ = MIN(col_2, col_max);

    return(STATUS_OK);
}

_Check_return_
extern ARRAY_HANDLE
style_change_between_cols(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW row,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    ARRAY_HANDLE h_col_table;
    ARRAY_INDEX style_docu_area_ix;
    P_STYLE_DOCU_AREA p_style_docu_area;
    P_COL p_col;
    REGION region;
    COL col_max = n_cols_logical(p_docu);

    if(col_max > 0)
        col_max--;
    else
        col_max = 0;

    { /* initialise array with col_start */
    STATUS status;
    SC_ARRAY_INIT_BLOCK col_init_block = aib_init(5, sizeof32(COL), FALSE);
    if(NULL != (p_col = al_array_alloc(&h_col_table, COL, 1, &col_init_block, &status)))
        *p_col = col_s;
    } /*block*/

    region.tl.col = col_s;
    region.tl.row = row;
    region.br.col = col_e;
    region.br.row = row + 1;
    region.whole_col = region.whole_row = 0;

    /* loop over style areas */
    for(style_docu_area_ix = 0, p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, style_docu_area_ix);
        h_col_table && style_docu_area_ix < array_elements(&p_docu->h_style_docu_area);
        ++style_docu_area_ix, ++p_style_docu_area)
    {
        BOOL implied;

        if(p_style_docu_area->deleted)
            continue;

        implied = (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id);

        if(region_intersect_docu_area(&p_style_docu_area->docu_area, &region)
           &&
           (!p_style_docu_area->docu_area.whole_row || implied))
        {
            STYLE_SELECTOR temp;

            /* are there any bits here we need ? */
            if(style_docu_area_selector_and(p_docu, &temp, p_style_docu_area, p_style_selector))
            {
                if(!implied || !style_selector_bic(&temp, &temp, &style_selector_row))
                {
                    status_break(col_add_to_list(&h_col_table,
                                                 col_max,
                                                 p_style_docu_area->docu_area.tl.slr.col,
                                                 p_style_docu_area->docu_area.br.slr.col));
                }
                else
                {
                    /* add all columns if we hit an implied style */
                    COL col, col_st, col_et;
                    limits_from_docu_area(p_docu, &col_st, &col_et, NULL, NULL, &p_style_docu_area->docu_area);
                    col_st = MAX(col_st, col_s);
                    col_et = MIN(col_et, col_e);
                    for(col = col_st; col < col_et; ++col)
                        status_break(col_add_to_list(&h_col_table, col_max, col, col + 1));

                    trace_1(TRACE_APP_STYLE,
                            TEXT("style_change between cols (implied): adding ") COL_TFMT TEXT(" columns"),
                            col_e - col_s);
                    break;
                }
            }
        }
    }

    if(0 != h_col_table)
    {
        ARRAY_INDEX col_ix;
        COL last_col;
        P_COL p_col_i, p_col_o;

        /* sort array */
        qsort(array_ptr(&h_col_table, COL, 0), array_elements32(&h_col_table), sizeof(COL), proc_col_sort);

        /* remove duplicates */
        for(col_ix = 0, last_col = -1, p_col_i = p_col_o = array_ptr(&h_col_table, COL, col_ix);
            col_ix < array_elements(&h_col_table);
            ++col_ix, ++p_col_i)
        {
            if(*p_col_i != last_col)
                *p_col_o++ = *p_col_i;

            last_col = *p_col_i;
        }

        /* free free space */
        if(p_col_i != p_col_o)
            al_array_shrink_by(&h_col_table, PtrDiffElemS32(p_col_o, p_col_i));
    }

    trace_1(TRACE_APP_STYLE, S32_TFMT TEXT(" changes between columns"), array_elements(&h_col_table));

    return(h_col_table);
}

/******************************************************************************
*
* produce an array of row numbers which
* represent rows containing a change of the
* selected style in the given row from the row before
* the first row is always output to the array
*
* --out--
* the array handle is owned by the caller and
* must be freed after use
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, proc_row_sort, ROW)
{
    QSORT_ARG1_VAR_DECL(PC_ROW, p_row_1);
    QSORT_ARG2_VAR_DECL(PC_ROW, p_row_2);

    const ROW row_1 = *p_row_1;
    const ROW row_2 = *p_row_2;

    profile_ensure_frame();

    if(row_1 > row_2)
        return(1);

    if(row_1 < row_2)
        return(-1);

    return(0);
}

_Check_return_
static STATUS
row_add_to_list(
    _InoutRef_  P_ARRAY_HANDLE p_h_row_table,
    _InVal_     ROW row_max,
    _InVal_     ROW row_1,
    _InVal_     ROW row_2,
    _InVal_     BOOL tl_f,
    _InVal_     BOOL br_f)
{
    U32 n_add = (tl_f ? 1 : 0) + (br_f ? 1 : 0);

    if(0 != n_add)
    {
        STATUS status;
        P_ROW p_row;

        if(NULL == (p_row = al_array_extend_by(p_h_row_table, ROW, n_add, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        {
            al_array_dispose(p_h_row_table);
            return(status);
        }

        if(tl_f)
            *p_row++ = MIN(row_1, row_max);
        if(br_f)
            *p_row++ = MIN(row_2, row_max);
    }

    return(STATUS_OK);
}

_Check_return_
extern ARRAY_HANDLE
style_change_between_rows(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_start,
    _InVal_     ROW row_end,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    ARRAY_HANDLE h_row_table;
    ARRAY_INDEX style_docu_area_ix;
    P_STYLE_DOCU_AREA p_style_docu_area;
    P_ROW p_row;
    ROW row_max = MAX(0, n_rows(p_docu) - 1);

    { /* initialise array with row_start */
    STATUS status;
    SC_ARRAY_INIT_BLOCK row_init_block = aib_init(5, sizeof32(ROW), FALSE);
    if(NULL != (p_row = al_array_alloc(&h_row_table, ROW, 1, &row_init_block, &status)))
        *p_row = row_start;
    } /*block*/

    /* loop over style areas */
    for(style_docu_area_ix = 0, p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, style_docu_area_ix);
        h_row_table && style_docu_area_ix < array_elements(&p_docu->h_style_docu_area);
        ++style_docu_area_ix, ++p_style_docu_area)
    {
        BOOL implied;

        if(p_style_docu_area->deleted)
            continue;

        implied = (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id);

        if(!p_style_docu_area->docu_area.whole_col
           ||
           implied)
        {
            BOOL tl_f = p_style_docu_area->docu_area.tl.slr.row >= row_start &&
                        p_style_docu_area->docu_area.tl.slr.row < row_end;
            BOOL br_f = p_style_docu_area->docu_area.br.slr.row >= row_start &&
                        p_style_docu_area->docu_area.br.slr.row < row_end;

            if(implied || tl_f || br_f)
            {
                STYLE_SELECTOR temp;

                /* are there any bits here we need ? */
                if(style_docu_area_selector_and(p_docu, &temp, p_style_docu_area, p_style_selector))
                {
                     /* !!!assume implied col granularity doesn't change between rows of this docu_area!!! */
                    if(!implied || !style_selector_bic(&temp, &temp, &style_selector_col))
                    {
                        status_break(row_add_to_list(&h_row_table,
                                                     row_max,
                                                     p_style_docu_area->docu_area.tl.slr.row,
                                                     p_style_docu_area->docu_area.br.slr.row,
                                                     tl_f,
                                                     br_f));
                    }
                    else
                    {
                        /* add all rows if we hit an implied style */
                        ROW row, row_st, row_et;
                        limits_from_docu_area(p_docu, NULL, NULL, &row_st, &row_et, &p_style_docu_area->docu_area);
                        row_st = MAX(row_st, row_start);
                        row_et = MIN(row_et, row_end);
                        for(row = row_st; row < row_et; row++)
                            status_break(row_add_to_list(&h_row_table, row_max, row, row + 1, TRUE, TRUE));

                        trace_1(TRACE_APP_STYLE,
                                TEXT("style_change between rows (implied): adding ") ROW_TFMT TEXT(" rows"),
                                row_end - row_start);
                        break;
                    }
                }
            }
        }
    }

    if(0 != h_row_table)
    {
        ARRAY_INDEX row_ix;
        ROW last_row;
        P_ROW p_row_i, p_row_o;

        /* sort array */
        qsort(array_ptr(&h_row_table, ROW, 0), array_elements32(&h_row_table), sizeof(ROW), proc_row_sort);

        /* remove duplicates */
        for(row_ix = 0, last_row = -1, p_row_i = p_row_o = array_ptr(&h_row_table, ROW, row_ix);
            row_ix < array_elements(&h_row_table);
            ++row_ix, ++p_row_i)
        {
            if(*p_row_i != last_row)
                *p_row_o++ = *p_row_i;

            last_row = *p_row_i;
        }

        /* free free space */
        if(p_row_i != p_row_o)
            al_array_shrink_by(&h_row_table, PtrDiffElemS32(p_row_o, p_row_i));
    }

    trace_1(TRACE_APP_STYLE, S32_TFMT TEXT(" changes between rows"), array_elements(&h_row_table));

    return(h_row_table);
}

/******************************************************************************
*
* make an array of sub-object positions which
* represent places where the style changes within the object
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, proc_sub_change_sort, S32)
{
    QSORT_ARG1_VAR_DECL(PC_S32, p_s32_1);
    QSORT_ARG2_VAR_DECL(PC_S32, p_s32_2);

    const S32 s32_1 = *p_s32_1;
    const S32 s32_2 = *p_s32_2;

    profile_ensure_frame();

    if(s32_1 > s32_2)
        return(1);

    if(s32_1 < s32_2)
        return(-1);

    return(0);
}

_Check_return_
static ARRAY_HANDLE /*h_sub_changes*/
style_change_in_object(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_SLR p_slr,
    _InRef_     PC_ARRAY_HANDLE p_array_handle)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(S32), FALSE);
    ARRAY_HANDLE h_sub_changes = 0;
    P_S32 p_sub_change;

    /* check that only character level styles are asked for */
#if TRACE_ALLOWED
    {
    STYLE_SELECTOR temp;
    void_style_selector_bic(&temp, p_style_selector, &style_selector_font_spec);
    style_selector_bit_clear(&temp, STYLE_SW_NAME);
    assert(!style_selector_any(&temp));
    } /*block*/
#endif

    if(NULL != (p_sub_change = al_array_extend_by(&h_sub_changes, S32, 2, &array_init_block, &status)))
    {
        const ARRAY_INDEX n_elements = array_elements(p_array_handle);
        ARRAY_INDEX style_docu_area_ix;
        P_STYLE_DOCU_AREA p_style_docu_area = array_range(p_array_handle, STYLE_DOCU_AREA, 0, n_elements);

        /* initialise change list with min and max values */
        p_sub_change[0] = 0;
        p_sub_change[1] = S32_MAX;

        /* loop over style areas */
        for(style_docu_area_ix = 0; h_sub_changes && style_docu_area_ix < n_elements; ++style_docu_area_ix, ++p_style_docu_area)
        {
            STYLE_SELECTOR temp;

            if(p_style_docu_area->deleted)
                continue;

            if(style_docu_area_selector_and(p_docu, &temp, p_style_docu_area, p_style_selector)
               &&
               !docu_area_is_empty(&p_style_docu_area->docu_area))
            {
                if((OBJECT_ID_NONE != p_style_docu_area->docu_area.tl.object_position.object_id) &&
                   slr_first_in_docu_area(&p_style_docu_area->docu_area, p_slr))
                {
                    if(NULL == (p_sub_change = al_array_extend_by(&h_sub_changes, S32, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
                    {
                        al_array_dispose(&h_sub_changes);
                        break;
                    }

                    *p_sub_change = p_style_docu_area->docu_area.tl.object_position.data;
                }

                if((OBJECT_ID_NONE != p_style_docu_area->docu_area.br.object_position.object_id) &&
                   slr_last_in_docu_area(&p_style_docu_area->docu_area, p_slr))
                {
                    if(NULL == (p_sub_change = al_array_extend_by(&h_sub_changes, S32, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
                    {
                        al_array_dispose(&h_sub_changes);
                        break;
                    }

                    *p_sub_change = p_style_docu_area->docu_area.br.object_position.data;
                }
            }
        }

        /* sort table and remove duplicates */
        if(0 != h_sub_changes)
        {
            ARRAY_INDEX change_ix;
            S32 last_change;
            P_S32 p_change_i, p_change_o;

            /* sort array */
            qsort(array_ptr(&h_sub_changes, S32, 0), array_elements32(&h_sub_changes), sizeof(S32), proc_sub_change_sort);

            /* remove duplicates */
            for(change_ix = 0, last_change = -1, p_change_i = p_change_o = array_ptr(&h_sub_changes, S32, change_ix);
                change_ix < array_elements(&h_sub_changes);
                ++change_ix, ++p_change_i)
            {
                if(*p_change_i != last_change)
                    *p_change_o++ = *p_change_i;

                last_change = *p_change_i;
            }

            /* free free space */
            if(p_change_i != p_change_o)
                al_array_shrink_by(&h_sub_changes, PtrDiffElemS32(p_change_o, p_change_i));

            { /* garbage collect */
            AL_GARBAGE_FLAGS al_garbage_flags;
            AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
            al_garbage_flags.shrink = 1;
            consume(S32, al_array_garbage_collect(&h_sub_changes, 0, NULL, al_garbage_flags));
            } /*block*/
        }
    }

    status_assert(status);

    return(h_sub_changes);
}

/******************************************************************************
*
* are there any styles at or above the given class level ?
*
******************************************************************************/

_Check_return_
extern BOOL
style_at_or_above_class(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area,
    _In_        REGION_CLASS region_class)
{
    BOOL found = FALSE;

    profile_ensure_frame();

    if(region_class < p_docu->region_class_limit)
    {
        ARRAY_INDEX i = array_elements(p_h_style_docu_area);
        PC_STYLE_DOCU_AREA p_style_docu_area = array_ptrc(p_h_style_docu_area, STYLE_DOCU_AREA, i);

        while(--i >= 0)
        {
            --p_style_docu_area;

            if(p_style_docu_area->deleted)
                continue;

            if(p_style_docu_area->region_class >= region_class)
            {
                found = TRUE;
                break;
            }
        }
    }

    return(found);
}

/******************************************************************************
*
* fill undefined bits of style with defaults
*
******************************************************************************/

extern void
style_copy_defaults(
    _DocuRef_   PC_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    STYLE_HANDLE style_base = style_handle_base(&p_docu->h_style_docu_area);

    if(style_base)
    {
        PC_STYLE p_def_style = array_ptrc(&p_docu->h_style_list, STYLE, style_base);
        STYLE_SELECTOR selector;
        void_style_selector_not(&selector, &p_style->selector);
        void_style_selector_and(&selector, &selector, p_style_selector); /* limit to just those defaults needed */
        style_copy(p_style, p_def_style, &selector);
    }
}

/******************************************************************************
*
* add a style region
*
* input can be from EITHER:
* style structure
* inline string
* style handle
*
******************************************************************************/

_Check_return_
extern STATUS
style_docu_area_add(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area,
    P_STYLE_DOCU_AREA_ADD_PARM p_style_docu_area_add_parm)
{
    STATUS status = STATUS_OK;
    P_STYLE_DOCU_AREA p_style_docu_area;
    P_ARRAY_HANDLE p_h_style_docu_area;
    ARRAY_INDEX ix_insert;

    PTR_ASSERT(p_docu_area);

    p_h_style_docu_area = p_style_docu_area_add_parm->p_array_handle
                        ? p_style_docu_area_add_parm->p_array_handle
                        : &p_docu->h_style_docu_area;

    /* record style at caret for later use */
    if(p_style_docu_area_add_parm->caret && !p_docu->flags.caret_style_on)
    {
        STYLE style_at_caret;
        style_init(&style_at_caret);
        style_from_position(p_docu,
                            &style_at_caret,
                            &style_selector_font_spec,
                            &p_docu_area->tl,
                            p_h_style_docu_area,
                            position_in_docu_area,
                            FALSE);
        p_docu->font_spec_at_caret = style_at_caret.font_spec;
        p_docu->flags.caret_style_on = 1;
    }
    else
        p_docu->flags.caret_style_on = 0;

    {
    ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);

    /* find place to insert according to region class */
    ix_insert = n_regions;

    if(n_regions)
    {
        ARRAY_INDEX i = n_regions;

        p_style_docu_area = array_ptr(p_h_style_docu_area, STYLE_DOCU_AREA, i);

        while(--i >= 0)
        {
            --p_style_docu_area;

            if(p_style_docu_area->deleted)
                continue;

            if(p_style_docu_area->region_class <= p_style_docu_area_add_parm->region_class)
            {
                ix_insert = i + 1;
                break;
            }
        }
    }
    } /*block*/

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(16, sizeof32(STYLE_DOCU_AREA), TRUE);
    if(NULL != (p_style_docu_area = al_array_insert_before(p_h_style_docu_area, STYLE_DOCU_AREA, 1, &array_init_block, &status, ix_insert)))
        al_array_auto_compact_set(p_h_style_docu_area);
    } /*block*/

    trace_3(TRACE_APP_MEMORY_USE,
            TEXT("region table ") S32_TFMT TEXT(" now ") U32_TFMT TEXT(" bytes, ") U32_TFMT TEXT(" elements"),
            (S32) p_style_docu_area_add_parm->data_space,
            array_elements32(p_h_style_docu_area) * sizeof32(STYLE_DOCU_AREA),
            array_elements32(p_h_style_docu_area));

    if(status_ok(status))
    {
        /* copy in data */
        PTR_ASSERT(p_style_docu_area);
        p_style_docu_area->docu_area = *p_docu_area;
        p_style_docu_area->data_space = p_style_docu_area_add_parm->data_space;
        p_style_docu_area->internal = p_style_docu_area_add_parm->internal;
        p_style_docu_area->base = p_style_docu_area_add_parm->base;
        p_style_docu_area->caret = p_style_docu_area_add_parm->caret;
        p_style_docu_area->object_message.object_id = OBJECT_ID_UNPACK(p_style_docu_area_add_parm->object_id);
        p_style_docu_area->object_message.t5_message = p_style_docu_area_add_parm->t5_message;
        p_style_docu_area->arg = p_style_docu_area_add_parm->arg;
        p_style_docu_area->region_class = p_style_docu_area_add_parm->region_class;

        switch(p_style_docu_area_add_parm->type)
        {
        case STYLE_DOCU_AREA_ADD_TYPE_IMPLIED:
            if(!p_style_docu_area_add_parm->data.p_style)
                break;

            /*FALLTHRU*/

        case STYLE_DOCU_AREA_ADD_TYPE_STYLE:
            {
            assert(p_style_docu_area_add_parm->data.p_style);

            if(NULL != (p_style_docu_area->p_style_effect = al_ptr_alloc_elem(STYLE, 1, &status)))
                *p_style_docu_area->p_style_effect = *p_style_docu_area_add_parm->data.p_style;

            break;
            }

        case STYLE_DOCU_AREA_ADD_TYPE_INLINE:
            {
            STYLE style;

            assert(!IS_P_DATA_NONE(p_style_docu_area_add_parm->data.ustr_inline));

            style_init(&style);
            status_assert(style_struct_from_ustr_inline(p_docu, &style, p_style_docu_area_add_parm->data.ustr_inline, &style_selector_all));

            if(style_bit_test(&style, STYLE_SW_HANDLE))
                p_style_docu_area->style_handle = style.style_handle;
            else
            {
                if(NULL != (p_style_docu_area->p_style_effect = al_ptr_alloc_elem(STYLE, 1, &status)))
                    *p_style_docu_area->p_style_effect = style;
            }
            break;
            }

        default: default_unhandled();
#if CHECKING
        case STYLE_DOCU_AREA_ADD_TYPE_HANDLE:
#endif
            assert(p_style_docu_area_add_parm->data.style_handle);
            p_style_docu_area->style_handle = p_style_docu_area_add_parm->data.style_handle;
            break;
        }
    }

    /* create dependency */
    if(status_ok(status) && !p_style_docu_area_add_parm->p_array_handle)
    {
        REGION region;

        if(status_ok(status = uref_add_dependency(p_docu, region_from_docu_area_max(&region, &p_style_docu_area->docu_area), proc_uref_event_style, next_client_handle, &p_style_docu_area->uref_handle, TRUE /* SKS 19feb97 */)))
            p_style_docu_area->client_handle = next_client_handle++;
    }

    /* MRJC 23.10.93: check for special text document column zero width region */
    if(status_ok(status))
    {
        if(p_style_docu_area->base
           &&
           p_style_docu_area->docu_area.whole_col
           &&
           p_style_docu_area->docu_area.whole_row)
        {
            const PC_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

            if(!IS_P_STYLE_NONE(p_style))
                if(style_bit_test(p_style, STYLE_SW_CS_WIDTH)
                   &&
                   p_style->col_style.width == 0)
                {
                    p_style_docu_area->column_zero_width = 1;
                    p_docu->flags.base_single_col = 1;
                    trace_0(TRACE_APP_SKEL, TEXT("1111 - style_docu_area_add found zero width base region"));
                }
        }
    }

    if(status_ok(status))
    {
        DOCU_REFORMAT docu_reformat;
        const PC_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);
        BOOL affected;
        BOOL reformat = TRUE;

        /* make the span as large as we can */
        style_docu_area_expand(p_docu, p_style_docu_area);

        /* work out what to reformat if requested */
        if(!p_style_docu_area->caret || !docu_area_is_empty(&p_style_docu_area->docu_area))
        {
            docu_reformat.action = REFORMAT_Y;
            docu_reformat.data_type = REFORMAT_DOCU_AREA;
            docu_reformat.data_space = p_style_docu_area->data_space;
            docu_reformat.data.docu_area = p_style_docu_area->docu_area;

            if(!IS_P_STYLE_NONE(p_style) && style_selector_test(&p_style->selector, &style_selector_extent_x))
                docu_reformat.action = REFORMAT_XY;
        }
        else
            reformat = FALSE;

        trace_6(TRACE_APP_STYLE,
                TEXT("style_docu_area_add tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT TEXT("; br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT,
                p_style_docu_area->docu_area.tl.slr.col, p_style_docu_area->docu_area.tl.slr.row,
                p_style_docu_area->docu_area.tl.object_position.data,
                p_style_docu_area->docu_area.br.slr.col, p_style_docu_area->docu_area.br.slr.row,
                p_style_docu_area->docu_area.br.object_position.data);

        if(p_style_docu_area_add_parm->add_without_subsume)
            affected = FALSE;
        else
            affected = style_docu_area_subsume(p_docu, p_h_style_docu_area, p_style_docu_area);

        if(affected || p_style_docu_area->caret)
        {
            STYLE_DOCU_AREA_CHANGED style_docu_area_changed;

            style_docu_area_changed.data_space = p_style_docu_area->data_space;
            style_docu_area_changed.docu_area = p_style_docu_area->docu_area;

            status_assert(maeve_event(p_docu, T5_MSG_STYLE_DOCU_AREA_CHANGED, &style_docu_area_changed));
        }

        trace_1(TRACE_APP_STYLE, TEXT("style_docu_area_add ") S32_TFMT TEXT(" regions"), array_elements(p_h_style_docu_area));

        /* generate reformat if requested */
        if(affected && reformat)
            status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
    }
    else if(NULL != p_style_docu_area)
        p_style_docu_area->deleted = 1;

    return(status);
}

/******************************************************************************
*
* add a 'base' area to a style list
*
******************************************************************************/

_Check_return_
extern STATUS
style_docu_area_add_base(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_array_handle,
    _InVal_     STYLE_HANDLE style_handle,
    _In_        DATA_SPACE data_space)
{
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    DOCU_AREA docu_area;

    /* add this as base region */
    docu_area.tl.slr.col = docu_area.br.slr.col = -1;
    docu_area.tl.slr.row = docu_area.br.slr.row = -1;
    docu_area.tl.object_position.object_id =
    docu_area.br.object_position.object_id = OBJECT_ID_NONE;
    docu_area.tl.object_position.data =
    docu_area.br.object_position.data = -1;

    docu_area.whole_col =
    docu_area.whole_row = 1;

    /* add internal base style area */
    STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle);

    style_docu_area_add_parm.base = 1;
    style_docu_area_add_parm.p_array_handle = p_array_handle;
    style_docu_area_add_parm.data_space = data_space;
    style_docu_area_add_parm.region_class = REGION_BASE;

    return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
}

/******************************************************************************
*
* add an 'internal' area to a style list
* the internal area covers the whole area
* and defines a set of default defaults when
* cruddy old files don't define the correct things
*
******************************************************************************/

#define SYSTEM_FACE_NAME        TEXT("Helvetica")
#define SYSTEM_FACE_Y           10 * PIXITS_PER_POINT

_Check_return_
extern STATUS
style_docu_area_add_internal(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_array_handle,
    _In_        DATA_SPACE data_space)
{
    STATUS status = STATUS_OK;
    STYLE style;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    DOCU_AREA docu_area;

    /* initialise style */
    zero_struct(style);
    style_init(&style);

    /* everything is defined in base style */
    style.selector = style_selector_all;

    /* except style handle */
    style_selector_bit_clear(&style.selector, STYLE_SW_HANDLE);
    /* and style name */
    style_selector_bit_clear(&style.selector, STYLE_SW_NAME);

    status_assert(font_spec_name_alloc(&style.font_spec, SYSTEM_FACE_NAME));
    style.font_spec.size_x = 0;
    style.font_spec.size_y = SYSTEM_FACE_Y;

    /* set default row height */
    style.row_style.height = 340;

    /* set default foreground colour */
    style.font_spec.colour = rgb_stash[COLOUR_OF_TEXT];

    /* set default background colour */
    style.para_style.rgb_back = rgb_stash[COLOUR_OF_PAPER];

    /* set default vertical spacing */
    style.para_style.line_space.type    = SF_LINE_SPACE_SINGLE;
    style.para_style.line_space.leading = PIXITS_PER_INCH / 20;

    /* set default justification */
    style.para_style.justify = SF_JUSTIFY_LEFT;
    style.para_style.justify_v = SF_JUSTIFY_V_TOP;

    /* set default object type */
    style.para_style.new_object = OBJECT_ID_TEXT;

    /* set up default borders */
    style.para_style.rgb_border = rgb_stash[15];        /* pale blue */
    style.para_style.border = SF_BORDER_NONE;

    /* default grid */
    style.para_style.rgb_grid_left =
    style.para_style.rgb_grid_top =
    style.para_style.rgb_grid_right =
    style.para_style.rgb_grid_bottom = rgb_stash[15];   /* pale blue */
    style.para_style.grid_left = SF_BORDER_STANDARD;
    style.para_style.grid_top = style.para_style.grid_right = style.para_style.grid_bottom = style.para_style.grid_left;

    /* add this as base region */
    docu_area.tl.slr.col = docu_area.br.slr.col = -1;
    docu_area.tl.slr.row = docu_area.br.slr.row = -1;
    docu_area.tl.object_position.object_id =
    docu_area.br.object_position.object_id = OBJECT_ID_NONE;
    docu_area.tl.object_position.data =
    docu_area.br.object_position.data = -1;

    docu_area.whole_col =
    docu_area.whole_row = 1;

    /* add internal base style area */
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);

    style_docu_area_add_parm.internal = 1;
    style_docu_area_add_parm.p_array_handle = p_array_handle;
    style_docu_area_add_parm.data_space = data_space;
    style_docu_area_add_parm.region_class = REGION_INTERNAL;

    status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);

    style_dispose(&style);

    return(status);
}

/******************************************************************************
*
* choose a region from the list
*
******************************************************************************/

extern S32 /* index of chosen region; -1 == end */
style_docu_area_choose(
    _InRef_     PC_ARRAY_HANDLE p_h_style_list,
    _InRef_opt_ PC_DOCU_AREA p_docu_area,
    _InRef_opt_ PC_POSITION p_position /* if p_docu_area == NULL*/,
    _InVal_     enum STYLE_DOCU_AREA_CHOOSE_ACTION action,
    _InVal_     S32 index /* -1 to start at either end */)
{
    ARRAY_INDEX style_docu_area_ix = 0;

    assert(p_docu_area || p_position);

    switch(action)
    {
    case STYLE_CHOOSE_DOWN:
        if(index < 0)
            style_docu_area_ix = array_elements(p_h_style_list) - 1;
        else
            style_docu_area_ix = index - 1;
        break;

    case STYLE_CHOOSE_UP:
        if(index < 0)
            style_docu_area_ix = 0;
        else
            style_docu_area_ix = index + 1;
        break;

    case STYLE_CHOOSE_LEAVE:
        assert(index > 0);
        style_docu_area_ix = index;
        break;

    default: default_unhandled(); break;
    }

    while((style_docu_area_ix >= 0) && (style_docu_area_ix < array_elements(p_h_style_list)))
    {
        PC_STYLE_DOCU_AREA p_style_docu_area = array_ptrc(p_h_style_list, STYLE_DOCU_AREA, style_docu_area_ix);

        if(!(p_style_docu_area->deleted || p_style_docu_area->internal || (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id)))
        {
            if(NULL != p_docu_area)
            {
                if(docu_area_in_docu_area(&p_style_docu_area->docu_area, p_docu_area))
                    break;
            }
            else if(position_in_docu_area(&p_style_docu_area->docu_area, p_position))
                break;
        }

        switch(action)
        {
        case STYLE_CHOOSE_LEAVE:
        case STYLE_CHOOSE_DOWN:
            style_docu_area_ix -= 1;
            break;

        case STYLE_CHOOSE_UP:
            style_docu_area_ix += 1;
            break;

        default: default_unhandled(); break;
        }
    }

    return((style_docu_area_ix < 0) || (style_docu_area_ix >= array_elements(p_h_style_list)) ? -1 : style_docu_area_ix);
}

/******************************************************************************
*
* clear regions contained by a docu_area
*
******************************************************************************/

extern void
style_docu_area_clear(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InVal_     DATA_SPACE data_space)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    P_STYLE_DOCU_AREA p_style_docu_area = array_range(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);

    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(!p_style_docu_area->internal && !p_style_docu_area->base)
            if(docu_area_in_docu_area(p_docu_area, &p_style_docu_area->docu_area))
                style_docu_area_delete(p_docu, p_style_docu_area);
    }

    {
    DOCU_REFORMAT docu_reformat;
    docu_reformat.action = REFORMAT_XY;
    docu_reformat.data_type = REFORMAT_DOCU_AREA;
    docu_reformat.data_space = data_space;
    docu_reformat.data.docu_area = *p_docu_area;
    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
    } /*block*/
}

/******************************************************************************
*
* count regions contained by a docu_area
*
******************************************************************************/

_Check_return_
extern S32
style_docu_area_count(
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    PC_STYLE_DOCU_AREA p_style_docu_area = array_rangec(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);
    S32 count = 0;

    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(!p_style_docu_area->internal && !p_style_docu_area->base)
            if(docu_area_in_docu_area(p_docu_area, &p_style_docu_area->docu_area))
                ++count;
    }

    return(count);
}

/******************************************************************************
*
* delete a style region and free resources
*
******************************************************************************/

extern void
style_docu_area_delete(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE_DOCU_AREA p_style_docu_area)
{
    STYLE_DOCU_AREA_CHANGED style_docu_area_changed;

    style_docu_area_changed.data_space = p_style_docu_area->data_space;
    style_docu_area_changed.docu_area = p_style_docu_area->docu_area;

    if(p_style_docu_area->p_style_effect)
    {
        const P_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

        if(!IS_P_STYLE_NONE(p_style))
            style_free_resources_all(p_style);

        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_style_docu_area->p_style_effect));
    }

    if(p_style_docu_area->uref_handle)
        uref_del_dependency(docno_from_p_docu(p_docu), p_style_docu_area->uref_handle);

    p_style_docu_area->deleted = 1;

    status_assert(maeve_event(p_docu, T5_MSG_STYLE_DOCU_AREA_CHANGED, &style_docu_area_changed));
}

PROC_ELEMENT_DELETED_PROTO(static, style_docu_area_deleted)
{
    return(((P_STYLE_DOCU_AREA) p_any)->deleted);
}

/******************************************************************************
*
* delete a style region and free resources
*
******************************************************************************/

extern P_STYLE_DOCU_AREA /* NULL == didn't find one */
style_docu_area_enum_implied(
    _DocuRef_   P_DOCU p_docu,
    /*inout*/ P_ARRAY_INDEX p_array_index /* -1 to start */,
    _InVal_     OBJECT_ID object_id,
    P_T5_MESSAGE p_t5_message /* NULL==don't care */,
    P_S32 p_arg /* NULL==don't care */)
{
    P_STYLE_DOCU_AREA p_style_docu_area_out = NULL;
    ARRAY_INDEX n_regions = array_elements(&p_docu->h_style_docu_area);

    profile_ensure_frame();

    if(*p_array_index < 0)
        *p_array_index = -1;

    for(;;)
    {
        P_STYLE_DOCU_AREA p_style_docu_area;

        *p_array_index += 1;

        if(*p_array_index >= n_regions)
            break;

        p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, *p_array_index);

        if(p_style_docu_area->deleted)
            continue;

        if(p_style_docu_area->object_message.object_id == object_id
           &&
           (!p_t5_message || *p_t5_message == p_style_docu_area->object_message.t5_message)
           &&
           (!p_arg || *p_arg == p_style_docu_area->arg))
        {
            p_style_docu_area_out = p_style_docu_area;
            break;
        }
    }

    return(p_style_docu_area_out);
}

/******************************************************************************
*
* delete a style region list
*
******************************************************************************/

extern void
style_docu_area_delete_list(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InVal_     S32 all /* FALSE leaves internal regions */)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    P_STYLE_DOCU_AREA p_style_docu_area = array_range(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);

    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(all || !p_style_docu_area->internal)
            style_docu_area_delete(p_docu, p_style_docu_area);
    }

    { /* garbage collect style region list */
    ARRAY_HANDLE h_style_docu_area = *p_h_style_docu_area;
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.remove_deleted = 1;
    al_garbage_flags.shrink = 1;
    consume(S32, al_array_garbage_collect(&h_style_docu_area, 0, style_docu_area_deleted, al_garbage_flags));
    assert(h_style_docu_area == *p_h_style_docu_area);
    } /*block*/
}

/******************************************************************************
*
* expand the area supplied to docu_area_add
* as much as possible - depending on the
* granularity of the effects being added
*
******************************************************************************/

static void
style_docu_area_expand(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE_DOCU_AREA p_style_docu_area)
{
    if(!p_style_docu_area->docu_area.whole_col && !p_style_docu_area->docu_area.whole_row)
    {
        PC_STYLE p_style;

        /* try clearing object positions */
        if(object_position_at_start(&p_style_docu_area->docu_area.tl.object_position))
            p_style_docu_area->docu_area.tl.object_position.object_id = OBJECT_ID_NONE;

        if(NULL != (p_style = p_style_from_docu_area(p_docu, p_style_docu_area)))
        {
            /* if we have character things, can't expand region */
            if(!style_selector_test(&p_style->selector, &style_selector_font_spec))
            {
                /* expand to paragraph granularity */
                p_style_docu_area->docu_area.tl.object_position.object_id = OBJECT_ID_NONE;
                p_style_docu_area->docu_area.br.object_position.object_id = OBJECT_ID_NONE;
                p_style_docu_area->docu_area.tl.object_position.data = -1;
                p_style_docu_area->docu_area.br.object_position.data = -1;

                /* check for paragraph granularity things */
                if((p_style_docu_area->data_space == DATA_SLOT) && !style_selector_test(&p_style->selector, &style_selector_para))
                {
                    /* expand to row granularity */
                    p_style_docu_area->docu_area.tl.slr.col = -1;
                    p_style_docu_area->docu_area.br.slr.col = -1;
                    p_style_docu_area->docu_area.whole_row = 1;
                }
            }
        }
    }
}

/******************************************************************************
*
* match region for a docu area
*
******************************************************************************/

_Check_return_
static BOOL
style_docu_area_position_update_sub(
    _InoutRef_  P_OBJECT_POSITION p_object_position,
    _InRef_     PC_OBJECT_POSITION_UPDATE p_object_position_update,
    _InVal_     S32 end_point)
{
    BOOL res = 0;

    profile_ensure_frame();

    if(OBJECT_ID_NONE != p_object_position->object_id)
    {
        S32 start = (OBJECT_ID_NONE == p_object_position_update->object_position.object_id)
                        ? 0
                        : p_object_position_update->object_position.data;

        if(p_object_position_update->data_update >= 0)
        {
            /* insert operation */
            if(p_object_position->data == start && end_point)
            {
                res = 1;
                p_object_position->data += p_object_position_update->data_update;
            }
            else if(p_object_position->data > start)
            {
                res = 1;
                p_object_position->data += p_object_position_update->data_update;
            }
        }
        else if(p_object_position->data >= start)
        {
            /* delete operation */
            S32 end = start - p_object_position_update->data_update;

            if(p_object_position->data <= end)
                p_object_position->data = start;
            else
                p_object_position->data += p_object_position_update->data_update;

            res = 1;
        }
    }

    return(res);
}

/******************************************************************************
*
* update sub-object positions in a style docu_area list
*
******************************************************************************/

extern void
style_docu_area_position_update(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_OBJECT_POSITION_UPDATE p_object_position_update)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    P_STYLE_DOCU_AREA p_style_docu_area = array_range(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);

    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        BOOL res = 0;
        REGION region;
        P_REGION p_region = NULL;

        if(p_style_docu_area->deleted)
            continue;

        if((p_object_position_update->data_ref.data_space != DATA_SLOT)
           ||
           slr_first_in_docu_area(&p_style_docu_area->docu_area, &p_object_position_update->data_ref.arg.slr))
        {
            if(style_docu_area_position_update_sub(&p_style_docu_area->docu_area.tl.object_position,
                                                   p_object_position_update,
                                                   FALSE))
                res = 1;
        }

        if((p_object_position_update->data_ref.data_space != DATA_SLOT)
           ||
           slr_last_in_docu_area(&p_style_docu_area->docu_area, &p_object_position_update->data_ref.arg.slr))
        {
            if(style_docu_area_position_update_sub(&p_style_docu_area->docu_area.br.object_position,
                                                   p_object_position_update,
                                                   p_style_docu_area->caret))
                res = 1;
        }

        if(res)
        {
            if(p_object_position_update->data_ref.data_space == DATA_SLOT)
            {
                region_from_two_slrs(&region,
                                     &p_object_position_update->data_ref.arg.slr,
                                     &p_object_position_update->data_ref.arg.slr,
                                     TRUE);
                p_region = &region;
            }

            style_cache_sub_dispose(p_docu, p_object_position_update->data_ref.data_space, p_region);
        }

        if((OBJECT_ID_NONE == p_style_docu_area->object_message.object_id)
           &&
           docu_area_is_empty(&p_style_docu_area->docu_area))
            style_docu_area_delete(p_docu, p_style_docu_area);
    }
}

/******************************************************************************
*
* remove from the list the style_docu_area
*
******************************************************************************/

extern void
style_docu_area_remove(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list /*data modified*/,
    _InVal_     ARRAY_INDEX ix)
{
    DOCU_REFORMAT docu_reformat;
    P_STYLE_DOCU_AREA p_style_docu_area = array_ptr(p_h_style_list, STYLE_DOCU_AREA, ix);

    docu_reformat.action = REFORMAT_XY;
    docu_reformat.data_type = REFORMAT_DOCU_AREA;
    docu_reformat.data_space = p_style_docu_area->data_space;
    docu_reformat.data.docu_area = p_style_docu_area->docu_area;

    style_docu_area_delete(p_docu, p_style_docu_area);

    { /* garbage collect style region list */
    ARRAY_HANDLE h_style_list = *p_h_style_list;
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.remove_deleted = 1;
    al_garbage_flags.shrink = 1;
    consume(S32, al_array_garbage_collect(&h_style_list, 0, style_docu_area_deleted, al_garbage_flags));
    assert(h_style_list == *p_h_style_list);
    } /*block*/

    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
}

/******************************************************************************
*
* AND the supplied selector with the (possibly indirected) selector in a style_docu_area
*
******************************************************************************/

_Check_return_
extern BOOL
style_docu_area_selector_and(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE_SELECTOR p_style_selector_out,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_in)
{
    const PC_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

    profile_ensure_frame();

    if(NULL == p_style)
    {
        style_selector_copy(p_style_selector_out, p_style_selector_in); /* keep to contract */
        return(style_selector_any(p_style_selector_in));
    }

    return(style_selector_and(p_style_selector_out, p_style_selector_in, &p_style->selector));
}

/******************************************************************************
*
* we're about to do a uref which will affect sub-object positions within a cell
*
* hold all these sub_objects inviolate for a moment
*
******************************************************************************/

extern void
style_docu_area_uref_hold(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_SLR p_slr)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    P_STYLE_DOCU_AREA p_style_docu_area = array_range(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);
    REGION region_slr;

    IGNOREPARM_DocuRef_(p_docu);

    region_from_two_slrs(&region_slr, p_slr, p_slr, TRUE);

    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        REGION region;

        if(p_style_docu_area->deleted)
            continue;

        region_from_docu_area_max(&region, &p_style_docu_area->docu_area);

        if(region_in_region(&region_slr, &region))
            p_style_docu_area->uref_hold = 1;
    }
}

extern void
style_docu_area_uref_release(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id /* new object id to use */)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    P_STYLE_DOCU_AREA p_style_docu_area = array_range(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);

    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        if(p_style_docu_area->uref_hold)
        {
            if(OBJECT_ID_NONE != p_style_docu_area->docu_area.tl.object_position.object_id)
                p_style_docu_area->docu_area.tl.object_position.object_id = object_id;
            if(OBJECT_ID_NONE != p_style_docu_area->docu_area.br.object_position.object_id)
                p_style_docu_area->docu_area.br.object_position.object_id = object_id;
            p_style_docu_area->uref_hold = 0;
        }
    }

    {
    STYLE_DOCU_AREA_CHANGED style_docu_area_changed;
    style_docu_area_changed.data_space = DATA_SLOT;
    docu_area_from_slr(&style_docu_area_changed.docu_area, p_slr);
    status_assert(maeve_event(p_docu, T5_MSG_STYLE_DOCU_AREA_CHANGED, &style_docu_area_changed));
    } /*block*/
}

/******************************************************************************
*
* compare the implied aspects of a docu_area and say if they're equal
*
******************************************************************************/

_Check_return_
static BOOL
style_docu_area_implied_equal(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area_1,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area_2)
{
    if(p_style_docu_area_1->object_message.object_id != p_style_docu_area_2->object_message.object_id)
        return(FALSE);

    if(OBJECT_ID_NONE == p_style_docu_area_1->object_message.object_id)
        return(TRUE);

    if( (p_style_docu_area_1->object_message.t5_message == p_style_docu_area_2->object_message.t5_message)
        &&
        (p_style_docu_area_1->arg == p_style_docu_area_2->arg)
        &&
        (p_style_docu_area_1->region_class == p_style_docu_area_2->region_class)
        &&
        (p_style_from_docu_area(p_docu, p_style_docu_area_1) == p_style_from_docu_area(p_docu, p_style_docu_area_2)) )
    {
        return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* subsume any regions contained by the supplied region
* or subsume the supplied region if overridden by another
*
******************************************************************************/

_Check_return_
static BOOL /* 0 == no effect, 1 == list changed*/
style_docu_area_subsume(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle /*data modified*/,
    P_STYLE_DOCU_AREA p_style_docu_area_in)
{
    STATUS status = STATUS_OK;
    BOOL stop_subsume = FALSE, had_intersection = FALSE, res = TRUE, res_set = FALSE;
    COALESCE_FLAGS coalesce_flags;

    trace_0(TRACE_APP_STYLE, TEXT("+++ style_docu_area_subsume - in"));

    zero_struct(coalesce_flags);
    coalesce_flags.rows =
    coalesce_flags.frags = 1;

    while(!stop_subsume && status_ok(status))
    {
        P_STYLE_DOCU_AREA p_style_docu_area = p_style_docu_area_in;
        P_STYLE_DOCU_AREA p_style_docu_area_base = array_base(p_array_handle, STYLE_DOCU_AREA);

        /* work down from the supplied region to the base - thus the supplied region always has greater priority */
        while(status_ok(status))
        {
            if(p_style_docu_area == p_style_docu_area_base) /* SKS 14dec93 - loop reorder for PC, SKS 03feb97 moves from loop bottom for empty lists */
                break;

            --p_style_docu_area; /* do NOT coalesce this into the comparison. SKS 03feb97 notes that this will therefore satisfy requirement that we ignore comparing with ourselves */

            if(p_style_docu_area->deleted)
                continue;

            if(p_style_docu_area->internal)
                continue;

            if(style_docu_area_implied_equal(p_docu, p_style_docu_area, p_style_docu_area_in))
            {
                BOOL both_same_handle = 0;
                BOOL both_effects = 0;
                BOOL both_valid = 0;
                DOCU_AREA docu_area_coalesced;
                const P_STYLE p_style_in = p_style_from_docu_area(p_docu, p_style_docu_area_in);
                const P_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

                if(p_style_docu_area->style_handle == p_style_docu_area_in->style_handle)
                {
                    if(p_style_docu_area->style_handle)
                        both_same_handle = 1;
                    else
                        both_effects = 1;
                }

                if(p_style && p_style_in)
                    both_valid = 1;

                /* clear caret region flag */
                if(!p_style_docu_area_in->caret)
                    p_style_docu_area->caret = 0;

                /* check for empty regions left over and blow them away
                 */
                if(!p_style_docu_area->caret && docu_area_is_empty(&p_style_docu_area->docu_area))
                {
                    trace_6(TRACE_APP_STYLE, TEXT("style_subsume (null region) tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT TEXT(" br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT,
                            p_style_docu_area->docu_area.tl.slr.col, p_style_docu_area->docu_area.tl.slr.row,
                            p_style_docu_area->docu_area.tl.object_position.data,
                            p_style_docu_area->docu_area.br.slr.col, p_style_docu_area->docu_area.br.slr.row,
                            p_style_docu_area->docu_area.br.object_position.data);

                    style_docu_area_delete(p_docu, p_style_docu_area);
                    res_set = TRUE;
                    break;
                }

                /* where new, higher region is contained by lower region, and all effects defined by higher
                 * region are defined by lower region, blow away higher (incoming) region
                 */
                if(!had_intersection
                   &&
                   (both_effects || both_same_handle)
                   &&
                   !p_style_docu_area_in->caret
                   &&
                   docu_area_in_docu_area(&p_style_docu_area->docu_area, &p_style_docu_area_in->docu_area))
                {
                    BOOL do_it = 0;

                    if(both_effects && both_valid)
                    {
                        STYLE_SELECTOR style_selector_diff;

                        consume_bool(style_compare(&style_selector_diff, p_style_in, p_style));

                        if(!res_set && !style_selector_any(&style_selector_diff))
                            res = 0;

                        if(!style_selector_test(&style_selector_diff, &p_style_in->selector))
                            do_it = 1;
                    }
                    else
                    {
                        do_it = 1;
                        if(!res_set)
                            res = 0;
                    }

                    if(do_it)
                    {
                        trace_6(TRACE_APP_STYLE, TEXT("style_subsume (new in existing) tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT TEXT(" br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT,
                                p_style_docu_area_in->docu_area.tl.slr.col, p_style_docu_area_in->docu_area.tl.slr.row,
                                p_style_docu_area_in->docu_area.tl.object_position.data,
                                p_style_docu_area_in->docu_area.br.slr.col, p_style_docu_area_in->docu_area.br.slr.row,
                                p_style_docu_area_in->docu_area.br.object_position.data);

                        style_docu_area_delete(p_docu, p_style_docu_area_in);
                        res_set = TRUE;
                        stop_subsume = TRUE;
                        break;
                    }
                }

                /* where new, higher region contains lower region and defines all effects defined
                 * in lower region, blow away the lower region
                 */
                if(both_valid && docu_area_in_docu_area(&p_style_docu_area_in->docu_area, &p_style_docu_area->docu_area))
                {
                    STYLE_SELECTOR selector;

                    if(!style_selector_bic(&selector, &p_style->selector, &p_style_in->selector))
                    {
                        trace_6(TRACE_APP_STYLE, TEXT("style_subsume (existing in new) tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT TEXT(" br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT,
                                p_style_docu_area->docu_area.tl.slr.col, p_style_docu_area->docu_area.tl.slr.row,
                                p_style_docu_area->docu_area.tl.object_position.data,
                                p_style_docu_area->docu_area.br.slr.col, p_style_docu_area->docu_area.br.slr.row,
                                p_style_docu_area->docu_area.br.object_position.data);

                        style_docu_area_delete(p_docu, p_style_docu_area);
                        res_set = TRUE;
                        break;
                    }
                    /* if both regions are effects, cancel the effects in the lower region */
                    else if(both_effects)
                    {
                        style_free_resources(p_style, &p_style_in->selector);
                        res_set = TRUE;
                    }
                }

                /* MRJC 7.4.95 caret region handling */
                if(!had_intersection
                   &&
                   both_valid
                   &&
                   p_style_docu_area_in->caret && p_style_docu_area->caret
                   &&
                   docu_area_coalesce_docu_area_out(&docu_area_coalesced,
                                                    &p_style_docu_area->docu_area,
                                                    &p_style_docu_area_in->docu_area,
                                                    coalesce_flags))
                {
                    STYLE style_at_caret;
                    STYLE_SELECTOR style_caret_diff;

                    /* clear caret bit which stops current area expanding (new area will expand instead) */
                    p_style_docu_area->caret = 0;

                    /* make style containing effects before any caret regions */
                    style_init(&style_at_caret);
                    style_at_caret.font_spec = p_docu->font_spec_at_caret;
                    style_selector_copy(&style_at_caret.selector, &style_selector_font_spec);

                    {
                    STYLE_SELECTOR unaltered_effects;
                    STYLE style_ongoing;

                    /* copy unaffected caret effects into incoming region to make ongoing style */
                    void_style_selector_bic(&unaltered_effects, &p_style->selector, &p_style_in->selector);
                    style_copy_into(&style_ongoing, p_style, &unaltered_effects);

                    /* compare ongoing region with original starting point */
                    consume_bool(style_compare(&style_caret_diff, &style_at_caret, &style_ongoing));
                    void_style_selector_not(&style_caret_diff, &style_caret_diff);

                    /* clear out effects which are the same as the starting point */
                    if(!style_selector_bic(&style_ongoing.selector, &style_ongoing.selector, &style_caret_diff))
                    {
                        /* no effects left - delete region */
                        style_docu_area_delete(p_docu, p_style_docu_area_in);
                        res_set = TRUE;
                        stop_subsume = TRUE;
                        p_docu->flags.caret_style_on = 0;
                        break;
                    }
                    else if(both_effects)
                    {
                        status_assert(style_duplicate(p_style_in, p_style, &unaltered_effects));
                        style_free_resources(p_style_in, &style_caret_diff);
                        res_set = TRUE;
                    }
                    } /*block*/

                }

                /* two style_docu_areas can be coalesced if 1) the docu_areas
                 * can be coalesced and 2) the styles are identical
                 */
                if(!had_intersection
                   &&
                   (both_effects || both_same_handle)
                   &&
                   docu_area_coalesce_docu_area_out(&docu_area_coalesced,
                                                    &p_style_docu_area->docu_area,
                                                    &p_style_docu_area_in->docu_area,
                                                    coalesce_flags))
                {
                    BOOL do_it = 0;

                    if(both_effects && both_valid)
                    {
                        STYLE_SELECTOR style_selector_diff;

                        consume_bool(style_compare(&style_selector_diff, p_style_in, p_style));

                        if(!style_selector_any(&style_selector_diff))
                            do_it = 1;
                    }
                    else
                        do_it = 1;

                    if(do_it)
                    {
                        trace_6(TRACE_APP_STYLE, TEXT("style_subsume (coalescing) tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT TEXT(" br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(",") S32_TFMT,
                                p_style_docu_area_in->docu_area.tl.slr.col, p_style_docu_area_in->docu_area.tl.slr.row,
                                p_style_docu_area_in->docu_area.tl.object_position.data,
                                p_style_docu_area_in->docu_area.br.slr.col, p_style_docu_area_in->docu_area.br.slr.row,
                                p_style_docu_area_in->docu_area.br.object_position.data);

                        /* adjust uref record */
                        if(p_style_docu_area->uref_handle)
                        {
                            REGION region;

                            uref_del_dependency(docno_from_p_docu(p_docu), p_style_docu_area->uref_handle);

                            status_assert(uref_add_dependency(p_docu,
                                                              region_from_docu_area_max(&region, &docu_area_coalesced),
                                                              proc_uref_event_style,
                                                              p_style_docu_area->client_handle,
                                                              &p_style_docu_area->uref_handle, FALSE));
                        }

                        p_style_docu_area->docu_area = docu_area_coalesced;
                        style_docu_area_delete(p_docu, p_style_docu_area_in);
                        res_set = TRUE;
                        stop_subsume = TRUE;
                        break;
                    }
                }

                /* look for a region overlapping with
                 * the input region both in area covered and effects selected;
                 * style indirection is forgotten temporarily, and we just look
                 * at an instantaneous snapshot
                 */
                if(docu_area_intersect_docu_area(&p_style_docu_area_in->docu_area, &p_style_docu_area->docu_area)
                   &&
                   both_valid
                   &&
                   style_selector_test(&p_style->selector, &p_style_in->selector))
                {
                   had_intersection = TRUE;
                }
            }
        }

        if(p_style_docu_area <= p_style_docu_area_base)
            stop_subsume = TRUE;
    }

    { /* garbage collect style region list */
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.remove_deleted = 1;
    al_garbage_flags.shrink = 1;
    consume(S32, al_array_garbage_collect(p_array_handle, 0, style_docu_area_deleted, al_garbage_flags));
    } /*block*/

    trace_0(TRACE_APP_STYLE, TEXT("+++ style_docu_area_subsume - out"));

    status_assertc(status);

    return(res);
}

/******************************************************************************
*
* find the source of an effect so that
* we can modify the source directly (glug)
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_STYLE_DOCU_AREA /* pointer to structure to modify */
style_effect_source_find(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list,
    _InVal_     BOOL implied_ok)
{
    P_STYLE_DOCU_AREA p_style_docu_area, p_style_docu_area_out = NULL;
    ARRAY_INDEX style_docu_area_ix;
    BOOL test_only = (1 == bitmap_count(p_style_selector->bitmap, N_BITS_ARG(STYLE_SW_COUNT)));

    /* look through region list top down for containing region */
    for(style_docu_area_ix = array_elements(p_h_style_list) - 1,
        p_style_docu_area = array_ptr(p_h_style_list, STYLE_DOCU_AREA, style_docu_area_ix);
        style_docu_area_ix >= 0;
        --style_docu_area_ix, --p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(position_in_docu_area(&p_style_docu_area->docu_area, p_position))
        {
            const PC_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);
            BOOL res;

            assert(!IS_P_STYLE_NONE(p_style));

            if(test_only)
                res = style_selector_test(&p_style->selector, p_style_selector);
            else
                res = !style_selector_compare(p_style_selector, &p_style->selector);

            if(res)
            {
                /* if we bump into an implied region, report that the search failed */
                if(implied_ok || (OBJECT_ID_NONE == p_style_docu_area->object_message.object_id))
                    p_style_docu_area_out = p_style_docu_area;
                else
                    p_style_docu_area_out = NULL;
                break;
            }
        }
    }

    return(p_style_docu_area_out);
}

/******************************************************************************
*
* work out what region to affect if we
* have to 'modify' the effect of an implied style region
*
******************************************************************************/

static void
style_docu_area_expand_implied(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    /* if we have character things, can't expand region */
    if(!style_selector_test(p_style_selector, &style_selector_font_spec))
    {
        /* expand to paragraph granularity */
        p_docu_area_out->tl.object_position.object_id =
        p_docu_area_out->br.object_position.object_id = OBJECT_ID_NONE;
        p_docu_area_out->tl.object_position.data =
        p_docu_area_out->br.object_position.data = -1;

        { /* make sure that we apply row alterations to row zero when we have a virtual row table */
        STYLE_SELECTOR temp;
        if(p_docu->flags.virtual_row_table
           &&
           !style_selector_bic(&temp, p_style_selector, &style_selector_row))
        {
            p_docu_area_out->tl.slr.row = 0;
            p_docu_area_out->br.slr.row = 1;
            p_docu_area_out->whole_row = 1;
            p_docu_area_out->whole_col = 0;
        }
        /* use incoming col positions */
        else
        {
            if(style_selector_test(p_style_selector, &style_selector_row))
            {
                p_docu_area_out->tl.slr.col = p_docu_area_in->tl.slr.col;
                p_docu_area_out->br.slr.col = p_docu_area_in->br.slr.col;
                p_docu_area_out->whole_row = p_docu_area_in->whole_row;
            }

            /* use incoming row positions */
            if(style_selector_test(p_style_selector, &style_selector_para_ruler))
            {
                p_docu_area_out->tl.slr.row = p_docu_area_in->tl.slr.row;
                p_docu_area_out->br.slr.row = p_docu_area_in->br.slr.row;
                p_docu_area_out->whole_col = p_docu_area_in->whole_col;
            }
        }
        } /*block*/

    }
}

/******************************************************************************
*
* modify the source of the specified effects
*
******************************************************************************/

extern void
style_effect_source_modify(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list,
    _InRef_     PC_POSITION p_position,
    _In_opt_count_c_(INLINE_OVH) PC_USTR_INLINE ustr_inline_in,
    _InRef_opt_ P_STYLE p_style_in)
{
    STYLE style;
    STYLE_BIT_NUMBER style_bit_number;

    if(NULL != ustr_inline_in)
    {
        assert(NULL == p_style_in);
        style_init(&style);
        status_assert(style_struct_from_ustr_inline(p_docu, &style, ustr_inline_in, &style_selector_all));
    }
    else
    {
        PTR_ASSERT(p_style_in);
        style_copy_into(&style, p_style_in, &style_selector_all);
    }

    /* find source region for each bit and modify it... */
    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit(&style.selector, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        STYLE_SELECTOR selector;
        P_STYLE_DOCU_AREA p_style_docu_area;

        style_selector_clear(&selector);
        style_selector_bit_set(&selector, style_bit_number);

        p_style_docu_area = style_effect_source_find(p_docu, &selector, p_position, p_h_style_list, TRUE);

        if( (NULL == p_style_docu_area)
            ||
            (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id))
        {
            /* implied region or no source region found - so make one */
            DOCU_AREA docu_area;
            docu_area_from_position(&docu_area, p_position);

            style_docu_area_expand_implied(p_docu, &docu_area, p_style_docu_area ? &p_style_docu_area->docu_area : &docu_area, &selector);

            {
            STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
            STYLE style_add;
            style_copy_into(&style_add, &style, &selector);
            STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style_add);
            status_assert(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
            } /*block*/
        }
        else
        {
            const P_STYLE p_style_source = p_style_from_docu_area(p_docu, p_style_docu_area);

            if(NULL != p_style_source)
            {
                style_free_resources(p_style_source, &selector);

                style_copy(p_style_source, &style, &selector);
                style_selector_bit_clear(&style.selector, style_bit_number);

                if(p_style_docu_area->style_handle)
                {
                    STYLE_CHANGED style_changed;

                    style_changed.style_handle = p_style_docu_area->style_handle;
                    style_selector_copy(&style_changed.selector, &selector);
                    status_assert(maeve_event(p_docu, T5_MSG_STYLE_CHANGED, &style_changed));
                }
                else
                {
                    STYLE_DOCU_AREA_CHANGED style_docu_area_changed;
                    DOCU_REFORMAT docu_reformat;

                    style_docu_area_changed.data_space = p_style_docu_area->data_space;
                    style_docu_area_changed.docu_area = p_style_docu_area->docu_area;
                    status_assert(maeve_event(p_docu, T5_MSG_STYLE_DOCU_AREA_CHANGED, &style_docu_area_changed));

                    docu_reformat.action = REFORMAT_XY;
                    docu_reformat.data_type = REFORMAT_DOCU_AREA;
                    docu_reformat.data_space = p_style_docu_area->data_space;
                    docu_reformat.data.docu_area = style_docu_area_changed.docu_area;
                    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
                }
            }
        }
    }

    style_free_resources(&style, &style_selector_all);
}

/******************************************************************************
*
* enumerate styles in list
*
******************************************************************************/

_Check_return_
extern STYLE_HANDLE /* new index out */
style_enum_styles(
    _DocuRef_   PC_DOCU p_docu,
    _OutRef_    P_P_STYLE p_p_style,
    _InoutRef_  P_STYLE_HANDLE p_style_handle /* -1 to start enum */)
{
    ARRAY_INDEX n_styles = array_elements(&p_docu->h_style_list);
    ARRAY_INDEX ix_style;

    profile_ensure_frame();

    for(ix_style = (*p_style_handle < 0) ? 1 : (*p_style_handle + 1); ix_style < n_styles; ++ix_style)
    {
        P_STYLE p_style = array_ptr(&p_docu->h_style_list, STYLE, ix_style);

        if(p_style->h_style_name_tstr)
        {
            *p_style_handle = ix_style;
            *p_p_style = p_style;
            return(ix_style);
        }
    }

    *p_p_style = NULL;
    return(-1);
}

/******************************************************************************
*
* free resources owned by a style
* but only if allowed by selector
*
******************************************************************************/

extern void
style_free_resources(
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    STYLE_SELECTOR selector;

    if(&p_style->selector != p_style_selector) /* SKS 04dec94 simplifies below tests */
        void_style_selector_and(&selector, &p_style->selector, p_style_selector);
    else
        style_selector_copy(&selector, &p_style->selector);

    /* free indirected resources */
    if(style_selector_bit_test(&selector, STYLE_SW_NAME))
        al_array_dispose(&p_style->h_style_name_tstr);

    if(style_selector_bit_test(&selector, STYLE_SW_CS_COL_NAME))
        al_array_dispose(&p_style->col_style.h_numform);

    if(style_selector_bit_test(&selector, STYLE_SW_RS_ROW_NAME))
        al_array_dispose(&p_style->row_style.h_numform);

    if(style_selector_bit_test(&selector, STYLE_SW_PS_TAB_LIST))
        al_array_dispose(&p_style->para_style.h_tab_list);

    if(style_selector_bit_test(&selector, STYLE_SW_PS_NUMFORM_NU))
        al_array_dispose(&p_style->para_style.h_numform_nu);

    if(style_selector_bit_test(&selector, STYLE_SW_PS_NUMFORM_DT))
        al_array_dispose(&p_style->para_style.h_numform_dt);

    if(style_selector_bit_test(&selector, STYLE_SW_PS_NUMFORM_SE))
        al_array_dispose(&p_style->para_style.h_numform_se);

    if(style_selector_bit_test(&selector, STYLE_SW_FS_NAME))
        font_spec_name_dispose(&p_style->font_spec);

    void_style_selector_bic(&p_style->selector, &p_style->selector, &selector);
}

/******************************************************************************
*
* return the style for a column
*
******************************************************************************/

extern void
style_from_col(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     COL col)
{
    POSITION position;

    position_init(&position);
    position.slr.col = col;

    style_from_position(p_docu, p_style, p_style_selector, &position, &p_docu->h_style_docu_area, position_col_in_docu_area, FALSE);

    trace_1(TRACE_APP_STYLE, TEXT("style_from_col: ") COL_TFMT, col);
}

/******************************************************************************
*
* read a style from a docu_area
*
******************************************************************************/

extern void
style_from_docu_area_no_indirection(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style_out,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area)
{
    if(p_style_docu_area->style_handle)
    {
        style_init(p_style_out);
        style_bit_set(p_style_out, STYLE_SW_HANDLE);
        p_style_out->style_handle = p_style_docu_area->style_handle;
    }
    else
    {
        const PC_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

        if(!IS_P_STYLE_NONE(p_style))
            *p_style_out = *p_style;
        else
            style_init(p_style_out);
    }
}

/******************************************************************************
*
* find the style at a given position - slr and sub_object position
*
******************************************************************************/

extern void
style_from_position(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InRef_     P_PROC_POSITION_COMPARE p_proc_position_compare,
    _InVal_     BOOL caret_check)
{
    ARRAY_INDEX style_docu_area_ix = array_elements(p_array_handle);

    /* assert that we are searching for something */
    assert(style_selector_any(p_style_selector));

    if(style_docu_area_ix) /* SKS 30apr95 stop array_ptr assert during close */
    {
        STYLE_SELECTOR selector;
        P_STYLE_DOCU_AREA p_style_docu_area;
        BOOL res = 1;

        style_selector_copy(&selector, p_style_selector);

        --style_docu_area_ix;
        p_style_docu_area = array_ptr(p_array_handle, STYLE_DOCU_AREA, style_docu_area_ix);

        /* look through region list backwards for containing region */
        for(; style_docu_area_ix >= 0; --style_docu_area_ix, --p_style_docu_area)
        {
            if(p_style_docu_area->deleted)
                continue;

            if(caret_check && p_style_docu_area->caret)
                p_style_docu_area->docu_area.br.object_position.data += 1;

            if((*p_proc_position_compare) (&p_style_docu_area->docu_area, p_position))
            {
                STYLE_SELECTOR temp;

                if(style_docu_area_selector_and(p_docu, &temp, p_style_docu_area, &selector))
                {
                    STYLE style;
                    PC_STYLE p_style;

                    /* call object to ask if implied style is active */
                    if( (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id)
                        &&
                        style_implied_query(p_docu, &style, p_style_docu_area, p_position))
                    {
                        p_style = &style;
                    }
                    else
                        p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

                    if(!IS_P_STYLE_NONE(p_style))
                    {
                        style_copy(p_style_out, p_style, &selector);
                        res = style_selector_bic(&selector, &selector, &p_style->selector);
                    }
                }
            }

            if(caret_check && p_style_docu_area->caret)
                p_style_docu_area->docu_area.br.object_position.data -= 1;

            if(!res)
                break;
        }

        /* assert that we found definitions for all bits */
#if TRACE_ALLOWED
        if(p_docu->flags.document_active)
            assert(!style_selector_any(&selector));
#endif

        trace_5(TRACE_APP_STYLE, TEXT("style_from_position: ") COL_TFMT TEXT(", row: ") ROW_TFMT TEXT(", data: ") S32_TFMT TEXT(", exit ix: ") S32_TFMT TEXT(", regions: ") S32_TFMT,
                p_position->slr.col, p_position->slr.row,
                (OBJECT_ID_NONE == p_position->object_position.object_id) ? -1 : p_position->object_position.data,
                style_docu_area_ix,
                array_elements(p_array_handle));
    }
}

/******************************************************************************
*
* return the style for a row
*
******************************************************************************/

extern void
style_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     ROW row)
{
    POSITION position;

    position_init(&position);
    position.slr.row = row;

    style_from_position(p_docu, p_style, p_style_selector, &position, &p_docu->h_style_docu_area, position_row_in_docu_area, FALSE);

    trace_1(TRACE_APP_STYLE, TEXT("style_from_row: ") ROW_TFMT, row);
}

/******************************************************************************
*
* given a cell reference and a style selector, update the supplied
* style structure with the selected effects for the cell
*
* the information in the structure must be disposed of when not needed anymore
*
******************************************************************************/

extern void
style_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_SLR p_slr)
{
    S32 entry = style_cache_slr_check(p_docu, p_style, p_style_selector, p_slr);

    if(entry == CACHE_HIT)
    {
        trace_2(TRACE_APP_STYLE, TEXT("*** cache hit style_from_slr: ") COL_TFMT TEXT(",") ROW_TFMT, p_slr->col, p_slr->row);
        return;
    }
    else
    {
        POSITION position;

        position_init(&position);
        position.slr = *p_slr;

        style_from_position(p_docu, p_style, p_style_selector, &position, &p_docu->h_style_docu_area, position_slr_in_docu_area, FALSE);

        style_cache_slr_insert(p_docu, p_style, p_style_selector, p_slr, entry);
        trace_2(TRACE_APP_STYLE, TEXT("style_from_slr ") COL_TFMT TEXT(",") ROW_TFMT, p_slr->col, p_slr->row);
    }
}

/******************************************************************************
*
* create a named style from a style structure
*
******************************************************************************/

_Check_return_
extern STATUS /* style handle */
style_handle_add(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style) /* always either stolen or discarded */
{
    {
    STYLE_HANDLE style_handle = STYLE_HANDLE_NONE;

    /* look for a style with the same name - SKS 28sep94 tries to allow temporary anonymous styles for layout editing */
    if(array_tstr(&p_style->h_style_name_tstr))
        style_handle = style_handle_from_name(p_docu, array_tstr(&p_style->h_style_name_tstr));

    if(status_done(style_handle)) /* ie style found with this name */
    {
        P_STYLE p_style_exist = p_style_from_handle(p_docu, style_handle);
        STYLE_CHANGED style_changed;
        BOOL diff;

        style_changed.style_handle = style_handle;
        void_style_selector_or(&style_changed.selector, &p_style->selector, &p_style_exist->selector);

        {
        STYLE_SELECTOR temp;
        diff = style_compare(&temp, p_style, p_style_exist);
        } /*block*/

        *p_style_exist = *p_style;

        if(diff)
            status_assert(maeve_event(p_docu, T5_MSG_STYLE_CHANGED, &style_changed));

        return(style_handle);
    }
    } /*block*/

    { /* allocate a style space for ourselves */
    STATUS status = STATUS_OK;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(STYLE), TRUE);
    U32 n_alloc = 1;
    P_STYLE p_style_new;

    if(0 == array_elements(&p_docu->h_style_list))
        /* avoid handle zero */
        n_alloc = 2;

    if(NULL != (p_style_new = al_array_extend_by(&p_docu->h_style_list, STYLE, n_alloc, &array_init_block, &status)))
    {
        STYLE_HANDLE style_handle = array_elements(&p_docu->h_style_list) - 1;
        if(n_alloc > 1)
            p_style_new += 1;
        *p_style_new = *p_style;
        return(style_handle);
    }

    style_free_resources_all(p_style);
    return(status);
    } /*block*/
}

/******************************************************************************
*
* return handle of base style for a document
*
******************************************************************************/

_Check_return_
extern STYLE_HANDLE
style_handle_base(
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    PC_STYLE_DOCU_AREA p_style_docu_area = array_rangec(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);
    STYLE_HANDLE style_handle = STYLE_HANDLE_NONE;

    profile_ensure_frame();

    /* base style is first base style region in list */
    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(p_style_docu_area->style_handle && (p_style_docu_area->region_class == REGION_BASE))
        {
            style_handle = p_style_docu_area->style_handle;
            break;
        }
    }

    return(style_handle);
}

/******************************************************************************
*
* return handle of current style for a document
* ***DO NOT USE THIS ROUTINE***
*
******************************************************************************/

_Check_return_
extern STYLE_HANDLE
style_handle_current(
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    P_STYLE_DOCU_AREA p_style_docu_area;
    STYLE_HANDLE style_handle = STYLE_HANDLE_NONE;

    profile_ensure_frame();

    for(i = n_regions - 1, p_style_docu_area = array_ptr(p_h_style_docu_area, STYLE_DOCU_AREA, i);
        i >= 0;
        --i, --p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if( (0 != p_style_docu_area->style_handle)
            &&
            (REGION_UPPER == p_style_docu_area->region_class)
            &&
            (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id)
            &&
            (T5_EXT_STYLE_CELL_CURRENT == p_style_docu_area->object_message.t5_message) )
        {
            style_handle = p_style_docu_area->style_handle;
            break;
        }
    }

    return(style_handle);
}

/******************************************************************************
*
* look through a region list for an occurence
* of the given style handle and find the earliest
* row on which it occurs
*
******************************************************************************/

_Check_return_
extern STATUS /* 1 == found one */
style_handle_find_in_docu_area_list(
    _OutRef_    P_ROW p_row /* minimum row of use */,
    _InRef_     PC_ARRAY_HANDLE p_h_docu_area_list,
    _InVal_     STYLE_HANDLE style_handle)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_docu_area_list);
    ARRAY_INDEX style_docu_area_ix;
    PC_STYLE_DOCU_AREA p_style_docu_area = array_rangec(p_h_docu_area_list, STYLE_DOCU_AREA, 0, n_regions);

    profile_ensure_frame();

    *p_row = MAX_ROW;

    for(style_docu_area_ix = 0; style_docu_area_ix < n_regions; ++style_docu_area_ix, ++p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(p_style_docu_area->style_handle == style_handle)
        {
            ROW row = p_style_docu_area->docu_area.whole_col ? 0 : p_style_docu_area->docu_area.tl.slr.row;

            if( *p_row > row)
                *p_row = row;
        }
    }

    return((*p_row < MAX_ROW) ? 1 : 0);
}

/******************************************************************************
*
* find a style given a name
*
******************************************************************************/

_Check_return_
extern STYLE_HANDLE
style_handle_from_name(
    _DocuRef_   PC_DOCU p_docu,
    _In_z_      PCTSTR name)
{
    P_STYLE p_style;
    STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;

    while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
    {
        PCTSTR tstr_style_name = array_tstr(&p_style->h_style_name_tstr);

        if(0x03 == *tstr_style_name) /* SKS 23jan95 skip unequal ctrlC */
            if(0x03 != *name)
                tstr_style_name++;

        if(0 == tstricmp(tstr_style_name, name))
            return(style_handle);
    }

    return(0);
}

/******************************************************************************
*
* modify an existing style handle
*
******************************************************************************/

extern void
style_handle_modify(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle                     /* handle to modify */,
    _InoutRef_  P_STYLE p_style_new                           /* data for modified effects */,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_modified   /* selector for data for modified effects */,
    _InRef_     PC_STYLE_SELECTOR p_style_selector            /* modified selector */)
{
    P_STYLE p_style = p_style_from_handle(p_docu, style_handle);
    STYLE_SELECTOR selector;
    STYLE_CHANGED style_changed;

    assert(!IS_P_STYLE_NONE(p_style));

    void_style_selector_or(&selector, p_style_selector, p_style_selector_modified);

    /* free all resources currently owned by listed style which it no longer owns,
     * and remove deselected effects from selector
     */
    style_free_resources(p_style, &selector);

    /* copy any modified bits of state into listed style,
     * and set listed style selector bits for new effects
     */
    style_copy(p_style, p_style_new, &selector);

    /* claim ownership of the things we got */
    void_style_selector_bic(&p_style_new->selector, &p_style_new->selector, &selector);

    style_changed.style_handle = style_handle;
    style_selector_copy(&style_changed.selector, &selector);
    status_assert(maeve_event(p_docu, T5_MSG_STYLE_CHANGED, &style_changed));
}

/******************************************************************************
*
* remove a style handle
*
******************************************************************************/

extern void
style_handle_remove(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle)
{
    if(array_index_valid(&p_docu->h_style_list, style_handle))
    {
        P_STYLE p_style = array_ptr_no_checks(&p_docu->h_style_list, STYLE, style_handle);
        STYLE_CHANGED style_changed;

        style_changed.style_handle = style_handle;
        style_selector_copy(&style_changed.selector, &p_style->selector);

        style_free_resources_all(p_style);

        status_assert(maeve_event(p_docu, T5_MSG_STYLE_CHANGED, &style_changed));
    }
}

/******************************************************************************
*
* find the use of a style handle in the region list
*
******************************************************************************/

_Check_return_
extern ARRAY_INDEX
style_handle_use_find(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list)
{
    ARRAY_INDEX style_docu_area_ix, style_docu_area_ix_out = -1;

    IGNOREPARM_DocuRef_(p_docu);

    profile_ensure_frame();

    /* look through region list top down for containing region */
    for(style_docu_area_ix = array_elements(p_h_style_list) - 1;
        style_docu_area_ix >= 0;
        --style_docu_area_ix)
    {
        P_STYLE_DOCU_AREA p_style_docu_area = array_ptr(p_h_style_list, STYLE_DOCU_AREA, style_docu_area_ix);

        if(p_style_docu_area->deleted)
            continue;

        if(!p_style_docu_area->base
           &&
           !p_style_docu_area->internal
           &&
           position_in_docu_area(&p_style_docu_area->docu_area, p_position)
           &&
           (OBJECT_ID_NONE == p_style_docu_area->object_message.object_id)
           &&
           p_style_docu_area->style_handle == style_handle)
        {
            style_docu_area_ix_out = array_indexof_element(p_h_style_list, STYLE_DOCU_AREA, p_style_docu_area);
            break;
        }
    }

    return(style_docu_area_ix_out);
}

/******************************************************************************
*
* enquire about active implied style
*
******************************************************************************/

_Check_return_ _Success_(return)
extern BOOL
style_implied_query(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style_out,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area,
    _InRef_     PC_POSITION p_position)
{
    if(OBJECT_ID_NONE != p_style_docu_area->object_message.object_id)
    {
        style_init(p_style_out);

        if(p_style_docu_area->region_class < p_docu->region_class_limit)
        {
            IMPLIED_STYLE_QUERY implied_style_query;
            implied_style_query.arg = p_style_docu_area->arg;
            implied_style_query.p_style_docu_area = p_style_docu_area;
            implied_style_query.p_style = p_style_out;
            implied_style_query.position = *p_position;
            status_consume(object_call_id(p_style_docu_area->object_message.object_id, p_docu, p_style_docu_area->object_message.t5_message, &implied_style_query));
        }

        return(TRUE);
    }

    return(FALSE);
}

/******************************************************************************
*
* given a style record with font spec
* and line space fields defined, calculate the line spacing
*
******************************************************************************/

_Check_return_
extern PIXIT
style_leading_from_style(
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_FONT_SPEC p_font_spec,
    _InVal_     BOOL draft_mode)
{
    /* calculate base line position */
    PIXIT leading = p_font_spec->size_y;

    if(!draft_mode) /* SKS 28nov94 allows true single and double spacing in draft without setting explicit spacings for RSA people */
        leading += p_font_spec->size_y / 5;

    switch(p_style->para_style.line_space.type)
    {
    case SF_LINE_SPACE_SINGLE:
        break;

    case SF_LINE_SPACE_ONEP5:
        leading += leading / 2;
        break;

    case SF_LINE_SPACE_DOUBLE:
        leading *= 2;
        break;

    case SF_LINE_SPACE_SET:
        leading = p_style->para_style.line_space.leading;
        break;
    }

    return(leading);
}

/******************************************************************************
*
* indicate what styles are used over the selection
* and their state - which may be fuzzy if the
* state is altered more than once
*
* remember to style_init the style before calling this
*
* MRJC re-written from slot based algorithm 16.1.95
*
* SKS 07jul96 re-written again 'cos 'twas garbage
*
******************************************************************************/

extern void
style_of_area(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style_out,
    _OutRef_    P_STYLE_SELECTOR p_style_selector_fuzzy_out,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_in,
    P_DOCU_AREA p_docu_area_in)
{
    STYLE_SELECTOR style_selector_loop;
    DOCU_AREA docu_area;

    style_init(p_style_out);
    docu_area_normalise(p_docu, &docu_area, p_docu_area_in);

    { /* read definition of style state at start of area */
    STYLE style;

    style_init(&style);
    style_from_position(p_docu, &style, p_style_selector_in, &docu_area.tl, &p_docu->h_style_docu_area, position_in_docu_area, TRUE);

    style_copy(p_style_out, &style, p_style_selector_in);

    /* all things not defined at the start of the region are fuzzy */
    void_style_selector_not(p_style_selector_fuzzy_out, &p_style_out->selector);
    } /*block*/

    style_selector_copy(&style_selector_loop, p_style_selector_in);

    /* look through region list backwards */
    if(style_selector_any(&style_selector_loop))
    {
        ARRAY_INDEX style_docu_area_ix = array_elements(&p_docu->h_style_docu_area);

        while(--style_docu_area_ix >= 0)
        {
            P_STYLE_DOCU_AREA p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, style_docu_area_ix);

            if(p_style_docu_area->deleted)
                continue;

            if(docu_area_intersect_docu_area(&p_style_docu_area->docu_area, &docu_area) && !docu_area_in_docu_area(&p_style_docu_area->docu_area, &docu_area))
            {
                const PC_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

                if(NULL != p_style)
                {
                    STYLE style_temp_a = *p_style;
                    STYLE style_temp_b = *p_style_out;

                    /* only compare bits of style we are currently interested in and that exist in both styles */
                    void_style_selector_and(&style_temp_a.selector, &style_temp_a.selector, &style_selector_loop);

                    if(style_selector_and(&style_temp_b.selector, &style_temp_b.selector, &style_temp_a.selector))
                    {   /* SKS 30sep96 only do style_compare when there is something to do */
                        STYLE_SELECTOR style_selector_diff;

                        if(style_compare(&style_selector_diff, &style_temp_a, &style_temp_b))
                        {
                            /* these bits are fuzzy */
                            void_style_selector_or(p_style_selector_fuzzy_out, p_style_selector_fuzzy_out, &style_selector_diff);
                            void_style_selector_bic(&style_selector_loop, &style_selector_loop, &style_selector_diff);
                            if(!style_selector_any(&style_selector_loop))
                                break; /* SKS 21aug95 get out early only on bit change not each time round loop */
                        }
                    }
                }
            }
        }
    }
}

static void
style_docu_colrow(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_old)
{
    P_STYLE_DOCU_AREA p_style_docu_area;
    ARRAY_INDEX array_index = -1;
    T5_MESSAGE t5_message = T5_EXT_STYLE_CELL_CURRENT;

    IGNOREPARM_InRef_(p_slr_old);

    if(NULL != (p_style_docu_area = style_docu_area_enum_implied(p_docu, &array_index, OBJECT_ID_IMPLIED_STYLE, &t5_message, NULL)))
        docu_area_from_slr(&p_style_docu_area->docu_area, &p_docu->cur.slr);
}

/*
main events
*/

static void
style_msg_caches_dispose(
    _DocuRef_   P_DOCU p_docu,
    P_CACHES_DISPOSE p_caches_dispose)
{
    style_cache_slr_dispose(p_docu, &p_caches_dispose->region);
    style_cache_sub_dispose(p_docu, DATA_NONE, &p_caches_dispose->region);
    skel_col_enum_cache_dispose(docno_from_p_docu(p_docu), &p_caches_dispose->region);
}

static void
style_msg_reformat(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat)
{
    REGION region;

    /* <<< if(NULL ==*/( region_from_docu_reformat(&region, p_docu_reformat))
        /* <<< return*/;

    style_cache_slr_dispose(p_docu, &region);
    style_cache_sub_dispose(p_docu, DATA_NONE, &region);
    skel_col_enum_cache_dispose(docno_from_p_docu(p_docu), &region);
}

static void
style_msg_style_changed(
    _DocuRef_   P_DOCU p_docu,
    P_STYLE_CHANGED p_style_changed)
{
    ROW row;

    /* work out what needs reformatting */
    if(style_handle_find_in_docu_area_list(&row, &p_docu->h_style_docu_area, p_style_changed->style_handle))
    {
        STYLE_DOCU_AREA_CHANGED style_docu_area_changed;
        style_docu_area_changed.data_space = DATA_SLOT;
        docu_area_init(&style_docu_area_changed.docu_area);
        style_docu_area_changed.docu_area.tl.slr.row = row;
        style_docu_area_changed.docu_area.br.slr.row = MAX_ROW;
        style_docu_area_changed.docu_area.whole_row = 1;
        status_assert(maeve_event(p_docu, T5_MSG_STYLE_DOCU_AREA_CHANGED, &style_docu_area_changed));

        if(style_selector_test(&p_style_changed->selector, &style_selector_extent_x))
            reformat_from_row(p_docu, row, REFORMAT_XY);
        else
            reformat_from_row(p_docu, row, REFORMAT_Y);
    }

    /* need a whole redraw if a mrofmun style has been altered */
    if(mrofmun_style_handle_in_use(p_docu, p_style_changed->style_handle))
        view_update_all(p_docu, UPDATE_PANE_CELLS_AREA);

    /* dispose of mrofmuns */
    mrofmun_dispose_list(p_docu);
}

static void
style_msg_style_docu_area_changed(
    _DocuRef_   P_DOCU p_docu,
    P_STYLE_DOCU_AREA_CHANGED p_style_docu_area_changed)
{
    REGION region;

    region_from_docu_area_max(&region, &p_style_docu_area_changed->docu_area);

    if(p_style_docu_area_changed->data_space == DATA_SLOT)
    {
        style_cache_slr_dispose(p_docu, &region);
        skel_col_enum_cache_dispose(docno_from_p_docu(p_docu), &region);
    }

    style_cache_sub_dispose(p_docu, p_style_docu_area_changed->data_space, &region);
}

static void
style_msg_style_docu_use_remove(
    _DocuRef_   P_DOCU p_docu,
    P_STYLE_USE_REMOVE p_style_use_remove)
{
    ROW row = style_use_remove(p_docu, &p_docu->h_style_docu_area, p_style_use_remove->style_handle);
    p_style_use_remove->row = MIN(row, p_style_use_remove->row);
}

MAEVE_EVENT_PROTO(static, maeve_event_style)
{
    const STATUS status = STATUS_OK;

    IGNOREPARM_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_CACHES_DISPOSE:
        style_msg_caches_dispose(p_docu, (P_CACHES_DISPOSE) p_data);
        break;

    case T5_MSG_REFORMAT:
        style_msg_reformat(p_docu, (PC_DOCU_REFORMAT) p_data);
        break;

    case T5_MSG_STYLE_CHANGED:
        style_msg_style_changed(p_docu, (P_STYLE_CHANGED) p_data);
        break;

    case T5_MSG_STYLE_DOCU_AREA_CHANGED:
        style_msg_style_docu_area_changed(p_docu, (P_STYLE_DOCU_AREA_CHANGED) p_data);
        break;

    case T5_MSG_STYLE_USE_REMOVE:
        style_msg_style_docu_use_remove(p_docu, (P_STYLE_USE_REMOVE) p_data);
        break;

    case T5_MSG_STYLE_USE_QUERY:
        style_use_query(p_docu, &p_docu->h_style_docu_area, (P_STYLE_USE_QUERY) p_data);
        break;

    /* MRJC 26.4.95: update current cell style docu_area when it changes */
    case T5_MSG_DOCU_COLROW:
        style_docu_colrow(p_docu, (PC_SLR) p_data);
        break;

    default:
        break;
    }

    return(status);
}

/******************************************************************************
*
* set region class limit
*
******************************************************************************/

extern void
style_region_class_limit_set(
    _DocuRef_   P_DOCU p_docu,
    _In_        REGION_CLASS region_class)
{
    STYLE_DOCU_AREA_CHANGED style_docu_area_changed;

    /* switch off regions above main */
    p_docu->region_class_limit = region_class;

    style_docu_area_changed.data_space = DATA_SLOT;
    docu_area_init(&style_docu_area_changed.docu_area);
    style_docu_area_changed.docu_area.whole_col =
    style_docu_area_changed.docu_area.whole_row = 1;

    /* tell style module to forget style caches */
    status_assert(maeve_event(p_docu, T5_MSG_STYLE_DOCU_AREA_CHANGED, &style_docu_area_changed));
}

/******************************************************************************
*
* work out whether to save a docu_area or not
*
******************************************************************************/

_Check_return_
extern BOOL
style_save_docu_area_save_from_index(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area,
    _InRef_     PC_DOCU_AREA p_docu_area_save,
    _In_        UBF save_index)
{
    BOOL do_save = FALSE;

    if(p_style_docu_area->deleted || p_style_docu_area->internal)
        return(FALSE);

    switch(save_index)
    {
    /* single fragment being saved (to clipboard 12.10.93) */
    case DATA_SAVE_CHARACTER:
        if(docu_area_in_docu_area(p_docu_area_save, &p_style_docu_area->docu_area))
            if(!p_style_docu_area->base)
            {
                const PC_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

                if(!IS_P_STYLE_NONE(p_style))
                    if(style_selector_test(&p_style->selector, &style_selector_font_spec))
                        do_save = TRUE;
            }
        break;

    /* clipboard save more than 1 frag */
    case DATA_SAVE_MANY:
        if(docu_area_in_docu_area(p_docu_area_save, &p_style_docu_area->docu_area))
            do_save = !p_style_docu_area->base;
        break;

    /* partial file save */
    case DATA_SAVE_DOC:

        /*FALLTHRU*/

    /* normal whole file save */
    case DATA_SAVE_WHOLE_DOC:
        if(docu_area_intersect_docu_area(p_docu_area_save, &p_style_docu_area->docu_area))
            do_save = TRUE;
        break;
    }

    return(do_save);
}

/******************************************************************************
*
* update the supplied style structure with the guts of a named style
*
******************************************************************************/

extern void
style_struct_from_handle(
    _DocuRef_   PC_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InVal_     STYLE_HANDLE style_handle,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    profile_ensure_frame();

    if(!array_index_valid(&p_docu->h_style_list, style_handle))
    {
        assert(style_handle < array_elements(&p_docu->h_style_list));
        return;
    }

    style_copy(p_style, p_style_from_handle(p_docu, style_handle), p_style_selector);
}

/******************************************************************************
*
* produce an array of style sub changes for an object
* send NULL for p_slr if using header/footer style list
*
******************************************************************************/

_Check_return_
extern ARRAY_HANDLE /*h_style_sub_changes*/
style_sub_changes(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_OBJECT_POSITION p_object_position,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list)
{
    POSITION position;
    ARRAY_HANDLE h_style_sub_changes;

    position_from_data_ref(p_docu, &position, p_data_ref, p_object_position); /* SKS 18may95 made into a function */

    if(0 != (h_style_sub_changes = style_cache_sub_check(p_docu, p_style_selector, p_data_ref)))
    {
        trace_2(TRACE_APP_STYLE, TEXT("*** cache hit style_sub: ") COL_TFMT TEXT(",") ROW_TFMT, p_data_ref->arg.slr.col, p_data_ref->arg.slr.row);
    }
    else
    {
        ARRAY_HANDLE h_style_change_object;

        trace_2(TRACE_APP_STYLE, TEXT("*** style_sub_changes: ") COL_TFMT TEXT(",") ROW_TFMT, p_data_ref->arg.slr.col, p_data_ref->arg.slr.row);

        if((h_style_change_object = style_change_in_object(p_docu, p_style_selector, &position.slr, p_h_style_list)) != 0)
        {
            STATUS status;
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(STYLE_SUB_CHANGE), FALSE);
            const ARRAY_INDEX n_style_change_object = array_elements(&h_style_change_object);
            ARRAY_INDEX ix;
            PC_S32 p_s32 = array_rangec(&h_style_change_object, S32, 0, n_style_change_object);

            for(ix = 0; (ix < n_style_change_object) && (*p_s32 < S32_MAX); ++ix, ++p_s32)
            {
                P_STYLE_SUB_CHANGE p_style_sub_change;

                if(NULL == (p_style_sub_change = al_array_extend_by(&h_style_sub_changes, STYLE_SUB_CHANGE, 1, &array_init_block, &status)))
                {
                    status_assert(status);
                    al_array_dispose(&h_style_sub_changes);
                    break;
                }

                position.object_position.data = *p_s32;
                p_style_sub_change->position = position;
                style_init(&p_style_sub_change->style);
                style_from_position(p_docu, &p_style_sub_change->style, p_style_selector, &position, p_h_style_list, position_in_docu_area, FALSE);
            }

            al_array_dispose(&h_style_change_object);
        }

        if(0 != h_style_sub_changes)
        {
            /* garbage collect */
            AL_GARBAGE_FLAGS al_garbage_flags;
            AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
            al_garbage_flags.shrink = 1;
            consume(S32, al_array_garbage_collect(&h_style_sub_changes, 0, NULL, al_garbage_flags));

            style_cache_sub_insert(p_docu, &h_style_sub_changes, p_style_selector, p_data_ref);
        }
    }

    return(h_style_sub_changes);
}

/******************************************************************************
*
* handle uref events for style regions
*
******************************************************************************/

static BOOL
style_uref_current_cell(
    P_STYLE_DOCU_AREA p_style_docu_area,
    _InVal_     T5_MESSAGE t5_message)
{
    /* MRJC: 27.4.95 check for current cell region which wants uref-ing only sometimes */
    if( (OBJECT_ID_NONE != p_style_docu_area->object_message.object_id)
        &&
        (p_style_docu_area->object_message.t5_message == T5_EXT_STYLE_CELL_CURRENT)
        &&
        (t5_message != T5_MSG_UREF_CLOSE1) )
    {
        return(TRUE);
    }

    return(FALSE);
}

PROC_UREF_EVENT_PROTO(static, proc_uref_event_style)
{
    switch(p_uref_event_block->reason.code)
    {
    case DEP_DELETE: /* dependency must be deleted */
        {
        switch(t5_message)
        {
        /* free a region */
        default:
            {
            /* style list is not freed till CLOSE2;
             * then we can dispose of style region list, too
             */
            if(p_uref_event_block->uref_id.client_handle != STYLE_LIST_DEP)
            {
                P_STYLE_DOCU_AREA p_style_docu_area;

                trace_6(TRACE_APP_UREF,
                        TEXT("style_uref_event CLOSE1 tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT("; br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(", whole_col: ") S32_TFMT TEXT(", whole_row: ") S32_TFMT,
                        p_uref_event_block->uref_id.region.tl.col,
                        p_uref_event_block->uref_id.region.tl.row,
                        p_uref_event_block->uref_id.region.br.col,
                        p_uref_event_block->uref_id.region.br.row,
                        (S32) p_uref_event_block->uref_id.region.whole_col,
                        (S32) p_uref_event_block->uref_id.region.whole_row);

                if((NULL != (p_style_docu_area = p_style_docu_area_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle)))
                   &&
                   !style_uref_current_cell(p_style_docu_area, t5_message))
                    style_docu_area_delete(p_docu, p_style_docu_area);
                else
                    assert0();
            }

            break;
            }

        /* free the resources owned by styles */
        case T5_MSG_UREF_CLOSE2:
            {
            P_STYLE p_style;
            STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;

            trace_0(TRACE_APP_UREF, TEXT("style_uref_event CLOSE2"));

            { /* garbage collect and free style region list */
            AL_GARBAGE_FLAGS al_garbage_flags;
            AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
            al_garbage_flags.remove_deleted = 1;
            al_garbage_flags.shrink = 1;
            al_garbage_flags.may_dispose = 1; /*hopefully does so*/
            consume(S32, al_array_garbage_collect(&p_docu->h_style_docu_area, 0, style_docu_area_deleted, al_garbage_flags));
            assert(0 == p_docu->h_style_docu_area);
            } /*block*/

            while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
                style_free_resources_all(p_style);

            mrofmun_dispose_list(p_docu);

            al_array_dispose(&p_docu->h_style_list);

            /* free style cache */
            al_array_dispose(&p_docu->h_style_cache_slr);
            al_array_dispose(&p_docu->h_style_cache_sub);

            /* remove style list dependency */
            uref_del_dependency(docno_from_p_docu(p_docu), p_uref_event_block->uref_id.uref_handle);

            break;
            }
        }

        break;
        }

    case DEP_UPDATE: /* dependency region must be updated */
    case DEP_INFORM:
        {
        P_STYLE_DOCU_AREA p_style_docu_area;

        /* find our entry */
        if(NULL != (p_style_docu_area = p_style_docu_area_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle)))
        {
            if(!style_uref_current_cell(p_style_docu_area, t5_message))
            {
                BOOL delete_it = FALSE;
                S32 res;

                res = uref_match_docu_area(&p_style_docu_area->docu_area, t5_message, p_uref_event_block);

                if(res == DEP_DELETE)
                    delete_it = TRUE;
                else if(res != DEP_NONE
                        &&
                        t5_message == T5_MSG_UREF_OVERWRITE
                        &&
                        !p_style_docu_area->uref_hold
                        &&
                        docu_area_is_frag(&p_style_docu_area->docu_area))
                {
                    /* delete overwritten regions less than a cell */
                    REGION region;
                    region_from_docu_area_max(&region, &p_style_docu_area->docu_area);
                    if(region_in_region(&p_uref_event_block->uref_parms.source.region, &region))
                        delete_it = TRUE;
                }

                if(delete_it)
                    style_docu_area_delete(p_docu, p_style_docu_area);
                else if(res != DEP_NONE)
                {
                    REGION region;
                    region_from_docu_area_max(&region, &p_style_docu_area->docu_area);
                    style_cache_slr_dispose(p_docu, &region);
                    style_cache_sub_dispose(p_docu, DATA_SLOT, &region);
                }
            }
        }

        break;
        }

    default: default_unhandled(); break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* answer a style use query for a style list
*
******************************************************************************/

extern void
style_use_query(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_style_docu_area,
    P_STYLE_USE_QUERY p_style_use_query)
{
    const ARRAY_INDEX n_elements = array_elements(&p_style_use_query->h_style_use);
    ARRAY_INDEX style_use_entry_ix;
    P_STYLE_USE_QUERY_ENTRY p_style_use_query_entry = array_range(&p_style_use_query->h_style_use, STYLE_USE_QUERY_ENTRY, 0, n_elements);

    for(style_use_entry_ix = 0; style_use_entry_ix < n_elements; ++style_use_entry_ix, ++p_style_use_query_entry)
    {
        if(!p_style_use_query_entry->use)
        {
            /* rather similar to style_handle_find_in_docu_area_list but also has intersection */
            const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
            ARRAY_INDEX style_docu_area_ix;
            PC_STYLE_DOCU_AREA p_style_docu_area = array_rangec(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);

            for(style_docu_area_ix = 0; style_docu_area_ix < n_regions; ++style_docu_area_ix, ++p_style_docu_area)
            {
                if(p_style_docu_area->deleted)
                    continue;

                if(p_style_docu_area->region_class >= p_style_use_query->region_class)
                    continue; /* SKS allows us to avoid saving CurrentCell style etc. in CHARACTER/MANY */

                if((p_style_docu_area->style_handle == p_style_use_query_entry->style_handle)
                   &&
                   style_save_docu_area_save_from_index(p_docu,
                                                        p_style_docu_area,
                                                        &p_style_use_query->docu_area,
                                                        p_style_use_query->data_class))
                {
                    p_style_use_query_entry->use++;
                    break;
                }
            }
        }
    }
}

/******************************************************************************
*
* remove all uses of a style from a style list
*
******************************************************************************/

/*ncr*/
extern ROW
style_use_remove(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InVal_     STYLE_HANDLE style_handle)
{
    const ARRAY_INDEX n_regions = array_elements(p_h_style_docu_area);
    ARRAY_INDEX i;
    P_STYLE_DOCU_AREA p_style_docu_area = array_range(p_h_style_docu_area, STYLE_DOCU_AREA, 0, n_regions);
    ROW row = MAX_ROW;

    for(i = 0; i < n_regions; ++i, ++p_style_docu_area)
    {
        if(p_style_docu_area->deleted)
            continue;

        if(!p_style_docu_area->base
           &&
           !p_style_docu_area->internal
           &&
           p_style_docu_area->style_handle == style_handle)
        {
            ROW row_s;
            limits_from_docu_area(p_docu, NULL, NULL, &row_s, NULL, &p_style_docu_area->docu_area);
            row = MIN(row, row_s);
            style_docu_area_delete(p_docu, p_style_docu_area);
        }
    }

    return(row);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_styl);

_Check_return_
static STATUS
sk_styl_msg_startup(void)
{
    /****** set up bitmap constants ******/

    /* set all set bitmap */
    bitmap_set(style_selector_all.bitmap, N_BITS_ARG(STYLE_SW_COUNT));

    /* set ruler style selector */
    style_selector_clear(&style_selector_para_ruler);
    style_selector_bit_set(&style_selector_para_ruler, STYLE_SW_CS_WIDTH);
    style_selector_bit_set(&style_selector_para_ruler, STYLE_SW_PS_MARGIN_LEFT);
    style_selector_bit_set(&style_selector_para_ruler, STYLE_SW_PS_MARGIN_RIGHT);
    style_selector_bit_set(&style_selector_para_ruler, STYLE_SW_PS_MARGIN_PARA);
    style_selector_bit_set(&style_selector_para_ruler, STYLE_SW_PS_TAB_LIST);

    /* set text inline style selector */
    style_selector_clear(&style_selector_font_spec);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_UNDERLINE);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_BOLD);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_ITALIC);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_SUPERSCRIPT);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_SUBSCRIPT);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_NAME);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_SIZE_X);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_SIZE_Y);
    style_selector_bit_set(&style_selector_font_spec, STYLE_SW_FS_COLOUR);

    /* set style leading selector */
    style_selector_clear(&style_selector_para_leading);
    style_selector_bit_set(&style_selector_para_leading, STYLE_SW_FS_SIZE_Y);
    style_selector_bit_set(&style_selector_para_leading, STYLE_SW_PS_LINE_SPACE);

    /* text global selector */
    style_selector_para_text = style_selector_para_ruler;
    void_style_selector_or(&style_selector_para_text, &style_selector_para_text, &style_selector_para_leading);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_LINE_SPACE);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_JUSTIFY);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_JUSTIFY_V);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_PARA_START);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_PARA_END);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_RGB_BACK);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_NUMFORM_NU);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_NUMFORM_DT);
    style_selector_bit_set(&style_selector_para_text, STYLE_SW_PS_NUMFORM_SE);

    /* format selector */
    style_selector_para_format = style_selector_para_text;
    style_selector_bit_set(&style_selector_para_format, STYLE_SW_PS_NEW_OBJECT);

    /* set border selector */
    style_selector_clear(&style_selector_para_border);
    style_selector_bit_set(&style_selector_para_border, STYLE_SW_PS_RGB_BORDER);
    style_selector_bit_set(&style_selector_para_border, STYLE_SW_PS_BORDER);

    /* set grid selector */
    style_selector_clear(&style_selector_para_grid);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_RGB_GRID_LEFT);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_RGB_GRID_TOP);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_RGB_GRID_RIGHT);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_RGB_GRID_BOTTOM);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_GRID_LEFT);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_GRID_TOP);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_GRID_RIGHT);
    style_selector_bit_set(&style_selector_para_grid, STYLE_SW_PS_GRID_BOTTOM);

    /* bits which affect page x extent */
    style_selector_clear(&style_selector_extent_x);
    style_selector_bit_set(&style_selector_extent_x, STYLE_SW_CS_WIDTH);

    /* set col style selector */
    style_selector_clear(&style_selector_col);
    style_selector_bit_set(&style_selector_col, STYLE_SW_CS_WIDTH);
    style_selector_bit_set(&style_selector_col, STYLE_SW_CS_COL_NAME);

    /* set row style selector */
    style_selector_clear(&style_selector_row);
    style_selector_bit_set(&style_selector_row, STYLE_SW_RS_HEIGHT);
    style_selector_bit_set(&style_selector_row, STYLE_SW_RS_HEIGHT_FIXED);
    style_selector_bit_set(&style_selector_row, STYLE_SW_RS_UNBREAKABLE);

    /* paragraph granularity */
    style_selector_para = style_selector_all;
    void_style_selector_bic(&style_selector_para, &style_selector_para, &style_selector_font_spec);
    style_selector_bit_clear(&style_selector_para, STYLE_SW_NAME);
    style_selector_bit_clear(&style_selector_para, STYLE_SW_KEY);
    void_style_selector_bic(&style_selector_para, &style_selector_para, &style_selector_row);
    style_selector_bit_clear(&style_selector_para, STYLE_SW_RS_ROW_NAME);

    /* numforms */
    style_selector_clear(&style_selector_numform);
    style_selector_bit_set(&style_selector_numform, STYLE_SW_PS_NUMFORM_NU);
    style_selector_bit_set(&style_selector_numform, STYLE_SW_PS_NUMFORM_DT);
    style_selector_bit_set(&style_selector_numform, STYLE_SW_PS_NUMFORM_SE);

    return(STATUS_OK);
}

_Check_return_
static STATUS
sk_styl_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;

    p_docu->h_style_list = 0;
    p_docu->h_style_docu_area = 0;
    p_docu->region_class_limit = REGION_END;
    p_docu->h_style_cache_slr = 0;
    p_docu->h_style_cache_sub = 0;
    p_docu->h_mrofmun = 0;

    /*****************************/

    { /* initialise named style list and create dependency */
    UREF_HANDLE uref_handle; /* not stored */
    REGION region = REGION_INIT;
    region.whole_col = TRUE;
    region.whole_row = TRUE;
    status_return(status = uref_add_dependency(p_docu, &region, proc_uref_event_style, STYLE_LIST_DEP, &uref_handle, FALSE));
    } /*block*/

    /*if(DOCNO_CONFIG != docno_from_p_docu(p_docu))*/ /* SKS 26apr95 ignore for config document */ /* SKS 15aug14 put back now we load a config document with styles */
    {
        if(status_ok(status = maeve_event_handler_add(p_docu, maeve_event_style, (CLIENT_HANDLE) 0)))
            status = style_docu_area_add_internal(p_docu, NULL, DATA_SLOT);
    }

    return(status);
}

_Check_return_
static STATUS
sk_styl_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_style, (CLIENT_HANDLE) 0);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_styl_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(sk_styl_msg_startup());

    case T5_MSG_IC__INIT1:
        return(sk_styl_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(sk_styl_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_styl)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_styl_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of sk_styl.c */
