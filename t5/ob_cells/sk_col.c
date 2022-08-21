/* sk_col.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Column routines */

/* MRJC January 1992 */

#include "common/gflags.h"

#ifndef    __cells_ob_cells_h
#include "ob_cells/ob_cells.h"
#endif

/*
internal routines
*/

_Check_return_
static S32
page_x_extent(
    _DocuRef_   P_DOCU p_docu);

/*
local data
*/

typedef struct COL_ENUM_CACHE
{
    ARRAY_HANDLE h_col_info;
    DOCNO docno;
    U8 locked;
    U8 _spare[2];
    COL col;
    ROW row;
    PAGE page;
}
COL_ENUM_CACHE, * P_COL_ENUM_CACHE; typedef const COL_ENUM_CACHE * PC_COL_ENUM_CACHE;

#define COL_ENUM_CACHE_SIZE 32

static ARRAY_HANDLE h_col_enum_cache;

/******************************************************************************
*
* work out which column contains a skeleton point
* >=0 column number of column containing point
* < 0 point is not in column
*
******************************************************************************/

_Check_return_
extern COL
col_at_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_col_info,
    _InRef_     PC_SKEL_POINT p_skel_point)
{
    COL res = -1;
    const ARRAY_INDEX n_elements = array_elements(&h_col_info);
    ARRAY_INDEX i;
    PC_COL_INFO p_col_info = array_range(&h_col_info, COL_INFO, 0, n_elements);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    for(i = 0; i < n_elements; ++i, ++p_col_info)
    {
        /* is column rhs past the input point ? */
        if(edge_skel_point_compare(&p_col_info->edge_right, p_skel_point, x) > 0)
        {
            /* if the point is to the right of lhs, then point is in column */
            if(edge_skel_point_compare(&p_col_info->edge_left, p_skel_point, x) <= 0)
                res = p_col_info->col;
            break;
        }
    }

    return(res);
}

/******************************************************************************
*
* produce an array of row numbers which represent
* rows which contain a change of column width from the previous row
*
******************************************************************************/

_Check_return_
extern ARRAY_HANDLE
col_changes_between_rows(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_start,
    _InVal_     ROW row_end)
{
    STYLE_SELECTOR selector;
    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_CS_WIDTH);
    return(style_change_between_rows(p_docu, row_start, row_end, &selector));
}

/******************************************************************************
*
* work out which column contains a skeleton point
* points 'drift to the left' if they are not contained
* by any column; i.e. gaps are considered owned by the
* column immediately to their left
*
******************************************************************************/

_Check_return_
extern COL
col_left_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_col_info,
    _InRef_     PC_SKEL_POINT p_skel_point)
{
    COL res = 0;
    const ARRAY_INDEX n_elements = array_elements(&h_col_info);
    ARRAY_INDEX i;
    PC_COL_INFO p_col_info = array_range(&h_col_info, COL_INFO, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_col_info)
    {
        /* is column rhs past the input point ? */
        res = p_col_info->col;
        if(edge_skel_point_compare(&p_col_info->edge_right, p_skel_point, x) > 0)
        {
            /* if the column lhs is past the input point too,
             * assume the point belongs to the previous column
             */
            if(res && edge_skel_point_compare(&p_col_info->edge_left, p_skel_point, x) >= 0)
                --res;
            break;
        }
    }

    /* stick on rightmost column */
    if(res == n_cols_logical(p_docu))
        res -= 1;

    return(res);
}

/******************************************************************************
*
* return width of a column
*
******************************************************************************/

_Check_return_
extern PIXIT
col_width(
    _InVal_     ARRAY_HANDLE h_col_info,
    _InVal_     COL col)
{
    const COL start_col = array_basec(&h_col_info, COL_INFO)->col;
    const PC_COL_INFO p_col_info = array_ptrc(&h_col_info, COL_INFO, col - start_col);
    return(p_col_info->edge_right.pixit - p_col_info->edge_left.pixit);
}

/******************************************************************************
*
* ask objects if they have their own extent applied to a document
*
******************************************************************************/

extern void
find_object_extent(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SLR p_slr_out)
{
    p_slr_out->col = 0;
    p_slr_out->row = 0;

    { /* ask objects about their cells extent */
    OBJECT_ID object_id = OBJECT_ID_ENUM_START;
    while(status_ok(object_next(&object_id)))
    {
        CELLS_EXTENT cells_extent;
        zero_struct(cells_extent);
        if(status_ok(status_wrap(object_call_id(object_id, p_docu, T5_MSG_CELLS_EXTENT, &cells_extent))))
        {
            p_slr_out->col = MAX(p_slr_out->col, cells_extent.region.br.col);
            p_slr_out->row = MAX(p_slr_out->row, cells_extent.region.br.row);
        }
    }
    } /*block*/
}

/******************************************************************************
*
* find the widest non-blank part of a document
*
******************************************************************************/

