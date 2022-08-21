/* sk_slot.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Cell (slot) module header */

/* MRJC January 1992 */

#ifndef __sk_slot_h
#define __sk_slot_h

/*
definition of cell structure
*/

typedef struct CELL
{
    /*********************/
    PACKED_OBJECT_ID packed_object_id;
    U8 _spare;
    U8 cell_at_row_bottom;
    U8 cell_new;
    /*********************/

    S32 object[1];
}
CELL, * P_CELL, ** P_P_CELL;

/* cell overheads */
#define CELL_OVH offsetof32(CELL, object)

#define object_id_from_cell(p_cell) \
    OBJECT_ID_UNPACK((p_cell)->packed_object_id)

/*
exported routines
*/

#if defined(UNUSED_KEEP_ALIVE)

enum DOCU_AREA_SCAN_CODES
{
    DOCU_AREA_FIRST_CELL = 0,
    DOCU_AREA_MIDDLE_CELL,
    DOCU_AREA_LAST_CELL,
    DOCU_AREA_END
};

_Check_return_
extern S32 /* docu_area scan code */
docu_area_next_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_POSITION p_position,
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in);

_Check_return_
extern S32 /* docu_area scan code */
docu_area_next_cell(
    _OutRef_    P_POSITION p_position,
    _InRef_     PC_DOCU_AREA p_docu_area);

#endif /* UNUSED_KEEP_ALIVE */

#define p_docu_area_from_scan_block(p_scan_block) ( \
    &((p_scan_block)->docu_area) )

_Check_return_
_Ret_maybenull_
extern P_CELL
p_cell_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

/*
cells scan state block
*/

typedef struct SCAN_BLOCK_STATE
{
    UBF scan_direction : 2;
    UBF new_block : 1;              /* move onto next markers block */
    UBF scan_what : 2;              /* what we are to scan */
}
SCAN_BLOCK_STATE;

typedef struct SCAN_BLOCK
{
    DOCU_AREA docu_area;
    SLR slr;
    SCAN_BLOCK_STATE state;
    ARRAY_INDEX ix_block;               /* block counter */
    ARRAY_HANDLE h_markers;             /* handle to array of markers blocks */
    OBJECT_ID object_id;
}
SCAN_BLOCK, * P_SCAN_BLOCK;

#define ix_marker_from_scan_block(p_scan_block) ( \
    (p_scan_block)->ix_block - 1 )

/*
codes for cells_scan_init
*/

enum SCAN_INIT_DIRECTION
{
    SCAN_ACROSS,                        /* scans logical, across columns first */
    SCAN_DOWN,                          /* scans logical, down rows first */
    SCAN_MATRIX                         /* scans physical, down rows first */
};

enum SCAN_INIT_WHAT
{
    SCAN_WHOLE,
    SCAN_MARKERS,
    SCAN_AREA,
    SCAN_FROM_CUR
};

_Check_return_
extern STATUS
cells_scan_init(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SCAN_BLOCK p_scan_block,
    _InVal_     enum SCAN_INIT_DIRECTION scan_direction,
    _InVal_     enum SCAN_INIT_WHAT scan_what,
    P_DOCU_AREA p_docu_area,
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS /* STATUS_OK = end, STATUS_DONE = have result */
cells_scan_next(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data,
    P_SCAN_BLOCK p_scan_block);

_Check_return_
extern S32
cells_scan_percent(
    P_SCAN_BLOCK p_scan_block);

_Check_return_
extern STATUS
cells_blank_make(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern BOOL
cells_block_is_blank(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW row_s,
    _InVal_     S32 n_rows);

_Check_return_
extern STATUS
cells_block_blank_make(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW row_s,
    _InVal_     S32 n_rows);

_Check_return_
extern STATUS
cells_block_copy_no_uref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_to,
    _InRef_     PC_SLR p_slr_from_s,
    _InRef_     PC_SLR p_slr_from_e);

extern void
cells_block_delete(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW at_row,
    _InVal_     S32 n_rows);

_Check_return_
extern STATUS
cells_block_insert(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW at_row,
    _InVal_     S32 n_rows,
    _InVal_     S32 add);

_Check_return_
extern STATUS
cells_column_delete(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL at_col,
    _InVal_     COL n_cols,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e);

_Check_return_
extern STATUS
cells_column_insert(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL at_col,
    _InVal_     COL n_cols,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e,
    _InVal_     S32 add);

_Check_return_
extern STATUS
cells_docu_area_delete(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InVal_     BOOL delete_whole_rows,
    _InVal_     BOOL before_and_after);

_Check_return_
extern STATUS
cells_docu_area_insert(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _OutRef_    P_POSITION p_position_out);

_Check_return_
extern STATUS
cells_object_split(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_POSITION p_position);

_Check_return_
static inline BOOL
slr_is_blank(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    return(NULL == p_cell_from_slr(p_docu, p_slr));
}

_Check_return_
extern STATUS
slr_realloc(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_CELL p_p_cell,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id,
    _InVal_     S32 object_size);

_Check_return_
extern STATUS
cells_swap_rows(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REGION p_region);

#endif /* __sk_slot_h */

/* end of sk_slot.h */
