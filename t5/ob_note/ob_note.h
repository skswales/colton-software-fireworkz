/* ob_note.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef         __xp_note_h
#include "ob_note/xp_note.h"
#endif

#define NOTE_UPDATE_ALL 0
#define NOTE_UPDATE_SELECTION_MARKS 1

/*
internal exports from ob_note.c
*/

extern void
note_install_layer_handler(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info);

extern void
note_update_later(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _In_        S32 note_update_flags);

_Check_return_
_Ret_maybenull_
extern P_NOTE_INFO
notelayer_selection_first(
    _DocuRef_   P_DOCU p_docu);

extern void
notelayer_view_update_later_full(
    _DocuRef_   P_DOCU p_docu);

extern void
relative_pixit_rect_from_note(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info,
    _InRef_     PC_PAGE_NUM p_page_num,
    _OutRef_    P_PIXIT_RECT p_pixit_rect);

extern void
relative_pixit_point_from_skel_point_in_layer(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_PAGE_NUM p_page_num,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _OutRef_    P_PIXIT_POINT p_pixit_point,
    _InVal_     LAYER layer);

/*
internal exports from ob_note2.c
*/

MAEVE_EVENT_PROTO(extern, maeve_event_ob_note);

_Check_return_
extern STATUS
notelayer_save_clip_data(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_ARRAY_HANDLE p_h_clip_data);

extern void
notelayer_mount_note(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info);

extern void
notelayer_selection_delete(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern REDRAW_TAG
redraw_tag_from_layer(
    _InVal_     LAYER layer);

extern void
notelayer_replace_selection(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    P_ANY object_data_ref);

extern void
note_delete(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info);

extern void
note_move(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _InRef_     PC_PIXIT_SIZE p_pixit_size);

extern void
note_pin_change(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_NOTE_INFO p_note_info,
    _In_        NOTE_PINNING new_note_pinning);

/*ncr*/
_Ret_maybenull_
extern P_NOTE_INFO
notelayer_new_note(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_NOTE_INFO p_note_info_in,
    _OutRef_    P_STATUS p_status);

T5_CMD_PROTO(extern, t5_cmd_note);
T5_CMD_PROTO(extern, t5_cmd_note_back);
T5_CMD_PROTO(extern, t5_cmd_note_embed);
T5_CMD_PROTO(extern, t5_cmd_note_swap);
T5_CMD_PROTO(extern, t5_cmd_backdrop_intro);
T5_CMD_PROTO(extern, t5_cmd_backdrop);

/* end of ob_note.h */
