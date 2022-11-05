/* mleci.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MultiLine Edit Controls for RISC OS */

/* RCM May 1991 */

typedef struct buff_region
{
    int            start;
    int            end;
}   buff_region;

typedef struct cursor_position
{
    int            lcol;        /* logical col                          */
    int            pcol;        /* physical col i.e. MIN(lcol, linelen) */
    int            row;         /* row number                           */
}   cursor_position;

typedef struct mark_position
{
    int            col;         /* a physical col number (I think) */
    int            row;         /* row number                      */
}   mark_position;

typedef struct marked_text
{
    mark_position  markstt;     /* start of marked text i.e. closest_to_text_home_of(cursor, selectanchor) */
    int            marklines;   /* number of line-ends in range (>=0) */
    buff_region    lower;
    buff_region    upper;
}
marked_text;

struct MLEC
{
    char            *buffptr;      /* ptr to the text buffer held in flex space                            */
    int              buffsiz;      /* the size we asked for                                                */
    buff_region      lower;        /* all characters left of, and rows above the physical cursor position  */
    buff_region      upper;        /* all characters right of, and rows below the physical cursor position */
    cursor_position  cursor;       /* cursor row number and logical & physical column number               */
    int              maxcol;       /* length of longest line (sort of!)                                    */
    int              linecount;    /* number of line terminators i.e. 1 less than number of display lines  */

    int              charwidth;    /* e.g. 16 } graphics mode specific */
    int              termwidth;    /* on screen representation of an EOL char in a selection, typically charwidth/4 */

    wimp_w           main;
    wimp_w           pane;
#ifdef MLEC_PANE
    BOOL             panemenu;
    BBox             paneextent;   /* work area limits */
#endif

    HOST_FONT        host_font;

    mlec_event_proc  callbackproc;
    P_ANY            callbackhand;

    BOOL             selectvalid;
    mark_position    selectanchor;
    int              selectEORcol; /* do gcol(3,selectEORcol) to show selection, repeat to remove */

    int              attributes[MLEC_ATTRIBUTE_MAX];
};

#define lineterm_CR    "\x0D"
#define lineterm_LF    "\x0A"
#define lineterm_CRLF  "\x0D\x0A"
#define lineterm_LFCR  "\x0A\x0D"

#if FALSE
#if SUPPORT_CUTPASTE
extern MLEC paste;      /* The paste buffer */
#endif
#endif

#ifdef MLEC_PANE /* Fireworkz doesn't have pane mlecs */
#define if_pane(mlec)               if(mlec->pane != window_NULL)
#define if_no_pane(mlec)            if(mlec->pane == window_NULL)
#else
#define if_pane(mlec)               if_constant(FALSE)
#define if_no_pane(mlec)            if_constant(TRUE)
#endif

#define reject_if_paste_buffer(mlec) \
    if(mlec == paste_buffer) return(create_error(MLEC_ERR_INVALID_PASTE_OP))

#ifndef MLEC_DEFAULT_BUFSIZ
#define MLEC_DEFAULT_BUFSIZ 8
#endif

#define window_NULL NULL

#define TAB_MASK 3      /* i.e. insert 1..4 spaces */

typedef union XFER_HANDLE
{
    FILE_HANDLE f;      /* a file           */
    MLEC p;             /* the paste buffer */
    struct XFER_HANDLE_S
    {
        P_U8 ptr;
        int siz;
        int len;
    } s;                /* a string         */
}
XFER_HANDLE, * P_XFER_HANDLE;

/* end of mleci.h */
