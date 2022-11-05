/* sk_hefod.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines for header/footer data */

/* MRJC November 1992 / 2nd iteration December 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal routines
*/

_Check_return_
_Ret_maybenull_
static P_PAGE_HEFO_BREAK
p_page_hefo_break_from_client_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     CLIENT_HANDLE client_handle);

_Check_return_
_Ret_maybenull_
static P_HEADFOOT_DEF
p_headfoot_def_from_row_and_page_y(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ROW p_row,
    _InVal_     PAGE page_y,
    _InVal_     OBJECT_ID object_id_focus);

static void
page_hefo_break_delete(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_PAGE_HEFO_BREAK p_page_hefo_break);

static void
page_hefo_break_garbage_collect(
    _DocuRef_   P_DOCU p_docu);

/*
local data
*/

#define HEFOD_LIST_DEP 0

static int next_uref_client_handle = HEFOD_LIST_DEP + 1;

/******************************************************************************
*
* get both header and footer for a page
*
******************************************************************************/

extern void
headfoot_both_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_HEADFOOT_BOTH p_headfoot_both_out,
    _InVal_     PAGE page_y)
{
    const ROW row = row_from_page_y(p_docu, page_y);

    headfoot_both_from_row_and_page_y(p_docu, p_headfoot_both_out, row, page_y);
}

/******************************************************************************
*
* read both header and footer size data for a row/page
*
******************************************************************************/

extern void
headfoot_both_from_row_and_page_y(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_HEADFOOT_BOTH p_headfoot_both_out,
    _InVal_     ROW row,
    _InVal_     PAGE page_y)
{
    P_HEADFOOT_DEF p_headfoot_def;
    ROW row_i;

    zero_struct_ptr(p_headfoot_both_out);

    row_i = row;
    if(NULL != (p_headfoot_def = p_headfoot_def_from_row_and_page_y(p_docu, &row_i, page_y, OBJECT_ID_HEADER)))
        p_headfoot_both_out->header = p_headfoot_def->headfoot_sizes;

    row_i = row;
    if(NULL != (p_headfoot_def = p_headfoot_def_from_row_and_page_y(p_docu, &row_i, page_y, OBJECT_ID_FOOTER)))
        p_headfoot_both_out->footer = p_headfoot_def->headfoot_sizes;
}

/******************************************************************************
*
* dispose of headfoot data
*
******************************************************************************/

extern void
headfoot_dispose(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_HEADFOOT_DEF p_headfoot_def,
    _InVal_     BOOL all)
{
    al_array_dispose(&p_headfoot_def->headfoot.h_data);

    style_docu_area_delete_list(p_docu, &p_headfoot_def->headfoot.h_style_list, all);

    if(all)
        al_array_dispose(&p_headfoot_def->headfoot.h_style_list);
}

/******************************************************************************
*
* initialise a headfoot entry
*
******************************************************************************/

_Check_return_
extern STATUS
headfoot_init(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_HEADFOOT_DEF p_headfoot_def,
    _In_        DATA_SPACE data_space)
{
    STATUS status = style_docu_area_add_internal(p_docu, &p_headfoot_def->headfoot.h_style_list, data_space);

    if(status_ok(status))
    {
        STYLE_HANDLE style_handle = style_handle_base(&p_docu->h_style_docu_area);

        if(style_handle)
            status = style_docu_area_add_base(p_docu, &p_headfoot_def->headfoot.h_style_list, style_handle, data_space);
    }

#if 0
    if(status_ok(status))
    {
        STYLE_HANDLE style_handle = style_handle_base(&p_headfoot_def->headfoot.h_style_list);

        if(style_handle)
            status = style_docu_area_add_base(p_docu, &p_headfoot_def->headfoot.h_style_list, style_handle, data_space);
    }
#endif

    p_headfoot_def->data_space = data_space;

    if(status_fail(status))
        style_docu_area_delete_list(p_docu, &p_headfoot_def->headfoot.h_style_list, TRUE);

    return(status);
}

/******************************************************************************
*
* loop over headers and footers to get minimum space
*
* SKS 14may93 used to be obtain maximum space for work area determination, which was wrong calc'n!
*
******************************************************************************/

