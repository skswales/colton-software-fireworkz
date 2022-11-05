/* ob_file.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* File service object module internal header */

/* SKS April 1992 */

#include "ob_file/xp_file.h"

/*
messages
*/

#define FILE_MSG_BASE   (STATUS_MSG_INCREMENT * OBJECT_ID_FILE)

#define FILE_MSG(n)     (FILE_MSG_BASE + (n))

#define FILE_MSG_INSERT_FILE_CAPTION    FILE_MSG(1)

/* end of ob_file.h */
