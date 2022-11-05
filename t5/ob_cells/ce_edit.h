/* ce_edit.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC August 1994 */

#ifndef __ce_edit_h
#define __ce_edit_h

/*
external routines
*/

#define OK_CELLS_EDIT TRUE
#define NO_CELLS_EDIT FALSE

_Check_return_
extern STATUS
cell_call_id(
    _InVal_     OBJECT_ID object_id,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InVal_     BOOL allow_cells_edit);

/*ncr*/
extern BOOL
cell_data_from_docu_area_br(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area);

/*ncr*/
extern BOOL
cell_data_from_docu_area_tl(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area);

/*ncr*/
extern BOOL
cell_data_from_position(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_POSITION p_position);

/*ncr*/
extern BOOL
cell_data_from_position_and_object_position(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_POSITION p_position,
    _InRef_maybenone_ PC_OBJECT_POSITION p_object_position_end);

/*ncr*/
extern BOOL
cell_data_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern OBJECT_ID
cell_owner_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

#endif /* __ce_edit_h */

/* end of ce_edit.h */
