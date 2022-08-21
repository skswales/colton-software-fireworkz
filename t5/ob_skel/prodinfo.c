/* prodinfo.c - included by each product.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#if !defined(__prodinfo_h)
/* ignore */
#else

/*#include "common/gflags.h"*/

/*#ifndef    __skel_flags_h*/
/*#include "ob_skel/flags.h"*/
/*#endif*/

/*#include "ob_skel/prodinfo.h"*/

/******************************************************************************
*
* Functions to return basic information about product
*
******************************************************************************/

/* poked at startup from Info (RISC OS) / Registry (Windows) */
/*extern*/ TCHARZ __user_name[64];
/*extern*/ TCHARZ __organisation_name[64];
/*extern*/ TCHARZ __registration_number[REG_NUMB_LENGTH + 1]; /* NB no spaces */

_Check_return_
_Ret_z_
extern PCTSTR
product_id(void)
{
    return(__product_name);
}

_Check_return_
_Ret_z_
extern PCTSTR
product_family_id(void)
{
    return(__product_family_name);
}

_Check_return_
_Ret_z_
extern PCTSTR
product_ui_id(void)
{
    return(__product_ui_name);
}

_Check_return_
_Ret_z_
extern PCTSTR
registration_number(void)
{
    static TCHARZ visRegistrationNumber[] = TEXT("0000 0000 0000 0000"); /* NB spaces */

    UINT grpidx;

    if(CH_NULL != __registration_number[0])
    {
        for(grpidx = 0; grpidx < 4; grpidx++)
        {
            /* copy groups of four over */
            memcpy32(&visRegistrationNumber[grpidx * 5], &__registration_number[grpidx * 4], 4 * sizeof(TCHAR));
        }
    }

    return(visRegistrationNumber);
}

_Check_return_
_Ret_z_
extern PCTSTR
user_id(void)
{
    return(__user_name);
}

_Check_return_
_Ret_z_
extern PCTSTR
user_organ_id(void)
{
    return(__organisation_name);
}

#endif /* __prodinfo_h */

/* end of prodinfo.c */