/*ncr*/
static PAGE
find_widest(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_opt_ P_SLR p_slr_widest,
    _InVal_     BOOL blank)
{
    PAGE page_x = last_page_x(p_docu) - 1;
    SLR slr_widest;

    if(blank)
    {
        slr_widest.col = n_cols_logical(p_docu) - 1;
        slr_widest.row = 0;
    }
    else
    {
        /* find last non-blank column */
        COL col = n_cols_physical(p_docu);

        while(--col >= 0)
        {
            if(!cells_block_is_blank(p_docu, col, col + 1, 0, n_rows(p_docu)))
                break;
        }

        slr_widest.col = col;
        slr_widest.row = 0;

        {
        SLR slr_extent;
        find_object_extent(p_docu, &slr_extent);
        slr_widest.col = MAX(slr_extent.col - 1, slr_widest.col);
        } /*block*/
    }

    {
    ARRAY_HANDLE h_row_changes;

    /* get list of changes of column information */
    if((h_row_changes = col_changes_between_rows(p_docu, 0, n_rows(p_docu))) != 0)
    {
        P_ROW p_row;
        ARRAY_INDEX row_index;
        PAGE page_col;
        SKEL_EDGE skel_edge_rhs;

        page_col = 0;
        skel_edge_rhs.page = 0;
        skel_edge_rhs.pixit = 0;

        /* for each different row... */
        for(row_index = 0, p_row = array_ptr(&h_row_changes, ROW, row_index);
            row_index < array_elements(&h_row_changes);
            ++row_index, ++p_row)
        {
            SLR slr;
            SKEL_RECT skel_rect;
            slr.col = slr_widest.col;
            slr.row = *p_row;
            skel_rect_from_slr(p_docu, &skel_rect, &slr);
            page_col = MAX(page_col, skel_rect.tl.page_num.x);

            if(p_slr_widest && skel_point_edge_compare(&skel_rect.br, &skel_edge_rhs, x) > 0)
            {
                slr_widest.row = *p_row;
                edge_set_from_skel_point(&skel_edge_rhs, &skel_rect.br, x);
            }
        }

        page_x = MIN(page_x, page_col);
    }

    al_array_dispose(&h_row_changes);
    } /*block*/

    if(NULL != p_slr_widest)
        *p_slr_widest = slr_widest;

    return(page_x);
}

/******************************************************************************
*
* return last x page in document
*
******************************************************************************/

_Check_return_
extern PAGE
last_page_x(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.x_extent_changed)
    {
        p_docu->_last_page_x = page_x_extent(p_docu);
        p_docu->flags.x_extent_changed = 0;
    }

    return(p_docu->_last_page_x);
}

_Check_return_
extern PAGE
last_page_x_non_blank(
    _DocuRef_   P_DOCU p_docu)
{
    PAGE page_x = find_widest(p_docu, NULL, FALSE);

    trace_1(TRACE_APP_FORMAT, TEXT("$$$$$$$$$$$$ non_blank_height: ") PIXIT_TFMT, non_blank_height(p_docu));

    return(page_x + 1);
}

/******************************************************************************
*
* return last y page in document
*
******************************************************************************/

_Check_return_
extern PAGE
last_page_y(
    _DocuRef_   P_DOCU p_docu)
{
    return(p_docu->_last_page_y);
}

_Check_return_
extern PAGE
last_page_y_non_blank(
    _DocuRef_   P_DOCU p_docu)
{
    PAGE page_y;
    ROW row_last = n_rows(p_docu);
    SLR slr_extent;

    find_object_extent(p_docu, &slr_extent);

    for(page_y = last_page_y(p_docu) - 1; page_y > 0; --page_y)
    {
        ROW row = row_from_page_y(p_docu, page_y);

        if( (row <= slr_extent.row) ||
            !cells_block_is_blank(p_docu, 0, n_cols_physical(p_docu), row, row_last))
            break;

        row_last = row;
    }

    return(page_y + 1);
}

/******************************************************************************
*
* work out the maximum number of columns in a group of rows
*
******************************************************************************/

_Check_return_
extern COL
n_cols_max(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e)
{
    COL col_max = 0;
    ROW row;

    for(row = row_s; row < row_e; row += 1)
    {
        COL col = n_cols_row(p_docu, row);
        col_max = MAX(col_max, col);
    }

    return(col_max);
}

/******************************************************************************
*
* synthesise the number of columns in a row
* by removing all those columns on the rhs
* with widths of zero
*
* hopefully when the correct data structure
* is made later, this data can be got from there
* rather than trying to work it out
*
******************************************************************************/

_Check_return_
extern COL
n_cols_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    ARRAY_HANDLE h_col_info;
    COL col = 0;

    status_assert(skel_col_enum(p_docu, row, (PAGE) -1, (COL) -1, &h_col_info));

    if(0 != array_elements(&h_col_info))
    {
        PC_COL_INFO p_col_info;

        for(col = n_cols_logical(p_docu) - 1, p_col_info = array_ptrc(&h_col_info, COL_INFO, col); col >= 0; --col, --p_col_info)
            if(p_col_info->edge_right.pixit - p_col_info->edge_left.pixit != 0)
                break;

        col += 1;
    }

    return(col);
}

/******************************************************************************
*
* find the non-blank height of a document
* - ignores pages
*
******************************************************************************/

