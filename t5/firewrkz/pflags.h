/* pflags.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* pflags.h for Fireworkz */

/* SKS Apr 1993 */

#ifndef FIREWORKZ
#error  FIREWORKZ is not defined
#elif   FIREWORKZ != 1
#error  FIREWORKZ is defined incorrectly
#endif

/*
objects required to be __bound__ into image
*/

#define BIND_OB_CELLS
#define BIND_OB_CELLS_EDIT
#define LOADED_OB_CHART
#define BIND_OB_DIALOG
#define LOADED_OB_DRAW
#define LOADED_OB_DRAW_IO
#define BIND_OB_FILE
/*#define BIND_OB_FOOTER installed by HEFO*/
/*#define BIND_OB_HEADER installed by HEFO*/
#define LOADED_OB_HEFO
#define BIND_OB_IMPLIED_STYLE
#define LOADED_OB_MAILSHOT
#if RISCOS
#define BIND_OB_MLEC
#endif
#define LOADED_OB_NOTE
#define LOADED_OB_RULER
#define LOADED_OB_SKEL_SPLIT
#define BIND_OB_SLE
#define BIND_OB_SPELB
#define LOADED_OB_SPELL
#define BIND_OB_SS
#define BIND_OB_STORY
#define BIND_OB_TEXT
#define BIND_OB_TOOLBAR

#define LOADED_FL_ASCII
#define LOADED_FL_CSV
#define LOADED_FL_FWP
#define LOADED_FL_LOTUS123
#define LOADED_FL_PDSS
#define LOADED_FL_PDTX
#define LOADED_FL_RTF
#define LOADED_FL_XLS

#define LOADED_FS_ASCII
#define LOADED_FS_CSV
#define LOADED_FS_LOTUS123
#define LOADED_FS_RTF

#define LOADED_OB_RECN

#if WINDOWS /* extra for Windows Fireworkz ('R-Comp sponsored') */
#define LOADED_FS_XLS
#endif

#if RISCOS /* until we implement LOAD_MESSAGES_FILE */
#define BOUND_MESSAGES_OBJECT_ID_CHART
#define BOUND_MESSAGES_OBJECT_ID_DIALOG
#define BOUND_MESSAGES_OBJECT_ID_DRAW
#define BOUND_MESSAGES_OBJECT_ID_DRAW_IO
#define BOUND_MESSAGES_OBJECT_ID_FILE
#define BOUND_MESSAGES_OBJECT_ID_HEFO
#define BOUND_MESSAGES_OBJECT_ID_MAILSHOT
#define BOUND_MESSAGES_OBJECT_ID_MLEC
#define BOUND_MESSAGES_OBJECT_ID_RULER
#define BOUND_MESSAGES_OBJECT_ID_SKEL
#define BOUND_MESSAGES_OBJECT_ID_SKEL_SPLIT
#define BOUND_MESSAGES_OBJECT_ID_SPELB
#define BOUND_MESSAGES_OBJECT_ID_SPELL
#define BOUND_MESSAGES_OBJECT_ID_SS
#define BOUND_MESSAGES_OBJECT_ID_TOOLB

#define BOUND_MESSAGES_OBJECT_ID_FL_CSV
#define BOUND_MESSAGES_OBJECT_ID_FL_RTF
#define BOUND_MESSAGES_OBJECT_ID_FL_LOTUS123
#define BOUND_MESSAGES_OBJECT_ID_FL_XLS

#define BOUND_MESSAGES_OBJECT_ID_RECN
#endif

/* end of pflags.h */
