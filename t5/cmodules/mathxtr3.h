/* mathxtr3.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2018 Stuart Swales */

/* Additional math routines */

/* SKS May 2012 */

#ifndef __mathxtr3_h
#define __mathxtr3_h

/*
exported functions
*/

/*
'random' numbers
*/

_Check_return_
extern F64
normal_distribution(void);

_Check_return_
extern F64
uniform_distribution(void);

extern void
uniform_distribution_seed(
    _InVal_     unsigned int seed);

/*ncr*/
extern BOOL
uniform_distribution_test_seeded(
    _InVal_     BOOL fEnsure);

#endif /* __mathxtr3_h */

/* end of mathxtr3.h  */