extern void
hefod_margins_min(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PIXIT p_header_margin_min,
    _OutRef_    P_PIXIT p_footer_margin_min)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);

    *p_header_margin_min =
    *p_footer_margin_min = S32_MAX;

    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_ODD))
            *p_header_margin_min = MIN(*p_header_margin_min, p_page_hefo_break->header_odd.headfoot_sizes.margin);
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_EVEN))
            *p_header_margin_min = MIN(*p_header_margin_min, p_page_hefo_break->header_even.headfoot_sizes.margin);
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_FIRST))
            *p_header_margin_min = MIN(*p_header_margin_min, p_page_hefo_break->header_first.headfoot_sizes.margin);

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_ODD))
            *p_footer_margin_min = MIN(*p_footer_margin_min, p_page_hefo_break->footer_odd.headfoot_sizes.margin);
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_EVEN))
            *p_footer_margin_min = MIN(*p_footer_margin_min, p_page_hefo_break->footer_even.headfoot_sizes.margin);
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_FIRST))
            *p_footer_margin_min = MIN(*p_footer_margin_min, p_page_hefo_break->footer_first.headfoot_sizes.margin);
    }

    if(*p_header_margin_min == S32_MAX)
        *p_header_margin_min = 0;

    if(*p_footer_margin_min == S32_MAX)
        *p_footer_margin_min = 0;
}

/******************************************************************************
*
* handle uref events
*
******************************************************************************/

PROC_UREF_EVENT_PROTO(static, sk_hefod_uref_event_dep_delete)
{
    /* dependency must be deleted */
    P_PAGE_HEFO_BREAK p_page_hefo_break;

    switch(uref_message)
    {
    /* free a region */
    default:
        {
        p_page_hefo_break = p_page_hefo_break_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

        if(NULL != p_page_hefo_break)
        {
            DOCU_REFORMAT docu_reformat;

            trace_6(TRACE_APP_UREF,
                    TEXT("hefod_uref_event CLOSE1 tl: ") COL_TFMT TEXT(",") ROW_TFMT TEXT("; br: ") COL_TFMT TEXT(",") ROW_TFMT TEXT(", whole_col: ") S32_TFMT TEXT(", whole_row: ") S32_TFMT,
                    p_uref_event_block->uref_id.region.tl.col,
                    p_uref_event_block->uref_id.region.tl.row,
                    p_uref_event_block->uref_id.region.br.col,
                    p_uref_event_block->uref_id.region.br.row,
                    (S32) p_uref_event_block->uref_id.region.whole_col,
                    (S32) p_uref_event_block->uref_id.region.whole_row);

            docu_reformat.data.row = p_page_hefo_break->region.tl.row;
            page_hefo_break_delete(p_docu, p_page_hefo_break);

            docu_reformat.action = REFORMAT_HEFO_Y;
            docu_reformat.data_type = REFORMAT_ROW;
            docu_reformat.data_space = DATA_NONE;
            status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
        }

        break;
        }

    case Uref_Msg_CLOSE1:
        if(p_uref_event_block->uref_id.client_handle > HEFOD_LIST_DEP)
        {
            p_page_hefo_break = p_page_hefo_break_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

            PTR_ASSERT(p_page_hefo_break);

            if(NULL != p_page_hefo_break)
                page_hefo_break_delete(p_docu, p_page_hefo_break);
        }
        break;

    /* free the whole header/footer stuff */
    case Uref_Msg_CLOSE2:
        /* can only be freeing whole by now */
        assert(p_uref_event_block->uref_id.client_handle == HEFOD_LIST_DEP);

        page_hefo_break_garbage_collect(p_docu);
        uref_del_dependency(docno_from_p_docu(p_docu), p_uref_event_block->uref_id.uref_handle);
        break;
    }

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, sk_hefod_uref_event_dep_update)
{
    /* dependency region must be updated */
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    REGION region_old;

    p_page_hefo_break = p_page_hefo_break_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

    PTR_ASSERT(p_page_hefo_break);

    region_old = p_page_hefo_break->region;

    consume(UREF_COMMS, uref_match_region(&p_page_hefo_break->region, uref_message, p_uref_event_block));

    if(region_old.tl.row != p_page_hefo_break->region.tl.row)
    {
        DOCU_REFORMAT docu_reformat;
        docu_reformat.action = REFORMAT_HEFO_Y;
        docu_reformat.data_type = REFORMAT_ROW;
        docu_reformat.data_space = DATA_NONE;
        docu_reformat.data.row = MIN(region_old.tl.row, p_page_hefo_break->region.tl.row);
        status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
    }

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, sk_hefod_uref_event)
{
#if TRACE_ALLOWED && 0
    uref_trace_reason(UBF_UNPACK(UREF_COMMS, p_uref_event_block->reason.code), TEXT("HEFOD_UREF"));
#endif

    switch(UBF_UNPACK(UREF_COMMS, p_uref_event_block->reason.code))
    {
    case Uref_Dep_Delete: /* dependency must be deleted */
        return(sk_hefod_uref_event_dep_delete(p_docu, uref_message, p_uref_event_block));

    case Uref_Dep_Update: /* dependency region must be updated */
        return(sk_hefod_uref_event_dep_update(p_docu, uref_message, p_uref_event_block));

    case Uref_Dep_Inform:
        return(STATUS_OK);

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* find the entry for a given client handle
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_PAGE_HEFO_BREAK
p_page_hefo_break_from_client_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     CLIENT_HANDLE client_handle)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(p_page_hefo_break->uref_client_handle == client_handle)
            return(p_page_hefo_break);
    }

    return(NULL);
}

