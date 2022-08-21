/* sk_mrof.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Part of the style subsystem */

/* MRJC November 1994 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/mrofmun.h"

/******************************************************************************
*
* build list of mrofmun strings for the autoformatter
*
* called from style maeve_event on STYLE_CHANGE
*
* one with explicit search values go at start of list,
* others being added to end of list
*
******************************************************************************/

_Check_return_
extern STATUS
mrofmun_build_list(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_mrofmun)
{
    STATUS status = STATUS_OK;
    STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
    P_STYLE p_style;

    while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
    {
        /* if this style has got some numforms... */
        if(!style_selector_test(&p_style->selector, &style_selector_numform))
            continue;

        if(style_handle == style_handle_base(&p_docu->h_style_docu_area))
            continue;

        { /* find a place to insert according to search weight */
        ARRAY_INDEX i, n_elements = array_elements(&p_docu->h_mrofmun);
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(MROFMUN_ENTRY), TRUE);
        P_MROFMUN_ENTRY p_mrofmun_entry;
        S32 search = S32_MAX;

        if(style_bit_test(p_style, STYLE_SW_SEARCH))
            search = p_style->search;

        for(i = 0, p_mrofmun_entry = array_ptr(&p_docu->h_mrofmun, MROFMUN_ENTRY, i); i < n_elements; i += 1, p_mrofmun_entry += 1)
            if(search < p_mrofmun_entry->search)
                break;

        if(NULL == (p_mrofmun_entry = al_array_insert_before(&p_docu->h_mrofmun, MROFMUN_ENTRY, 1, &array_init_block, &status, i)))
            break;

        p_mrofmun_entry->search = search;
        p_mrofmun_entry->style_handle = style_handle;

        if(style_bit_test(p_style, STYLE_SW_PS_NUMFORM_NU))
            p_mrofmun_entry->numform_parms.ustr_numform_numeric = array_ustr(&p_style->para_style.h_numform_nu);

        if(style_bit_test(p_style, STYLE_SW_PS_NUMFORM_DT))
            p_mrofmun_entry->numform_parms.ustr_numform_datetime = array_ustr(&p_style->para_style.h_numform_dt);

        if(style_bit_test(p_style, STYLE_SW_PS_NUMFORM_SE))
            p_mrofmun_entry->numform_parms.ustr_numform_texterror = array_ustr(&p_style->para_style.h_numform_se);
        } /*block*/
    }

    if(status_ok(status))
        *p_h_mrofmun = p_docu->h_mrofmun;

    return(status);
}

_Check_return_
extern STATUS
mrofmun_get_list(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_mrofmun)
{
    /* list only gets rebuilt as necessary */
    if(0 != p_docu->h_mrofmun)
    {
        *p_h_mrofmun = p_docu->h_mrofmun;
        return(STATUS_OK);
    }

    return(mrofmun_build_list(p_docu, p_h_mrofmun));
}

_Check_return_
extern BOOL
mrofmun_style_handle_in_use(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle)
{
    BOOL found = FALSE;
    ARRAY_HANDLE h_mrofmun;

    if(status_ok(mrofmun_get_list(p_docu, &h_mrofmun)))
    {
        const ARRAY_INDEX n_elements = array_elements(&h_mrofmun);
        ARRAY_INDEX i;
        PC_MROFMUN_ENTRY p_mrofmun_entry = array_rangec(&h_mrofmun, MROFMUN_ENTRY, 0, n_elements);

        for(i = 0; i < n_elements; ++i, ++p_mrofmun_entry)
        {
            if(p_mrofmun_entry->style_handle != style_handle)
                continue;

            found = TRUE;
            break;
        }
    }

    return(found);
}

/* end of sk_mrof.c */