_Check_return_
extern PIXIT
non_blank_height(
    _DocuRef_   P_DOCU p_docu)
{
    PIXIT height = 0;
    ROW row = n_rows(p_docu);

    /* find last blank row */
    while(row > 0)
    {
        --row;

        if(!cells_block_is_blank(p_docu, 0, n_cols_physical(p_docu), row, row + 1))
            break;
    }

    {
    ROW row_limit = row + 1;
    for(row = 0; row < row_limit; row += 1)
    {
        ROW_ENTRY row_entry;
        row_entry_from_row(p_docu, &row_entry, row);
        height += row_entry.rowtab.edge_bot.pixit - row_entry.rowtab.edge_top.pixit;
    }
    } /*block*/

    return(height);
}

_Check_return_
extern PIXIT
total_docu_height(
    _DocuRef_   P_DOCU p_docu)
{
    PIXIT height = 0;
    ROW row = n_rows(p_docu) - 1;

    {
    ROW row_limit = row + 1;
    for(row = 0; row < row_limit; row += 1)
    {
        ROW_ENTRY row_entry;
        row_entry_from_row(p_docu, &row_entry, row);
        height += row_entry.rowtab.edge_bot.pixit - row_entry.rowtab.edge_top.pixit;
    }
    } /*block*/

    return(height);
}

/******************************************************************************
*
* return the non-blank width of a document
* - ignores pages
*
******************************************************************************/

_Check_return_
extern PIXIT
non_blank_width(
    _DocuRef_   P_DOCU p_docu)
{
    SLR slr_widest, slr;
    STYLE style;
    PIXIT width = 0;
    STYLE_SELECTOR selector;

    consume(PAGE, find_widest(p_docu, &slr_widest, FALSE));

    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_CS_WIDTH);
    style_init(&style);
    for(slr.col = 0, slr.row = slr_widest.row; slr.col <= slr_widest.col; ++slr.col)
    {
        style_from_slr(p_docu, &style, &selector, &slr);
        width += style.col_style.width;
    }

    return(width);
}

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
extern PIXIT
total_docu_width(
    _DocuRef_   P_DOCU p_docu)
{
    SLR slr_widest, slr;
    STYLE style;
    PIXIT width = 0;
    STYLE_SELECTOR selector;

    consume(PAGE, find_widest(p_docu, &slr_widest, TRUE));

    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_CS_WIDTH);
    style_init(&style);
    for(slr.col = 0, slr.row = slr_widest.row; slr.col <= slr_widest.col; ++slr.col)
    {
        style_from_slr(p_docu, &style, &selector, &slr);
        width += style.col_style.width;
    }

    return(width);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* work out columns and page extents for a document
*
******************************************************************************/

_Check_return_
static PAGE
page_x_extent(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_HANDLE h_row_changes;
    PAGE page_max = -1;

    trace_0(TRACE_APP_FORMAT, TEXT("--- page_x_extent in"));

    /* get list of changes of column information */
    if((h_row_changes = col_changes_between_rows(p_docu, 0, n_rows(p_docu))) != 0)
    {
        const ARRAY_INDEX n_row_changes = array_elements(&h_row_changes);
        ARRAY_INDEX row_index;
        P_ROW p_row = array_range(&h_row_changes, ROW, 0, n_row_changes);

        /* for each different row... */
        for(row_index = 0; row_index < n_row_changes; ++row_index, ++p_row)
        {
            ARRAY_HANDLE h_col_info;

            status_assert(skel_col_enum(p_docu, *p_row, (PAGE) -1, (COL) -1, &h_col_info));

            if(0 != array_elements(&h_col_info))
            {
                const PC_COL_INFO p_col_info = array_ptrc(&h_col_info, COL_INFO, array_elements(&h_col_info) - 1);

                page_max = MAX(page_max, p_col_info->edge_right.page);
            }
        }
    }

    trace_0(TRACE_APP_FORMAT, TEXT("--- page_x_extent out"));

    al_array_dispose(&h_row_changes);

    return(page_max + 1);
}

/******************************************************************************
*
* set new x extent for document
*
******************************************************************************/

extern void
page_x_extent_set(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->flags.x_extent_changed = 1;

    docu_flags_new_extent_set(p_docu);

    view_update_all(p_docu, UPDATE_BORDER_HORZ);
    view_update_all(p_docu, UPDATE_RULER_HORZ);
    view_update_all(p_docu, UPDATE_PANE_MARGIN_COL);
}

/******************************************************************************
*
* is the object visible at the moment ?
*
******************************************************************************/

/*ncr*/
_Ret_valid_
extern P_REGION
region_visible(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_REGION p_region)
{
    ARRAY_HANDLE h_row_range_visible;

    zero_struct_ptr(p_region);

    if(status_ok(skel_get_visible_row_ranges(p_docu, &h_row_range_visible)))
    {
        const ARRAY_INDEX n_elements = array_elements(&h_row_range_visible);
        ARRAY_INDEX i;
        PC_ROW_RANGE p_row_range = array_rangec(&h_row_range_visible, ROW_RANGE, 0, n_elements);

        for(i = 0; i < n_elements; ++i, ++p_row_range)
        {
            p_region->tl.row = MIN(p_region->tl.row, p_row_range->top);
            p_region->br.row = MAX(p_region->br.row, p_row_range->bot);
        }

        p_region->whole_row = 1;

        al_array_dispose(&h_row_range_visible);
    }

    return(p_region);
}

/******************************************************************************
*
* is the object visible at the moment ?
*
******************************************************************************/