/******************************************************************************
*
* find the header or footer for a given page
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_HEADFOOT_DEF
p_headfoot_def_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW p_row,
    _InVal_     PAGE page_y,
    _InVal_     OBJECT_ID object_id_focus)
{
    ROW row = row_from_page_y(p_docu, page_y);
    P_HEADFOOT_DEF p_headfoot_def = p_headfoot_def_from_row_and_page_y(p_docu, &row, page_y, object_id_focus);

    *p_row = row;

    return(p_headfoot_def);
}

/******************************************************************************
*
* find the header or footer for a given row
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_HEADFOOT_DEF
p_headfoot_def_from_row_and_page_y(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ROW p_row,
    _InVal_     PAGE page_y,
    _InVal_     OBJECT_ID object_id_focus)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);
    ARRAY_INDEX close, close_odd = -1, close_even = -1, close_first = -1;
    U32 bit_odd, bit_even, bit_first;
    BOOL odd, first;
    P_HEADFOOT_DEF p_headfoot_def = NULL;

    if(OBJECT_ID_HEADER == object_id_focus)
    {
        bit_odd   = PAGE_HEFO_HEADER_ODD;
        bit_even  = PAGE_HEFO_HEADER_EVEN;
        bit_first = PAGE_HEFO_HEADER_FIRST;
    }
    else
    {
        bit_odd   = PAGE_HEFO_FOOTER_ODD;
        bit_even  = PAGE_HEFO_FOOTER_EVEN;
        bit_first = PAGE_HEFO_FOOTER_FIRST;
    }

    /* search forward to find closest entry */
    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(p_page_hefo_break->region.tl.row > *p_row)
            break;

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_odd))
            close_odd = i;
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_even))
            close_even = i;
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_first))
            close_first = i;
    }

    if(close_even < 0 || ((page_y + 1) & 1))
    {
        odd = 1;
        close = close_odd;
    }
    else
    {
        odd = 0;
        close = close_even;
    }

    first = 0;
    if(close_first >= 0)
    {
        ROW row;

        p_page_hefo_break = array_ptr(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, close_first);
        row = p_page_hefo_break->region.whole_col ? 0 : p_page_hefo_break->region.tl.row;

        if(row == *p_row)
        {
            close = close_first;
            first = 1;
        }
        else
        {
            ROW_ENTRY row_entry;
            row_entry_from_row(p_docu, &row_entry, row);
            if(row_entry.rowtab.edge_top.page == page_y)
            {
                close = close_first;
                first = 1;
            }
        }
    }

    if(close >= 0)
    {
        /* select entry */
        p_page_hefo_break = array_ptr(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, close);

        /* select pointer */
        switch(object_id_focus)
        {
        default: default_unhandled();
#if CHECKING
        case OBJECT_ID_HEADER:
#endif
            p_headfoot_def =
                first
                    ? &p_page_hefo_break->header_first
                    : odd
                    ? &p_page_hefo_break->header_odd
                    : &p_page_hefo_break->header_even;
            break;

        case OBJECT_ID_FOOTER:
            p_headfoot_def =
                first
                    ? &p_page_hefo_break->footer_first
                    : odd
                    ? &p_page_hefo_break->footer_odd
                    : &p_page_hefo_break->footer_even;
            break;

        }

        *p_row = p_page_hefo_break->region.whole_col ? 0 : p_page_hefo_break->region.tl.row;
    }

    return(p_headfoot_def);
}

