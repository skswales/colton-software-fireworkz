/* sk_col.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC January 1992 */

#ifndef __sk_col_h
#define __sk_col_h

/*
exported routines
*/

_Check_return_
extern COL
col_at_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_col_info,
    _InRef_     PC_SKEL_POINT p_skel_point);

_Check_return_
extern ARRAY_HANDLE
col_changes_between_rows(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_start,
    _InVal_     ROW row_end);

_Check_return_
extern COL
col_left_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_col_info,
    _InRef_     PC_SKEL_POINT p_skel_point);

_Check_return_
extern COL
col_overlap_from_slr_grid(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_col_info,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern PIXIT
col_width(
    _InVal_     ARRAY_HANDLE h_col_info,
    _InVal_     COL col);

extern void
find_object_extent(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SLR p_slr_out);

_Check_return_
extern PAGE
last_page_x(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern PAGE
last_page_x_non_blank(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern PAGE
last_page_y(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern PAGE
last_page_y_non_blank(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern COL
n_cols_max(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e);

_Check_return_
extern COL
n_cols_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

_Check_return_
extern PIXIT
non_blank_height(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern PIXIT
non_blank_width(
    _DocuRef_   P_DOCU p_docu);

extern void
page_x_extent_set(
    _DocuRef_   P_DOCU p_docu);

/*ncr*/
_Ret_valid_
extern P_REGION
region_visible(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_REGION p_region);

_Check_return_
extern BOOL /* TRUE == object is visible */
row_is_visible(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

extern void
skel_col_enum_cache_dispose(
    _InVal_     DOCNO docno,
    _InRef_     PC_REGION p_region);

extern void
skel_col_enum_cache_lock(
    _InVal_     ARRAY_HANDLE h_col_info);

extern void
skel_col_enum_cache_release(
    _InVal_     ARRAY_HANDLE h_col_info);

_Check_return_
extern STATUS
skel_col_enum(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _InVal_     PAGE page,
    _InVal_     COL col,
    _OutRef_    P_ARRAY_HANDLE p_h_col_info); /* NOT owned by caller - do not dispose */

extern void
skel_point_from_slr_tl(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_POINT p_skel_point,
    _InRef_     PC_SLR p_slr);

extern void
skel_point_normalise(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKEL_POINT p_skel_point,
    _InVal_     REDRAW_TAG redraw_tag);

extern void
skel_rect_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_RECT p_skel_rect,
    _InRef_     PC_DOCU_AREA p_docu_area);

extern void
skel_rect_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_RECT p_skel_rect,
    _InRef_     PC_SLR p_slr);

enum slr_owner_direction
{
    ON_ROW_EDGE_GO_UP = 0,
    ON_ROW_EDGE_GO_DOWN
};

_Check_return_
extern STATUS
slr_owner_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SLR p_slr,
    P_SKEL_POINT p_tl_out,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InVal_     S32 direction /* go up or down on row edge ? */);

_Check_return_
extern BOOL /* 1 == in table */
cell_in_table(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern BOOL /* 1 == in table */
docu_area_spans_across_table(
    _DocuRef_   P_DOCU p_docu,
    P_DOCU_AREA p_docu_area);

_Check_return_
extern PIXIT
cell_width(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern PIXIT
total_docu_height(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern PIXIT
total_docu_width(
    _DocuRef_   P_DOCU p_docu);

#endif /* __sk_col_h */

/* end of sk_col.h */
