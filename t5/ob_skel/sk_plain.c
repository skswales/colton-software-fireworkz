/* sk_plain.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* High level routines to output a page full of text to a file */

/* MRJC December 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/******************************************************************************
*
* output chars
*
******************************************************************************/

_Check_return_
extern STATUS
plain_text_chars_out(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(count) PC_UCHARS uchars,
    _InVal_     S32 count)
{
    if(count <= 0)
        return(0);

    status_return(quick_ublock_uchars_add(p_quick_ublock, uchars, count));

    return(count);
}

/******************************************************************************
*
* turn an effect switch into an inline
*
******************************************************************************/

_Check_return_
extern STATUS
plain_text_effect_switch(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     enum PLAIN_EFFECT effect,
    _InVal_     S32 state)
{
    IL_CODE il_code = IL_NONE;
    U8 u8_state;

    switch(effect)
    {
    case PLAIN_EFFECT_BOLD:
        il_code = IL_STYLE_FS_BOLD;
        break;

    case PLAIN_EFFECT_UNDERLINE:
        il_code = IL_STYLE_FS_UNDERLINE;
        break;

    case PLAIN_EFFECT_ITALIC:
        il_code = IL_STYLE_FS_ITALIC;
        break;

    case PLAIN_EFFECT_SUPER:
        il_code = IL_STYLE_FS_SUPERSCRIPT;
        break;

    case PLAIN_EFFECT_SUB:
        il_code = IL_STYLE_FS_SUBSCRIPT;
        break;

    default:  default_unhandled();break;
    }

    u8_state = (U8) state;

    return(inline_quick_ublock_from_data(p_quick_ublock, il_code, IL_TYPE_U8, &u8_state, 0));
}

/******************************************************************************
*
* output codes to update effects to state in font_spec
*
******************************************************************************/

_Check_return_
extern STATUS
plain_text_effects_update(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_U8 effects,
    _InRef_     PC_FONT_SPEC p_font_spec)
{
    /* get effects to correct state */
    if(effects[PLAIN_EFFECT_BOLD] != p_font_spec->bold)
    {
        status_return(plain_text_effect_switch(p_quick_ublock, PLAIN_EFFECT_BOLD, !effects[PLAIN_EFFECT_BOLD]));
        effects[PLAIN_EFFECT_BOLD] ^= 1;
    }
    if(effects[PLAIN_EFFECT_UNDERLINE] != p_font_spec->underline)
    {
        status_return(plain_text_effect_switch(p_quick_ublock, PLAIN_EFFECT_UNDERLINE, !effects[PLAIN_EFFECT_UNDERLINE]));
        effects[PLAIN_EFFECT_UNDERLINE] ^= 1;
    }
    if(effects[PLAIN_EFFECT_ITALIC] != p_font_spec->italic)
    {
        status_return(plain_text_effect_switch(p_quick_ublock, PLAIN_EFFECT_ITALIC, !effects[PLAIN_EFFECT_ITALIC]));
        effects[PLAIN_EFFECT_ITALIC] ^= 1;
    }
    if(effects[PLAIN_EFFECT_SUPER] != p_font_spec->superscript)
    {
        status_return(plain_text_effect_switch(p_quick_ublock, PLAIN_EFFECT_SUPER, !effects[PLAIN_EFFECT_SUPER]));
        effects[PLAIN_EFFECT_SUPER] ^= 1;
    }
    if(effects[PLAIN_EFFECT_SUB] != p_font_spec->subscript)
    {
        status_return(plain_text_effect_switch(p_quick_ublock, PLAIN_EFFECT_SUB, !effects[PLAIN_EFFECT_SUB]));
        effects[PLAIN_EFFECT_SUB] ^= 1;
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
plain_text_effects_off(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_U8 effects)
{
    FONT_SPEC font_spec;
    zero_struct(font_spec);
    return(plain_text_effects_update(p_quick_ublock, effects, &font_spec));
}

/******************************************************************************
*
* output a completed segment to the output array
*
******************************************************************************/

_Check_return_
extern STATUS
plain_text_segment_out(
    _InoutRef_  P_ARRAY_HANDLE p_h_plain_text /*extended*/,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*terminated*/)
{
    STATUS status;
    P_ARRAY_HANDLE p_h_segment_new;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(ARRAY_HANDLE), TRUE);

    /* delimit output string */
    status_return(plain_text_chars_out(p_quick_ublock, ustr_empty_string, 1));

    if(NULL != (p_h_segment_new = al_array_extend_by_ARRAY_HANDLE(p_h_plain_text, 1, &array_init_block, &status)))
        status = al_array_add(p_h_segment_new, BYTE, quick_ublock_bytes(p_quick_ublock), &array_init_block_u8, quick_ublock_uchars(p_quick_ublock));

    return(status);
}

/******************************************************************************
*
* output spaces
*
******************************************************************************/

_Check_return_
extern STATUS
plain_text_spaces_out(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     S32 count)
{
    STATUS status;
    P_USTR ustr;
    S32 i;

    if(count <= 0)
        return(STATUS_OK);

    if(NULL == (ustr = quick_ublock_extend_by(p_quick_ublock, count, &status)))
        return(status);

    for(i = 0; i < count; ++i)
        PtrPutByteOff(ustr, i, CH_SPACE);

    return(count);
}

/******************************************************************************
*
* dispose of object plain text output
*
******************************************************************************/

extern void
plain_text_dispose(
    _InoutRef_  P_ARRAY_HANDLE p_h_plain_text)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(p_h_plain_text); i += 1)
        al_array_dispose(array_ptr(p_h_plain_text, ARRAY_HANDLE, i));

    al_array_dispose(p_h_plain_text);
}

/* end of sk_plain.c */
