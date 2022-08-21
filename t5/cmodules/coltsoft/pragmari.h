/* pragmari.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Controls pragmas for RISC OS Norcroft compiler */

/* SKS 15-Jan-1991 */

/* Designed for multiple inclusion */

#if defined(__CC_NORCROFT)

/* --- stack overflow checking --- */

/* allows for one level of nesting, default is check_stack */

#ifdef    PRAGMA_CHECK_STACK_ON
#undef    PRAGMA_CHECK_STACK_ON
#ifdef  __PRAGMA_CHECK_STACK_OFF_ALREADY
#undef  __PRAGMA_CHECK_STACK_OFF_ALREADY
#else
#pragma check_stack
#undef  __PRAGMA_CHECK_STACK_OFF
#endif
#endif

#ifdef    PRAGMA_CHECK_STACK_OFF
#undef    PRAGMA_CHECK_STACK_OFF
#ifdef  __PRAGMA_CHECK_STACK_OFF
#define __PRAGMA_CHECK_STACK_OFF_ALREADY
#else
#pragma no_check_stack
#define __PRAGMA_CHECK_STACK_OFF
#endif
#endif

/* --- CSE optimisation --- */

#ifdef    PRAGMA_SIDE_EFFECTS
#undef    PRAGMA_SIDE_EFFECTS
#pragma side_effects
#endif

#ifdef    PRAGMA_SIDE_EFFECTS_OFF
#undef    PRAGMA_SIDE_EFFECTS_OFF
#pragma no_side_effects
#endif

/* --- printf-type arg checking --- */

#ifdef    PRAGMA_STDARG_CHECK
#undef    PRAGMA_STDARG_CHECK
#pragma check_printf_formats
#endif

#ifdef    PRAGMA_STDARG_CHECK_OFF
#undef    PRAGMA_STDARG_CHECK_OFF
#pragma no_check_printf_formats
#endif

#endif /* __CC_NORCROFT */

/* end of pragmari.h */