_Check_return_
extern BOOL /* TRUE == object is visible */
row_is_visible(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    BOOL is_visible = FALSE;
    ARRAY_HANDLE h_row_range_visible;

    if(status_ok(skel_get_visible_row_ranges(p_docu, &h_row_range_visible)))
    {
        const ARRAY_INDEX n_elements = array_elements(&h_row_range_visible);
        ARRAY_INDEX i;
        PC_ROW_RANGE p_row_range = array_rangec(&h_row_range_visible, ROW_RANGE, 0, n_elements);

        for(i = 0; i < n_elements; ++i, ++p_row_range)
        {
            if( (row >= p_row_range->top) &&
                (row <  p_row_range->bot) )
            {
                is_visible = TRUE;
                break;
            }
        }

        al_array_dispose(&h_row_range_visible);
    }

    return(is_visible);
}

/******************************************************************************
*
* check if the col info we need is in the cache
*
******************************************************************************/

_Check_return_
static ARRAY_HANDLE /*h_col_info*/
skel_col_enum_cache_check(
    _InVal_     DOCNO docno,
    _InVal_     ROW row,
    _InVal_     PAGE page,
    _InVal_     COL col)
{
    const ARRAY_INDEX n_cache_elements = array_elements(&h_col_enum_cache);
    ARRAY_INDEX i;
    PC_COL_ENUM_CACHE p_col_enum_cache = array_rangec(&h_col_enum_cache, COL_ENUM_CACHE, 0, n_cache_elements);

    for(i = 0; i < n_cache_elements; ++i, ++p_col_enum_cache)
    {
        if(0 == p_col_enum_cache->h_col_info)
            continue;

        if( (p_col_enum_cache->docno == docno)
            &&
            (p_col_enum_cache->row == row)
            &&
            (((page >= 0) && (p_col_enum_cache->page == page)) || ((page < 0) && (p_col_enum_cache->page < 0)))
            &&
            (((col >= 0) && (p_col_enum_cache->col == col)) || ((col < 0) && (p_col_enum_cache->col < 0))) )
        {
            return(p_col_enum_cache->h_col_info);
        }
    }

    return(0);
}

/******************************************************************************
*
* remove deleted entries from the array
*
******************************************************************************/

PROC_ELEMENT_IS_DELETED_PROTO(static, col_enum_cache_is_deleted)
{
    const PC_COL_ENUM_CACHE p_col_enum_cache = (PC_COL_ENUM_CACHE) p_any;

    return(0 == p_col_enum_cache->h_col_info);
}

static void
skel_col_enum_cache_garbage_collect(
    _InVal_     BOOL may_dispose)
{
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.remove_deleted = 1; /* just shuffle remaining used elements together */
    /* al_garbage_flags.shrink = 1; */
    al_garbage_flags.may_dispose = may_dispose;
    consume(S32, al_array_garbage_collect(&h_col_enum_cache, 0, col_enum_cache_is_deleted, al_garbage_flags));
}

extern void
skel_col_enum_cache_dispose(
    _InVal_     DOCNO docno,
    _InRef_     PC_REGION p_region)
{
    BOOL do_garbage_collect = FALSE;
    const ARRAY_INDEX n_cache_elements = array_elements(&h_col_enum_cache);
    ARRAY_INDEX i;

    for(i = 0; i < n_cache_elements; ++i)
    {
        P_COL_ENUM_CACHE p_col_enum_cache = array_ptr(&h_col_enum_cache, COL_ENUM_CACHE, i);

        if(0 == p_col_enum_cache->h_col_info)
            continue;

        if( (docno == p_col_enum_cache->docno)
            &&
            row_in_region(p_region, p_col_enum_cache->row) )
        {
            assert(0 == p_col_enum_cache->locked);
            al_array_dispose(&p_col_enum_cache->h_col_info);
            do_garbage_collect = TRUE;
        }
    }

    if(do_garbage_collect)
        skel_col_enum_cache_garbage_collect(FALSE /*may_dispose*/);
}

static void
skel_col_enum_cache_insert(
    _InVal_     DOCNO docno,
    _InVal_     ROW row,
    _InVal_     PAGE page,
    _InVal_     COL col,
    _InVal_     ARRAY_HANDLE h_col_info)
{
    ARRAY_INDEX n_cache_elements = array_elements(&h_col_enum_cache);
    P_COL_ENUM_CACHE p_col_enum_cache;

    assert(array_elements(&h_col_info));

    if(n_cache_elements < COL_ENUM_CACHE_SIZE)
    {   /* cache not yet full - return another fresh entry at the end */
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(COL_ENUM_CACHE_SIZE, sizeof32(COL_ENUM_CACHE), TRUE);
        STATUS status;

        /* garbage collect doesn't shrink allocation, so we should be able to trivially extend allocation back up to CACHE_SIZE_COL_ENUM */
        p_col_enum_cache = al_array_extend_by(&h_col_enum_cache, COL_ENUM_CACHE, 1, &array_init_block, &status);
        status_assert(status);

        trace_1(TRACE_APP_MEMORY_USE,
                TEXT("col_enum_cache_size is ") U32_TFMT TEXT(" bytes"),
                array_size32(&h_col_enum_cache) * sizeof32(COL_ENUM_CACHE));

        n_cache_elements = array_elements(&h_col_enum_cache);
    }
    else
    {
        /* cache full - find a random unlocked cache entry to use */
        S32 count = 0;
        ARRAY_INDEX i = host_rand_between(0, n_cache_elements);

        for(;;)
        {
            if(i >= n_cache_elements)
                i = 0;

            p_col_enum_cache = array_ptr(&h_col_enum_cache, COL_ENUM_CACHE, i);
            if(0 == p_col_enum_cache->locked)
                break;

            ++count;
            if(count >= n_cache_elements)
                break;

            ++i;
        }

        assert(0 == p_col_enum_cache->locked);

        al_array_dispose(&p_col_enum_cache->h_col_info);
    }

    p_col_enum_cache->h_col_info = h_col_info;
    p_col_enum_cache->docno = docno;
    p_col_enum_cache->row = row;
    p_col_enum_cache->col = (col >= 0) ? col : (COL) -1;
    p_col_enum_cache->page = (page >= 0) ? page : (PAGE) -1;

    trace_3(TRACE_APP_FORMAT, TEXT("skel_col_enum_cache_insert: row: ") ROW_TFMT TEXT(", col: ") COL_TFMT TEXT(", page: ") S32_TFMT,
            p_col_enum_cache->row, p_col_enum_cache->col, p_col_enum_cache->page);
}

