/* fl_csv.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header file for CSV load module */

/* SKS January 1993 */

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

/*
error numbers
*/

#define CSV_ERR_BASE    (STATUS_ERR_INCREMENT * OBJECT_ID_FL_CSV)

#define CSV_ERR(n)      (CSV_ERR_BASE - (n))

#define CSV_ERR_DOC_NOT_SAVED                   CSV_ERR(0)
#define CSV_ERR_CANT_CREATE_DB_ISAFILE          CSV_ERR(2)

/*
messages
*/

#define CSV_MSG_BASE    (STATUS_MSG_INCREMENT * OBJECT_ID_FL_CSV)

#define CSV_MSG(n)      (CSV_MSG_BASE + (n))

#define CSV_MSG_LOAD_QUERY_CAPTION                  CSV_MSG(1)
#define CSV_MSG_LOAD_QUERY_HELP_TOPIC               CSV_MSG(2)
#define CSV_MSG_LOAD_QUERY_INSERT_AS_TABLE          CSV_MSG(3)
#define CSV_MSG_LOAD_QUERY_LABELS                   CSV_MSG(4)
#define CSV_MSG_LOAD_QUERY_ACROSS                   CSV_MSG(5)
#define CSV_MSG_LOAD_QUERY_OVERWRITE_BLANK_CELLS    CSV_MSG(7)
#define CSV_MSG_LOAD_QUERY_DATABASE                 CSV_MSG(8)
#define CSV_MSG_LOAD_QUERY_DATABASE_CAPTION         CSV_MSG(9)
#define CSV_MSG_LOAD_QUERY_DELIMITED_TEXT_CAPTION   CSV_MSG(10)

/* end of fl_csv.h */