/******************************************************************************
*
* find a page hefo entry for a given row
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK
p_page_hefo_break_from_row_below(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);
    P_PAGE_HEFO_BREAK p_page_hefo_break_out = NULL;

    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(p_page_hefo_break->region.tl.row > row)
            break;

        p_page_hefo_break_out = p_page_hefo_break;
    }

    return(p_page_hefo_break_out);
}

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK
p_page_hefo_break_from_row_exact(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);
    P_PAGE_HEFO_BREAK p_page_hefo_break_out = NULL;

    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(p_page_hefo_break->region.tl.row > row)
            break;

        if(p_page_hefo_break->region.tl.row == row)
        {
            p_page_hefo_break_out = p_page_hefo_break;
            break;
        }
    }

    return(p_page_hefo_break_out);
}

/******************************************************************************
*
* create a new page entry for a given row
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK
p_page_hefo_break_new_for_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _OutRef_    P_STATUS p_status)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);
    P_PAGE_HEFO_BREAK p_page_hefo_break_out = NULL;
    ARRAY_INDEX close = -1;

    *p_status = STATUS_OK;

    /* search forward to find closest entry */
    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(p_page_hefo_break->region.tl.row >= row)
        {
            close = i;
            break;
        }
    }

    if((close >= 0) && ((p_page_hefo_break = array_ptr(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, close))->region.tl.row == row))
    {
        p_page_hefo_break_out = p_page_hefo_break;
    }
    else
    {
        P_PAGE_HEFO_BREAK p_page_hefo_break_new;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(PAGE_HEFO_BREAK), FALSE);

        if(NULL != (p_page_hefo_break_new = al_array_extend_by(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 1, &array_init_block, p_status)))
        {
            /* shift down existing entries to make space */
            if(close >= 0)
            {
                for(i = array_elements(&p_docu->h_page_hefo) - 1; i > close; i -= 1)
                {
                    p_page_hefo_break_new[0] = p_page_hefo_break_new[-1];
                    p_page_hefo_break_new -= 1;
                }
            }

            PTR_ASSERT(p_page_hefo_break_new); /* VS2008 analyze for [-1] */
            zero_struct_ptr(p_page_hefo_break_new);
            p_page_hefo_break = p_page_hefo_break_new;

            p_page_hefo_break->header_odd.data_space =
            p_page_hefo_break->footer_odd.data_space =
            p_page_hefo_break->header_even.data_space =
            p_page_hefo_break->footer_even.data_space =
            p_page_hefo_break->header_first.data_space =
            p_page_hefo_break->footer_first.data_space = DATA_NONE;

            /* create dependency */
            p_page_hefo_break->region.tl.col = p_page_hefo_break->region.br.col = -1;
            p_page_hefo_break->region.tl.row = row;
            p_page_hefo_break->region.br.row = row + 1;
            if(row)
                p_page_hefo_break->region.whole_col = 0;
            else
                p_page_hefo_break->region.whole_col = 1;
            p_page_hefo_break->region.whole_row = 1;

            if(status_ok(*p_status = uref_add_dependency(p_docu, &p_page_hefo_break->region, sk_hefod_uref_event, next_uref_client_handle, &p_page_hefo_break->uref_handle, FALSE)))
                p_page_hefo_break->uref_client_handle = next_uref_client_handle++;

            if(status_fail(*p_status))
                p_page_hefo_break->is_deleted = 1;
            else
                p_page_hefo_break_out = p_page_hefo_break;
        }
    }

    return(p_page_hefo_break_out);
}

/******************************************************************************
*
* does this row have a page break
*
******************************************************************************/

_Check_return_
extern BOOL
page_break_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    const P_PAGE_HEFO_BREAK p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, row);

    if(NULL != p_page_hefo_break)
        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_BREAK))
            return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* delete a header or footer entry
*
******************************************************************************/

static void
page_hefo_break_delete(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_PAGE_HEFO_BREAK p_page_hefo_break)
{
    headfoot_dispose(p_docu, &p_page_hefo_break->header_odd, TRUE);
    headfoot_dispose(p_docu, &p_page_hefo_break->header_even, TRUE);
    headfoot_dispose(p_docu, &p_page_hefo_break->footer_odd, TRUE);
    headfoot_dispose(p_docu, &p_page_hefo_break->footer_even, TRUE);
    headfoot_dispose(p_docu, &p_page_hefo_break->header_first, TRUE);
    headfoot_dispose(p_docu, &p_page_hefo_break->footer_first, TRUE);

    p_page_hefo_break->is_deleted = 1;
    uref_del_dependency(docno_from_p_docu(p_docu), p_page_hefo_break->uref_handle);
}