extern void
skel_col_enum_cache_lock(
    _InVal_     ARRAY_HANDLE h_col_info)
{
    const ARRAY_INDEX n_cache_elements = array_elements(&h_col_enum_cache);
    ARRAY_INDEX i;

    if(0 == h_col_info)
        return;

    for(i = 0; i < n_cache_elements; ++i)
    {
        const P_COL_ENUM_CACHE p_col_enum_cache = array_ptr(&h_col_enum_cache, COL_ENUM_CACHE, i);

        if(p_col_enum_cache->h_col_info == h_col_info)
        {
            assert(p_col_enum_cache->locked >= 0);
            p_col_enum_cache->locked += 1;
            trace_1(TRACE_APP_FORMAT, TEXT("skel_col_enum_cache_lock: ") S32_TFMT, (S32) p_col_enum_cache->locked);
            return;
        }
    }

    assert((i < n_cache_elements) || (0 == n_cache_elements));
}

extern void
skel_col_enum_cache_release(
    _InVal_     ARRAY_HANDLE h_col_info)
{
    const ARRAY_INDEX n_cache_elements = array_elements(&h_col_enum_cache);
    ARRAY_INDEX i;

    if(0 == h_col_info)
        return;

    for(i = 0; i < n_cache_elements; ++i)
    {
        const P_COL_ENUM_CACHE p_col_enum_cache = array_ptr(&h_col_enum_cache, COL_ENUM_CACHE, i);

        if(p_col_enum_cache->h_col_info == h_col_info)
        {
            assert(p_col_enum_cache->locked > 0);
            p_col_enum_cache->locked -= 1;
            trace_1(TRACE_APP_FORMAT, TEXT("skel_col_enum_cache_release: ") S32_TFMT, (S32) p_col_enum_cache->locked);
            return;
        }
    }

    assert((i < n_cache_elements) || (0 == n_cache_elements));
}

/******************************************************************************
*
* derive column information for a given row
* from the style database
*
* --in--
* either page required specified by page
* or column required specified by col;
* if both invalid (-1), all columns returned
*
* --out--
* handle to array of COL_INFO structures returned
* 0 handle if no columns on page
*
******************************************************************************/

