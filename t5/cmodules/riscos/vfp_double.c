/* riscos/vfp_double.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2021-2022 Stuart Swales */

#include "common/gflags.h"

#if RISCOS

#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"

#include "cmodules/riscos/vfp_double.h"

static int vfp_context_inited = 0;
static void * vfp_context = NULL;

extern bool
vfp_context_ensure(void)
{
    STATUS status;
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    if(vfp_context_inited)
        return(vfp_context_inited > 0); /* skipped if already inited, or previous init failed */

    vfp_context_inited = -1; /* mark as failed until success */

    rs.r[0] = (1<<1)  /* Application Space flag */
            | (1<<0); /* User mode access required */
    rs.r[1] = 16; /* Number of doubleword registers required  */
    if(NULL != (e = _kernel_swi(0x58EC0 /*VFPSupport_CheckContext*/, &rs, &rs)))
    {   /* not that bothered if this fails - caller will fall back to FPA hopefully */
        //wimp_reporterror_simple(e);
        return(false);
    }

    if(NULL == (vfp_context = _al_ptr_alloc(rs.r[0] /*n_bytes*/, &status)))
        return(false);

    rs.r[0] = (1<<1)  /* Application Space flag */
            | (1<<0); /* User mode access required */
    rs.r[0] |= (1<<30); /* Lazy activation flag */
    rs.r[1] = 16; /* Number of doubleword registers required  */
    rs.r[2] = (intptr_t) vfp_context;
    rs.r[3] = 0; /* Initial FPSCR */
#if 0
    rs.r[3] |= (1<<24); /*  */
    rs.r[3] |= (1<<25); /*  */
#endif
    if(NULL != (e = _kernel_swi(0x58EC1 /*VFPSupport_CreateContext*/, &rs, &rs)))
    {
        //wimp_reporterror_simple(e);
        al_ptr_dispose(&vfp_context);
        return(false);
    }

    vfp_context_inited = 1; /* success */

    return(true);
}

extern void
vfp_context_destroy(void)
{
    _kernel_swi_regs rs;

    if(vfp_context_inited < 1)
        return;

    vfp_context_inited = 0;

    rs.r[0] = (intptr_t) vfp_context;
    rs.r[1] = 0;
    if(NULL != _kernel_swi(0x58EC2 /*VFPSupport_DestroyContext*/, &rs, &rs))
        /*return*/;

    al_ptr_dispose(&vfp_context);
}

#endif /* RISCOS */

/* end of riscos/vfp_double.c */
