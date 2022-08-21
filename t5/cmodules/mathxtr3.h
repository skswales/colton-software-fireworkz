/* mathxtr3.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2021 Stuart Swales */

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
extern double
normal_distribution(void);

/* Generates a random number on [0,1) */ 

_Check_return_
extern double
uniform_distribution(void);

extern void
uniform_distribution_seed(
    _In_    const unsigned int seed);

/*ncr*/
extern bool
uniform_distribution_test_seeded(
    _In_    const bool fEnsure);

#endif /* __mathxtr3_h */

/* end of mathxtr3.h  */
