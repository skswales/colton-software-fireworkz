/* tx_main.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
exported routines
*/

PROC_EVENT_PROTO(extern, proc_event_tx_main);

extern void
text_redisplay_fast(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_TEXT_REDISPLAY_INFO p_text_redisplay_info,
    P_TEXT_CACHE p_text_cache,
    _InRef_     PC_SKEL_RECT p_skel_rect_text_old,
    _InRef_     PC_SKEL_RECT p_skel_rect_text_new,
    _InVal_     REDRAW_TAG redraw_tag,
    P_STYLE p_style_text_global);

extern void
text_redisplay_info_dispose(
    P_TEXT_REDISPLAY_INFO p_text_redisplay_info);

extern void
text_redisplay_info_save(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_REDISPLAY_INFO p_text_redisplay_info,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object,
    _InVal_     S32 inline_ix);

/*
types returned by text_classify_char
*/

enum TEXT_CHAR_TYPE
{
    CHAR_PUNCT = 0,
    CHAR_TEXT,
    CHAR_SPACE,
    CHAR_N
};

/* end of tx_main.h */
