/* ob_skspt.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS October 1995 Split off UI for skeleton internal header; SKS May 2014 other UIxxx modules merged back */

#ifndef __ob_skspt_h
#define __ob_skspt_h

/*
errors
*/

#define SKEL_SPLIT_ERR_BASE  (STATUS_ERR_INCREMENT * OBJECT_ID_SKEL_SPLIT)

#define SKEL_SPLIT_ERR(n)    (SKEL_SPLIT_ERR_BASE - (n))

#define SKEL_SPLIT_ERR_CANT_DELETE_BASE_STYLE                   SKEL_SPLIT_ERR(0)
/* -1 is ERROR_RQ */
#define SKEL_SPLIT_ERR_spare                                    SKEL_SPLIT_ERR(2)

/*
messages
*/

#define SKEL_SPLIT_MSG_BASE (STATUS_MSG_INCREMENT * OBJECT_ID_SKEL_SPLIT)

#define SKEL_SPLIT_MSG(n)   (SKEL_SPLIT_MSG_BASE + (n))

#define SKEL_SPLIT_MSG_DIALOG_STYLE_CAPTION                     SKEL_SPLIT_MSG(1)
#define SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_CAPTION                 SKEL_SPLIT_MSG(2)
#define SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_SUGGESTED               SKEL_SPLIT_MSG(3)
#define SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_NAME                    SKEL_SPLIT_MSG(4)
#define SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_BASED_ON                SKEL_SPLIT_MSG(5)
#define SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_DEFINING                SKEL_SPLIT_MSG(6)
#define SKEL_SPLIT_MSG_DIALOG_EDITING_STYLE                     SKEL_SPLIT_MSG(7)
#define SKEL_SPLIT_MSG_DIALOG_APPLY_EFFECTS                     SKEL_SPLIT_MSG(8)
#define SKEL_SPLIT_MSG_DIALOG_REMOVE_STYLE_USE_QUERY            SKEL_SPLIT_MSG(9)
#define SKEL_SPLIT_MSG_DIALOG_STYLE_HELP_TOPIC                  SKEL_SPLIT_MSG(10)
#define SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_HELP_TOPIC              SKEL_SPLIT_MSG(11)
#define SKEL_SPLIT_MSG_NEW_OBJECT_CAPTION                       SKEL_SPLIT_MSG(12)
#define SKEL_SPLIT_MSG_DIALOG_REGION_EDIT_CAPTION               SKEL_SPLIT_MSG(13)
#define SKEL_SPLIT_MSG_DIALOG_REGION_EDIT_HELP_TOPIC            SKEL_SPLIT_MSG(14)
#define SKEL_SPLIT_MSG_DIALOG_NEW_STYLE_STYLE                   SKEL_SPLIT_MSG(15)
#define SKEL_SPLIT_MSG_DIALOG_REMOVE_STYLE_USE_QUERY_HELP_TOPIC SKEL_SPLIT_MSG(16)

#define SKEL_SPLIT_MSG_DIALOG_ES_STYLE_HELP_TOPIC               SKEL_SPLIT_MSG(20)
#define SKEL_SPLIT_MSG_DIALOG_ES_FS_HELP_TOPIC                  SKEL_SPLIT_MSG(21)
#define SKEL_SPLIT_MSG_DIALOG_ES_PS2_HELP_TOPIC                 SKEL_SPLIT_MSG(22)
#define SKEL_SPLIT_MSG_DIALOG_ES_PS1_HELP_TOPIC                 SKEL_SPLIT_MSG(23)
#define SKEL_SPLIT_MSG_DIALOG_ES_PS3_HELP_TOPIC                 SKEL_SPLIT_MSG(24)
#define SKEL_SPLIT_MSG_DIALOG_ES_PS4_HELP_TOPIC                 SKEL_SPLIT_MSG(25)
#define SKEL_SPLIT_MSG_DIALOG_ES_RS_HELP_TOPIC                  SKEL_SPLIT_MSG(26)

#endif /* __ob_skspt_h */

/* end of ob_skspt.h */
