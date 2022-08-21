/* mathxtri.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS July 1991 */

#define F64_MANT_DIG DBL_MANT_DIG
#define F64_EPSILON  DBL_EPSILON
#define F64_HUGE_VAL HUGE_VAL

#if RISCOS || WINDOWS
#define F64_MAX_SQUAREABLE 1.3407807929942596E154
#endif

#define F64_ZERO  0.0
#define F64_HALF  0.5
#define F64_ONE   1.0
#define F64_TWO   2.0
#define F64_EIGHT 8.0

#if (FLT_RADIX == 2)
#define __LN_RADIX_BASE _ln_two
#elif (FLT_RADIX == 10)
#define __LN_RADIX_BASE _ln_ten
#else
#error Unable to compile mathxtra.c code for this radix
#endif /* FLT_RADIX */

/* end of mathxtri.h */
