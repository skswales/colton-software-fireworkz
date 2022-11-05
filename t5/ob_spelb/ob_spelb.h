/* ob_spelb.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spell (bound) object module internal header */

/* SKS Sep 1993 */

#ifndef __ob_spelb_h
#define __ob_spelb_h

/*
messages
*/

#define SPELB_MSG_BASE  (STATUS_MSG_INCREMENT * OBJECT_ID_SPELB)

#define SPELB_MSG(n)    (SPELB_MSG_BASE + (n))

#define MSG_DIALOG_CHOICES_SPELB_GROUP          SPELB_MSG(0)
#define MSG_DIALOG_CHOICES_SPELB_AUTO_CHECK     SPELB_MSG(1)
#define MSG_DIALOG_CHOICES_SPELB_DICT_GROUP     SPELB_MSG(2)
#define MSG_DIALOG_CHOICES_SPELB_LOAD_MASTER    SPELB_MSG(3)
#define MSG_DIALOG_CHOICES_SPELB_LOAD_USER      SPELB_MSG(4)
#define MSG_DIALOG_CHOICES_SPELB_WRITE_USER     SPELB_MSG(5)

#endif /* __ob_spelb_h */

/* end of ob_spelb.h */
