/* ob_hefo.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header/footer object module internal header */

/* MRJC September 1992 */

/*
messages
*/

#define HEFO_MSG_BASE   (STATUS_MSG_INCREMENT * OBJECT_ID_HEFO)

#define HEFO_MSG(n)     (HEFO_MSG_BASE + (n))

#define MSG_DIALOG_PAGE_HEFO_BREAK_INTRO_CAPTION        HEFO_MSG(0)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_PAGE_BREAK    HEFO_MSG(1)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_PAGE_NUMBER   HEFO_MSG(2)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_MARGIN        HEFO_MSG(3)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_OFFSET        HEFO_MSG(4)
#define MSG_DIALOG_HEFO_REPR_PAGE                       HEFO_MSG(5)
#define MSG_DIALOG_HEFO_REPR_ROW                        HEFO_MSG(6)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_HEADER_ODD    HEFO_MSG(7)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_HEADER_EVEN   HEFO_MSG(8)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_HEADER_FIRST  HEFO_MSG(9)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_FOOTER_ODD    HEFO_MSG(10)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_FOOTER_EVEN   HEFO_MSG(11)
#define MSG_DIALOG_PAGE_HEFO_BREAK_VALUES_FOOTER_FIRST  HEFO_MSG(12)
#define MSG_DIALOG_PAGE_HEFO_BREAK_INTRO_HELP_TOPIC     HEFO_MSG(15)

#define MSG_STATUS_HEFO_HEADER                          HEFO_MSG(20)
#define MSG_STATUS_HEFO_FOOTER                          HEFO_MSG(21)

#include "ob_story/ob_story.h"

#include "ob_hefo/xp_hefo.h"

/*
hefo block
*/

typedef struct HEFO_BLOCK
{
#define HEFO_BLOCK_MAGIC 0x6F466548     /* HeFo (LE) */
    U32 magic;
    P_ANY p_data;                       /* message data */
    P_ARRAY_HANDLE p_h_data;            /* pointer to array of hefo data */
    P_ARRAY_HANDLE p_h_style_list;      /* pointer to array of style info */
    REDRAW_TAG redraw_tag;              /* redraw tag for this data */
    SKEL_RECT skel_rect_work;           /* work area */
    SKEL_RECT skel_rect_object;         /* object area */
    OBJECT_ID event_focus;              /* focus id of event */
    OBJECT_DATA object_data;
}
HEFO_BLOCK, * P_HEFO_BLOCK;

_Check_return_
_Ret_writes_(bytesof_elem)
static inline P_BYTE
_p_data_from_hefo_block(
    _InRef_     P_HEFO_BLOCK p_hefo_block
    CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 bytesof_elem))
{
    assert(HEFO_BLOCK_MAGIC == p_hefo_block->magic);
    CODE_ANALYSIS_ONLY(IGNOREPARM_InVal_(bytesof_elem));
    return((P_BYTE) p_hefo_block->p_data);
}

#define P_DATA_FROM_HEFO_BLOCK(__base_type, p_hefo_block) ( \
    (__base_type *) _p_data_from_hefo_block(p_hefo_block  \
    CODE_ANALYSIS_ONLY_ARG(sizeof32(__base_type)))      )

/*
header/footer sent several keys at once
*/

typedef struct HEFO_KEYS
{
    P_SKELEVENT_KEYS p_skelevent_keys;
    OBJECT_POSITION object_position;
}
HEFO_KEYS, * P_HEFO_KEYS;

/*
initialise a hefo docu_area
*/

#define docu_area_init_hefo(p_docu_area) (                          \
    (p_docu_area)->whole_col = (p_docu_area)->whole_row = 0,        \
    (p_docu_area)->tl.slr.col = 0,                                  \
    (p_docu_area)->tl.slr.row = 0,                                  \
    (p_docu_area)->br.slr.col = 1,                                  \
    (p_docu_area)->br.slr.row = 1,                                  \
    (p_docu_area)->tl.object_position.object_id = OBJECT_ID_NONE,   \
    (p_docu_area)->br.object_position.object_id = OBJECT_ID_NONE    )

/*
initialise a position inside a hefo
*/

#define position_init_hefo(p_position, ob_pos) (                \
    (p_position)->slr.col = 0,                                  \
    (p_position)->slr.row = 0,                                  \
    (p_position)->object_position.object_id = OBJECT_ID_TEXT,   \
    (p_position)->object_position.data = (ob_pos)               )

/* end of ob_hefo.h */
