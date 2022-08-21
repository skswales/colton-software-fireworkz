/* fs_ascii.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header file for ASCII save module */

/* SKS January 1993 */

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

/* #define ASCII_MSG_BASE (STATUS_MSG_INCREMENT * OBJECT_ID_FS_ASCII) */

T5_MSG_PROTO(extern, ascii_msg_save_foreign, _InoutRef_ P_MSG_SAVE_FOREIGN p_msg_save_foreign);

/* end of fs_ascii.h */
