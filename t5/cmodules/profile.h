/* profile.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Profiler interface */

/* SKS February 1992 */

#ifndef __profile_h
#define __profile_h

#if RISCOS

#ifndef  _kernel_h
#include "kernel.h"
#endif

#define Profiler_Init       0x088000
/* these from profilei.h */

#ifndef Profiler_Init
#define Profiler_Init                   0x088000
#define Profiler_On                     0x088001
#define Profiler_Off                    0x088002
#define Profiler_Output                 0x088003
#define Profiler_Reset                  0x088004
#define Profiler_TaskChange             0x088005
#define Profiler_Fini                   0x088006
#endif

enum PROFILE_SORT_BY
{
    PROFILE_SORT_BY_NONE = 0,
    PROFILE_SORT_BY_ADDR,
    PROFILE_SORT_BY_COUNT,
    PROFILE_SORT_BY_NAME
};

/*
external functions
*/

static inline _kernel_oserror *
profile_init(void)
{
    _kernel_swi_regs rs;
    return(_kernel_swi(Profiler_Init, &rs, &rs));
}

static inline _kernel_oserror *
profile_on(void)
{
    _kernel_swi_regs rs;
    return(_kernel_swi(Profiler_On, &rs, &rs));
}

static inline _kernel_oserror *
profile_off(void)
{
    _kernel_swi_regs rs;
    return(_kernel_swi(Profiler_Off, &rs, &rs));
}

static inline _kernel_oserror *
profile_output(
    enum PROFILE_SORT_BY sort_by,
    const char * filename,
    const char * sep)
{
    _kernel_swi_regs rs;
    rs.r[0] =       sort_by;
    rs.r[1] = (int) filename;
    rs.r[2] = (int) sep;
    return(_kernel_swi(Profiler_Output, &rs, &rs));
}

static inline _kernel_oserror *
profile_reset(void)
{
    _kernel_swi_regs rs;
    return(_kernel_swi(Profiler_Reset, &rs, &rs));
}

static inline _kernel_oserror *
profile_taskchange(void)
{
    _kernel_swi_regs rs;
    return(_kernel_swi(Profiler_TaskChange, &rs, &rs));
}

static inline _kernel_oserror *
profile_fini(void)
{
    _kernel_swi_regs rs;
    return(_kernel_swi(Profiler_Fini, &rs, &rs));
}

#endif /* RISCOS */

#endif /* __profile_h */

/* end of profile.h */
