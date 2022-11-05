/* ob_mails.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
error numbers
*/

#define MAILSHOT_ERR_BASE   (STATUS_ERR_INCREMENT * OBJECT_ID_MAILSHOT)

#define MAILSHOT_ERR(n)     (MAILSHOT_ERR_BASE - (n))

#define ERR_MAILSHOT_NONE_SELECTED                  MAILSHOT_ERR(0)
#define ERR_MAILSHOT_NONE_TO_SELECT                 MAILSHOT_ERR(2)

/*
messages
*/

#define MAILSHOT_MSG_BASE   (STATUS_MSG_INCREMENT * OBJECT_ID_MAILSHOT)

#define MAILSHOT_MSG(n)     (MAILSHOT_MSG_BASE + (n))

#define MAILSHOT_MSG_DIALOG_SELECT_CAPTION          MAILSHOT_MSG(1)
#define MAILSHOT_MSG_DIALOG_SELECT_HELP_TOPIC       MAILSHOT_MSG(2)
#define MAILSHOT_MSG_DIALOG_SELECT_ROW              MAILSHOT_MSG(3)
#define MAILSHOT_MSG_DIALOG_SELECT_BLANK_GROUP      MAILSHOT_MSG(4)
#define MAILSHOT_MSG_DIALOG_SELECT_BLANK_BLANK      MAILSHOT_MSG(5)
#define MAILSHOT_MSG_DIALOG_SELECT_BLANK_STT        MAILSHOT_MSG(6)
#define MAILSHOT_MSG_DIALOG_SELECT_BLANK_END        MAILSHOT_MSG(7)

#define MAILSHOT_MSG_DIALOG_INSERT_FIELD_CAPTION    MAILSHOT_MSG(10)
#define MAILSHOT_MSG_DIALOG_INSERT_FIELD_HELP_TOPIC MAILSHOT_MSG(11)
#define MAILSHOT_MSG_DIALOG_INSERT_FIELD_NUMBER     MAILSHOT_MSG(12)

/* end of ob_mails.h */
