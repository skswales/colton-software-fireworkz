/* ob_story.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Story object module internal header */

/* MRJC July 1994 */

#ifndef __ob_story_h
#define __ob_story_h

/*
ob_story instance data
*/

/*
record of formatted paragraph
*/

typedef struct FORMATTED_TEXT
{
    ARRAY_HANDLE h_segments;
    ARRAY_HANDLE h_chunks;
    PIXIT para_start;
    PIXIT para_end;
    PIXIT justify_v_offset_y;
    SKEL_RECT skel_rect;
}
FORMATTED_TEXT, * P_FORMATTED_TEXT; typedef const FORMATTED_TEXT * PC_FORMATTED_TEXT;

/*
position in text
*/

typedef struct TEXT_LOCATION
{
    ARRAY_INDEX seg_ix;
    SKEL_POINT skel_point;
    S32 string_ix;
    PIXIT caret_height;
}
TEXT_LOCATION, * P_TEXT_LOCATION;

/*
info required to format text
*/

typedef struct TEXT_FORMAT_INFO
{
    STYLE style_text_global;            /* global style information for text object */
    SKEL_RECT skel_rect_object;         /* object area */
    SKEL_RECT skel_rect_work;           /* work area for text object; starts at 0,0; includes border areas */
    ARRAY_HANDLE h_style_list;
    BOOL object_visible;
    BOOL paginate;                      /* paginate when bottom of text area is reached */
    PIXIT text_area_border_y;           /* free space to leave at top/bottom of text_area */
}
TEXT_FORMAT_INFO, * P_TEXT_FORMAT_INFO;

typedef struct TEXT_MESSAGE_BLOCK
{
    P_ANY p_data;                       /* pointer to message data */
    TEXT_FORMAT_INFO text_format_info;
    INLINE_OBJECT inline_object;
}
TEXT_MESSAGE_BLOCK, * P_TEXT_MESSAGE_BLOCK;

/*
structure for saving redisplay info
*/

typedef struct TEXT_REDISPLAY_INFO
{
    SKEL_RECT skel_rect_object;         /* rectangle covering object */
    FORMATTED_TEXT formatted_text;      /* description of formatted text before object was altered */
    ARRAY_HANDLE_USTR h_ustr_inline;    /* object data before alteration */
    BOOL object_visible;                /* was the object visible ? */
    SKEL_RECT skel_rect_work;           /* work area for text object */
}
TEXT_REDISPLAY_INFO, * P_TEXT_REDISPLAY_INFO;

/*
structure for redisplay messages
*/

typedef struct TEXT_INLINE_REDISPLAY
{
    PC_QUICK_UBLOCK p_quick_ublock;     /* may be NULL */
    SKEL_RECT skel_rect_para_after;
    REDRAW_TAG redraw_tag;              /* redraw tag for this data */
    BOOL do_redraw;                     /* -in- */
    BOOL redraw_done;                   /* -out- was the object able to redraw itself incrementally? */
    BOOL size_changed;                  /* did the object's size change? */
    BOOL do_position_update;            /* -in- do update position */
    SKEL_RECT skel_rect_text_before;    /* rectangle covering text before alteration */
    SKEL_RECT skel_rect_para_before;    /* rectangle covering para before alteration */
}
TEXT_INLINE_REDISPLAY, * P_TEXT_INLINE_REDISPLAY;

#include "ob_story/tx_form.h"

#include "ob_story/tx_cache.h"

#include "ob_story/tx_main.h"

#endif /* __ob_story_h */

/* end of ob_story.h */