PROC_ELEMENT_IS_DELETED_PROTO(static, page_hefo_break_is_deleted)
{
    const P_PAGE_HEFO_BREAK p_page_hefo_break = (P_PAGE_HEFO_BREAK) p_any;

    return(p_page_hefo_break->is_deleted);
}

/******************************************************************************
*
* delete a header/footer definition
*
******************************************************************************/

extern void
page_hefo_break_delete_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    P_PAGE_HEFO_BREAK p_page_hefo_break;

    p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, row);
    PTR_ASSERT(p_page_hefo_break);

    if(p_page_hefo_break->region.tl.row == row)
    {
        page_hefo_break_delete(p_docu, p_page_hefo_break);
        page_hefo_break_garbage_collect(p_docu);
    }
}

/******************************************************************************
*
* remove deleted entries from the array
*
******************************************************************************/

static void
page_hefo_break_garbage_collect(
    _DocuRef_   P_DOCU p_docu)
{
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.remove_deleted = 1;
    al_garbage_flags.shrink = 1;
    al_garbage_flags.may_dispose = 1;
    consume(S32, al_array_garbage_collect(&p_docu->h_page_hefo, 0, page_hefo_break_is_deleted, al_garbage_flags));
}

/******************************************************************************
*
* enumerate page_hefo entries
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK /* NULL at end */
page_hefo_break_enum(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ROW p_row /* set to -1 to start */)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(p_page_hefo_break->region.tl.row <= *p_row)
            continue;

        *p_row = p_page_hefo_break->region.tl.row;

        return(p_page_hefo_break);
    }

    return(NULL);
}

/******************************************************************************
*
* find the page number for a given page number
*
******************************************************************************/

_Check_return_
extern PAGE
page_number_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y)
{
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_page_hefo);
    ARRAY_INDEX i;
    P_PAGE_HEFO_BREAK p_page_hefo_break = array_range(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, 0, n_elements);
    ARRAY_INDEX close = -1;
    PAGE page_out = page_y;
    ROW row = row_from_page_y(p_docu, page_y);

    /* search forward to find closest entry */
    for(i = 0; i < n_elements; ++i, ++p_page_hefo_break)
    {
        if(p_page_hefo_break->is_deleted)
            continue;

        if(p_page_hefo_break->region.tl.row > row)
            break;

        if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_NUM))
            close = i;
    }

    if(close >= 0)
    {
        PAGE home_page;
        ROW_ENTRY row_entry;

        p_page_hefo_break = array_ptr(&p_docu->h_page_hefo, PAGE_HEFO_BREAK, close);
        row = p_page_hefo_break->region.whole_col ? 0 : p_page_hefo_break->region.tl.row;
        row_entry_from_row(p_docu, &row_entry, row);
        home_page = row_entry.rowtab.edge_top.page;

        page_out = p_page_hefo_break->page_y + page_y - home_page;
    }

    return(page_out);
}

T5_CMD_PROTO(extern, t5_cmd_insert_page_break)
{
    const ROW row = p_docu->cur.slr.row;
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /* ensure thing at this exact row */
    if(NULL == (p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, row)))
        if(NULL == (p_page_hefo_break = p_page_hefo_break_new_for_row(p_docu, row, &status)))
            return(status);

    page_hefo_selector_bit_set(&p_page_hefo_break->selector, PAGE_HEFO_PAGE_BREAK);

    {
    DOCU_REFORMAT docu_reformat;
    docu_reformat.action = REFORMAT_HEFO_Y;
    docu_reformat.data_type = REFORMAT_ROW;
    docu_reformat.data_space = DATA_NONE;
    docu_reformat.data.row = row;
    status_assert(maeve_event(p_docu, T5_MSG_REFORMAT, &docu_reformat));
    } /*block*/

    caret_show_claim(p_docu, OBJECT_ID_CELLS, TRUE);

    return(STATUS_OK);
}

/*
exported services hook
*/

_Check_return_
static STATUS
sk_hefod_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    UREF_HANDLE uref_handle; /* not stored */
    REGION region = REGION_INIT;

    region.whole_col = TRUE;
    region.whole_row = TRUE;

    p_docu->h_page_hefo = 0;

    return(uref_add_dependency(p_docu, &region, sk_hefod_uref_event, HEFOD_LIST_DEP, &uref_handle, FALSE));
}

T5_MSG_PROTO(static, maeve_services_sk_hefod_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(sk_hefod_msg_init1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_hefod);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_hefod)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_hefod_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of sk_hefod.c */