_Check_return_
extern STATUS
skel_col_enum(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     PAGE page /* invalid page gets col only */,
    _InVal_     COL col /* both page and col invalid gets whole doc */,
    _OutRef_    P_ARRAY_HANDLE p_h_col_info)
{
    STATUS status = STATUS_OK;
    ARRAY_HANDLE h_col_info;

    DOCU_ASSERT(p_docu);
    assert(row >= 0);
    assert((col < 0) || (col < p_docu->cols_logical));

    h_col_info = skel_col_enum_cache_check(p_docu->docno, row, page, col);

    if(0 == array_elements(&h_col_info))
    {
        ARRAY_HANDLE h_col_changes;
        ARRAY_INDEX col_change_index = 0;
        SLR slr;
        COL page_first_col;
        SKEL_EDGE skel_edge;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(COL_INFO), TRUE);
        SC_ARRAY_INIT_BLOCK tab_init_block = aib_init(1, sizeof32(TAB_INFO), TRUE);
        STYLE style;

        if_constant(tracing(TRACE_APP_FORMAT))
        {
            if(page >= 0)
                trace_2(TRACE_APP_FORMAT, TEXT("skel_col_enum, row: ") ROW_TFMT TEXT(", page: ") S32_TFMT TEXT(" --- :"), row, page);
            else if(col >= 0)
                trace_2(TRACE_APP_FORMAT, TEXT("skel_col_enum, row: ") ROW_TFMT TEXT(", col: ") COL_TFMT TEXT(" --- :"), row, col);
            else
                trace_1(TRACE_APP_FORMAT, TEXT("skel_col_enum, row: ") ROW_TFMT TEXT(", <whole doc> --- :"), row);
        }

        /* avoid mega style from slrs */
        if(n_cols_logical(p_docu) > COL_CHANGE_THRESHOLD)
            h_col_changes = style_change_between_cols(p_docu, 0, n_cols_logical(p_docu), row, &style_selector_para_ruler);
        else
            h_col_changes = 0;

        style_init(&style);

        for(h_col_info = 0, skel_edge.page = 0, skel_edge.pixit = 0, page_first_col = 0, slr.col = 0, slr.row = row;
            slr.col < n_cols_logical(p_docu) && ((page >= 0) ? (skel_edge.page <= page) : (col >= 0) ? (slr.col <= col) : TRUE);
            ++slr.col)
        {
            if(!h_col_changes)
                style_from_slr(p_docu, &style, &style_selector_para_ruler, &slr);
            else if(slr.col == *array_ptr(&h_col_changes, COL, col_change_index))
            {
                style_from_slr(p_docu, &style, &style_selector_para_ruler, &slr);
                if(col_change_index + 1 < array_elements(&h_col_changes))
                    ++col_change_index;
            }

            if(style.col_style.width
               &&
               skel_edge.pixit + style.col_style.width > p_docu->page_def.cells_usable_x
               &&
               slr.col != page_first_col)
            {
                /* move onto next page */
                skel_edge.page += 1;
                skel_edge.pixit = 0;
                page_first_col = slr.col;
            }

            /* output column to block if in desired page range */
            if((page >= 0) ? (skel_edge.page == page) : (col >= 0) ? (slr.col == col) : TRUE)
            {
                P_COL_INFO p_col_info;

                /* get an extra space */
                if(NULL == (p_col_info = al_array_extend_by(&h_col_info, COL_INFO, 1, &array_init_block, &status)))
                {
                    status_assert(status);
                    al_array_dispose(&h_col_info);
                    style_dispose(&style);
                    break;
                }

                al_array_auto_compact_set(&h_col_info);

                p_col_info->col = slr.col;
                p_col_info->edge_left = skel_edge;
                p_col_info->edge_right = skel_edge;
                p_col_info->edge_right.pixit += style.col_style.width;
                p_col_info->margin_right = style.para_style.margin_right;
                p_col_info->margin_left = style.para_style.margin_left;
                p_col_info->margin_para = style.para_style.margin_para;
                p_col_info->h_tab_list = style.para_style.h_tab_list;
                p_col_info->tab_init_block = tab_init_block;
            }

            skel_edge.pixit += style.col_style.width;
        }

        style_dispose(&style);
        al_array_dispose(&h_col_changes);

        if(0 != h_col_info)
            skel_col_enum_cache_insert(p_docu->docno, row, page, col, h_col_info);
    }

    *p_h_col_info = h_col_info;

    return(status);
}

/******************************************************************************
*
* find the top left of a cell -
* ensures the row containing the cell is formatted
*
******************************************************************************/

extern void
skel_point_from_slr_tl(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_POINT p_skel_point,
    _InRef_     PC_SLR p_slr)
{
    ARRAY_HANDLE h_col_info;

    status_assert(skel_col_enum(p_docu, p_slr->row, (PAGE) -1, p_slr->col, &h_col_info));

    if(0 != array_elements(&h_col_info))
    {
        const PC_COL_INFO p_col_info = array_basec(&h_col_info, COL_INFO);

        skel_point_update_from_edge(p_skel_point, &p_col_info->edge_left, x);

        {
        ROW_ENTRY row_entry;
        row_entry_from_row(p_docu, &row_entry, p_slr->row);
        skel_point_update_from_edge(p_skel_point, &row_entry.rowtab.edge_top, y);
        } /*block*/
    }
#if CHECKING || defined(CODE_ANALYSIS)
    else
    {
        assert0();
        zero_struct_ptr(p_skel_point);
    }
#endif /* CHECKING */
}

/******************************************************************************
*
* renormalise a skel_point in a given layer
*
******************************************************************************/

extern void
skel_point_normalise(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKEL_POINT p_skel_point,
    _InVal_     REDRAW_TAG redraw_tag)
{
    PIXIT_POINT work_area;

    /* try adding page heights to pixit_point.y to make it +ve (page_num.y MUST stay +ve) */
    while(p_skel_point->pixit_point.y < 0)
    {
        if(p_skel_point->page_num.y <= 0)
            break;

        p_skel_point->page_num.y -= 1;

        page_limits_from_page(p_docu, &work_area, redraw_tag, &p_skel_point->page_num);

        p_skel_point->pixit_point.y += work_area.y;
    }

    /* try subtracting page heights from pixit_point.y to bring it within a page (page_num.y MUST stay < last_page_y) */
    for(;;)
    {
        page_limits_from_page(p_docu, &work_area, redraw_tag, &p_skel_point->page_num);

        if(p_skel_point->pixit_point.y < work_area.y)
            break;

        if(p_skel_point->page_num.y + 1 >= last_page_y(p_docu))
            break;

        p_skel_point->page_num.y += 1;

        p_skel_point->pixit_point.y -= work_area.y;
    }

    /* try adding page widths to pixit_point.x to make it +ve (page_num.x MUST stay +ve) */
    while(p_skel_point->pixit_point.x < 0)
    {
        if(p_skel_point->page_num.x <= 0)
            break;

        p_skel_point->page_num.x -= 1;

        page_limits_from_page(p_docu, &work_area, redraw_tag, &p_skel_point->page_num);

        p_skel_point->pixit_point.x += work_area.x;
    }

    /* try subtracting page widths from pixit_point.x to bring it within a page (page_num.x MUST stay < last_page_x) */
    for(;;)
    {
        page_limits_from_page(p_docu, &work_area, redraw_tag, &p_skel_point->page_num);

        if(p_skel_point->pixit_point.x < work_area.x)
            break;

        if(p_skel_point->page_num.x + 1 >= last_page_x(p_docu))
            break;

        p_skel_point->page_num.x += 1;

        p_skel_point->pixit_point.x -= work_area.x;
    }
}

