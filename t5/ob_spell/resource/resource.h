/* ob_spell/resource/resource.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#define SPELL_MSG_BASE (STATUS_MSG_INCREMENT * OBJECT_ID_SPELL)

#define OB_SPELL_MSG_DICTIONARY_HELP_TOPIC (SPELL_MSG_BASE + 1)
#define OB_SPELL_MSG_DICTIONARY_CAPTION (SPELL_MSG_BASE + 3)
#define OB_SPELL_MSG_WORD               (SPELL_MSG_BASE + 4)
#define OB_SPELL_MSG_ADD_WORD           (SPELL_MSG_BASE + 5)
#define OB_SPELL_MSG_DELETE_WORD        (SPELL_MSG_BASE + 6)

#define OB_SPELL_MSG_CHECK_HELP_TOPIC   (SPELL_MSG_BASE + 2)
#define OB_SPELL_MSG_CHECK_CAPTION      (SPELL_MSG_BASE + 9)
#define OB_SPELL_MSG_REPLACE            (SPELL_MSG_BASE +10)
#define OB_SPELL_MSG_SKIP               (SPELL_MSG_BASE +11)
#define OB_SPELL_MSG_GUESS              (SPELL_MSG_BASE +12)
#define OB_SPELL_MSG_CANCEL             (SPELL_MSG_BASE +13)

#define OB_SPELL_MSG_CHECK_COMPLETE     (SPELL_MSG_BASE +19)
#define OB_SPELL_MSG_CHECK_WITH_ERR     (SPELL_MSG_BASE +20)
#define OB_SPELL_MSG_CHECK_WITH_ERRS    (SPELL_MSG_BASE +21)

#define OB_SPELL_MSG_ALL_DICTS          (SPELL_MSG_BASE +22)

#define SPELL_ERR_NO_USER_DICTS         (SPELL_ERR_BASE - 100)

/*
error definition
*/

#define SPELL_ERR_BASE  (STATUS_ERR_INCREMENT * OBJECT_ID_SPELL)

#define SPELL_ERR(n)    (SPELL_ERR_BASE - (n))

#define SPELL_ERR_EXISTS        SPELL_ERR(0)
#define SPELL_ERR_ERROR_RQ      SPELL_ERR(1)
#define SPELL_ERR_FILE          SPELL_ERR(2)
#define SPELL_ERR_CANTOPEN      SPELL_ERR(3)
#define SPELL_ERR_BADDICT       SPELL_ERR(4)
#define SPELL_ERR_spare_5       SPELL_ERR(5)
#define SPELL_ERR_BADWORD       SPELL_ERR(6)
#define SPELL_ERR_READONLY      SPELL_ERR(7)
#define SPELL_ERR_CANTCLOSE     SPELL_ERR(8)
#define SPELL_ERR_NOTIMP        SPELL_ERR(9)
#define SPELL_ERR_NOTFOUND      SPELL_ERR(10)
#define SPELL_ERR_ESCAPE        SPELL_ERR(11)
#define SPELL_ERR_CANTWRITE     SPELL_ERR(12)
#define SPELL_ERR_CANTREAD      SPELL_ERR(13)
#define SPELL_ERR_CANTENLARGE   SPELL_ERR(14)
#define SPELL_ERR_BADDEFFILE    SPELL_ERR(15)
#define SPELL_ERR_DEFCHARERR    SPELL_ERR(16)
#define SPELL_ERR_CANTOPENDEFN  SPELL_ERR(17)
#define SPELL_ERR_CANTCLOSEDEFN SPELL_ERR(18)

#define SPELL_ERR_END           SPELL_ERR(19)

/* icon ids */

/* bitmap ids */
#if WINDOWS
#define SPELL_ID_BM_COM11X11_ID 200
#define SPELL_ID_BM_COM11X11(n) T5_RESOURCE_COMMON_BMP(SPELL_ID_BM_COM11X11_ID, n)

#define SPELL_ID_BM_INC    SPELL_ID_BM_COM11X11(0)
#define SPELL_ID_BM_DEC    SPELL_ID_BM_COM11X11(1)
#else
#define SPELL_ID_BM_INC    "inc"
#define SPELL_ID_BM_DEC    "dec"
#endif

/* end of ob_spell/resource/resource.h */
