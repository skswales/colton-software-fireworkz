/* ob_drwio.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2001-2015 R W Colton */

/* Save as Drawfile object module for Fireworkz */

/* SKS July 2001 */

/*
messages
*/

#define DRAW_IO_MSG_BASE    (STATUS_MSG_INCREMENT * OBJECT_ID_DRAW_IO)

#define DRAW_IO_MSG(n)      (DRAW_IO_MSG_BASE + (n))

#define MSG_DIALOG_SAVE_AS_DRAWFILE_INTRO_CAPTION       DRAW_IO_MSG(1)
#define MSG_DIALOG_SAVE_AS_DRAWFILE_INTRO_HELP_TOPIC    DRAW_IO_MSG(2)

#define ARG_HYBRID_SETUP_FILETYPE   0
#define ARG_HYBRID_SETUP_PAGE_RANGE 1
#define ARG_HYBRID_SETUP_N_ARGS     2

/*
exported routines
*/

_Check_return_
extern STATUS
save_as_drawfile_host_print_document(
    _DocuRef_   P_DOCU p_docu,
    P_PRINT_CTRL p_save_as_drawfile_ctrl,
    _In_opt_z_  PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
find_fireworkz_data_in_drawfile(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format);

/* end of ob_drwio.h */