/******************************************************************************
*
* calculate the rectangle covering an area of a document
*
******************************************************************************/

extern void
skel_rect_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_RECT p_skel_rect,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    ARRAY_HANDLE h_row_changes;
    COL col_s, col_e;
    ROW row_s, row_e;

    limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, p_docu_area);

    { /* set top and bottom */
    ROW_ENTRY row_entry;
    row_entry_from_row(p_docu, &row_entry, row_s);
    skel_point_update_from_edge(&p_skel_rect->tl, &row_entry.rowtab.edge_top, y);
    row_entry_from_row(p_docu, &row_entry, row_e - 1);
    skel_point_update_from_edge(&p_skel_rect->br, &row_entry.rowtab.edge_bot, y);
    } /*block*/

    /* get list of changes of column information */
    if((h_row_changes = col_changes_between_rows(p_docu, row_s, row_e)) != 0)
    {
        P_ROW p_row;
        ARRAY_INDEX row_index;
        S32 col_set;

        /* for each different row... */
        for(row_index = 0, p_row = array_ptr(&h_row_changes, ROW, row_index), col_set = 0;
            row_index < array_elements(&h_row_changes);
            ++row_index, ++p_row)
        {
            ARRAY_HANDLE h_col_info;

            status_assert(skel_col_enum(p_docu, *p_row, (PAGE) -1, (COL) -1, &h_col_info));

            if(0 != array_elements(&h_col_info))
            {
                PC_COL_INFO p_col_info;

                /* check for leftmost edge in rectangle */
                p_col_info = array_ptrc(&h_col_info, COL_INFO, col_s);

                if(!col_set || skel_point_edge_compare(&p_skel_rect->tl, &p_col_info->edge_left, x) > 0)
                    skel_point_update_from_edge(&p_skel_rect->tl, &p_col_info->edge_left, x);

                /* check for rightmost edge in rectangle */
                p_col_info = array_ptrc(&h_col_info, COL_INFO, col_e - 1);

                if(!col_set || skel_point_edge_compare(&p_skel_rect->br, &p_col_info->edge_right, x) < 0)
                    skel_point_update_from_edge(&p_skel_rect->br, &p_col_info->edge_right, x);

                col_set = 1;
            }
        }
    }
    al_array_dispose(&h_row_changes);
}

/******************************************************************************
*
* return skeleton rectangle for a cell, using the strict col/row grid
*
******************************************************************************/

extern void
skel_rect_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_RECT p_skel_rect,
    _InRef_     PC_SLR p_slr)
{
    ARRAY_HANDLE h_col_info;
    
    status_assert(skel_col_enum(p_docu, p_slr->row, (PAGE) -1, p_slr->col, &h_col_info));

    if(0 != array_elements(&h_col_info))
    {
        const PC_COL_INFO p_col_info = array_basec(&h_col_info, COL_INFO);
        skel_point_update_from_edge(&p_skel_rect->tl, &p_col_info->edge_left,  x);
        skel_point_update_from_edge(&p_skel_rect->br, &p_col_info->edge_right, x);

        {
        ROW_ENTRY row_entry;
        row_entry_from_row(p_docu, &row_entry, p_slr->row);
        skel_point_update_from_edge(&p_skel_rect->tl, &row_entry.rowtab.edge_top, y);
        skel_point_update_from_edge(&p_skel_rect->br, &row_entry.rowtab.edge_bot, y);
        } /*block*/
    }
#if CHECKING || defined(CODE_ANALYSIS)
    else
    {
        assert0();
        zero_struct_ptr(p_skel_rect);
    }
#endif /* CHECKING */
}

/******************************************************************************
*
* given a skeleton point, work out the slr (thus object) which owns the point
*
******************************************************************************/

