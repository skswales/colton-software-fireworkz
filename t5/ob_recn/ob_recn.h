/* ob_recn.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2021 Stuart Swales */

/* rec-dummy object module internal header */

/* SKS Aug 2014 */

#ifndef __ob_recn_h
#define __ob_recn_h

/*
messages
*/

#define RECN_MSG_BASE   (STATUS_MSG_INCREMENT * OBJECT_ID_RECN)

#define RECN_MSG(n)     (RECN_MSG_BASE + (n))

#define RECN_MSG_FIRST  RECN_MSG(0)

/*
error numbers
*/

#define RECN_ERR_BASE   (STATUS_ERR_INCREMENT * OBJECT_ID_RECN)

#define RECN_ERR(n)     (RECN_ERR_BASE - (n))

#define RECN_ERR_ERROR_FIRST                RECN_ERR(0)
#define RECN_ERR_ERROR_RQ                   RECN_ERR(1)
#define RECN_ERR_NO_DATABASE_MODULE         RECN_ERR(2)
/* -3,-4,...*/

#endif /* __ob_recn_h */

/* end of ob_recn.h */
