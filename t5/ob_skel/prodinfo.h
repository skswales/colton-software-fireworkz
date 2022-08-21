/* prodinfo.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __prodinfo_h
#define __prodinfo_h

/*
private data visible through prodinfo.c functions, set by winmain.c/main.c - do not use elsewhere
*/

#define REG_NAME_LENGTH 29 /* forever value */
#define REG_NUMB_LENGTH 16 /* forever value */

extern const PCTSTR __product_name;
extern const PCTSTR __product_family_name;
extern const PCTSTR __product_ui_name;
extern TCHARZ __user_name[64];
extern TCHARZ __organisation_name[64];
extern TCHARZ __registration_number[REG_NUMB_LENGTH + 1]; /* NB no spaces */

#if RISCOS
extern const PC_U8Z g_product_sprite_name;
#endif

#endif /* __prodinfo_h */

/* end of prodinfo.h */