_Check_return_
extern STATUS
slr_owner_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SLR p_slr,
    P_SKEL_POINT p_tl_out,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InVal_     S32 direction /* go up or down on row edge ? */)
{
    STATUS status = STATUS_FAIL;
    SLR slr;
    ARRAY_HANDLE h_col_info;

    {
    ROW_ENTRY row_entry;
    row_entry_from_skel_point(p_docu, &row_entry, p_skel_point, direction != ON_ROW_EDGE_GO_UP);
    slr.col = 0;
    slr.row = row_entry.row;
    p_tl_out->pixit_point.x = 0;
    p_tl_out->pixit_point.y = row_entry.rowtab.edge_top.pixit;
    p_tl_out->page_num.x = 0;
    p_tl_out->page_num.y = row_entry.rowtab.edge_top.page;
    } /*block*/

    status_assert(skel_col_enum(p_docu, slr.row, p_skel_point->page_num.x, (COL) -1, &h_col_info));

    if(0 != array_elements(&h_col_info))
    {
        const COL n_cols = (COL) array_elements(&h_col_info);
        PC_COL_INFO p_col_info_last_non_zero = P_COL_INFO_NONE;
        COL start_col = array_ptr(&h_col_info, COL_INFO, 0)->col;
        ARRAY_INDEX i = 0;
        PC_COL_INFO p_col_info = array_ptrc(&h_col_info, COL_INFO, i);

        assert(0 != n_cols); /* otherwise ^^^ would have gone bang */

        slr.col = start_col;

        for(;;)
        {
            BOOL doing_last_col = ((i + 1) == n_cols);

            /* skip zero width columns */
            if(0 == col_width(h_col_info, p_col_info->col))
            {
                /* but trailing ones must use the last kosher one */
                if(doing_last_col)
                {
                    assert(P_DATA_NONE != p_col_info_last_non_zero);
                    if(P_DATA_NONE != p_col_info_last_non_zero)
                    {
                        /* set up tl of last non-zero width slr found */
                        skel_point_update_from_edge(p_tl_out, &p_col_info_last_non_zero->edge_left, x);
                        status = STATUS_OK;
                    }
                    break;
                }
            }
            else
            {
                p_col_info_last_non_zero = p_col_info;

                slr.col = p_col_info->col;

                if(skel_point_edge_compare(p_skel_point, &array_ptr(&h_col_info, COL_INFO, slr.col - start_col)->edge_right, x) < 0)
                {
                    /* set up tl of slr found */
                    skel_point_update_from_edge(p_tl_out, &p_col_info->edge_left, x);
                    status = STATUS_DONE;
                    break;
                }

                if(doing_last_col)
                {
                    /* set up tl of slr found (to the left of where we are) */
                    skel_point_update_from_edge(p_tl_out, &p_col_info->edge_left, x);
                    status = STATUS_OK;
                    break;
                }
            }

            ++i;
            ++p_col_info;
        }
    }

    *p_slr = slr;
    return(status);
}

/******************************************************************************
*
* work out whether a given cell is in a table
*
******************************************************************************/

_Check_return_
extern BOOL /* 1 == in table */
cell_in_table(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    ARRAY_HANDLE h_col_info;
    S32 n_visible = 0;

    status_assert(skel_col_enum(p_docu, p_slr->row, (PAGE) -1, (COL) -1, &h_col_info));

    if(0 != array_elements(&h_col_info))
    {
        const ARRAY_INDEX n_elements = array_elements(&h_col_info);
        ARRAY_INDEX i;
        PC_COL_INFO p_col_info = array_rangec(&h_col_info, COL_INFO, 0, n_elements);

        for(i = 0; (i < n_elements) && (n_visible < 2); ++i, ++p_col_info)
        {
            /* column visible with non-zero width ? */
            if((p_col_info->edge_right.pixit - p_col_info->edge_left.pixit) != 0)
                n_visible += 1;
        }
    }

    return((n_visible > 1) ? 1 : 0);
}

/******************************************************************************
*
* work out if a docu_area spans across a table
*
******************************************************************************/

_Check_return_
extern BOOL /* 1 == in table */
docu_area_spans_across_table(
    _DocuRef_   P_DOCU p_docu,
    P_DOCU_AREA p_docu_area)
{
    ARRAY_HANDLE h_col_info;
    BOOL res = 0;

    if(!p_docu->flags.base_single_col)
        return(0);

    status_assert(skel_col_enum(p_docu, p_docu_area->tl.slr.row, (PAGE) -1, (COL) -1, &h_col_info));

    if(0 != array_elements(&h_col_info))
    {
        const ARRAY_INDEX n_elements = array_elements(&h_col_info);
        ARRAY_INDEX i;
        PC_COL_INFO p_col_info = array_rangec(&h_col_info, COL_INFO, 0, n_elements);
        COL last_visible = 0;

        for(i = 0; i < n_elements; i += 1, ++p_col_info)
        {
            /* column visible with non-zero width ? */
            if((p_col_info->edge_right.pixit - p_col_info->edge_left.pixit) != 0)
                last_visible = MAX(last_visible, (COL) i);
        }

        {
        REGION region;
        region_from_docu_area_min(&region, p_docu_area);
        /* assume table starts in col 1 or 0 */
        if(region.tl.col < 2 && region.br.col > last_visible)
            res = 1;
        } /*block*/

    }

    return(res);
}

/******************************************************************************
*
* find out the width of a particular cell
*
******************************************************************************/

_Check_return_
extern PIXIT
cell_width(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    ARRAY_HANDLE h_col_info;
    PIXIT width = 0;

    status_assert(skel_col_enum(p_docu, p_slr->row, (PAGE) -1, p_slr->col, &h_col_info));

    if(0 != array_elements(&h_col_info))
    {
        const PC_COL_INFO p_col_info = array_basec(&h_col_info, COL_INFO);

        width = p_col_info->edge_right.pixit - p_col_info->edge_left.pixit;
    }

    return(width);
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_col);

_Check_return_
static STATUS
sk_col_msg_exit2(void)
{
    skel_col_enum_cache_garbage_collect(TRUE /*may_dispose*/);

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_services_sk_col_msg_initclose(
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__EXIT2:
        return(sk_col_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_col)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_col_msg_initclose((PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of sk_col.c */
