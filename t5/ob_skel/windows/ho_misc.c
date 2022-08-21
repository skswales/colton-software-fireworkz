/* windows/ho_misc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Misc host stuff - no fixed abode */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/* Make a beep sound.  This is used to inform the user
 * that something has happened.  We use the MessageBeep
 * within Windows to make the noise.
 */

extern void
host_bleep(void)
{
    MessageBeep((UINT) -1); /* default system beep */
}

/* There are no scrap file transfers on Windows so files are always safe */

_Check_return_
extern BOOL
host_xfer_loaded_file_is_safe(void)
{
    return(TRUE);
}

_Check_return_
extern BOOL
host_xfer_saved_file_is_safe(void)
{
    return(TRUE);
}

_Check_return_
extern U32
host_rand_between(
    _InVal_ U32 lo /*incl*/,
    _InVal_ U32 hi /*excl*/) /* NB NOT like RANDBETWEEN() */
{
    U32 res = lo;

    if(hi > lo)
    {
        const U32 n = hi - lo;
        const U32 r = (U32) rand();
        assert(n < (U32) RAND_MAX);
        res = lo + (r % n);
    }

    return(res);
}

/* end of windows/ho_misc.c */
