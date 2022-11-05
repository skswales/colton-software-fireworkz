/* sk_draw.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MRJC January 1992 */

extern void
object_background(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_STYLE p_style);

extern void
object_border(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    _InRef_     PC_STYLE p_style);

T5_MSG_PROTO(extern, skel_event_redraw, _InoutRef_ P_SKELEVENT_REDRAW p_skelevent_redraw);

/* end of sk_draw.h */
