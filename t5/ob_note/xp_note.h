/* xp_note.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Note object module header */

/* MRJC September 1992 */

#ifndef __xp_note_h
#define __xp_note_h

/*
exported structure
*/

/*
NB EXT_ID_NOTE_PIN_xxx are 'forever' ids - they appear in files
*/

enum EXT_ID_NOTE_PINNING
{
    EXT_ID_NOTE_PIN_CELLS_SINGLE    = 1,
    EXT_ID_NOTE_PIN_CELLS_TWIN      = 2,
    EXT_ID_NOTE_PIN_CELLS_AREA      = 3,
    EXT_ID_NOTE_PIN_PRINT_AREA      = 4,
    EXT_ID_NOTE_PIN_PAPER           = 5
};

typedef enum NOTE_PINNING
{
    NOTE_UNPINNED = 0,      /* this can be present on any layer */
    NOTE_PIN_CELLS_SINGLE,  /* this must be LAYER_CELLS_AREA / LAYER_CELLS_BELOW */
    NOTE_PIN_CELLS_TWIN     /* ditto */
}
NOTE_PINNING;

typedef enum NOTE_SELECTION
{
    NOTE_SELECTION_NONE = 0,
    NOTE_SELECTION_SELECTED,
    NOTE_SELECTION_IN_EDIT
}
NOTE_SELECTION;

typedef struct NOTE_INFO_FLAGS
{
    UBF all_pages       : 1; /* only valid for NOTE_UNPINNED */
    UBF dont_print      : 1;
    UBF scale_to_fit    : 1;
    UBF uref_registered : 1; /* TRUE if uref registered (for NOTE_PIN_CELLS_SINGLE and NOTE_PIN_CELLS_TWIN) */
    UBF skel_rect_valid : 1;
}
NOTE_INFO_FLAGS;

typedef struct NOTE_INFO
{
    NOTE_INFO_FLAGS flags;

    LAYER layer;

    NOTE_PINNING note_pinning;

    SKEL_RECT skel_rect; /* current tl,br corners of note */ /* both points valid and recalculated on cell movement */

    PIXIT_SIZE pixit_size; /* only set when object is happy to cooperate with RESIZE */
    GR_SCALE_PAIR gr_scale_pair;

    PIXIT_POINT offset_tl, offset_br; /* offsets of tl,br corners of note in whichever layer (paper/print/work area or cell relative) */

    REGION region; /* just tl,br SLRs (NOTE_PIN_CELLS_SINGLE and NOTE_PIN_CELLS_TWIN) but REGION simplifies uref for NOTE_PIN_CELLS_TWIN */

    UREF_HANDLE uref_handle;        /* UREF's handle to us */
    CLIENT_HANDLE client_handle;    /* our handle to UREF */

    OBJECT_ID object_id;
    P_ANY object_data_ref;

    NOTE_SELECTION note_selection;
}
NOTE_INFO, * P_NOTE_INFO, * P_P_NOTE_INFO; typedef const NOTE_INFO * PC_NOTE_INFO;

typedef struct NOTE_REF
{
    S32 extref;
    P_ANY object_data_ref;
    P_OF_IP_FORMAT p_of_ip_format;
}
NOTE_REF, * P_NOTE_REF;

typedef struct NOTE_ENSURE_SAVED
{
    S32 extref;
    P_ANY object_data_ref;
    P_OF_OP_FORMAT p_of_op_format;
}
NOTE_ENSURE_SAVED, * P_NOTE_ENSURE_SAVED;

typedef struct NOTE_ENSURE_EMBEDDED
{
    P_ANY object_data_ref;
}
NOTE_ENSURE_EMBEDDED, * P_NOTE_ENSURE_EMBEDDED;

typedef struct NOTE_MENU_QUERY
{
    MENU_ROOT_ID menu_root_id; /*OUT*/
}
NOTE_MENU_QUERY, * P_NOTE_MENU_QUERY;

typedef struct NOTE_OBJECT_REDRAW
{
    OBJECT_REDRAW object_redraw;
    GR_SCALE_PAIR gr_scale_pair;
}
NOTE_OBJECT_REDRAW, * P_NOTE_OBJECT_REDRAW;

typedef struct NOTE_OBJECT_CLICK
{
    P_ANY object_data_ref;
    T5_MESSAGE t5_message;
    PIXIT_POINT pixit_point;
    P_NOTE_INFO p_note_info; /* back ref so we can update_now */
    P_SKELEVENT_CLICK p_skelevent_click;
    S32 processed; /* 0 -> not processed, 1 -> completely handled else help!!! */
    PIXIT_RECT subselection_pixit_rect; /*OUT*/
}
NOTE_OBJECT_CLICK, * P_NOTE_OBJECT_CLICK;

typedef struct NOTE_OBJECT_SIZE
{
    P_ANY object_data_ref;
    PIXIT_SIZE pixit_size;
    BOOL processed;
}
NOTE_OBJECT_SIZE, * P_NOTE_OBJECT_SIZE;

typedef struct NOTE_OBJECT_SNAPSHOT
{
    P_ANY object_data_ref;
    P_NOTE_INFO p_note_info; /* back ref so we can update note */
    P_ANY p_more_info; /* so recipient can pass it on */
}
NOTE_OBJECT_SNAPSHOT, * P_NOTE_OBJECT_SNAPSHOT;

typedef struct NOTE_OBJECT_SELECTION_CLEAR
{
    P_ANY object_data_ref;
    P_NOTE_INFO p_note_info; /* back ref so we can update note */
}
NOTE_OBJECT_SELECTION_CLEAR, * P_NOTE_OBJECT_SELECTION_CLEAR;

typedef struct NOTELAYER_SELECTION_INFO
{
    OBJECT_ID object_id;
    P_ANY object_data_ref;
}
NOTELAYER_SELECTION_INFO, * P_NOTELAYER_SELECTION_INFO;

typedef struct NOTE_UPDATE_OBJECT
{
    OBJECT_ID object_id;
    P_ANY object_data_ref;
}
NOTE_UPDATE_OBJECT, * P_NOTE_UPDATE_OBJECT;

typedef struct NOTE_UPDATE_OBJECT_INFO
{
    OBJECT_ID object_id;
    P_ANY object_data_ref;
    P_NOTE_INFO p_note_info;
}
NOTE_UPDATE_OBJECT_INFO, * P_NOTE_UPDATE_OBJECT_INFO;

typedef struct NOTE_UPDATE_NOW
{
    P_NOTE_INFO p_note_info;
    REDRAW_FLAGS redraw_flags;
}
NOTE_UPDATE_NOW, * P_NOTE_UPDATE_NOW;

#endif /* __xp_note_h */

/* end of xp_note.h */
