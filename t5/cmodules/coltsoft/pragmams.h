/* pragmams.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Controls pragmas for Microsoft C compiler */

/* SKS 15-Jan-1991 */

/* Designed for multiple inclusion */

#if WINDOWS

/* --- /Gs --- */

#ifdef PRAGMA_CHECK_STACK
#undef PRAGMA_CHECK_STACK
#pragma check_stack()
#endif

#ifdef PRAGMA_CHECK_STACK_ON
#undef PRAGMA_CHECK_STACK_ON
#pragma check_stack(on)
#endif

#ifdef PRAGMA_CHECK_STACK_OFF
#undef PRAGMA_CHECK_STACK_OFF
#pragma check_stack(off)
#endif

#endif /* WINDOWS */

/* end of pragmams.h */
