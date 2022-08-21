/* sk_form.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* rowtab module header */

/* MRJC December 1991 */

#ifndef __sk_form_h
#define __sk_form_h

/*
row table entry structure
*/

typedef struct ROWTAB
{
    SKEL_EDGE edge_top;         /* top edge (inclusive) */
    SKEL_EDGE edge_bot;         /* bottom edge (exclusive) */
}
ROWTAB, * P_ROWTAB;

typedef struct ROW_ENTRY
{
    ROWTAB rowtab;
    ROW row;
}
ROW_ENTRY, * P_ROW_ENTRY;

/*
row information structure
*/

typedef struct ROW_INFO
{
    ROW row;                    /* row number */
    PIXIT height;               /* height of row */
    U8 height_fixed;            /* height of row */
    U8 unbreakable;             /* row unbreakable ? */
    PAGE page;                  /* current y page */
    PIXIT page_height;          /* height of row area on current page */
}
ROW_INFO, * P_ROW_INFO;

/*
exported routines
*/

extern void
docu_flags_new_extent_clear(
    _DocuRef_   P_DOCU p_docu);

extern void
docu_flags_new_extent_set(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
format_col_row_extents_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL n_cols,
    _InVal_     ROW n_rows);

extern void
format_object_how_big(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_HOW_BIG p_object_how_big);

_Check_return_
extern STATUS /* >0 object - please update yourself */
format_object_size_set(
    _DocuRef_   P_DOCU p_docu,
    _InRef_opt_ PC_SKEL_RECT p_skel_rect_new /* new object rect */,
    _InRef_opt_ PC_SKEL_RECT p_skel_rect_old /* old rectangle */,
    _InRef_     PC_SLR p_slr,
    _InVal_     BOOL object_self_update /* TRUE = object wants to update itself if it can */);

_Check_return_
static inline ROW
n_rows(
    _DocuRef_   PC_DOCU p_docu)
{
    return(p_docu->rows_logical);
}

_Check_return_
extern STATUS
n_rows_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW rows);

_Check_return_
extern PIXIT
page_height_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

extern void
reformat_all(
    _DocuRef_   P_DOCU p_docu);

extern void
reformat_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     enum DOCU_REFORMAT_ACTION reformat_action /*REFORMAT_Y,REFORMAT_XY*/);

_Check_return_
extern STATUS
row_entry_at_skel_point(
    _DocuRef_   P_DOCU p_docu,
    P_ROW_ENTRY p_row_entry,
    _InRef_     PC_SKEL_POINT p_skel_point);

extern void
row_entry_from_row(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW_ENTRY p_row_entry,
    _InVal_     ROW row);

extern void
row_entries_from_row(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW_ENTRY p_row_entry,
    _OutRef_    P_ROW_ENTRY p_row_entry_next,
    _InVal_     ROW row);

extern void
row_entry_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    P_ROW_ENTRY p_row_entry,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InVal_     BOOL down /* points on row boundaries go down */);

_Check_return_
extern ROW
row_from_docu_reformat(
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat);

_Check_return_
extern ROW
row_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y);

_Check_return_
extern STATUS
virtual_row_table_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL new_vrt);

#endif /* __sk_form_h */

/* end of sk_form.h */
