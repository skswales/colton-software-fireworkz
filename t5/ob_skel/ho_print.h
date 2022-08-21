/* ho_print.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Host specific print routines for Fireworkz */

/* RCM Aug 1992 */

#ifndef __ho_print_h
#define __ho_print_h

extern void
host_read_default_paper_details(
    P_PAPER p_paper);

_Check_return_
extern STATUS
host_read_printer_paper_details(
    P_PAPER p_paper);

extern void
host_printer_name_query(
    _OutRef_    P_UI_TEXT p_ui_text);

_Check_return_
extern STATUS
host_print_document(
    _DocuRef_   P_DOCU p_docu,
    P_PRINT_CTRL p_print_ctrl);

#if WINDOWS

_Check_return_
extern STATUS
host_printer_set(
    _In_opt_z_  PCTSTR printername);

#endif /* OS */

#endif /* ho_print_h */

/* end of ho_print.h */
