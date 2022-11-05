/* fl_123.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header file for Lotus 1-2-3 spreadsheet load object module */

/* MRJC March 1988 / July 1993 */

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#include "fl_123/ff_123.h"

/* #define LOTUS123_MSG_BASE (STATUS_MSG_INCREMENT * OBJECT_ID_FL_LOTUS123) */

/*
error numbers
*/

#define LOTUS123_ERR_BASE   (STATUS_ERR_INCREMENT * OBJECT_ID_FL_LOTUS123)

#define LOTUS123_ERR(n)     (LOTUS123_ERR_BASE - (n))

#define LOTUS123_ERR_BADFILE        LOTUS123_ERR(0)
#define LOTUS123_ERR_EXP            LOTUS123_ERR(2) /* -1 is reserved */

/* end of fl_123.h */
