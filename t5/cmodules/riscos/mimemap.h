/* riscos/mimemap.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2020-2021 Stuart Swales */

#ifndef __riscos_mimemap_h
#define __riscos_mimemap_h

#define MMM_TYPE_RISCOS         0
#define MMM_TYPE_RISCOS_STRING  1
#define MMM_TYPE_MIME           2
#define MMM_TYPE_DOT_EXTN       3
#define MMM_TYPE_MAC            4
#define MMM_TYPE_DOT_EXTNS      5

_Check_return_
static inline BOOL /*TRUE if extension found*/
riscos_mimemap_extension_from_t5_filetype(
    _Out_writes_z_(128) P_U8Z extension /*[128]*/, /* hopefully language-independent */
    _InVal_     T5_FILETYPE t5_filetype)
{
    _kernel_swi_regs rs;

    rs.r[0] = MMM_TYPE_RISCOS;
    rs.r[1] = (int) t5_filetype;
    rs.r[2] = MMM_TYPE_DOT_EXTN; /* actually a string WITHOUT the dot... */
    rs.r[3] = (int) extension;

    return(NULL == _kernel_swi(0x050b00 /*MimeMap_Translate*/, &rs, &rs));
}

_Check_return_
static inline T5_FILETYPE
riscos_mimemap_t5_filetype_from_extension(
    _In_opt_z_  PC_U8Z extension) /* hopefully language-independent */
{
    _kernel_swi_regs rs;

    rs.r[0] = MMM_TYPE_DOT_EXTN; /* actually a string WITHOUT the dot... */
    rs.r[1] = (int) extension;
    rs.r[2] = MMM_TYPE_RISCOS;
    rs.r[3] = 0;

    if(NULL != extension)
        if(NULL == _kernel_swi(0x050b00 /*MimeMap_Translate*/, &rs, &rs))
            return((T5_FILETYPE) rs.r[3]);

    return(FILETYPE_UNDETERMINED);
}

#endif /* __riscos_mimemap_h */

/* end of riscos/mimemap.h */
