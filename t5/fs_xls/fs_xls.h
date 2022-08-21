/* fs_xls.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2018 Stuart Swales */

/* Local header file for Excel spreadsheet save object module */

/* SKS April 2014 */

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#include "fl_xls/ff_xls.h"

_Check_return_
extern STATUS
xls_save_biff(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format);

_Check_return_
extern STATUS
xls_save_xml(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format);

/* end of fs_xls.h */
