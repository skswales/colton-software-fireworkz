/* personal.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
Copy these as needed into PERSONAL_TRACE_FLAGS
Master list is always in debug.h
*/

#define PERSONAL_TRACE_FLAGS_PARKING ( 0 | \
    TRACE_MODULE_EVAL       | \
    TRACE_MODULE_SPELL      | \
    TRACE_MODULE_ALIGATOR   | \
    TRACE_MODULE_NUMFORM    | \
    TRACE_MODULE_FILE       | \
    TRACE_MODULE_ALLOC      | \
    TRACE_MODULE_CFBF       | \
    TRACE_MODULE_UREF       | \
    TRACE_MODULE_GR_CHART   | \
    TRACE_APP_FONTS         | \
    TRACE_APP_SKEL          | \
    TRACE_APP_PD4           | \
    TRACE_APP_DPLIB         | \
    TRACE_MODULE_LIST       | \
    TRACE_APP_CLICK         | \
    TRACE_APP_MAEVE         | \
    TRACE_APP_LOAD          | \
    TRACE_APP_HOST_PAINT    | \
    TRACE_APP_HOST          | \
    TRACE_APP_PRINT         | \
    TRACE_APP_UREF          | \
    TRACE_APP_ARGLIST       | \
    TRACE_APP_FORMAT        | \
    TRACE_APP_SKEL_DRAW     | \
    TRACE_APP_MEASUREMENT   | \
    TRACE_APP_STYLE         | \
    TRACE_APP_DIALOG        | \
    TRACE_APP_FOREIGN       | \
    TRACE_APP_CLIPBOARD     | \
    TRACE_APP_MEMORY_USE    | \
    TRACE_APP_WM_EVENT      | \
    0 )

/* always set TRACE_OUT to get past first check of tracing() macro */
#define PERSONAL_TRACE_FLAGS ( TRACE_OUT | \
    TRACE_APP_CLIPBOARD     | \
    TRACE_APP_FOREIGN       | \
    0 )

#define STUART 1

#if !RELEASED && STUART && 0
#define TRACE_MAIN_ALLOCS 1
#define VALIDATE_MAIN_ALLOCS 1
#define VALIDATE_MAIN_ALLOCS_START 1
#define ALLOC_CLEAR_FREE 1
#endif

/* end of personal.h */
