/* xp_skel.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __xp_skel_h
#define __xp_skel_h

typedef struct DOCU DOCU, * P_DOCU, ** P_P_DOCU; typedef const DOCU * PC_DOCU; /* rare use */

#if 0
#define _DocuRef_           _InoutRef_
#define _DocuRef_maybenone_ _InoutRef_
#define UNREFERENCED_PARAMETER_DocuRef_(p_docu) UNREFERENCED_PARAMETER_InoutRef_(p_docu)
#else
#define _DocuRef_           _Inout_ /*const*/
#define _DocuRef_maybenone_ _Inout_maybenone_ /*const*/
#define UNREFERENCED_PARAMETER_DocuRef_(p_docu) UNREFERENCED_PARAMETER_CONST(p_docu)
#endif

typedef U8 DOCNO;
typedef DOCNO * P_DOCNO;
typedef U8 PACKED_DOCNO; /* packed version for critical structures */
#define    DOCNO_BITS 8

#define DOCNO_MAX           ((DOCNO) 255)
#define DOCNO_NONE          ((DOCNO) 0)
#define DOCNO_FIRST         ((DOCNO) 1)
#define DOCNO_CONFIG        ((DOCNO) 1)
#define DOCNO_SKIP_CONFIG   ((DOCNO) 1) /* start value for docno_enum_docs() to skip config */

typedef struct VIEW VIEW, * P_VIEW, ** P_P_VIEW; typedef const VIEW * PC_VIEW; /* rare use */

#if 1
#define _ViewRef_           _InoutRef_
#define _ViewRef_maybenone_ _InoutRef_maybenone_
#define UNREFERENCED_PARAMETER_ViewRef_(p_view) UNREFERENCED_PARAMETER_InoutRef_(p_view)
#else
#define _ViewRef_           const
#define _ViewRef_maybenone_ const
#define UNREFERENCED_PARAMETER_ViewRef_(p_view) UNREFERENCED_PARAMETER_CONST(p_view)
#endif

typedef U32 VIEWNO; /* full version for argument passing without masking */
typedef VIEWNO * P_VIEWNO;
typedef U8 PACKED_VIEWNO; /* packed version for critical structures */

#define VIEWNO_MAX          ((VIEWNO) 255)
#define VIEWNO_NONE         ((VIEWNO) 0)
#define VIEWNO_FIRST        ((VIEWNO) 1)

typedef struct PANE PANE, * P_PANE, * P_P_PANE; typedef const PANE * PC_PANE; /* rare use */

typedef S32 COL; typedef COL * P_COL; typedef const COL * PC_COL;
#define     COL_BITS     32
#define     COL_TFMT    S32_TFMT
#define MAX_COL         ((COL) 0x7FFFFFFF)
#define BAD_COL         ((COL) 0x80000000)

typedef S32 ROW; typedef ROW * P_ROW; typedef const ROW * PC_ROW;
#define     ROW_BITS     32
#define     ROW_FMT     S32_FMT
#define     ROW_TFMT    S32_TFMT
#define MAX_ROW         ((ROW) 0x7FFFFFFF)
#define BAD_ROW         ((ROW) 0x80000000)

/*
only UCHARS/USTR can contain CH_INLINE
*/

#if ((!RELEASED && 1) || defined(CODE_ANALYSIS))
#define STRUCT_UCHARS_INLINE_IS_UCHARS 1 /* NB this is really useful for debugging (and is needed for Code Analysis) */
#endif

#ifdef STRUCT_UCHARS_INLINE_IS_UCHARS
#define       _UCHARS_INLINE         _UCHARS
#else /* NOT STRUCT_UCHARS_INLINE_IS_UCHARS */
/* make it so that we cannot dereference / obtain size of pointer to UCHARS_INLINE */
#define       _UCHARS_INLINE struct __UCHARS_INLINE
#endif /* STRUCT_UCHARS_INLINE_IS_UCHARS */

typedef       _UCHARS_INLINE *  P_UCHARS_INLINE;
typedef const _UCHARS_INLINE * PC_UCHARS_INLINE;

typedef       _UCHARS_INLINE *  P_USTR_INLINE; /*_Null_terminated_*/
typedef const _UCHARS_INLINE * PC_USTR_INLINE; /*_Null_terminated_*/

typedef        UCHARB             UCHARB_INLINE; /* this is needed to make arrays of them on the stack */

#ifndef                   __filetype_h
#include "cmodules/coltsoft/filetype.h"
#endif

#if WINDOWS
#define _HdcRef_    _InRef_

#define HDC_ASSERT(hdc) \
    assert(NULL != hdc);
#endif /* OS */

/*
module includes
*/

#include "cmodules/aligator.h"
#include "cmodules/allocblk.h"
#include "cmodules/bitmap.h"
#include "cmodules/gr_coord.h"
#include "cmodules/im_cache.h"
#include "cmodules/list.h"
#include "cmodules/monotime.h"
#include "cmodules/muldiv.h"
#include "cmodules/quickblk.h"
#include "cmodules/quickwblk.h"
#include "cmodules/quicktblk.h"
#include "cmodules/quickublk.h"
#include "cmodules/ss_const.h"
#include "cmodules/file.h"
#include "cmodules/numform.h"
#include "cmodules/xstring.h"
#include "cmodules/xwstring.h"
#include "cmodules/xtstring.h"
#include "cmodules/xustring.h"
#include "cmodules/ucs4.h"
#include "cmodules/utf8.h"
#include "cmodules/utf8str.h"

/*
basic structure
*/

#define PRODUCT_ID_GENERIC      0
#define PRODUCT_ID_FIREWORKZ    4
#define PRODUCT_ID_FPROWORKZ    5

#define HUGENUMBER              ((S32) 0x40000000) /* a large positive number */

#define PIXITS_PER_INCH         1440    /* pixits (twips) per inch */
#define PIXITS_PER_HALF_INCH    720
#define PIXITS_PER_QUARTER_INCH 360
#define PIXITS_PER_POINT        20      /* pixits (twips) per point */

#define MILLIPOINTS_PER_PIXIT   50      /* 1000 / 20 */

#define FP_PIXITS_PER_METRE     56692.91338583
#define PIXITS_PER_METRE        56693

#define FP_PIXITS_PER_CM        566.9291338583
#define PIXITS_PER_CM           567     /* approx */

#define FP_PIXITS_PER_MM        56.69291338583
#define PIXITS_PER_MM           57      /* approx */

/*#define FP_INCHES_PER_METRE     39.37*/
#define INCHES_PER_METRE_MUL    40315   /* use as pairs in muldiv64 */
#define INCHES_PER_METRE_DIV    1024

#define pixits_from_millipoints_ceil(mp_positive) ( \
    ((PIXIT) (mp_positive) + MILLIPOINTS_PER_PIXIT - 1) / MILLIPOINTS_PER_PIXIT)

#define BACKGROUND_SLICE        MONOTIMEDIFF_VALUE_FROM_MS(100)

/******************************************************************************
*
* machine variable types
*
******************************************************************************/

typedef S32 PARA;
#define MAX_PARA        0x7FFF

typedef S32 PAGE;
#define MAX_PAGE        0x7FFF

#if RISCOS
#define  MAX_WMUNIT     0x007FFFFF /* never trust RISC OS Window Manager */
#else
#define  MAX_WMUNIT     0x7FFFFFFF
#endif

#define LARGECOL        0x7FFFFFFF
#define LARGEROW        0x7FFFFFFF

/* other universal types */

typedef S32 PIXIT; typedef PIXIT * P_PIXIT; typedef const PIXIT * PC_PIXIT;

#define PIXIT_TFMT S32_TFMT

typedef F64 FP_PIXIT; typedef FP_PIXIT * P_FP_PIXIT; typedef const FP_PIXIT * PC_FP_PIXIT;

typedef S32 UREF_HANDLE; typedef UREF_HANDLE * P_UREF_HANDLE;

/*
definition of object numbers

note that the object character binding is now supplied
by the Config file on a per-product basis, e.g.
'S' maps to OBJECT_ID_SS in Resultz/Fireworkz,
but maps to OBJECT_ID_CONST in Wordz/Recordz

think VERY CAREFULLY about changing any of these!
*/

typedef enum OBJECT_ID
{
    OBJECT_ID_ENUM_START = -1,  /* for while(status_ok(object_next(&object_id))) loops */

    OBJECT_ID_FIRST = 0,        /* for explicit for(;;) loops */

    OBJECT_ID_SKEL = 0,         /*  0 * */
    OBJECT_ID_SS = 1,           /*  1 S (see above) */
    OBJECT_ID_REC = 2,          /*  2 B */
    OBJECT_ID_TEXT = 3,         /*  3 X */
    OBJECT_ID_DRAW = 4,         /*  4 D */
    OBJECT_ID_was_CONST = 5,    /*  5 S (see above) */
    OBJECT_ID_CHART = 6,        /*  6 C */
    OBJECT_ID_HEFO = 7,         /*  7 H */
    OBJECT_ID_SPELL = 8,        /*  8 W */
    OBJECT_ID_SKEL_SPLIT = 9,
    OBJECT_ID_CELLS = 10,       /* 10   MUST be this value for file compatibility e.g. flow object */
    OBJECT_ID_FOOTER = 11,
    OBJECT_ID_NOTE = 12,
    OBJECT_ID_HEADER = 13,
    OBJECT_ID_SLE = 14,         /* 14 s */
    OBJECT_ID_DIALOG = 15,
    OBJECT_ID_STORY = 16,
    OBJECT_ID_DRAW_IO = 17,     /* 17 d */
    OBJECT_ID_TOOLBAR = 18,     /* 18 t */
    OBJECT_ID_RULER = 19,
    OBJECT_ID_FL_PDSS = 20,
    OBJECT_ID_MAILSHOT = 21,    /* 21 M */
    OBJECT_ID_FL_PDTX = 22,
    OBJECT_ID_FL_ASCII = 23,
    OBJECT_ID_FL_CSV = 24,
    OBJECT_ID_FL_FWP = 25,
    OBJECT_ID_FL_XLS = 26,
    OBJECT_ID_FL_LOTUS123 = 27,
    OBJECT_ID_FL_RTF = 28,
    OBJECT_ID_SPELB = 29,       /* 29 w */
    OBJECT_ID_MLEC = 30,
    OBJECT_ID_FILE = 31,        /* 31 f */
    OBJECT_ID_NONE = 32,        /* 32   MUST be this value for file compatibility */
    OBJECT_ID_FS_ASCII = 33,
    OBJECT_ID_CELLS_EDIT = 34,  /* 34 E */
    OBJECT_ID_FS_CSV = 35,
    OBJECT_ID_IMPLIED_STYLE = 36,
    OBJECT_ID_RECB = 37,        /* 37 b */
    OBJECT_ID_RECN = 38,        /* 38 B when no ob_rec (see above)  */
    OBJECT_ID_REC_FLOW = 39,    /* 39   MUST be this value for file compatibility (used in Flow construct) */
    OBJECT_ID_FS_LOTUS123 = 40,
    OBJECT_ID_FS_RTF = 41,
    OBJECT_ID_FS_XLS = 42,

    OBJECT_ID_MAX,              /* for object_call_between() upper limit */

#define OBJECT_ID_SS_SPLIT OBJECT_ID_SS

    MAX_OBJECTS = 64            /* makes for a two 32-bit word bitmap */
}
OBJECT_ID, * P_OBJECT_ID;

#define OBJECT_ID_INCR(object_id__ref) \
    ENUM_INCR(OBJECT_ID, object_id__ref)

#define OBJECT_ID_DECR(object_id__ref) \
    ENUM_DECR(OBJECT_ID, object_id__ref)

typedef U8 PACKED_OBJECT_ID; /* packed version for critical structures */

#define OBJECT_ID_PACK(__packed_type, object_id) \
    ENUM_PACK(__packed_type, object_id)

#define OBJECT_ID_UNPACK(packed_object_id) \
    ENUM_UNPACK(OBJECT_ID, packed_object_id)

#define IS_OBJECT_ID_VALID(object_id) \
    ((U32) (object_id) < (U32) OBJECT_ID_MAX)

/*
definition of messages
*/

typedef enum T5_MESSAGE
{
    T5_EVENT_NONE = 0,

    /* 1..255 are reserved for inlines */

    /*******************************************************/

    T5_EVENT__START = 0x0100,

    T5_EVENT_CLICK_LEFT_SINGLE = T5_EVENT__START,
    T5_EVENT_CLICK_LEFT_DOUBLE,
    T5_EVENT_CLICK_LEFT_TRIPLE,
    T5_EVENT_CLICK_LEFT_DRAG,
    T5_EVENT_CLICK_RIGHT_SINGLE,
    T5_EVENT_CLICK_RIGHT_DOUBLE,
    T5_EVENT_CLICK_RIGHT_TRIPLE,
    T5_EVENT_CLICK_RIGHT_DRAG,
    T5_EVENT_CLICK_DRAG_STARTED,
    T5_EVENT_CLICK_DRAG_MOVEMENT,               /* } Only sent if host_drag_start() is called by recipient */
    T5_EVENT_CLICK_DRAG_FINISHED,               /* } of T5_EVENT_CLICK_LEFT_DRAG or T5_EVENT_CLICK_RIGHT_DRAG */
    T5_EVENT_CLICK_DRAG_ABORTED,
    T5_EVENT_CLICK_FILTER,

    T5_EVENT_POINTER_ENTERS_WINDOW,
    T5_EVENT_POINTER_LEAVES_WINDOW,
    T5_EVENT_POINTER_MOVEMENT,                  /* Only sent if host_track_start() is called by recipient of T5_EVENT_POINTER_ENTERS_WINDOW */

    T5_EVENT_FILEINSERT_DOINSERT,
    T5_EVENT_FILEINSERT_DOINSERT_1,
    T5_EVENT_KEYS,
    T5_EVENT_NULL,
    T5_EVENT_NULL_QUERY,
    T5_EVENT_PRINT,
    T5_EVENT_REDRAW,
    T5_EVENT_REDRAW_AFTER,
    T5_EVENT_SCHEDULED,
    T5_EVENT_TRY_AUTO_SAVE,
    T5_EVENT_TRY_AUTO_SAVE_ALL,
    T5_EVENT_VISIBLEAREA_CHANGED,

    T5_EVENT_SPARE_1,
    T5_EVENT_SPARE_2,
    T5_EVENT_SPARE_3,
    T5_EVENT_SPARE_4,

    T5_EVENT__END,

    /*******************************************************/

    T5_MSG__START = T5_EVENT__END,

    T5_MSG_INITCLOSE = T5_MSG__START, /* all the T5_MSG_INITCLOSE_MESSAGE group only go via MSG_INITCLOSE */

    T5_MSG_UREF_UREF,
    T5_MSG_UREF_DELETE,
    T5_MSG_UREF_SWAP_ROWS,
    T5_MSG_UREF_CHANGE,
    T5_MSG_UREF_OVERWRITE,
    T5_MSG_UREF_CLOSE1,
    T5_MSG_UREF_CLOSE2,

    T5_MSG_LOAD_STARTED,
    T5_MSG_LOAD_ENDED,
    T5_MSG_LOAD_CONSTRUCT_OWNFORM,
    T5_MSG_LOAD_CONSTRUCT_OWNFORM_DIRECTED,
    T5_MSG_LOAD_CELL_OWNFORM,
    T5_MSG_LOAD_FRAG_OWNFORM, /* SKS 11apr97 */

    T5_MSG_LOAD_CELL_FOREIGN,

    T5_MSG_SAVE, /* all the T5_MSG_SAVE_MESSAGE group are issued only as sub-messages in a SAVE_BLOCK */

    T5_MSG_SAVE_CONSTRUCT_OWNFORM,
    T5_MSG_SAVE_CELL_OWNFORM,

    T5_MSG_SAVE_FOREIGN,
    T5_MSG_SAVE_PICTURE,
    T5_MSG_SAVE_PICTURE_FILETYPES_REQUEST,

    T5_MSG_INSERT_OWNFORM,
    T5_MSG_INSERT_FOREIGN,

    T5_MSG_INSERT_FILE_WINDOWS,
    T5_MSG_INSERT_FILE_WINDOWS_PICTURE,

    T5_MSG_CHOICES_CHANGED,
    T5_MSG_CHOICE_CHANGED,
    T5_MSG_CHOICES_QUERY,
    T5_MSG_CHOICES_SAVE,
    T5_MSG_CHOICES_SET,
    T5_MSG_CHOICES_ENDED,

    T5_MSG_DOCU_CARETMOVE,
    T5_MSG_DOCU_COLROW,                         /* current col and/or row changed */
    T5_MSG_DOCU_DEPENDENTS,
    T5_MSG_DOCU_FOCUS_QUERY,
    T5_MSG_DOCU_NAME,                           /* document name changed           } */
    T5_MSG_DOCU_MODCHANGE,                      /* modified flag changed */
    T5_MSG_DOCU_MODSTATUS,                      /* modified flag changed           } */
    T5_MSG_DOCU_READWRITE,                      /* read-only/writable flag changed } */
    T5_MSG_DOCU_RENAME,                         /* rename the document */
    T5_MSG_DOCU_SUPPORTERS,
    T5_MSG_DOCU_VIEWCOUNT,                      /* number of views changed         } so redraw title bars */
    T5_MSG_DOCU_SPARE_1,
    T5_MSG_DOCU_SPARE_2,

    T5_MSG_VIEW_DESTROY,
    T5_MSG_VIEW_NEW,
    T5_MSG_VIEW_SHOWHIDEBORDER,
    T5_MSG_VIEW_SHOWHIDEPAPER,
    T5_MSG_VIEW_SHOWHIDERULER,
    T5_MSG_VIEW_ZOOMFACTOR,
    T5_MSG_VIEW_SPARE_1,
    T5_MSG_VIEW_SPARE_2,

    T5_MSG_DATA_REF_FROM_POSITION,
    T5_MSG_DATA_REF_FROM_SLR,
    T5_MSG_DATA_REF_TO_POSITION,
    T5_MSG_DATA_REF_TO_SLR,
    T5_MSG_DATA_REF_NAME_COMPARE,

    T5_MSG_OBJECT_AT_CURRENT_POSITION,          /* question asked of focus owners */
    T5_MSG_OBJECT_CHECK,
    T5_MSG_OBJECT_COMPARE,
    T5_MSG_OBJECT_COPY,
    T5_MSG_OBJECT_DATA_READ,
    T5_MSG_OBJECT_DELETE_SUB,
    T5_MSG_OBJECT_EDITABLE,
    T5_MSG_OBJECT_HOW_BIG,                      /* request to object for its size (may cause reformat) */
    T5_MSG_OBJECT_HOW_WIDE,
    T5_MSG_OBJECT_IN_CELL_ALLOWED,
    T5_MSG_OBJECT_spare_was_KEY,                          
    T5_MSG_OBJECT_KEYS,                         /* key press(es) sent to object */
    T5_MSG_OBJECT_LOGICAL_MOVE,                 /* object reports how much to move up or down */
    T5_MSG_OBJECT_NEW_TYPE_RQ,
    T5_MSG_OBJECT_POSITION_AT_START,
    T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT,     /* request to object to find sub-object position from SKEL_POINT */
    T5_MSG_OBJECT_POSITION_SET,                 /* set object position to start/end etc. */
    T5_MSG_OBJECT_POSITION_SET_START,
    T5_MSG_OBJECT_READ_TEXT,
    T5_MSG_OBJECT_READ_TEXT_DRAFT,
    T5_MSG_OBJECT_STRING_REPLACE,
    T5_MSG_OBJECT_STRING_SEARCH,
    T5_MSG_OBJECT_WANTS_LOAD,
    T5_MSG_OBJECT_WORD_CHECK,

    T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION,     /* set a skel point from object position data */

    T5_MSG_ANCHOR_FINISHED,
    T5_MSG_ANCHOR_NEW,
    T5_MSG_ANCHOR_UPDATE,

    T5_MSG_SELECTION_CLEAR,                     /* clear selections, everybody */
    T5_MSG_SELECTION_HIDE,
    T5_MSG_SELECTION_MAKE,
    T5_MSG_SELECTION_NEW,                       /* new selection has been created */
    T5_MSG_SELECTION_REPLACE,
    T5_MSG_SELECTION_SHOW,                      /* show selection */
    T5_MSG_SELECTION_STYLE,

    T5_MSG_SET_TAB_LEFT,
    T5_MSG_SET_TAB_CENTRE,
    T5_MSG_SET_TAB_RIGHT,
    T5_MSG_SET_TAB_DECIMAL,

    T5_MSG_STYLE_CHANGE_BOLD,
    T5_MSG_STYLE_CHANGE_ITALIC,
    T5_MSG_STYLE_CHANGE_UNDERLINE,
    T5_MSG_STYLE_CHANGE_SUPERSCRIPT,
    T5_MSG_STYLE_CHANGE_SUBSCRIPT,
    T5_MSG_STYLE_CHANGE_JUSTIFY_LEFT,
    T5_MSG_STYLE_CHANGE_JUSTIFY_CENTRE,
    T5_MSG_STYLE_CHANGE_JUSTIFY_RIGHT,
    T5_MSG_STYLE_CHANGE_JUSTIFY_FULL,

    T5_MSG_AFTER_SKEL_COMMAND,
    T5_MSG_BOX_APPLY,
    T5_MSG_CACHES_DISPOSE,
    T5_MSG_CARET_SHOW_CLAIM,            /* show caret */
    T5_MSG_CELLS_EXTENT,
    T5_MSG_CHECK_PROTECTION,
    T5_MSG_CLICK_LEFT_DOUBLE,
    T5_MSG_CLIPBOARD_NEW,
    T5_MSG_COL_AUTO_WIDTH,
    T5_MSG_COL_WIDTH_ADJUST,
    T5_MSG_COMPARE_OBJECT_POSITIONS,
    T5_MSG_CSV_READ_RECORD,
    T5_MSG_CUR_CHANGE_AFTER,
    T5_MSG_CUR_CHANGE_BEFORE,
    T5_MSG_DRAFT_PRINT,
    T5_MSG_DRAFT_PRINT_TO_FILE,
    T5_MSG_DRAWING_DONATE,
    T5_MSG_DELETE_INLINE_REDISPLAY,
    T5_MSG_ERROR_RQ,
    T5_MSG_FIELD_DATA_READ,
    T5_MSG_FIELD_DATA_N_RECORDS,
    T5_MSG_FOCUS_CHANGED,
    T5_MSG_FONTS_RECACHE,
    T5_MSG_FRAME_WINDOWS_DESCRIBE,
    T5_MSG_FRAME_WINDOWS_POSN,
    T5_MSG_FROM_WINDOWS,
    T5_MSG_GOTO_SLR,
    T5_MSG_HAD_SERIOUS_ERROR,
    T5_MSG_IMPLIED_STYLE_ARG_QUERY,
    T5_MSG_IMPLIED_STYLE_ARG_SET,
    T5_MSG_INIT_FLOW,
    T5_MSG_INSERT_INLINE_REDISPLAY,
    T5_MSG_MARK_INFO_READ,
    T5_MSG_MENU_OTHER_INFO,
    T5_MSG_MOVE_LEFT_CELL,
    T5_MSG_MOVE_RIGHT_CELL,
    T5_MSG_MOVE_UP_CELL,
    T5_MSG_MOVE_DOWN_CELL,
    T5_MSG_NAME_UREF,
    T5_MSG_NEW_OBJECT_FROM_TEXT,
    T5_MSG_NOTELAYER_SELECTION_INFO,
    T5_MSG_was_QUERY_CLOSE_LINKED,
    T5_MSG_QUIT_OBJECTION,
    T5_MSG_READ_MAIL_TEXT,
    T5_MSG_RECALCED,
    T5_MSG_REFORMAT,
    T5_MSG_REDRAW_REGION,
    T5_MSG_REDRAW_CELLS,                /* request to skeleton to redraw cells */
    T5_MSG_ROW_EXTENT_CHANGED,
    T5_MSG_ROW_MOVE_Y,
    T5_MSG_RULER_INFO,                  /* read info about columns */
    T5_MSG_SKEL_CONTROLS,
    T5_MSG_SKELCMD_VIEW_DESTROY,
    T5_MSG_CELL_MERGE,
    T5_MSG_OBJECT_SNAPSHOT,
    /*T5_MSG_STATUS_LINE_MESSAGE_QUERY,*/
    T5_MSG_STATUS_LINE_SET,
    T5_MSG_STYLE_CHANGED,
    T5_MSG_STYLE_DOCU_AREA_CHANGED,
    T5_MSG_STYLE_USE_QUERY,
    T5_MSG_STYLE_USE_REMOVE,
    T5_MSG_SUPPORTER_LOADED,
    T5_MSG_TAB_WANTED,
    T5_MSG_TABLE_STYLE_ADD,
    T5_MSG_WIN_HOST_NULL,
    T5_MSG_WORD_FROM_THESAURUS,

    /*******************************************************/

    T5_MSG_NOTE_DELETE,
    T5_MSG_NOTE_ENSURE_EMBEDED,
    T5_MSG_NOTE_ENSURE_SAVED,
    T5_MSG_NOTE_LOAD_INTREF_FROM_EXTREF,
    T5_MSG_NOTE_MENU_QUERY,
    T5_MSG_NOTE_NEW,
    T5_MSG_NOTE_OBJECT_CLICK,
    T5_MSG_NOTE_OBJECT_EDIT_END,
    T5_MSG_NOTE_OBJECT_EDIT_START,
    T5_MSG_NOTE_OBJECT_SIZE_QUERY,
    T5_MSG_NOTE_OBJECT_SIZE_SET,
    T5_MSG_NOTE_OBJECT_SIZE_SET_POSSIBLE,
    T5_MSG_NOTE_OBJECT_SNAPSHOT,
    T5_MSG_NOTE_UPDATE_NOW,
    T5_MSG_NOTE_UPDATE_OBJECT,
    T5_MSG_NOTE_UPDATE_OBJECT_INFO,

    /*******************************************************/

    T5_MSG_SS_ALERT_EXEC,
    T5_MSG_SS_FUNCTION_ARGUMENT_HELP,
    T5_MSG_SS_INPUT_EXEC,
    T5_MSG_SS_LINEST,
    T5_MSG_SS_NAME_CHANGE,
    T5_MSG_SS_NAME_ENSURE,
    T5_MSG_SS_NAME_MAKE,
    T5_MSG_SS_NAME_READ,
    T5_MSG_SS_NAME_ID_FROM_HANDLE,
    T5_MSG_SS_RPN_EXEC,

    T5_MSG_ACTIVATE_EDITLINE,
    T5_MSG_PASTE_EDITLINE,

    /*******************************************************/

    T5_MSG_CHART_DIALOG,
    T5_MSG_CHART_GALLERY,
    T5_MSG_CHART_EDIT_INSERT_PICTURE,

    /*******************************************************/

    T5_MSG_SPELL_AUTO_CHECK,
    T5_MSG_SPELL_ISUPPER,
    T5_MSG_SPELL_ISWORDC,
    T5_MSG_SPELL_TOLOWER,
    T5_MSG_SPELL_TOUPPER,
    T5_MSG_SPELL_VALID1,
    T5_MSG_SPELL_WORD_CHECK,

    /*******************************************************/

    T5_MSG_UISTYLE_COLOUR_PICKER,
    T5_MSG_UISTYLE_STYLE_EDIT,

    /*******************************************************/

    T5_MSG_FIELD_NEXT,
    T5_MSG_FIELD_PREV,
    T5_MSG_RECORDZ_IMPORT,
    T5_MSG_STYLE_RECORDZ,

    /*******************************************************/

    T5_MSG_TOOLBAR_TOOLS,
    T5_MSG_TOOLBAR_TOOL_QUERY,
    T5_MSG_TOOLBAR_TOOL_SET,
    T5_MSG_TOOLBAR_TOOL_ENABLE,
    T5_MSG_TOOLBAR_TOOL_ENABLE_QUERY,
    T5_MSG_TOOLBAR_TOOL_DISABLE,
    T5_MSG_TOOLBAR_TOOL_NOBBLE,
    T5_MSG_BACK_WINDOW_EVENT,               /* sent FROM back window TO toolbar */

    T5_MSG_TOOLBAR_TOOL_USER_VIEW_NEW,      /* sent FROM toolbar TO client */
    T5_MSG_TOOLBAR_TOOL_USER_VIEW_DELETE,   /* sent FROM toolbar TO client */
    T5_MSG_TOOLBAR_TOOL_USER_SIZE_QUERY,    /* sent FROM toolbar TO client */
    T5_MSG_TOOLBAR_TOOL_USER_POSN_SET,      /* sent FROM toolbar TO client */
    T5_MSG_TOOLBAR_TOOL_USER_REDRAW,        /* sent FROM toolbar TO client */
    T5_MSG_TOOLBAR_TOOL_USER_MOUSE,         /* sent FROM toolbar TO client */

    /*******************************************************/

    T5_MSG__END,

    /*******************************************************/

    /* NB these T5_EXT_xxx messages are cast in stone:
     * their numbers are bound into ownform files
     */

    T5_EXT_STYLE_CELL_TYPE = 1000,
    T5_EXT_STYLE_CELL_CURRENT = 1001,
    T5_EXT_STYLE_MROFMUN = 1010,
    T5_EXT_STYLE_STRIPE_ROWS = 1011,
    T5_EXT_STYLE_STRIPE_COLS = 1012,
    T5_EXT_STYLE_RECORDZ = 1066,
    T5_EXT_STYLE_RECORDZ_FIELDS = 1067,

    /*******************************************************/

    T5_CMD__START = 0x0800 /*2*1024*/,

    /* main commands for ownform I/O - keep together for switch jump table */
    T5_CMD_OF_VERSION = T5_CMD__START,
    T5_CMD_OF_GROUP,
    T5_CMD_OF_START_OF_DATA,
    T5_CMD_OF_END_OF_DATA,

    T5_CMD_OF_BLOCK,
    T5_CMD_OF_CELL,
    T5_CMD_OF_STYLE,

    T5_CMD_OF_BASE_REGION,
    T5_CMD_OF_REGION,

    T5_CMD_OF_BASE_IMPLIED_REGION,
    T5_CMD_OF_IMPLIED_REGION,

    T5_CMD_OF_STYLE_BASE,
    T5_CMD_OF_STYLE_CURRENT,
    T5_CMD_OF_STYLE_HEFO,
    T5_CMD_OF_STYLE_TEXT,

    T5_CMD_OF_FLOW,
    T5_CMD_OF_ROWTABLE,
    T5_CMD_OF_BASE_SINGLE_COL,

    T5_CMD_OF_HEFO_DATA,
    T5_CMD_OF_HEFO_BASE_REGION,
    T5_CMD_OF_HEFO_REGION,

    /*******************************************************/
 
    T5_CMD_CHOICES,
    T5_CMD_CHOICES_AUTO_SAVE,
    T5_CMD_CHOICES_DITHERING,
    T5_CMD_CHOICES_DISPLAY_PICTURES,
    T5_CMD_CHOICES_EMBED_INSERTED_FILES,
    T5_CMD_CHOICES_KERNING,
    T5_CMD_CHOICES_STATUS_LINE,
    T5_CMD_CHOICES_TOOLBAR,
    T5_CMD_CHOICES_UPDATE_STYLES_FROM_CHOICES,

    T5_CMD_CHOICES_SS_CALC_AUTO,
    T5_CMD_CHOICES_SS_CALC_BACKGROUND,
    T5_CMD_CHOICES_SS_CALC_ON_LOAD,
    T5_CMD_CHOICES_SS_CALC_ADDITIONAL_ROUNDING,
    T5_CMD_CHOICES_SS_EDIT_IN_CELL,
    T5_CMD_CHOICES_SS_ALTERNATE_FORMULA_STYLE,

    T5_CMD_CHOICES_ASCII_LOAD_AS_DELIMITED,
    T5_CMD_CHOICES_ASCII_LOAD_DELIMITER,
    T5_CMD_CHOICES_ASCII_LOAD_AS_PARAGRAPHS,
    T5_CMD_CHOICES_CHART_UPDATE_AUTO,
    T5_CMD_CHOICES_REC_DP_CLIENT,
    T5_CMD_CHOICES_SPELL_AUTO_CHECK,
    T5_CMD_CHOICES_SPELL_WRITE_USER,

    T5_CMD_BIND_FILE_TYPE,
    T5_CMD_OBJECT_BIND_CONSTRUCT,
    T5_CMD_OBJECT_BIND_LOADER,
    T5_CMD_OBJECT_BIND_SAVER,

    T5_CMD_CTYPETABLE,

    T5_CMD_FONTMAP,
    T5_CMD_FONTMAP_END,
    T5_CMD_FONTMAP_REMAP,

    T5_CMD_SET_INTERACTIVE,

    T5_CMD_NEW_DOCUMENT,
    T5_CMD_OPEN_DOCUMENT,
    T5_CMD_CLOSE_DOCUMENT_REQ,

    T5_CMD_VIEW_CONTROL,
    T5_CMD_VIEW_CONTROL_INTRO,
    T5_CMD_BORDER_TOGGLE,
    T5_CMD_RULER_SCALE,
    T5_CMD_RULER_SCALE_V,
    T5_CMD_RULER_TOGGLE,
    T5_CMD_TOGGLE_H_SPLIT,
    T5_CMD_TOGGLE_V_SPLIT,

    T5_CMD_VIEW_CLOSE_REQ,
    T5_CMD_VIEW_CREATE,
    T5_CMD_VIEW_MAXIMIZE,
    T5_CMD_VIEW_MINIMIZE,
    T5_CMD_VIEW_NEW,
    T5_CMD_VIEW_POSN,
    T5_CMD_VIEW_SCROLL,
    T5_CMD_VIEW_SIZE,
    T5_CMD_VIEW_ZOOM,

    T5_CMD_BUTTON,
    T5_CMD_COMMENT,

    T5_CMD_CURSOR_LEFT,
    T5_CMD_CURSOR_RIGHT,
    T5_CMD_CURSOR_DOWN,
    T5_CMD_CURSOR_UP,
    T5_CMD_TAB_LEFT,
    T5_CMD_TAB_RIGHT,
    T5_CMD_WORD_LEFT,
    T5_CMD_WORD_RIGHT,
    T5_CMD_LINE_START,
    T5_CMD_LINE_END,
    T5_CMD_FIRST_COLUMN,
    T5_CMD_LAST_COLUMN,
    T5_CMD_PAGE_DOWN,
    T5_CMD_PAGE_UP,
    T5_CMD_DOCUMENT_BOTTOM,
    T5_CMD_DOCUMENT_TOP,

    T5_CMD_SHIFT_CURSOR_LEFT,
    T5_CMD_SHIFT_CURSOR_RIGHT,
    T5_CMD_SHIFT_CURSOR_DOWN,
    T5_CMD_SHIFT_CURSOR_UP,
    T5_CMD_SHIFT_WORD_LEFT,
    T5_CMD_SHIFT_WORD_RIGHT,
    T5_CMD_SHIFT_LINE_START,
    T5_CMD_SHIFT_LINE_END,
    T5_CMD_SHIFT_PAGE_DOWN,
    T5_CMD_SHIFT_PAGE_UP,
    T5_CMD_SHIFT_DOCUMENT_BOTTOM,
    T5_CMD_SHIFT_DOCUMENT_TOP,

    T5_CMD_DELETE_CHARACTER_LEFT,
    T5_CMD_DELETE_CHARACTER_RIGHT,
    T5_CMD_DELETE_LINE,
    T5_CMD_RETURN,
    T5_CMD_ESCAPE,

    T5_CMD_SELECT_CELL,
    T5_CMD_SELECT_DOCUMENT,
    T5_CMD_SELECT_WORD,
    T5_CMD_SELECTION_MAKE,
    T5_CMD_TOGGLE_MARKS,
    T5_CMD_SELECTION_CLEAR,
    T5_CMD_SELECTION_COPY,
    T5_CMD_SELECTION_CUT,
    T5_CMD_SELECTION_DELETE,
    T5_CMD_PASTE_AT_CURSOR,

    T5_CMD_DEFINE_KEY,
    T5_CMD_DEFINE_KEY_RAW,
    T5_CMD_LEARN_KEY,

    T5_CMD_MENU_ADD,
    T5_CMD_MENU_ADD_RAW,
    T5_CMD_MENU_DELETE,
    T5_CMD_MENU_NAME,

    T5_CMD_NOTE,
    T5_CMD_NOTE_TWIN,           /* maps onto T5_CMD_NOTE, exists so that very old files still load and for compatibility with older readers */
    T5_CMD_NOTE_BACKDROP,       /* maps onto T5_CMD_NOTE, exists so that backdrops can be rejected on file insert */
    T5_CMD_NOTE_BACK,
    T5_CMD_NOTE_SWAP,
    T5_CMD_NOTE_EMBED,

    T5_CMD_CURRENT_DOCUMENT,
    T5_CMD_CURRENT_VIEW,
    T5_CMD_CURRENT_PANE,
    T5_CMD_CURRENT_POSITION,

    T5_CMD_EXECUTE,

    T5_CMD_LOAD,
    T5_CMD_LOAD_TEMPLATE,
    T5_CMD_LOAD_FOREIGN,

    T5_CMD_SAVE_OWNFORM,

    T5_CMD_SAVE_OWNFORM_AS,
    T5_CMD_SAVE_OWNFORM_AS_INTRO,

    T5_CMD_SAVE_TEMPLATE,
    T5_CMD_SAVE_TEMPLATE_INTRO,

    T5_CMD_SAVE_FOREIGN,
    T5_CMD_SAVE_FOREIGN_INTRO,

    T5_CMD_SAVE_AS_DRAWFILE,
    T5_CMD_SAVE_AS_DRAWFILE_INTRO,

    T5_CMD___SAVE_AS_METAFILE,  /* disabled for now */
    T5_CMD___SAVE_AS_METAFILE_INTRO,

    T5_CMD_SAVE_CLIPBOARD,
    T5_CMD_SAVE_PICTURE_INTRO,

    T5_CMD_PRINT,
    T5_CMD_PRINT_INTRO,
    T5_CMD_PRINT_EXTRA,
    T5_CMD_PRINT_EXTRA_INTRO,
    T5_CMD_PRINT_QUALITY,
    T5_CMD_PRINT_SETUP,         /* non-RISC OS versions */

    T5_CMD_PAPER,
    T5_CMD_PAPER_INTRO,

    T5_CMD_PAPER_SCALE,
    T5_CMD_PAPER_SCALE_INTRO,

    T5_CMD_BOX,
    T5_CMD_BOX_INTRO,

    T5_CMD_SEARCH,
    T5_CMD_SEARCH_INTRO,

    T5_CMD_SORT,
    T5_CMD_SORT_INTRO,

    T5_CMD_STYLE_APPLY,
    T5_CMD_STYLE_APPLY_SOURCE,
    T5_CMD_STYLE_APPLY_STYLE_HANDLE,
    T5_CMD_STYLE_BUTTON,
    T5_CMD_STYLE_FOR_CONFIG,
    T5_CMD_STYLE_RECORDZ,

    T5_CMD_APPLY_EFFECTS,
    T5_CMD_EFFECTS_BUTTON,

    T5_CMD_STYLE_REGION_COUNT,
    T5_CMD_STYLE_REGION_CLEAR,
    T5_CMD_STYLE_REGION_EDIT,

    T5_CMD_SNAPSHOT,
    T5_CMD_BLOCK_CLEAR,

    T5_CMD_REPLICATE_DOWN,
    T5_CMD_REPLICATE_RIGHT,
    T5_CMD_REPLICATE_UP,
    T5_CMD_REPLICATE_LEFT,

    T5_CMD_AUTO_SUM,

    T5_CMD_STRADDLE_HORZ,

    T5_CMD_NEW_EXPRESSION,

    T5_CMD_IMAGE_FILE_EMBEDDED,
    T5_CMD_DRAWFILE_EMBEDDED,
    T5_CMD_DRAWFILE_REFERENCE,

    T5_CMD_AUTO_WIDTH,
    T5_CMD_AUTO_HEIGHT,
    T5_CMD_AUTO_SAVE,

    T5_CMD_SETC_UPPER,
    T5_CMD_SETC_LOWER,
    T5_CMD_SETC_INICAP,
    T5_CMD_SETC_SWAP,

    T5_CMD_INSERT_FIELD_DATE,
    T5_CMD_INSERT_FIELD_FILE_DATE,
    T5_CMD_INSERT_FIELD_PAGE_X,
    T5_CMD_INSERT_FIELD_PAGE_Y,
    T5_CMD_INSERT_FIELD_SS_NAME,
    T5_CMD_INSERT_FIELD_MS_FIELD,
    T5_CMD_INSERT_FIELD_WHOLENAME,
    T5_CMD_INSERT_FIELD_LEAFNAME,
    /*T5_CMD_INSERT_FIELD_HARD_HYPHEN,*/
    T5_CMD_INSERT_FIELD_RETURN,
    T5_CMD_INSERT_FIELD_SOFT_HYPHEN,
    T5_CMD_INSERT_FIELD_TAB,

    T5_CMD_INSERT_FIELD_INTRO_DATE,
    T5_CMD_INSERT_FIELD_INTRO_TIME,
    T5_CMD_INSERT_FIELD_INTRO_PAGE,
    T5_CMD_INSERT_FIELD_INTRO_MS_FIELD, /* mailshot */

    T5_CMD_NUMFORM_LOAD,
    T5_CMD_NUMFORM_DATA,

    T5_CMD_PAGE_HEFO_BREAK_BASE_VALUES,
    T5_CMD_PAGE_HEFO_BREAK_INTRO,
    T5_CMD_PAGE_HEFO_BREAK_VALUES,
    T5_CMD_PAGE_HEFO_BREAK_VALUES_INTRO,

    T5_CMD_ADD_COLS,
    T5_CMD_ADD_COLS_INTRO,
    T5_CMD_ADD_ROWS,
    T5_CMD_ADD_ROWS_INTRO,
    T5_CMD_DELETE_COL,
    T5_CMD_INSERT_COL,
    T5_CMD_DELETE_ROW,
    T5_CMD_INSERT_ROW,
    T5_CMD_INSERT_TABLE,
    T5_CMD_INSERT_TABLE_INTRO,

    T5_CMD_INSERT_PAGE_BREAK,

    T5_CMD_PLAIN_TEXT_TEMP,
    T5_CMD_WORD_COUNT,

    T5_CMD_BACKDROP,
    T5_CMD_BACKDROP_INTRO,

    T5_CMD_MAILSHOT_SELECT,
    T5_CMD_MAILSHOT_PRINT,

    T5_CMD_THESAURUS,

    T5_CMD_OBJECT_CONVERT,
    T5_CMD_OBJECT_ENSURE,

    T5_CMD_FORCE_RECALC,
    T5_CMD_ACTIVATE_MENU_FUNCTION_SELECTOR,
    T5_CMD_ACTIVATE_MENU_CHART,

    T5_CMD_SS_CONTEXT,
    T5_CMD_SS_EDIT_FORMULA,
    T5_CMD_SS_FUNCTIONS,
    T5_CMD_SS_NAME,
    T5_CMD_SS_NAME_INTRO,

    T5_CMD_SS_MAKE_TEXT,
    T5_CMD_SS_MAKE_NUMBER,

    T5_CMD_INSERT_OPERATOR_PLUS,
    T5_CMD_INSERT_OPERATOR_MINUS,
    T5_CMD_INSERT_OPERATOR_TIMES,
    T5_CMD_INSERT_OPERATOR_DIVIDE,

    T5_CMD_SPELL_CHECK,
    T5_CMD_SPELL_DICTIONARY,
    T5_CMD_SPELL_DICTIONARY_ADD_WORD,
    T5_CMD_SPELL_DICTIONARY_DELETE_WORD,

    /*******************************************************/

    T5_CMD_CHART_GALLERY,
    T5_CMD_CHART_EDIT,
    T5_CMD_CHART_STYLE,
    T5_CMD_CHART_EDITX,

    T5_CMD_CHART_IO_1,
    T5_CMD_CHART_IO_2,
    T5_CMD_CHART_IO_3,
    T5_CMD_CHART_IO_4,
    T5_CMD_CHART_IO_5,

    /*
    start of direct injection
    */

    T5_CMD_CHART_IO_DIRECT_CHART_STT, /* = wherever we are now */

    T5_CMD_CHART_IO_AXES_MAX = T5_CMD_CHART_IO_DIRECT_CHART_STT,

    T5_CMD_CHART_IO_CORE_LAYOUT,
    T5_CMD_CHART_IO_CORE_MARGINS,

    T5_CMD_CHART_IO_LEGEND_BITS,
    T5_CMD_CHART_IO_LEGEND_POSN,

    T5_CMD_CHART_IO_D3_BITS,
    T5_CMD_CHART_IO_D3_DROOP,
    T5_CMD_CHART_IO_D3_TURN,

    T5_CMD_CHART_IO_BARCH_SLOT_2D_OVERLAP,

    T5_CMD_CHART_IO_LINECH_SLOT_2D_SHIFT,

    T5_CMD_CHART_IO_DIRECT_CHART_END,

    T5_CMD_CHART_IO_DIRECT_AXES_STT = T5_CMD_CHART_IO_DIRECT_CHART_END,

    T5_CMD_CHART_IO_AXES_BITS = T5_CMD_CHART_IO_DIRECT_AXES_STT,
    T5_CMD_CHART_IO_AXES_SERIES_TYPE,
    T5_CMD_CHART_IO_AXES_CHART_TYPE,

    T5_CMD_CHART_IO_DIRECT_AXES_END,

    T5_CMD_CHART_IO_DIRECT_AXIS_STT = T5_CMD_CHART_IO_DIRECT_AXES_END,

    T5_CMD_CHART_IO_AXIS_BITS = T5_CMD_CHART_IO_DIRECT_AXIS_STT,
    T5_CMD_CHART_IO_AXIS_PUNTER_MIN,
    T5_CMD_CHART_IO_AXIS_PUNTER_MAX,

    T5_CMD_CHART_IO_AXIS_MAJOR_BITS,
    T5_CMD_CHART_IO_AXIS_MAJOR_PUNTER,

    T5_CMD_CHART_IO_AXIS_MINOR_BITS,
    T5_CMD_CHART_IO_AXIS_MINOR_PUNTER,

    T5_CMD_CHART_IO_DIRECT_AXIS_END,

    T5_CMD_CHART_IO_DIRECT_SERIES_STT = T5_CMD_CHART_IO_DIRECT_AXIS_END,

    T5_CMD_CHART_IO_SERIES_BITS = T5_CMD_CHART_IO_DIRECT_SERIES_STT,
    T5_CMD_CHART_IO_SERIES_PIE_HEADING,
    T5_CMD_CHART_IO_SERIES_SERIES_TYPE,
    T5_CMD_CHART_IO_SERIES_CHART_TYPE,

    T5_CMD_CHART_IO_DIRECT_SERIES_END,

    /*
    end of direct injection
    */

    T5_CMD_CHART_IO_FILLSTYLEB = T5_CMD_CHART_IO_DIRECT_SERIES_END,
    T5_CMD_CHART_IO_LINESTYLE,
    T5_CMD_CHART_IO_TEXTSTYLE,

    T5_CMD_CHART_IO_BARCHSTYLE,
    T5_CMD_CHART_IO_BARLINECHSTYLE,
    T5_CMD_CHART_IO_LINECHSTYLE,
    T5_CMD_CHART_IO_PIECHDISPLSTYLE,
    T5_CMD_CHART_IO_PIECHLABELSTYLE,
    T5_CMD_CHART_IO_SCATCHSTYLE,

    T5_CMD_CHART_IO_TEXTPOS,

    T5_CMD_CHART_IO_PICT_TRANS_REF,

    T5_CMD_CHART_IO_OBJID,
    T5_CMD_CHART_IO_TEXTCONTENTS,
    T5_CMD_CHART_IO_AXES_START_SERIES,
    T5_CMD_CHART_IO_PICT_TRANS_EMB,
    T5_CMD_CHART_IO_FILLSTYLEC,

    /*******************************************************/

    T5_CMD_ADD_RECORDZ,
    T5_CMD_APPEND_RECORDZ,
    T5_CMD_CREATE_RECORDZ,
    T5_CMD_CLOSE_RECORDZ,
    T5_CMD_COPY_RECORDZ,
    T5_CMD_DELETE_RECORDZ,
    T5_CMD_EXPORT_RECORDZ,
    T5_CMD_LAYOUT_RECORDZ,
    T5_CMD_PROP_RECORDZ,
    T5_CMD_SEARCH_RECORDZ,
    T5_CMD_SORT_RECORDZ,
    T5_CMD_VIEW_RECORDZ,

    T5_CMD_RECORDZ_NEXT,
    T5_CMD_RECORDZ_PREV,
    T5_CMD_RECORDZ_ERSTE,
    T5_CMD_RECORDZ_LETSTE,
    T5_CMD_RECORDZ_FFWD,
    T5_CMD_RECORDZ_REWIND,
    T5_CMD_RECORDZ_GOTO_RECORD,
    T5_CMD_RECORDZ_GOTO_RECORD_INTRO,

    T5_CMD_SEARCH_BUTTON_POSS_DB_QUERIES,
    T5_CMD_SEARCH_BUTTON_POSS_DB_QUERY,

    T5_CMD_RECORDZ_IO_TABLE,
    T5_CMD_RECORDZ_IO_TITLE,
    T5_CMD_RECORDZ_IO_FRAME,
    T5_CMD_RECORDZ_IO_LAYOUT,
    T5_CMD_RECORDZ_IO_QUERY,
    T5_CMD_RECORDZ_IO_PATTERN,
    T5_CMD_RECORDZ_IO_SPARE4,
    T5_CMD_RECORDZ_IO_SPARE5,

    /*******************************************************/

    T5_CMD_HELP_CONTENTS,
    T5_CMD_HELP_SEARCH_KEYWORD,
    T5_CMD_HELP_URL,
    T5_CMD_INFO,
    T5_CMD_QUIT,

    T5_CMD_NYI,
    T5_CMD_REMOVED,
    T5_CMD_TRACE,
    T5_CMD_TEST,

    /*******************************************************/

    T5_CMD_TOOLBAR_TOOL,

    /*******************************************************/

    T5_CMD__ACTUAL_END,

    T5_CMD__END = T5_CMD__START + 512
}
T5_MESSAGE, * P_T5_MESSAGE; typedef const T5_MESSAGE * PC_T5_MESSAGE;

#define T5_MESSAGE_INCR(t5_message__ref) \
    ENUM_INCR(T5_MESSAGE, t5_message__ref)

#if CHECKING && defined(T5_MSG_STRUCT_INIT)
#define msgclr(msg) memset32(&(msg), '\xBC', sizeof32(msg))
#else
#define msgclr(msg) /* nothing */
#endif

_Check_return_
static inline BOOL
t5_message_is_inline(_InVal_ T5_MESSAGE t5_message)
{
    return(t5_message < 256);
}

_Check_return_
static inline BOOL
t5_message_is_eom(_InVal_ T5_MESSAGE t5_message)
{
    return((t5_message >= T5_EVENT__START) && (t5_message < T5_MSG__END));
}

#define T5_MESSAGE_CMD_OFFSET(t5_message) ( \
    ((U32) (t5_message) - (U32) T5_CMD__START) )

#define T5_MESSAGE_IS_CMD(t5_message) ( \
    T5_MESSAGE_CMD_OFFSET(t5_message) < T5_MESSAGE_CMD_OFFSET(T5_CMD__END) )

_Check_return_
static inline BOOL
t5_message_is_cmd(_InVal_ T5_MESSAGE t5_message)
{
    return((t5_message >= T5_CMD__START) && (t5_message < T5_CMD__END));
}

/*
colour definition
*/

typedef struct RGB
{
    U8 r;
    U8 g;
    U8 b;
    U8 transparent;
}
RGB, * P_RGB; typedef const RGB * PC_RGB;

#define RGB_INIT_BLACK { 0, 0, 0, 0 }

#define RGB_INIT(r, g, b) { r, g, b, 0 }

static inline void
rgb_set(
    _OutRef_    P_RGB p_rgb,
    _InVal_     U8 r,
    _InVal_     U8 g,
    _InVal_     U8 b)
{
    p_rgb->r = r;
    p_rgb->g = g;
    p_rgb->b = b;
    p_rgb->transparent = 0;
}

_Check_return_
static inline BOOL
rgb_compare_not_equals(
    _InRef_     PC_RGB p_rgb_1,
    _InRef_     PC_RGB p_rgb_2)
{
    if(p_rgb_1->transparent && p_rgb_2->transparent)
        return(FALSE); /* same - both transparent */

    if(p_rgb_1->transparent != p_rgb_2->transparent)
        return(TRUE); /* differ - one transparent, one not */

    /* both are solid colours - any components differ? */
    return( (p_rgb_1->r != p_rgb_2->r) ||
            (p_rgb_1->g != p_rgb_2->g) ||
            (p_rgb_1->b != p_rgb_2->b) );
}

typedef struct PIXIT_POINT
{
    PIXIT x, y;
}
PIXIT_POINT, * P_PIXIT_POINT; typedef const PIXIT_POINT * PC_PIXIT_POINT;

#define PIXIT_POINT_TFMT \
    TEXT("x = ") PIXIT_TFMT TEXT(", y = ") PIXIT_TFMT

#define PIXIT_POINT_ARGS(pixit_point__ref) \
    (pixit_point__ref).x, \
    (pixit_point__ref).y

#define pixit_point_distance(xy, pixit_point_br__ref, pixit_point_tl__ref) ( \
    (pixit_point_br__ref).xy - (pixit_point_tl__ref).xy )

typedef struct PIXIT_SIZE
{
    PIXIT cx, cy;
}
PIXIT_SIZE, * P_PIXIT_SIZE; typedef const PIXIT_SIZE * PC_PIXIT_SIZE;

typedef struct PIXIT_RECT
{
    PIXIT_POINT tl, br;
}
PIXIT_RECT, * P_PIXIT_RECT; typedef const PIXIT_RECT * PC_PIXIT_RECT;

#define PIXIT_RECT_TFMT \
    TEXT("tl = ") PIXIT_TFMT TEXT(",") PIXIT_TFMT  TEXT("; ") \
    TEXT("br = ") PIXIT_TFMT TEXT(",") PIXIT_TFMT

#define PIXIT_RECT_ARGS(pixit_rect__ref) \
    (pixit_rect__ref).tl.x, \
    (pixit_rect__ref).tl.y, \
    (pixit_rect__ref).br.x, \
    (pixit_rect__ref).br.y

_Check_return_
static inline PIXIT
pixit_rect_width(
    _InRef_     PC_PIXIT_RECT p_pixit_rect)
{
    return(p_pixit_rect->br.x - p_pixit_rect->tl.x);
}

_Check_return_
static inline PIXIT
pixit_rect_height(
    _InRef_     PC_PIXIT_RECT p_pixit_rect)
{
    return(p_pixit_rect->br.y - p_pixit_rect->tl.y);
}

typedef struct PIXIT_LINE
{
    PIXIT_POINT tl, br;
    BOOL horizontal;
}
PIXIT_LINE, * P_PIXIT_LINE; typedef const PIXIT_LINE * PC_PIXIT_LINE;

typedef struct PAGE_NUM
{
    PAGE x, y;
}
PAGE_NUM, * P_PAGE_NUM; typedef const PAGE_NUM * PC_PAGE_NUM;

#define P_PAGE_NUM_NONE _P_DATA_NONE(P_PAGE_NUM)

typedef struct SKEL_POINT
{
    PIXIT_POINT pixit_point;
    PAGE_NUM    page_num;
}
SKEL_POINT, * P_SKEL_POINT; typedef const SKEL_POINT * PC_SKEL_POINT;

typedef struct SKEL_EDGE
{
    PIXIT pixit;
    PAGE page;
}
SKEL_EDGE, * P_SKEL_EDGE;

typedef struct SKEL_RECT
{
    SKEL_POINT tl, br;
}
SKEL_RECT, * P_SKEL_RECT; typedef const SKEL_RECT * PC_SKEL_RECT;

_Check_return_
static inline PIXIT
skel_rect_width(
    _InRef_     PC_SKEL_RECT p_skel_rect)
{
    return(p_skel_rect->br.pixit_point.x - p_skel_rect->tl.pixit_point.x);
}

_Check_return_
static inline PIXIT
skel_rect_height(
    _InRef_     PC_SKEL_RECT p_skel_rect)
{
    return(p_skel_rect->br.pixit_point.y - p_skel_rect->tl.pixit_point.y);
}

/* macro to compare two skeleton points */
#define skel_point_compare(p_a, p_b, xy) (                           \
     (p_a)->page_num.xy > (p_b)->page_num.xy                         \
        ? 1                                                          \
        : ((p_a)->page_num.xy < (p_b)->page_num.xy                   \
              ? -1                                                   \
              : ((p_a)->pixit_point.xy > (p_b)->pixit_point.xy       \
                    ? 1                                              \
                    : ((p_a)->pixit_point.xy < (p_b)->pixit_point.xy \
                          ? -1                                       \
                          : 0)))                                     )

/* macro to compare two skeleton edges */
#define skel_edge_compare(e_a, e_b) (                  \
     (e_a)->page > (e_b)->page                         \
        ? 1                                            \
        : ((e_a)->page < (e_b)->page                   \
              ? -1                                     \
              : ((e_a)->pixit > (e_b)->pixit           \
                    ? 1                                \
                    : ((e_a)->pixit < (e_b)->pixit     \
                          ? -1                         \
                          : 0)))                       )

/* set edge from a point and vice versa */
#define skel_point_update_from_edge(p_s, p_e, xy) ( \
    (p_s)->pixit_point.xy = (p_e)->pixit,           \
    (p_s)->page_num.xy = (p_e)->page                )

#define edge_set_from_skel_point(p_e, p_s, xy) (    \
    (p_e)->pixit = (p_s)->pixit_point.xy,           \
    (p_e)->page = (p_s)->page_num.xy                )

/* macro to compare a skeleton point to an edge */
#define skel_point_edge_compare(p_s, p_e, xy) (             \
     (p_s)->page_num.xy > (p_e)->page                       \
        ? 1                                                 \
        : ((p_s)->page_num.xy < (p_e)->page                 \
              ? -1                                          \
              : ((p_s)->pixit_point.xy > (p_e)->pixit       \
                    ? 1                                     \
                    : ((p_s)->pixit_point.xy < (p_e)->pixit \
                         ? -1                               \
                         : 0)))                             )

/* macro to compare an edge to a skeleton point */
#define edge_skel_point_compare(p_e, p_s, xy) (         \
     (p_e)->page > (p_s)->page_num.xy                   \
        ? 1                                             \
        : ((p_e)->page < (p_s)->page_num.xy             \
           ? -1                                         \
           : ((p_e)->pixit > (p_s)->pixit_point.xy      \
                ? 1                                     \
                : ((p_e)->pixit < (p_s)->pixit_point.xy \
                      ? -1                              \
                      : 0)))                            )

typedef struct VIEW_POINT
{
    PIXIT_POINT pixit_point;
    P_VIEW      p_view;
}
VIEW_POINT, * P_VIEW_POINT; typedef const VIEW_POINT * PC_VIEW_POINT;

typedef struct VIEW_RECT
{
    PIXIT_RECT  rect;
    P_VIEW      p_view;
}
VIEW_RECT, * P_VIEW_RECT; typedef const VIEW_RECT * PC_VIEW_RECT;

/*
cell (slot) reference
*/

typedef struct SLR
{
    COL col;
    ROW row;
}
SLR, * P_SLR; typedef const SLR * PC_SLR;

#define SLR_INIT { 0, 0 }

/*
data reference
*/

typedef U8 DATA_SPACE; typedef DATA_SPACE * P_DATA_SPACE;

enum DATA_REF_IDS
{
    DATA_NONE,
    DATA_SLOT,
    DATA_HEADER_ODD,
    DATA_FOOTER_ODD,
    DATA_HEADER_EVEN,
    DATA_FOOTER_EVEN,
    DATA_HEADER_FIRST,
    DATA_FOOTER_FIRST,
    DATA_DB_FIELD,
    DATA_DB_TITLE,
    DATA_COUNT
};

/* Recordz related typedefs */

typedef U32 PROJECTOR_TYPE;

#define PROJECTOR_TYPE_ERROR 0xFFFFFFFFU
#define PROJECTOR_TYPE_SHEET 0U /* forever value - saved in files */
#define PROJECTOR_TYPE_CARD  1U /* forever value - saved in files */

typedef U32 DB_ID;
#define DB_ID_BAD 0U

typedef U32 FIELD_ID;
#define FIELD_ID_BAD 0xCC000000U /* I don't know whether this means bad to Neil or what */

typedef struct RECORDZ_DEFAULTS
{
    PROJECTOR_TYPE projector_type; /* PROJECTOR_TYPE_CARD or PROJECTOR_TYPE_SHEET */
    COL cols;
    ROW rows;
}
RECORDZ_DEFAULTS;

typedef struct DB_FIELD
{
    PROJECTOR_TYPE projector_type;
    DB_ID db_id;
    S32 record;
    FIELD_ID field_id;
}
DB_FIELD;

typedef struct DB_TITLE
{
    PROJECTOR_TYPE projector_type;  /* offsetof32() must be same as DB_FIELD */
    DB_ID db_id;                    /* offsetof32() must be same as DB_FIELD */
    S32 record;                     /* allegedly not valid as part of this DATA_SPACE - so why is it set & compared? */
    FIELD_ID field_id;
}
DB_TITLE;

typedef union DATA_REF_ARG
{
    ROW row;
    SLR slr;
    DB_FIELD db_field;
    DB_TITLE db_title;
}
DATA_REF_ARG;

typedef struct DATA_REF
{
    DATA_SPACE data_space;
    DATA_REF_ARG arg;
}
DATA_REF, * P_DATA_REF; typedef const DATA_REF * PC_DATA_REF;

/* Used to get objects to convert one to the other, e.g. T5_MSG_DATA_REF_FROM_SLR */

typedef struct DATA_REF_AND_SLR
{
    DATA_REF data_ref;
    SLR slr;
}
DATA_REF_AND_SLR, * P_DATA_REF_AND_SLR;

/*
region
*/

typedef struct REGION
{
    SLR tl;
    SLR br;
    BOOL whole_col;
    BOOL whole_row;
}
REGION, * P_REGION; typedef const REGION * PC_REGION;

#define REGION_INIT { SLR_INIT, SLR_INIT, FALSE, FALSE }

/*
row range
*/

typedef struct ROW_RANGE
{
    ROW top;
    ROW bot;
}
ROW_RANGE, * P_ROW_RANGE; typedef const ROW_RANGE * PC_ROW_RANGE;

/* compare two slrs, cols first */
#define slr_compare(pc_slr1, pc_slr2) (         \
     (pc_slr1)->col > (pc_slr2)->col            \
      ? 1                                       \
      : (pc_slr1)->col < (pc_slr2)->col         \
         ? -1                                   \
         : (pc_slr1)->row > (pc_slr2)->row      \
            ? 1                                 \
            : (pc_slr1)->row < (pc_slr2)->row   \
               ? -1                             \
               : 0                              )

/* compare two slrs for equality (faster than slr_compare) */
#define slr_equal(pc_slr1, pc_slr2) (   \
    (pc_slr1)->col == (pc_slr2)->col && \
    (pc_slr1)->row == (pc_slr2)->row    )

/* compare an slr with a region end point*/
#define slr_equal_end(pc_slre, pc_slr) (    \
    (pc_slre)->col - 1 == (pc_slr)->col &&  \
    (pc_slre)->row - 1 == (pc_slr)->row     )

/* check slr is in range */
#define slr_in_range(p_docu, pc_slr) (          \
    (pc_slr)->col >= 0                      &&  \
    (pc_slr)->col < n_cols_logical(p_docu)  &&  \
    (pc_slr)->row >= 0                      &&  \
    (pc_slr)->row < n_rows(p_docu)              )

/* make data ref from slr */
#define data_ref_from_slr(p_data_ref, pc_slr) ( \
    (p_data_ref)->data_space = DATA_SLOT,       \
    (p_data_ref)->arg.slr = *(pc_slr)           )

/*
paper details
*/

typedef struct PAPER
{
    /* paper variables */
    PIXIT x_size;                       /* x dimension of paper */
    PIXIT y_size;                       /* y dimension of paper */
    PIXIT lm;                           /* paper left margin */
    PIXIT tm;                           /* paper top margin */
    PIXIT rm;                           /* paper right margin */
    PIXIT bm;                           /* paper bottom margin */
    PIXIT print_area_x;                 /* width before horizontal break */
    PIXIT print_area_y;                 /* height before vertical break */
}
PAPER, * P_PAPER;

/*
paper
*/

typedef struct PAGE_DEF
{
    PIXIT size_x;
    PIXIT size_y;
    PIXIT margin_left;
    PIXIT margin_right;
    PIXIT margin_top;
    PIXIT margin_bottom;
    PIXIT margin_bind;
    PIXIT margin_col;
    PIXIT margin_row;
    PIXIT grid_size;
    PIXIT cells_usable_x;
    PIXIT cells_usable_y;
    S32 draw_file_ref;
    U8 landscape;
    U8 margin_oe_swap;
    U8 draw_file_does_print;
    U8 _spare[1];
}
PAGE_DEF, * P_PAGE_DEF;

typedef struct PHYS_PAGE_DEF
{
    UCHARZ ustr_name[36]; /* name of paper */
    PIXIT size_x;
    PIXIT size_y;
    PIXIT margin_left;
    PIXIT margin_right;
    PIXIT margin_top;
    PIXIT margin_bottom;
    PIXIT margin_bind;
    U8 landscape;
    U8 margin_oe_swap;
    U8 _spare[2];
}
PHYS_PAGE_DEF, * P_PHYS_PAGE_DEF; typedef const PHYS_PAGE_DEF * PC_PHYS_PAGE_DEF;

/*
tab list
*/

enum TAB_TYPES
{
    TAB_LEFT    = 0,
    TAB_CENTRE  = 1,
    TAB_RIGHT   = 2,
    TAB_DECIMAL = 3,
    N_TAB_TYPES
};

typedef U8 TAB_TYPE; /* SKS 05apr94 now packed */

typedef struct TAB_INFO
{
    TAB_TYPE type;
    U8 _spare[3];
    PIXIT offset;
}
TAB_INFO, * P_TAB_INFO; typedef const TAB_INFO * PC_TAB_INFO;

/*
column info block - communication about changes to column styles
*/

typedef struct COL_INFO
{
    COL col;
    SKEL_EDGE edge_left;
    SKEL_EDGE edge_right;
    PIXIT margin_left;
    PIXIT margin_right;
    PIXIT margin_para;
    ARRAY_HANDLE h_tab_list;
    ARRAY_INIT_BLOCK tab_init_block;
}
COL_INFO, * P_COL_INFO; typedef const COL_INFO * PC_COL_INFO;

#define P_COL_INFO_NONE _P_DATA_NONE(P_COL_INFO)

/*
object owned position data
*/

typedef struct OBJECT_POSITION
{
    S32 data;
    S32 more_data; /* This is so that Recordz can keep a field id in it */
    OBJECT_ID object_id;
}
OBJECT_POSITION, * P_OBJECT_POSITION; typedef const OBJECT_POSITION * PC_OBJECT_POSITION;

#define P_OBJECT_POSITION_NONE _P_DATA_NONE(P_OBJECT_POSITION)

#define object_position_init(p_object_position) ( \
    (p_object_position)->data = 0, \
    (p_object_position)->more_data = -1, \
    (p_object_position)->object_id = OBJECT_ID_NONE )

/*
position structure
*/

typedef struct POSITION
{
    SLR slr; /* This should be changed to be a data_ref */
    OBJECT_POSITION object_position;
}
POSITION, * P_POSITION; typedef const POSITION * PC_POSITION;

#define P_POSITION_NONE _P_DATA_NONE(P_POSITION)

#define position_init(p_position) ( \
    (p_position)->slr.col = 0, \
    (p_position)->slr.row = 0, \
    object_position_init(&(p_position)->object_position) )

#define position_init_from_col_row(p_position, col_i, row_i) ( \
    (p_position)->slr.col = (col_i), \
    (p_position)->slr.row = (row_i), \
    object_position_init(&(p_position)->object_position) )

/* Used to convert position information to/from data_ref information via the messages

  T5_MSG_DATA_REF_TO_POSITION and
  T5_MSG_DATA_REF_FROM_POSITION

  See ob_rec and also see T5_MSG_DATA_REF_TO/FROM/_SLR
*/

typedef struct DATA_REF_AND_POSITION
{
    DATA_REF data_ref;
    POSITION position;
}
DATA_REF_AND_POSITION, * P_DATA_REF_AND_POSITION;

typedef struct OBJECT_POSITION_COMPARE
{
    OBJECT_POSITION first;
    OBJECT_POSITION second;
    S32 resultant_order; /*OUT*/
}
OBJECT_POSITION_COMPARE, * P_OBJECT_POSITION_COMPARE;

/*
object data specifier
*/

typedef struct OBJECT_DATA
{
    DATA_REF data_ref;
    OBJECT_ID object_id;
    OBJECT_POSITION object_position_start;
    OBJECT_POSITION object_position_end;
    union OBJECT_DATA_U                 /* union saves nasty casts */
    {
        P_ANY p_object;
        P_USTR_INLINE ustr_inline;      /* one extremely common usage for OBJECT_ID_TEXT */
        struct EV_CELL * p_ev_cell;     /* another for OBJECT_ID_SS */
    } u;
}
OBJECT_DATA, * P_OBJECT_DATA; typedef const OBJECT_DATA * PC_OBJECT_DATA;

/*
inline type object data
*/

typedef struct INLINE_OBJECT
{
    OBJECT_DATA object_data;
    S32 inline_len;
}
INLINE_OBJECT, * P_INLINE_OBJECT;

/*
position in header/footer
*/

typedef struct HEFO_POSITION
{
    DATA_REF data_ref;
    PAGE_NUM page_num;
    OBJECT_POSITION object_position;
}
HEFO_POSITION, * P_HEFO_POSITION;

/*
area of a document
*/

typedef struct DOCU_AREA
{
    POSITION tl;
    POSITION br;
    BOOL whole_col;
    BOOL whole_row;
}
DOCU_AREA, * P_DOCU_AREA; typedef const DOCU_AREA * PC_DOCU_AREA;

static inline void
docu_area_init(
    _OutRef_    P_DOCU_AREA p_docu_area)
{
    zero_struct_ptr(p_docu_area);
    p_docu_area->tl.object_position.object_id = OBJECT_ID_NONE;
    p_docu_area->br.object_position.object_id = OBJECT_ID_NONE;
}

static inline void
docu_area_set_cols(
    _InoutRef_  P_DOCU_AREA p_docu_area,
    _InVal_     COL tl_col,
    _InVal_     COL n_cols)
{
    p_docu_area->tl.slr.col = tl_col;
    p_docu_area->br.slr.col = tl_col + n_cols;
    p_docu_area->whole_row = 0;
}

static inline void
docu_area_set_rows(
    _InoutRef_  P_DOCU_AREA p_docu_area,
    _InVal_     ROW tl_row,
    _InVal_     ROW n_rows)
{
    p_docu_area->tl.slr.row = tl_row;
    p_docu_area->br.slr.row = tl_row + n_rows;
    p_docu_area->whole_col = 0;
}

static inline void
docu_area_set_whole_col(
    _InoutRef_  P_DOCU_AREA p_docu_area)
{
    p_docu_area->tl.slr.row = -1;
    p_docu_area->br.slr.row = -1;
    p_docu_area->whole_col = 1;
}

static inline void
docu_area_set_whole_row(
    _InoutRef_  P_DOCU_AREA p_docu_area)
{
    p_docu_area->tl.slr.col = -1;
    p_docu_area->br.slr.col = -1;
    p_docu_area->whole_row = 1;
}

/*
font specification
*/

typedef struct FONT_SPEC /* Fireworkz spec */
{
    ARRAY_HANDLE_TSTR h_app_name_tstr; /* Fireworkz system-independent 'font' name: [] of TCHAR, CH_NULL-terminated */
    PIXIT size_x;
    PIXIT size_y;

    RGB colour;

    U8 bold;
    U8 italic;
    U8 underline;
    U8 superscript;
    U8 subscript;
    U8 _padding_to_stop_compiler_winge[3];
}
FONT_SPEC, * P_FONT_SPEC; typedef const FONT_SPEC * PC_FONT_SPEC;

_Check_return_
static inline STATUS
font_spec_name_alloc(
    _InoutRef_  P_FONT_SPEC p_font_spec,
    _In_z_      PCTSTR app_name_tstr)
{
    return(al_tstr_set(&p_font_spec->h_app_name_tstr, app_name_tstr));
}

static inline void
font_spec_name_dispose(
    _InoutRef_  P_FONT_SPEC p_font_spec)
{
    al_array_dispose(&p_font_spec->h_app_name_tstr);
}

static inline void
font_spec_dispose(
    _InoutRef_  P_FONT_SPEC p_font_spec)
{
    font_spec_name_dispose(p_font_spec);
}

typedef struct HOST_FONT_SPEC /* host-specific spec */
{
    ARRAY_HANDLE_TSTR h_host_name_tstr; /* host font name: [] of TCHAR, CH_NULL-terminated  */
    PIXIT size_x;
    PIXIT size_y;

    /* RISC OS: Bold & Italic have been factored into the h_host_name_tstr */
    /* Windows: Bold & Italic have been factored into the logfont */
    U8 underline;
    U8 superscript;
    U8 subscript;
    U8 _padding_to_stop_compiler_winge[1];

#if WINDOWS
    LOGFONT logfont;
#endif
}
HOST_FONT_SPEC, * P_HOST_FONT_SPEC; typedef const HOST_FONT_SPEC * PC_HOST_FONT_SPEC;

static inline void
host_font_spec_dispose(
    _InoutRef_  P_HOST_FONT_SPEC p_host_font_spec)
{
    al_array_dispose(&p_host_font_spec->h_host_name_tstr);
}

/*
event procedure prototypes
*/

typedef STATUS (* P_PROC_EVENT) (
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data);

#define PROC_EVENT_PROTO(_e_s, _proc_name) \
_e_s STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InVal_     T5_MESSAGE t5_message, \
    /*_Inout_*/ P_ANY p_data)

typedef P_PROC_EVENT * P_P_PROC_EVENT;

#define OBJECT_PROTO(_e_s, _object_name) \
    PROC_EVENT_PROTO(_e_s, _object_name)

typedef P_PROC_EVENT P_PROC_OBJECT;

#define T5_MSG_PROTO(_e_s, _proc_name, _data_type_and_name) \
_e_s STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InVal_     T5_MESSAGE t5_message, \
    _data_type_and_name)

#include "cmodules/resource.h"

#ifndef __ob_skel_resource_resource_h
#include "ob_skel/resource/resource.h"
#endif

extern BOOL g_has_colour_picker;

/*
but best not to test on these
*/

extern const int has_real_database;

/*
other structure & code exports
*/

/*
host specific window and event routines
*/

extern const PCTSTR leafname_helpfile_tstr;

extern const UINT g_product_id;

extern const PCTSTR dll_store;

#if WINDOWS

extern const PCTSTR key_program;

extern const PCTSTR atom_program;

extern UINT g_nColours;

extern HWND g_hwnd_sdi_back;

/* Actual product-specific class names are stored in ob_skel/{product}.c --- keep in step */
enum APP_WINDOW_CLASS
{
    APP_WINDOW_CLASS_DDE,
    APP_WINDOW_CLASS_BACK,
    APP_WINDOW_CLASS_PANE,
    APP_WINDOW_CLASS_BORDER,
    APP_WINDOW_CLASS_TOOLBAR,
    APP_WINDOW_CLASS_STATUS_LINE,
    APP_WINDOW_CLASS_SLE,
    APP_WINDOW_CLASS_SPLASH,

    APP_WINDOW_CLASS_MAX
};

extern const PCTSTR window_class[APP_WINDOW_CLASS_MAX];

/* Dialog manager related stuff (coordinate bashing) */

#define PIXITS_PER_RISCOS_ON_WINDOWS ((PIXIT) 8)

_Check_return_
extern HICON
host_get_prog_icon_large(void);

_Check_return_
extern HICON
host_get_prog_icon_small(void);

extern void
splash_window_create(
    _HwndRef_opt_ HWND hwnd,
    _In_        UINT splash_timeout_ms);

extern void
splash_window_remove(void);

extern HINSTANCE _g_hInstance;

_Check_return_
static inline HINSTANCE
GetInstanceHandle(void)
{
    return(_g_hInstance);
}

extern HINSTANCE g_hInstancePrev;

extern int g_nCmdShow;

_Check_return_
extern BOOL
bttncur_LibMain(
    HINSTANCE hInstance);

extern void
bttncur_WEP(void);

#define WIN32_SHELLDLG2_VERSION 0x0500 /* Windows 2000 or better.  None of 95/98/Me. */

_Check_return_
extern U32
MyGetProfileString(
    _In_z_      PCTSTR ptzKey,
    _In_z_      PCTSTR ptzDefault,
    _Out_writes_z_(cchReturnBuffer) PTSTR ptzReturnBuffer,
    _InVal_     U32 cchReturnBuffer);

#endif /* WINDOWS */

#if RISCOS

#define RISCOS_5_2 0xAA /* 5.22 */
#define RISCOS_4_3 0xA9 /* 4.39 (Adjust 1i2) */
#define RISCOS_4_0 0xA8 /* 4.02 */
#define RISCOS_3_7 0xA7
#define RISCOS_3_6 0xA6
#define RISCOS_3_5 0xA5
#define RISCOS_3_1 0xA4
#define RISCOS_3_0 0xA3

extern void
host_os_version_determine(void);

_Check_return_
extern UINT
host_os_version_query(void);

#endif /* RISCOS */

#define FAST_UPDATE_CALLS_NOTELAYER TRUE        /* when FALSE, fast_update marks all panes on all views for update_later */
                                                /*             (as well as doing a fast update of the caret pane), this  */
                                                /*             is to ensure that notes are redraw.                       */
                                                /* when TRUE,  we try to redraw notes in the caret pane directly as part */
                                                /*             of the fast update loop, instead of calling update later. */

#define PIXITS_PER_RISCOS       ((PIXIT) 8)     /* PIXITS_PER_INCH (1440) / RISCOS_PER_INCH (180) */
#if RISCOS
#define PIXITS_PER_PIXEL        ((PIXIT) 16)    /* PIXITS_PER_INCH (1440) / 90 dpi */
#elif WINDOWS
#define PIXITS_PER_PIXEL        ((PIXIT) 15)    /* PIXITS_PER_INCH (1440) / 96 dpi */
#endif
#define MILLIPOINTS_PER_RISCOS  400             /* 72000 / 180 */
#define RISCDRAW_PER_RISCOS     256

#if RISCOS
#define RISCOS_PER_PROGRAM_PIXEL_X 2 /* small units that we deal in as real machine pixels can get too small! */
#define RISCOS_PER_PROGRAM_PIXEL_Y 4

#define PIXITS_PER_PROGRAM_PIXEL_X (RISCOS_PER_PROGRAM_PIXEL_X * PIXITS_PER_RISCOS)
#define PIXITS_PER_PROGRAM_PIXEL_Y (RISCOS_PER_PROGRAM_PIXEL_Y * PIXITS_PER_RISCOS)
#elif WINDOWS
#define DU_PER_PROGRAM_PIXEL_X 1 /* small units that we deal in as real machine pixels can get too small! (on screen) */
#define DU_PER_PROGRAM_PIXEL_Y 2

#define PIXITS_PER_PROGRAM_PIXEL_X (DU_PER_PROGRAM_PIXEL_X * PIXITS_PER_PIXEL) /* in fact these are very similar to RISC OS ones (15,30 against 16,32) */
#define PIXITS_PER_PROGRAM_PIXEL_Y (DU_PER_PROGRAM_PIXEL_Y * PIXITS_PER_PIXEL)
#endif

#if RISCOS
#define T5_GDI_MIN_X -0x7F00
#define T5_GDI_MIN_Y -0x7F00
#define T5_GDI_MAX_X  0x7F00
#define T5_GDI_MAX_Y  0x7F00
#elif WINDOWS
#define T5_GDI_MIN_X -0x7000
#define T5_GDI_MIN_Y -0x7000
#define T5_GDI_MAX_X  0x7000
#define T5_GDI_MAX_Y  0x7000
#endif

/* definition of host font handle */

#if RISCOS
typedef int HOST_FONT; /*font*/
#define HOST_FONT_NONE ((HOST_FONT) 0)
#elif WINDOWS
#define HOST_FONT HFONT
#define HOST_FONT_NONE ((HOST_FONT) NULL)
#endif

typedef HOST_FONT * P_HOST_FONT;

#if RISCOS
#define _HfontRef_ _InVal_
#define _HfontRef_opt_ _InVal_ /* may be 0 */
#elif WINDOWS
#define _HfontRef_ _InRef_
#define _HfontRef_opt_ _InRef_opt_ /* may be NULL */
#endif

typedef enum REDRAW_TAG
{
    /* NB. there are tables in view.c that correspond to these tags */
    UPDATE_BORDER_HORZ          = 0,
    UPDATE_BORDER_VERT          = 1,
    UPDATE_RULER_HORZ           = 2,
    UPDATE_RULER_VERT           = 3,
    UPDATE_PANE_MARGIN_HEADER   = 4,
    UPDATE_PANE_MARGIN_FOOTER   = 5,
    UPDATE_PANE_MARGIN_COL      = 6,
    UPDATE_PANE_MARGIN_ROW      = 7,
    UPDATE_PANE_CELLS_AREA      = 8,
    UPDATE_PANE_PRINT_AREA      = 9,
    UPDATE_PANE_PAPER           = 10,
    UPDATE_BACK_WINDOW          = 11
}
REDRAW_TAG, * P_REDRAW_TAG; /* abstract 32-bit quantity for stricter type checking */

typedef struct REDRAW_TAG_AND_EVENT
{
    REDRAW_TAG redraw_tag;
    P_PROC_EVENT p_proc_event;
}
REDRAW_TAG_AND_EVENT, * P_REDRAW_TAG_AND_EVENT;

typedef enum LAYER
{
    /* -ve layers are below */
    LAYER_PAPER_BELOW       = -128,
    LAYER_PRINT_AREA_BELOW  = -2,
    LAYER_CELLS_AREA_BELOW  = -1,

    LAYER_CELLS             = 0,

    LAYER_CELLS_AREA_ABOVE  = +1,
    LAYER_PRINT_AREA_ABOVE  = +2,
    LAYER_PAPER_ABOVE       = +3
}
LAYER;

/******************************************************************************
*
* Redraw area modifiers for update_later, update_now, host_clip_rectangle/2
*
******************************************************************************/

typedef struct RECT_FLAGS
{
    UBF extend_right_by_1   : 1;        /* was 0..3 now 0..1 */
    UBF extend_right_page   : 1;
    UBF extend_right_window : 1;

    UBF reduce_right_by_1   : 1;
    UBF reduce_right__spare : 1;

    UBF extend_down_by_1    : 1;        /* was 0..3 now 0..1 */
    UBF extend_down__spare  : 1;
    UBF extend_down_page    : 1;
    UBF extend_down_window  : 1;

    UBF reduce_down_by_1    : 1;
    UBF reduce_down__spare  : 1;

    UBF extend_left_currently_unused : 1;

    UBF reduce_left_by_2    : 1;
    UBF reduce_left_by_1    : 1;

    UBF extend_up_currently_unused   : 1;

    UBF reduce_up_by_2      : 1;
    UBF reduce_up_by_1      : 1;

    UBF extend_right_ppixels    : 5;    /* PROGRAM_PIXEL_X (0..31) */
    UBF extend_down_ppixels     : 4;    /* PROGRAM_PIXEL_Y (0..15) */
    UBF extend_left_ppixels     : 3;    /* PROGRAM_PIXEL_X (0..7)  */
    UBF extend_up_ppixels       : 3;    /* PROGRAM_PIXEL_Y (0..7)  */

    /* that's it, I'm afraid. 32 bits full */
}
RECT_FLAGS, * P_RECT_FLAGS; typedef const RECT_FLAGS * PC_RECT_FLAGS;

#define RECT_FLAGS_INIT { \
    0, 0, 0, 0,     0, 0, \
    0, 0, 0, 0,     0, 0, \
    0,              0, 0, \
    0,              0, 0, \
    0, 0, 0, 0            } /* this aggregate initialiser gives poor code on Norcroft */

/* better to do by hand on Norcroft */
#if 1
#define RECT_FLAGS_CLEAR(flags) \
    zero_32(flags)
#else
#define RECT_FLAGS_CLEAR(flags) \
    zero_struct(flags)
#endif

/*
*                                                text  selection printing
* normal screen redraw (e.g. after update_later) TRUE  TRUE      FALSE
* printing                                       TRUE  FALSE     TRUE
*
* dragging a selection (i.e. during update_now)  FALSE TRUE      FALSE
*
*/

typedef struct REDRAW_FLAGS
{
    UBF show_content   : 1;
    UBF show_selection : 1;
    UBF update_now     : 1;
    UBF reserved       : sizeof(int)*8 - 3*1;
}
REDRAW_FLAGS;

#define REDRAW_FLAGS_INIT \
{ 0, 0, 0, 0 } /* this aggregate initialiser gives poor code on Norcroft */

/* better to do by hand on Norcroft */
#if 1
#define REDRAW_FLAGS_CLEAR(flags) \
    zero_32(flags)
#else
#define REDRAW_FLAGS_CLEAR(flags) \
    zero_struct(flags)
#endif

typedef struct EDGE_FLAGS
{
    UBF show     : 1;
    UBF dashed   : 1;
    UBF reserved : sizeof(int)*8 - 2*1;
}
EDGE_FLAGS, * P_EDGE_FLAGS;

typedef struct BORDER_FLAGS
{
    EDGE_FLAGS left;
    EDGE_FLAGS top;
    EDGE_FLAGS right;
    EDGE_FLAGS bottom;
}
BORDER_FLAGS, * P_BORDER_FLAGS; typedef const BORDER_FLAGS * PC_BORDER_FLAGS;

typedef S32 OS_UNIT;

typedef struct HOST_XFORM
{
    struct HOST_XFORM_SCALE
    {
        PIXIT_POINT t, b;
    } scale;
    BOOL do_x_scale, do_y_scale;
#if RISCOS
    struct HOST_XFORM_RISCOS
    {
        U32 d_x; /* x-pixel size in OS units */
        U32 d_y; /* y-pixel size in OS units */
        U32 eig_x; /* x-EIG shift */
        U32 eig_y; /* y-EIG shift */
    } riscos;
#elif WINDOWS
    struct HOST_XFORM_WINDOWS
    {
        POINT pixels_per_inch; /* pixels per inch */
        PIXIT_POINT d; /* WINDOWS: pixels per metre */
        PIXIT_POINT multiplier_of_pixels; /* pixit_point.x = point.x * multiplier_of_pixels.x / divisor_of_pixels.x using precision */
        PIXIT_POINT divisor_of_pixels;
    } windows;
#endif
}
HOST_XFORM, * P_HOST_XFORM; typedef const HOST_XFORM * PC_HOST_XFORM;

typedef struct CLICK_CONTEXT
{
    P_VIEW p_view;

    GDI_POINT gdi_org; /* offset to tl of window in screen units (0 when printing - hope we don't get clicks!) */
    HOST_XFORM host_xform;      /*>>>should this be hidden from all except host???*/
    PIXIT_POINT pixit_origin;
    PIXIT_POINT one_real_pixel; /* PIXITS per real screen pixel */
    PIXIT_POINT one_program_pixel; /* PIXITS per program pixel (e.g. for note ears) */
    PIXIT_POINT border_width;
    S32 display_mode;

    HOST_WND hwnd;
    BOOL ctrl_pressed;
    BOOL shift_pressed;
}
CLICK_CONTEXT, * P_CLICK_CONTEXT; typedef const CLICK_CONTEXT * PC_CLICK_CONTEXT;

typedef struct REDRAW_CONTEXT_CACHE
{
#if WINDOWS
    HPALETTE init_h_palette;

    HFONT init_h_font;
    HFONT h_font;

    HBRUSH init_h_brush;
    HBRUSH h_brush;
    RGB current_brush_rgb;

    HPEN init_h_pen;
    HPEN h_pen;
    RGB current_pen_rgb;
    int current_pen_width;

    BOOL h_font_delete_after;
#else
    void * foo;
#endif
}
REDRAW_CONTEXT_CACHE, * P_REDRAW_CONTEXT_CACHE;

typedef struct REDRAW_CONTEXT_FLAGS
{
    U8 printer;
    U8 metafile;
    U8 drawfile;
    U8 _spare;
}
REDRAW_CONTEXT_FLAGS;

#if RISCOS
typedef struct REDRAW_CONTEXT_RISCOS
{
    HOST_WND hwnd;
    GDI_BOX host_machine_clip_box;
}
REDRAW_CONTEXT_RISCOS;
#elif WINDOWS
typedef struct REDRAW_CONTEXT_WINDOWS
{
    PAINTSTRUCT paintstruct;
    RECT host_machine_clip_rect;
}
REDRAW_CONTEXT_WINDOWS;
#endif /* OS */

typedef struct REDRAW_CONTEXT
{
    P_DOCU p_docu;
    P_VIEW p_view;

    GDI_POINT gdi_org; /* offset to tl of window in screen units (0 when printing) */
    HOST_XFORM host_xform;
    PIXIT_POINT pixit_origin;
    PIXIT_POINT one_real_pixel;
    PIXIT_POINT one_program_pixel; /* PIXITS per program pixel (e.g. for note ears) */
    PIXIT_POINT border_width;
    S32 display_mode;

    PIXIT_POINT line_width;
    PIXIT_POINT line_width_eff;
    PIXIT_POINT thin_width;
    PIXIT_POINT thin_width_eff;
    REDRAW_CONTEXT_FLAGS flags;

    GDI_SIZE pixels_per_inch;
    PIXIT_POINT border_width_2;

    P_GR_RISCDIAG p_gr_riscdiag; /* SKS 15apr01 GR_DIAG -> SKS 09.05.2006 GR_RISCDIAG*/
    P_GR_RISCDIAG lookup_gr_riscdiag; /* ditto */
    PIXIT_POINT page_pixit_origin_draw;

#if RISCOS
    REDRAW_CONTEXT_RISCOS riscos;
#elif WINDOWS
    REDRAW_CONTEXT_WINDOWS windows;
#endif

    P_REDRAW_CONTEXT_CACHE p_redraw_context_cache;
}
REDRAW_CONTEXT, * P_REDRAW_CONTEXT; typedef const REDRAW_CONTEXT * PC_REDRAW_CONTEXT;

#define   P_REDRAW_CONTEXT_NONE                 _P_DATA_NONE(P_REDRAW_CONTEXT)
#define IS_REDRAW_CONTEXT_NONE(p_redraw_context) IS_PTR_NONE(p_redraw_context)

typedef enum RULER_MARKER
{
    RULER_NO_MARK_VALID_AREA = -2,
    RULER_NO_MARK = -1,

    RULER_MARKER_MARGIN_LEFT = 0, /* markers shown in horzizontal ruler */
    RULER_MARKER_MARGIN_PARA,
    RULER_MARKER_MARGIN_RIGHT,
    RULER_MARKER_COL_RIGHT,
    RULER_MARKER_TAB_LEFT,
    RULER_MARKER_TAB_CENTRE,
    RULER_MARKER_TAB_RIGHT,
    RULER_MARKER_TAB_DECIMAL,

    RULER_MARKER_HEADER_EXT, /* markers shown in vertical ruler */
    RULER_MARKER_HEADER_OFF,
    RULER_MARKER_FOOTER_EXT,
    RULER_MARKER_FOOTER_OFF,
    RULER_MARKER_ROW_BOTTOM,

    BORDER_MARKER_COL_SEPARATOR,
    BORDER_MARKER_ROW_SEPARATOR,

    RULER_MARKER_COUNT
}
RULER_MARKER, * P_RULER_MARKER;

_Check_return_
extern RECT_FLAGS
host_marker_rect_flags(
    _InVal_     RULER_MARKER ruler_marker);

_Check_return_
extern BOOL
host_over_marker(
    _InRef_     PC_CLICK_CONTEXT p_click_context,
    _InVal_     RULER_MARKER ruler_marker,
    _InRef_     PC_PIXIT_POINT p_marker_pos,
    _InRef_     PC_PIXIT_POINT p_pointer_pos);

extern void
host_paint_marker(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     RULER_MARKER ruler_marker,
    _InRef_     PC_PIXIT_POINT p_point);

/* pointer shapes - if renumbered or added to, pointer_table in ho_paint.c needs amending */

typedef enum POINTER_SHAPE
{
    POINTER_UNINIT = -1,

    POINTER_DEFAULT = 0,
    POINTER_CARET = 1,              /* resembles a caret */
    POINTER_MENU = 2,               /* an arrow pointing to a menu */

    POINTER_FORMULA_LINE = 3,

    POINTER_MOVE = 4,               /* a hand */

    POINTER_DRAG_ALL = 5,           /* arrows pointing in all directions */
    POINTER_DRAG_LEFTRIGHT = 6,     /* arrows pointing left and right */
    POINTER_DRAG_UPDOWN = 7,        /* arrows pointing up and down */

    POINTER_DRAG_MARGIN = POINTER_DRAG_LEFTRIGHT,
    POINTER_DRAG_TAB = POINTER_DRAG_LEFTRIGHT,
    POINTER_DRAG_HEFO = POINTER_DRAG_UPDOWN,

    POINTER_DRAG_COLUMN = 8,        /* symbol with arrows pointing left and right */
    POINTER_DRAG_COLUMN_LEFT = 9,   /* symbol with arrow pointing left */
    POINTER_DRAG_COLUMN_RIGHT = 10, /* symbol with arrow pointing right */

    POINTER_DRAG_ROW = 11,          /* symbol with arrows pointing up and down */
    POINTER_DRAG_ROW_UP = 12,       /* symbol with arrow pointing up */
    POINTER_DRAG_ROW_DOWN = 13,     /* symbol with arrow pointing down */

#if WINDOWS
    POINTER_SPLIT_ALL = 14,
    POINTER_SPLIT_LEFTRIGHT = 15,
    POINTER_SPLIT_UPDOWN = 16,
#endif

    POINTER_SHAPE_COUNT
}
POINTER_SHAPE, * P_POINTER_SHAPE;

#define FRAMED_BOX_NONE 0
#define FRAMED_BOX_PLAIN 1
#define FRAMED_BOX_BUTTON_OUT 2
#define FRAMED_BOX_BUTTON_IN 3
#define FRAMED_BOX_DEFBUTTON_OUT 4
#define FRAMED_BOX_DEFBUTTON_IN 5
#define FRAMED_BOX_CHANNEL_OUT 6
#define FRAMED_BOX_CHANNEL_IN 7
#define FRAMED_BOX_EDIT 8
#define FRAMED_BOX_PLINTH 9
#define FRAMED_BOX_TROUGH 10
#define FRAMED_BOX_TROUGH_LBOX 11
#define FRAMED_BOX_W31_BUTTON_OUT 12
#define FRAMED_BOX_W31_BUTTON_IN 13

#define FRAMED_BOX_GROUP FRAMED_BOX_CHANNEL_OUT

#define FRAMED_BOX_DISABLED 0x80

#define FRAMED_BOX_STYLE int

typedef struct VIEWEVENT_REDRAW
{
    P_VIEW              p_view;
    P_PANE              p_pane;
    VIEW_RECT           area;
    REDRAW_FLAGS        flags;
    REDRAW_CONTEXT      redraw_context;
}
VIEWEVENT_REDRAW, * P_VIEWEVENT_REDRAW;

extern void
filer_opendir(
    _In_z_      PCTSTR filename);

extern void
filer_launch(
    _In_z_      PCTSTR filename);

extern void
host_longjmp_to_event_loop(void);

extern void
host_bleep(void);

extern void
host_read_default_paper_details(
    P_PAPER p_paper);

_Check_return_
extern STATUS
host_read_printer_paper_details(
    P_PAPER p_paper);

extern void
host_recache_visible_row_range(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
host_settitle(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_z_      PCTSTR title);

extern void
host_view_destroy(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

_Check_return_
extern STATUS
host_view_init(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_PIXIT_POINT p_tl,
    _InRef_     PC_PIXIT_SIZE p_size);

extern void
host_view_show(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
host_view_reopen(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
host_view_minimize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
host_view_maximize(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
host_view_query_posn(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _OutRef_    P_PIXIT_POINT p_tl,
    _OutRef_    P_PIXIT_SIZE p_size);

extern void
host_set_pointer_shape(
    _In_        POINTER_SHAPE pointer_shape);

_Check_return_
extern BOOL
host_drag_in_progress(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_ANY p_p_reason_data);

extern void
host_drag_start(
    _In_opt_    P_ANY p_reason_data);

extern void
host_paint_end(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

extern void
host_paint_start(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

typedef struct BORDER_LINE_FLAGS
{
    UBF border_style  : 8;
    UBF add_gw_to_l   : 1;
    UBF add_gw_to_t   : 1;
    UBF add_gw_to_r   : 1;
    UBF add_gw_to_b   : 1;
    UBF sub_gw_from_l : 1;
    UBF sub_gw_from_t : 1;
    UBF sub_gw_from_r : 1;
    UBF sub_gw_from_b : 1;
    UBF add_lw_to_l   : 1;
    UBF add_lw_to_t   : 1;
    UBF add_lw_to_r   : 1;
    UBF add_lw_to_b   : 1;
    UBF reserved      : sizeof(U32)*8 - 12*1 - 8;
}
BORDER_LINE_FLAGS; /* 20 bits so far */

#define CLEAR_BORDER_LINE_FLAGS(flags) \
    zero_32(flags)

extern void
host_paint_border_line(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _In_        BORDER_LINE_FLAGS flags);

extern void
host_paint_underline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_pixit_line,
    _InRef_     PC_RGB p_rgb,
    _InVal_     PIXIT line_thickness);

#if WINDOWS

extern void
host_paint_bitmap(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_        HBITMAP hbitmap);

extern void
host_paint_gdip_image(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_        GdipImage gdip_image);

#endif /* OS */

extern void
host_paint_drawfile(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GR_SCALE_PAIR p_gr_scale_pair,
    _InRef_     PC_DRAW_DIAG p_draw_diag,
    _InVal_     BOOL eor_paths);

#if RISCOS

extern void
host_fonty_text_paint_uchars_rubout(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_point,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background,
    _HfontRef_opt_ HOST_FONT host_font,
    _InRef_opt_ PC_PIXIT_RECT p_pixit_rect_rubout);

#endif /* OS */

#ifndef TA_LEFT
#define TA_LEFT                      0
#define TA_RIGHT                     2
#define TA_CENTER                    6
#endif

extern void
host_fonty_text_paint_uchars_simple(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_point,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background,
#if RISCOS
    _HfontRef_opt_ HOST_FONT host_font,
#elif WINDOWS
    _HfontRef_  HOST_FONT host_font,
#endif
    _InVal_     int text_align_lcr);

extern void
host_paint_plain_text_counted(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb);

extern void
host_fonty_text_paint_uchars_in_rectangle(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_BORDER_FLAGS p_border_flags,
    _InRef_     PC_RGB p_rgb_fill,
    _InRef_     PC_RGB p_rgb_line,
    _InRef_     PC_RGB p_rgb_text,
#if RISCOS
    _HfontRef_opt_ HOST_FONT host_font);
#elif WINDOWS
    _HfontRef_  HOST_FONT host_font);
#endif

extern void
host_paint_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect,
    _InRef_     PC_RGB p_rgb);

extern void
host_paint_rectangle_outline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect,
    _InRef_     PC_RGB p_rgb);

extern void
host_paint_rectangle_crossed(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect,
    _InRef_     PC_RGB p_rgb);

extern void
host_paint_line_dashed(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_line,
    _InRef_     PC_RGB p_rgb);

extern void
host_paint_line_solid(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_LINE p_line,
    _InRef_     PC_RGB p_rgb);

extern void
host_restore_clip_rectangle(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

extern BOOL
host_set_clip_rectangle(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InVal_     RECT_FLAGS rect_flags);

extern BOOL
host_set_clip_rectangle2(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect_paper,
    _InRef_     PC_PIXIT_RECT p_rect_object,
    _InVal_     RECT_FLAGS rect_flags_object);

extern void
host_invert_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background);

extern void
host_invert_rectangle_outline(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect,
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background);

extern void
host_read_drawfile_pixit_size(
    /*_In_reads_bytes_(DRAW_FILE_HEADER)*/ PC_ANY p_any,
    _OutRef_    P_PIXIT_SIZE p_pixit_size);

extern void
host_update(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect,
    _InRef_opt_ PC_RECT_FLAGS p_rect_flags,
    _InVal_     REDRAW_FLAGS redraw_flags,
    _InVal_     REDRAW_TAG redraw_tag);

extern void
host_update_all(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag);

_Check_return_
extern BOOL
host_update_fast_start(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_VIEWEVENT_REDRAW p_event_info,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_VIEW_RECT p_view_rect,
    RECT_FLAGS rect_flags);

_Check_return_
extern BOOL
host_update_fast_continue(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_VIEWEVENT_REDRAW p_event_info);

extern void
host_all_update_later(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_RECT p_view_rect);

extern void
host_main_claim_focus(
    _DocuRef_   P_DOCU p_docu,
    P_VIEW_POINT p_view_point,
    _InVal_     PIXIT height);

extern void
host_main_hide_caret(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
host_main_show_caret(
    _DocuRef_   P_DOCU p_docu,
    P_VIEW_POINT p_view_point,
    _InVal_     PIXIT height);

extern void
host_main_scroll_caret(
    _DocuRef_   P_DOCU p_docu,
    P_VIEW_POINT p_view_point,
    _InVal_     PIXIT left,
    _InVal_     PIXIT right,
    _InVal_     PIXIT top,
    _InVal_     PIXIT bottom);

extern void
host_scroll_pane(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     T5_MESSAGE t5_message,
    P_VIEW_POINT p_view_point);

extern void
host_set_extent_this_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
set_host_xform_for_view(
    _OutRef_    P_HOST_XFORM p_host_xform,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL do_x_scale,
    _InVal_     BOOL do_y_scale);

extern void
set_host_xform_for_view_common( /* only use in set_host_xform_for_view() */
    _OutRef_    P_HOST_XFORM p_host_xform,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL do_x_scale,
    _InVal_     BOOL do_y_scale);

#if RISCOS
extern void
frame_windows_set_margins(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);
#endif

#define HIC_PURGE 0x01
#define HIC_BG    0x02
#define HIC_FG    0x04
#define HIC_FONT  0x08

#define HIC_PLOTICON          (HIC_BG | HIC_FG | HIC_FONT) /* HIC_FONT needed if RISC OS plot text icon goes fonty */
#define HIC_REDRAW_LOOP_START (HIC_BG | HIC_FG | HIC_FONT)

extern void
host_invalidate_cache(
    _InVal_     S32 bits);

extern void
host_recache_mode_dependent_vars(void);

extern void
host_fixup_system_sprites(void);

extern void
host_framed_box_paint_frame(
    _InRef_     PC_GDI_BOX p_box_abs,
    _In_        FRAMED_BOX_STYLE border_style);

extern void
host_framed_box_paint_core(
    _InRef_     PC_GDI_BOX p_box_abs,
    _InRef_     PC_RGB p_rgb);

extern void
host_framed_box_trim_frame(
    _InoutRef_  P_GDI_BOX p_box,
    _In_        FRAMED_BOX_STYLE border_style);

extern void
host_fonty_text_paint_uchars_in_framed_box(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_rect,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _In_        FRAMED_BOX_STYLE border_style,
    _InVal_     S32 fill_wimpcolour,
    _InVal_     S32 text_wimpcolour,
#if RISCOS
    _HfontRef_opt_ HOST_FONT host_font);
#elif WINDOWS
    _HfontRef_  HOST_FONT host_font);
#endif

_Check_return_
extern BOOL
host_xfer_loaded_file_is_safe(void);

_Check_return_
extern BOOL
host_xfer_saved_file_is_safe(void);

extern void
host_xfer_set_saved_file_is_safe(
    _InVal_     BOOL value);

extern void
host_reenter_window(void);

extern void
host_work_area_gdi_size_query(
    _OutRef_    P_GDI_SIZE p_gdi_size);

#if RISCOS

_Check_return_
extern BOOL
host_setbgcolour(
    _InRef_     PC_RGB p_rgb);

_Check_return_
extern BOOL
host_setfgcolour(
    _InRef_     PC_RGB p_rgb);

#define HOST_FONT_KERNING 0x00000001

_Check_return_
extern U32
host_version_font_m_read(
    _InVal_     U32 flags);

extern void
host_version_font_m_reset(void);

_Check_return_
extern STATUS
host_setfontcolours_for_mlec(
    _InRef_     PC_RGB p_rgb_foreground,
    _InRef_     PC_RGB p_rgb_background);

extern BOOL
riscos_colour_picker(
    HOST_WND    parent_window_handle,
    _InoutRef_  P_RGB p_rgb);

#endif /* RISCOS */

extern void
host_palette_cache_reset(void);

#if WINDOWS

extern BOOL
windows_colour_picker(
    HOST_WND    parent_window_handle,
    _InoutRef_  P_RGB p_rgb);

extern void
host_rgb_stash(
    _HdcRef_    HDC hDC);

extern void
host_rgb_stash_colour(
    _HdcRef_    HDC hDC,
    _In_        UINT index);

#else

extern void
host_rgb_stash(void);

#endif /* OS */

/* indexes into rgb_stash */

#define COLOUR_OF_BUTTON_IN         2
#define COLOUR_OF_BUTTON_OUT        1
#define COLOUR_OF_DESKTOP           3
#define COLOUR_OF_HEFO_LINE         1
#define COLOUR_OF_MARGIN            1
#define COLOUR_OF_PAPER             0
#define COLOUR_OF_BORDER            COLOUR_OF_BUTTON_OUT
#define COLOUR_OF_BORDER_CURRENT    COLOUR_OF_BUTTON_IN
#define COLOUR_OF_BORDER_FRAME      4
#if WINDOWS
#define COLOUR_OF_RULER             2
#else
#define COLOUR_OF_RULER             1
#endif
#define COLOUR_OF_RULER_SCALE       5
#define COLOUR_OF_TEXT              7

#define COLOUR_STASH_MAX            16

/*
Exported variables
*/

extern RGB
rgb_stash[COLOUR_STASH_MAX];

/*
ho_xform.c
*/

extern void
gdi_point_from_pixit_point_and_context(
    _OutRef_    P_GDI_POINT p_gdi_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

_Check_return_
extern STATUS
gdi_rect_from_pixit_rect_and_context(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

_Check_return_
extern STATUS
gdi_rect_limited_from_pixit_rect_and_context(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

#if WINDOWS

_Check_return_
extern STATUS
RECT_limited_from_pixit_rect_and_context(
    _OutRef_    PRECT p_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

#endif /* OS */

_Check_return_
extern STATUS
gdi_rect_from_pixit_rect_and_context_draw(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

extern void
host_redraw_context_set_host_xform(
    _InoutRef_  P_REDRAW_CONTEXT p_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/);

extern void
host_redraw_context_fillin(
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context);

extern void
host_set_click_context(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_CLICK_CONTEXT p_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/);

extern void
pixit_point_from_window_point(
    _OutRef_    P_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GDI_POINT p_gdi_point,
    _InRef_     PC_HOST_XFORM p_host_xform);

extern void
pixit_rect_from_screen_rect_and_context(
    _OutRef_    P_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_GDI_RECT p_gdi_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

extern void
pixit_rect_from_window_rect(
    _OutRef_    P_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_GDI_RECT p_gdi_rect,
    _InRef_     PC_HOST_XFORM p_host_xform);

extern void
window_point_from_pixit_point(
    _OutRef_    P_GDI_POINT p_gdi_point,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_HOST_XFORM p_host_xform);

_Check_return_
extern STATUS
window_rect_from_pixit_rect(
    _OutRef_    P_GDI_RECT p_gdi_rect,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_HOST_XFORM p_host_xform);

_Check_return_
extern PIXIT
scale_pixit_x(
    _InVal_     PIXIT pixit_x,
    _InRef_     PC_HOST_XFORM p_host_xform);

_Check_return_
extern PIXIT
scale_pixit_y(
    _InVal_     PIXIT pixit_y,
    _InRef_     PC_HOST_XFORM p_host_xform);

#if RISCOS

_Check_return_
extern GDI_COORD
os_unit_from_pixit_x(
    _InVal_     PIXIT pixit_x,
    _InRef_     PC_HOST_XFORM p_host_xform);

_Check_return_
extern GDI_COORD
os_unit_from_pixit_y(
    _InVal_     PIXIT pixit_y,
    _InRef_     PC_HOST_XFORM p_host_xform);

extern void
host_paint_bitmap(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    /*_In_*/    PC_ANY p_bmp,
    _InVal_     BOOL stretch);

extern void
host_paint_sprite(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InoutRef_  P_SCB p_scb,
    _InVal_     BOOL stretch);

extern void
host_paint_sprite_abs(
    _InRef_     PC_GDI_BOX p_gr_box,
    _InoutRef_  P_SCB p_scb,
    _InVal_     BOOL stretch);

extern void
host_clg(void);

#endif /* RISCOS */

extern void
host_cache_view_info(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

#if WINDOWS

extern HBITMAP
gdiplus_load_bitmap_from_file(
    _In_z_      PCTSTR filename);

#define TYPE5_PROPERTY_WORD TEXT("TYPE5WORD")

enum EVENT_HANDLER_IDS
{
    EVENT_HANDLER_PANE_SPLIT_DIAG = 0,
    EVENT_HANDLER_PANE_SPLIT_VERT,
    EVENT_HANDLER_PANE_SPLIT_HORZ,
    EVENT_HANDLER_PANE,

    EVENT_HANDLER_BORDER_HORZ_SPLIT,
    EVENT_HANDLER_BORDER_HORZ,
    EVENT_HANDLER_BORDER_VERT_SPLIT,
    EVENT_HANDLER_BORDER_VERT,

    EVENT_HANDLER_RULER_HORZ_SPLIT,
    EVENT_HANDLER_RULER_HORZ,
    EVENT_HANDLER_RULER_VERT_SPLIT,
    EVENT_HANDLER_RULER_VERT,

    EVENT_HANDLER_HSCROLL_SPLIT,
    EVENT_HANDLER_HSCROLL,
    EVENT_HANDLER_VSCROLL_SPLIT,
    EVENT_HANDLER_VSCROLL,

    EVENT_HANDLER_BACK_WINDOW,
    EVENT_HANDLER_TOOLBAR,
    EVENT_HANDLER_SLE,
    EVENT_HANDLER_STATUS_LINE,

    NUMEVENTS
};

typedef U16 EVENT_HANDLER;

_Check_return_
extern DOCNO
resolve_hwnd(
    _HwndRef_   HWND hwnd,
    _OutRef_    P_P_VIEW p_p_view,
    _Out_       EVENT_HANDLER * const p_event_handler);

extern void
host_dithering_set(
    _InVal_     BOOL dither);

extern void
host_get_pixel_size(
    _HwndRef_opt_ HWND hwnd /* NULL for screen */,
    _OutRef_    PSIZE p_size);

/* Windows specific code for handling the instance information */

extern void
host_clear_tracking_for_window(
    _HwndRef_   HWND hwnd);

_Check_return_
extern BOOL
host_set_tracking_for_window(
    _HwndRef_   HWND hwnd);

#if defined(USE_GLOBAL_CLIPBOARD)

_Check_return_
extern BOOL
host_acquire_global_clipboard(
    _DocuRef_   PC_DOCU p_docu,
    _ViewRef_   PC_VIEW p_view);

_Check_return_
extern BOOL
host_open_global_clipboard(
    _DocuRef_   PC_DOCU p_docu,
    _ViewRef_   PC_VIEW p_view);

extern void
host_release_global_clipboard(
    _InVal_     BOOL render_if_acquired);

#endif /* USE_GLOBAL_CLIPBOARD */

_Check_return_
extern HDC /* actually an IC */
host_get_hic_format_pixits(void);

extern void
host_create_default_palette(void);

extern void
host_destroy_default_palette(void);

extern void
host_select_default_palette(
    _HdcRef_    HDC hdc,
    _Out_       HPALETTE * const p_h_palette);

extern void
host_select_old_palette(
    _HdcRef_    HDC hdc,
    _Inout_     HPALETTE * const p_h_palette);

extern void
ho_paint_SetCursor(void);

#if USTR_IS_SBSTR

/* uchars STATUS returning functions */

_Check_return_
extern STATUS
uchars_ExtTextOut(
    _HdcRef_    HDC hdc,
    _In_        int x,
    _In_        int y,
    _In_        UINT options,
    _In_opt_    CONST RECT *pRect,
    _In_reads_opt_(uchars_n) PC_UCHARS pString,
    _InVal_     U32 uchars_n,
    _In_opt_    CONST INT *pDx);

_Check_return_
extern STATUS
uchars_GetTextExtentPoint32(
    _HdcRef_    HDC hdc,
    _In_reads_(uchars_n) PC_UCHARS pString,
    _InVal_     U32 uchars_n,
    _OutRef_    PSIZE pSize);

#endif /* USTR_IS_SBSTR */

#endif /* WINDOWS */

extern void
host_modify_click_point(
    _InoutRef_  P_GDI_POINT p_gdi_point);

extern void
ho_paint_msg_exit2(void);

/*ncr*/
extern BOOL
host_show_caret(
    _HwndRef_   HOST_WND hwnd,
    _In_        int width,
    _In_        int height,
    _In_        int x,
    _In_        int y);

#if RISCOS

extern void
reply_to_help_request(
    _DocuRef_   P_DOCU p_docu,
    PC_ANY p_any_wimp_message /*HelpRequest*/);

_Check_return_
_Ret_z_
extern PCTSTR
string_for_object(
    _In_z_      PCTSTR tag_and_default,
    _InVal_     OBJECT_ID object_id);

/*ncr*/
_Ret_z_
extern PTSTR
make_var_name(
    _Out_cap_(elemof_buffer) PTSTR buffer,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR suffix);

#endif

typedef enum WM_EVENT
{
    WM_EVENT_FG_NULL    = 0,
    WM_EVENT_EXIT       = 1,
    WM_EVENT_PROCESSED  = 2
}
WM_EVENT;

_Check_return_
extern WM_EVENT
wm_event_get(
    _InVal_     BOOL fgNullEventsWanted);

/*
*** Null event processing ***
*/

/*
structure
*/

#define NULL_QUERY 1
#define NULL_EVENT 2

#define NULL_EVENT_REASON_CODE int

#define NULL_EVENT_UNKNOWN    0x00

/* from null_events_do_query() */
#define NULL_EVENTS_OFF       0x10
#define NULL_EVENTS_REQUIRED  0x11

/* from null_events_do_events() */
#define NULL_EVENT_COMPLETED  0x20
#define NULL_EVENT_TIMED_OUT  0x21

#define NULL_EVENT_RETURN_CODE int

typedef struct NULL_EVENT_BLOCK
{
    NULL_EVENT_RETURN_CODE rc;  /*OUT*/

    CLIENT_HANDLE   client_handle;  /*IN*/
    MONOTIME        initial_time;   /*IN*/
    MONOTIMEDIFF    max_slice;      /*IN*/
}
NULL_EVENT_BLOCK, * P_NULL_EVENT_BLOCK;

/*ncr*/
extern NULL_EVENT_RETURN_CODE
null_events_do_events(void);

_Check_return_
extern NULL_EVENT_RETURN_CODE
null_events_do_query(void);

#define SCHEDULED_EVENT_REASON_CODE int

#define SCHEDULED_EVENT_UNKNOWN    0x00

/* from scheduled_events_do_query() */
#define SCHEDULED_EVENTS_OFF       0x10
#define SCHEDULED_EVENTS_REQUIRED  0x11

/* from scheduled_events_do_events() */
#define SCHEDULED_EVENT_COMPLETED  0x20
#define SCHEDULED_EVENT_TIMED_OUT  0x21

#define SCHEDULED_EVENT_RETURN_CODE int

typedef struct SCHEDULED_EVENT_BLOCK
{
    SCHEDULED_EVENT_RETURN_CODE rc; /*OUT*/

    CLIENT_HANDLE client_handle; /*IN*/
}
SCHEDULED_EVENT_BLOCK, * P_SCHEDULED_EVENT_BLOCK;

/*ncr*/
extern SCHEDULED_EVENT_RETURN_CODE
scheduled_events_do_events(void);

_Check_return_
extern SCHEDULED_EVENT_RETURN_CODE
scheduled_events_do_query(void);

#if WINDOWS

extern void
host_font_delete(
    _HfontRef_  HOST_FONT host_font,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context);

extern HPALETTE g_hPalette;

_Check_return_
extern BOOL
some_document_windows(void);

#endif /* OS */

extern void
host_shutdown(void);

_Check_return_
extern BOOL
host_shift_pressed(void);

_Check_return_
extern BOOL
host_ctrl_pressed(void);

/*
object dragging structures
*/

/*
Reason codes and data registered with host_drag_start
*/

typedef enum DRAG_REASONCODE
{
    CB_CODE_BORDER_HORZ_ADJUST_COLUMN_SPLIT = 1,
    CB_CODE_BORDER_HORZ_ADJUST_COLUMN_WIDTH = 2,
    CB_CODE_BORDER_HORZ_EXTEND_SELECTION    = 3,

    CB_CODE_BORDER_VERT_ADJUST_ROW_HEIGHT = 10,
    CB_CODE_BORDER_VERT_EXTEND_SELECTION  = 11, /* dragging in border to extend selection in pane */

    CB_CODE_RULER_HORZ_ADJUST_MARKER_POSITION = 20,

    CB_CODE_RULER_VERT_ADJUST_MARKER_POSITION = 25,

    CB_CODE_NOTELAYER_NOTE_TRANSLATE = 30,
    CB_CODE_NOTELAYER_NOTE_RESCALE = 31,
    CB_CODE_NOTELAYER_NOTE_RESIZE = 32,
    CB_CODE_NOTELAYER_NOTE_TRANSLATE_FOR_CLIENT = 33,
    CB_CODE_NOTELAYER_NOTE_RESIZE_FOR_CLIENT = 34,

    CB_CODE_NOREASON = 1000,

    CB_CODE_PASS_TO_OBJECT = 4096 /* add in your OBJECT_ID */
}
DRAG_REASONCODE, * P_DRAG_REASONCODE;

#define minmax_limited(value, minlimit, maxlimit) \
    MIN(MAX((value),(minlimit)), (maxlimit))

/*
exported from sk_bord.c
*/

typedef enum SNAP_TO_CLICK_STOP_MODE
{
    SNAP_TO_CLICK_STOP_ROUND = 0,
    SNAP_TO_CLICK_STOP_FLOOR,
    SNAP_TO_CLICK_STOP_CEIL,
    SNAP_TO_CLICK_STOP_ROUND_COARSE
}
SNAP_TO_CLICK_STOP_MODE;

_Check_return_
extern PIXIT
skel_ruler_snap_to_click_stop(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     BOOL horizontal,
    _InVal_     PIXIT pixit_in,
    _InVal_     SNAP_TO_CLICK_STOP_MODE mode);

/*
fonty.c
*/

/* bits used for font difference bitmaps */

enum FONT_SPEC_BITS
{
    FONTY_NAME = 0,
    FONTY_SIZE_X,
    FONTY_SIZE_Y,

    FONTY_UNDERLINE,
    FONTY_BOLD,
    FONTY_ITALIC,
    FONTY_SUPERSCRIPT,
    FONTY_SUBSCRIPT,

    FONTY_COLOUR,

    FONTY_BIT_COUNT
};

/*
font context
*/

typedef struct FONT_CONTEXT_FLAGS
{
    UBF deleted : 1;                        /* font context entry may be reused */

    UBF host_font_formatting_set : 1;       /* font handle with no scaling for formatting is valid */
    UBF host_font_redraw_set : 1;           /* font handle for scaled font for redraw is valid */

#if RISCOS
    UBF host_font_utf8_formatting_set : 1;  /* UTF-8 font handle with no scaling for formatting is valid */
    UBF host_font_utf8_redraw_set : 1;      /* UTF-8 font handle for scaled font for redraw is valid */
#endif
}
FONT_CONTEXT_FLAGS;

#if WINDOWS && !APP_UNICODE && 1
#define USE_CACHED_ABC_WIDTHS 1
#endif

typedef struct FONT_CONTEXT
{
    FONT_SPEC font_spec;
    PIXIT space_width;              /* width of a single space */
    PIXIT ascent;                   /* unsigned ascent */

    FONT_CONTEXT_FLAGS flags;

    HOST_FONT host_font_formatting; /* host font handle with no scaling for formatting */
    HOST_FONT host_font_redraw;     /* host font handle for scaled font for redraw */

#if RISCOS
    HOST_FONT host_font_utf8_formatting; /* UTF-8 host font handle with no scaling for formatting */
    HOST_FONT host_font_utf8_redraw; /* UTF-8 host font handle for scaled font for redraw */
#endif

#if WINDOWS && defined(USE_CACHED_ABC_WIDTHS)
    ARRAY_HANDLE h_abc_widths;      /* handle to ABC width table */
#endif
}
FONT_CONTEXT, * P_FONT_CONTEXT; typedef const FONT_CONTEXT * PC_FONT_CONTEXT;

typedef ARRAY_INDEX FONTY_HANDLE; typedef FONTY_HANDLE * P_FONTY_HANDLE;

extern ARRAY_HANDLE h_font_cache;   /* indexed by FONTY_HANDLE, holds 'session' cache of host font handles - don't GC as handles persist in text cache */

#define p_font_context_from_fonty_handle(fonty_handle) \
    array_ptrc(&h_font_cache, FONT_CONTEXT, (fonty_handle))

typedef struct FONTY_CHUNK
{
    FONTY_HANDLE fonty_handle;  /* internal font handle for chunk */
    PIXIT width;                /* width of chunk (inc spaces) */
    PIXIT trail_space;          /* width of trailing spaces */
    PIXIT ascent;               /* ascent of this chunk */
#if RISCOS
    S32 width_mp;               /* width in millipoints */
#endif
    U8 underline;               /* this bit underlined ? */
    U8 _spare[3];
}
FONTY_CHUNK, * P_FONTY_CHUNK; typedef const FONTY_CHUNK * PC_FONTY_CHUNK;

/*
exported routines
*/

#define fonty_base_line(leading, font_size_y, ascent) ( \
    ascent )

_Check_return_
extern HOST_FONT
fonty_host_font_from_fonty_handle_formatting(
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode);

_Check_return_
extern HOST_FONT
fonty_host_font_from_fonty_handle_redraw(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode);

#if RISCOS

_Check_return_
extern HOST_FONT
fonty_host_font_utf8_from_fonty_handle_formatting(
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode);

_Check_return_
extern HOST_FONT
fonty_host_font_utf8_from_fonty_handle_redraw(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InVal_     BOOL draft_mode);

#endif /* RISCOS */

_Check_return_
extern STATUS
font_spec_duplicate(
    _OutRef_    P_FONT_SPEC p_font_spec_out,
    _InRef_     PC_FONT_SPEC p_font_spec_in);

extern void
fonty_cache_trash(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

_Check_return_
extern S32
fonty_chunk_index_from_pixit(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_FONTY_CHUNK p_fonty_chunk,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     S32 uchars_n,
    _InoutRef_  P_PIXIT p_pos);

extern void
fonty_chunk_info_read_uchars(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle,
    _In_reads_(uchars_len_no_spaces) PC_UCHARS uchars,
    _InVal_     S32 uchars_len_no_spaces,
    _InVal_     S32 trail_spaces);

#if RISCOS && USTR_IS_SBSTR

extern void
fonty_chunk_info_read_utf8(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle,
    _In_reads_(uchars_len_no_spaces) PC_UCHARS utf8,
    _InVal_     S32 uchars_len_no_spaces,
    _InVal_     S32 trail_spaces);

#endif /* RISCOS && USTR_IS_SBSTR */

#if WINDOWS && TSTR_IS_SBSTR

extern void
fonty_chunk_info_read_wchars(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle,
    _In_reads_(wchars_len_no_spaces) PCWCH wchars,
    _InVal_     S32 wchars_len_no_spaces,
    _InVal_     S32 trail_spaces);

#endif /* WINDOWS && TSTR_IS_SBSTR */

extern void
fonty_chunk_init(
    P_FONTY_CHUNK p_fonty_chunk,
    _InVal_     FONTY_HANDLE fonty_handle);

_Check_return_
extern STATUS /*FONTY_HANDLE*/
fonty_handle_from_font_spec(
    _InRef_     PC_FONT_SPEC p_font_spec,
    _InVal_     BOOL draft_mode);

_Check_return_
extern PIXIT
drawfile_fonty_paint_calc_shift_y(
    _InRef_     PC_FONT_CONTEXT p_font_context);

#if RISCOS

extern PIXIT
riscos_fonty_paint_calc_shift_y(
    _InRef_     PC_FONT_CONTEXT p_font_context);

_Check_return_
extern STATUS
riscos_fonty_paint_shift_x(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     PIXIT shift_x,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

_Check_return_
extern STATUS
riscos_fonty_paint_shift_y(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     PIXIT shift_y,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

_Check_return_
extern STATUS
riscos_fonty_paint_underline(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/);

_Check_return_
extern STATUS
riscos_fonty_paint_font_change(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _HfontRef_  HOST_FONT host_font);

#endif /* OS */

_Check_return_
extern PIXIT
host_fonty_uchars_width(
    _HfontRef_  HOST_FONT host_font,
    _In_reads_(uchars_n) PC_USTR uchars,
    _InVal_     U32 uchars_n);

extern void
fonty_text_paint_rubout_uchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     S32 uchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_rubout);

extern void
fonty_text_paint_simple_uchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     S32 uchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle);

#if TSTR_IS_SBSTR

extern void
fonty_text_paint_rubout_wchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(wchars_n) PCWCH wchars,
    _InVal_     S32 wchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_rubout);

extern void
fonty_text_paint_simple_wchars(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(wchars_n) PCWCH wchars,
    _InVal_     S32 wchars_n,
    _InVal_     PIXIT base_line,
    _InRef_     PC_RGB p_rgb_back,
    _InVal_     FONTY_HANDLE fonty_handle);

#endif /* TSTR_IS_SBSTR */

_Check_return_
extern HOST_FONT
host_font_find(
    _InRef_     PC_HOST_FONT_SPEC p_host_font_spec,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context);

extern void
host_font_dispose(
    _InoutRef_  P_HOST_FONT p_host_font,
    _InRef_maybenone_ PC_REDRAW_CONTEXT p_redraw_context);

#if WINDOWS

extern void
host_font_select(
    _HfontRef_  HOST_FONT host_font,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InVal_     BOOL delete_after);

#endif /* OS */

/*
fontmap.c
*/

/*
exported routines
*/

_Check_return_
extern STATUS
fontmap_font_spec_from_host_base_name(
    _OutRef_    P_FONT_SPEC p_font_spec,
    _In_z_      PCTSTR p_host_base_name);

_Check_return_
extern STATUS
fontmap_font_spec_from_app_index(
    _OutRef_    P_FONT_SPEC p_font_spec,
    _InVal_     ARRAY_INDEX app_index);

_Check_return_
extern STATUS
fontmap_host_base_name_from_app_index(
    _OutRef_    P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _InVal_     ARRAY_INDEX app_index);

_Check_return_
extern STATUS
fontmap_host_base_name_from_app_font_name(
    _OutRef_    P_ARRAY_HANDLE_TSTR p_array_handle_tstr,
    _In_z_      PCTSTR tstr_app_font_name);

extern ARRAY_INDEX
fontmap_host_base_names(void);

_Check_return_
extern STATUS
fontmap_host_fonts_available(
    P_BITMAP p_bitmap_out,
    _InVal_     ARRAY_INDEX app_index);

_Check_return_
extern STATUS
fontmap_host_font_spec_from_font_spec(
    _OutRef_    P_HOST_FONT_SPEC p_host_font_spec,
    _InRef_     PC_FONT_SPEC p_font_spec_key,
    _InVal_     BOOL for_riscos_utf8);

#if RISCOS
#define fontmap_host_font_spec_riscos_from_font_spec(p_host_font_spec, p_font_spec_key) \
    fontmap_host_font_spec_from_font_spec(p_host_font_spec, p_font_spec_key, FALSE)
#else
_Check_return_
extern STATUS
fontmap_host_font_spec_riscos_from_font_spec(
    _OutRef_    P_HOST_FONT_SPEC p_host_font_spec,
    _InRef_     PC_FONT_SPEC p_font_spec_key);
#endif

_Check_return_
extern STATUS
fontmap_app_index_from_app_font_name(
    _In_z_      PCTSTR tstr_app_font_name);

_Check_return_
extern PCTSTR
fontmap_remap(
    _In_z_      PCTSTR tstr_app_font_name);

_Check_return_
extern STATUS
fontmap_rtf_class_from_font_spec(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InRef_     PC_FONT_SPEC p_font_spec);

typedef enum FONTMAP_BITS
{
    FONTMAP_INVALID = -1,

    FONTMAP_BASIC = 0, /* these four bits are used internally by fontmap as indexes into its host tables */
    FONTMAP_BOLD,
    FONTMAP_ITALIC,
    FONTMAP_BOLDITALIC,

    FONTMAP_N_BITS
}
FONTMAP_BITS;

/*
df_paint.c
*/

extern void
drawfile_paint_rectangle_filled(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_RGB p_rgb);

extern void
drawfile_paint_uchars(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_RGB p_rgb_back,
    _InRef_     PC_FONT_CONTEXT p_font_context);

#if WINDOWS

extern void
drawfile_paint_line(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InVal_     S32 thickness,
    _InRef_opt_ PC_DRAW_DASH_HEADER dash_pattern,
    _InRef_     PC_RGB p_rgb);

#endif

extern void
drawfile_paint_drawfile(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_GR_SCALE_PAIR p_gr_scale_pair,
    _InRef_     PC_DRAW_DIAG p_draw_diag);

/*
keys.c
*/

#define     MAX_KMAP_SHORTCUT 8
#define BUF_MAX_KMAP_SHORTCUT 9

/*
extra bits added in to basic key codes
*/

#define KMAP_CODE_ADDED_FN    0x0100 /* function keys */
#define KMAP_CODE_ADDED_SHIFT 0x0200
#define KMAP_CODE_ADDED_CTRL  0x0400
#define KMAP_CODE_ADDED_ALT   0x0800 /* 'alt' letters */

#define RISCOS_EKEY_ESCAPE 27

#define KMAP_ESCAPE 27

/* function keys starting at F0 .. F15 */

#define KMAP_BASE_FUNC          (KMAP_CODE_ADDED_FN + 0x00)
#define KMAP_BASE_SFUNC         (KMAP_BASE_FUNC | KMAP_CODE_ADDED_SHIFT)
#define KMAP_BASE_CFUNC         (KMAP_BASE_FUNC                         | KMAP_CODE_ADDED_CTRL)
#define KMAP_BASE_CSFUNC        (KMAP_BASE_FUNC | KMAP_CODE_ADDED_SHIFT | KMAP_CODE_ADDED_CTRL)

/* function-like keys (1) */

#define KMAP_FUNC_PRINT         (KMAP_BASE_FUNC    + 0x00)
#define KMAP_FUNC_SPRINT        (KMAP_BASE_SFUNC   + 0x00)
#define KMAP_FUNC_CPRINT        (KMAP_BASE_CFUNC   + 0x00)
#define KMAP_FUNC_CSPRINT       (KMAP_BASE_CSFUNC  + 0x00)

#define KMAP_FUNC_INSERT        (KMAP_BASE_FUNC    + 0x0D)
#define KMAP_FUNC_SINSERT       (KMAP_BASE_SFUNC   + 0x0D)
#define KMAP_FUNC_CINSERT       (KMAP_BASE_CFUNC   + 0x0D)
#define KMAP_FUNC_CSINSERT      (KMAP_BASE_CSFUNC  + 0x0D)

/* function-like keys (2) */

#define KMAP_BASE_FUNC2         (KMAP_CODE_ADDED_FN + 0x10)
#define KMAP_BASE_SFUNC2        (KMAP_BASE_FUNC2 | KMAP_CODE_ADDED_SHIFT)
#define KMAP_BASE_CFUNC2        (KMAP_BASE_FUNC2                         | KMAP_CODE_ADDED_CTRL)
#define KMAP_BASE_CSFUNC2       (KMAP_BASE_FUNC2 | KMAP_CODE_ADDED_SHIFT | KMAP_CODE_ADDED_CTRL)

#define RISCOS_EKEY_DELETE 0x7F /* transforms to ... */

#define KMAP_FUNC_DELETE        (KMAP_BASE_FUNC2   + 0x00)
#define KMAP_FUNC_SDELETE       (KMAP_BASE_SFUNC2  + 0x00)
#define KMAP_FUNC_CDELETE       (KMAP_BASE_CFUNC2  + 0x00)
#define KMAP_FUNC_CSDELETE      (KMAP_BASE_CSFUNC2 + 0x00)

#define RISCOS_EKEY_HOME 30 /* transforms to ... */

#define KMAP_FUNC_HOME          (KMAP_BASE_FUNC2   + 0x01)
#define KMAP_FUNC_SHOME         (KMAP_BASE_SFUNC2  + 0x01)
#define KMAP_FUNC_CHOME         (KMAP_BASE_CFUNC2  + 0x01)
#define KMAP_FUNC_CSHOME        (KMAP_BASE_CSFUNC2 + 0x01)

#define RISCOS_EKEY_BACKSPACE 0x08 /* transforms to ... */

#define KMAP_FUNC_BACKSPACE     (KMAP_BASE_FUNC2   + 0x02)
#define KMAP_FUNC_SBACKSPACE    (KMAP_BASE_SFUNC2  + 0x02)
#define KMAP_FUNC_CBACKSPACE    (KMAP_BASE_CFUNC2  + 0x02)
#define KMAP_FUNC_CSBACKSPACE   (KMAP_BASE_CSFUNC2 + 0x02)

#define RISCOS_EKEY_RETURN 0x0D /* transforms to ... */

#define KMAP_FUNC_RETURN        (KMAP_BASE_FUNC2   + 0x03)
#define KMAP_FUNC_SRETURN       (KMAP_BASE_SFUNC2  + 0x03)
#define KMAP_FUNC_CRETURN       (KMAP_BASE_CFUNC2  + 0x03)
#define KMAP_FUNC_CSRETURN      (KMAP_BASE_CSFUNC2 + 0x03)

#define KMAP_FUNC_PAGE_DOWN     (KMAP_BASE_FUNC2   + 0x04)
#define KMAP_FUNC_SPAGE_DOWN    (KMAP_BASE_SFUNC2  + 0x04) /* only distinguished on Windows */

#define KMAP_FUNC_PAGE_UP       (KMAP_BASE_FUNC2   + 0x05)
#define KMAP_FUNC_SPAGE_UP      (KMAP_BASE_SFUNC2  + 0x05) /* only distinguished on Windows */

/* room for expansion 6..9 here */

#define KMAP_FUNC_TAB           (KMAP_BASE_FUNC2   + 0x0A)
#define KMAP_FUNC_STAB          (KMAP_BASE_SFUNC2  + 0x0A)
#define KMAP_FUNC_CTAB          (KMAP_BASE_CFUNC2  + 0x0A)
#define KMAP_FUNC_CSTAB         (KMAP_BASE_CSFUNC2 + 0x0A)

#define KMAP_FUNC_END           (KMAP_BASE_FUNC2   + 0x0B)
#define KMAP_FUNC_SEND          (KMAP_BASE_SFUNC2  + 0x0B)
#define KMAP_FUNC_CEND          (KMAP_BASE_CFUNC2  + 0x0B)
#define KMAP_FUNC_CSEND         (KMAP_BASE_CSFUNC2 + 0x0B)

#define KMAP_FUNC_ARROW_LEFT    (KMAP_BASE_FUNC2   + 0x0C)
#define KMAP_FUNC_SARROW_LEFT   (KMAP_BASE_SFUNC2  + 0x0C)
#define KMAP_FUNC_CARROW_LEFT   (KMAP_BASE_CFUNC2  + 0x0C)
#define KMAP_FUNC_CSARROW_LEFT  (KMAP_BASE_CSFUNC2 + 0x0C)

#define KMAP_FUNC_ARROW_RIGHT   (KMAP_BASE_FUNC2   + 0x0D)
#define KMAP_FUNC_SARROW_RIGHT  (KMAP_BASE_SFUNC2  + 0x0D)
#define KMAP_FUNC_CARROW_RIGHT  (KMAP_BASE_CFUNC2  + 0x0D)
#define KMAP_FUNC_CSARROW_RIGHT (KMAP_BASE_CSFUNC2 + 0x0D)

#define KMAP_FUNC_ARROW_DOWN    (KMAP_BASE_FUNC2   + 0x0E)
#define KMAP_FUNC_SARROW_DOWN   (KMAP_BASE_SFUNC2  + 0x0E)
#define KMAP_FUNC_CARROW_DOWN   (KMAP_BASE_CFUNC2  + 0x0E)
#define KMAP_FUNC_CSARROW_DOWN  (KMAP_BASE_CSFUNC2 + 0x0E)

#define KMAP_FUNC_ARROW_UP      (KMAP_BASE_FUNC2   + 0x0F)
#define KMAP_FUNC_SARROW_UP     (KMAP_BASE_SFUNC2  + 0x0F)
#define KMAP_FUNC_CARROW_UP     (KMAP_BASE_CFUNC2  + 0x0F)
#define KMAP_FUNC_CSARROW_UP    (KMAP_BASE_CSFUNC2 + 0x0F)

typedef S32 KMAP_CODE; typedef KMAP_CODE * P_KMAP_CODE; typedef const KMAP_CODE * PC_KMAP_CODE;

/*
exported routines
*/

_Check_return_
extern KMAP_CODE
#if RISCOS
ri_kmap_convert(
    _InVal_     S32 chcode /*from Wimp_EKeyPressed*/);
#elif WINDOWS
ri_kmap_convert(
    _InVal_     UINT vk /* from WM_KEYthing */);
#endif

extern void
host_key_buffer_flush(void);

extern void
host_key_cache_init(void);

_Check_return_
extern STATUS
host_key_cache_emit_events(void);

_Check_return_
extern STATUS
host_key_cache_emit_events_for(
    _InVal_     DOCNO docno);

_Check_return_
extern STATUS
host_key_cache_event(
    _InVal_     DOCNO docno,
    _InVal_     S32 keycode,
    _InVal_     BOOL fn_key,
    _InVal_     BOOL emit_if_none_in_buffer);

_Check_return_
extern BOOL
host_keys_in_buffer(void);

/*
inline.c
*/

#if 1

/******************************************************************************
*
* in-line sequence:
* +--------+------+-------+------+------+------+-----+------+-------+---------+
* | leadin | null | count | code | type | arg1 | ... | argn | count | leadout |
* +--------+------+-------+------+------+------+-----+------+-------+---------+
*
******************************************************************************/

/* in-line identifier code is CH_INLINE */

#define IL_OFF_LEAD     0
#define IL_OFF_NULL     1  /* opportunity for much faster scanning of strings with inlines */
#define IL_OFF_COUNT    2  /* offsets - to count */
#define IL_OFF_CODE     3
#define IL_OFF_TYPE     4
#define IL_OFF_DATA     5

#define INLINE_SOVH     5U  /* leadin, zero, count, code, type */

/*
backwards methods look for a sequence *before* the current position, hence non-zero IL_OFF_LEADB
*/

#define IL_OFF_LEAD_B   1

#define INLINE_EOVH     1U  /* count, leadout */

#else

/******************************************************************************
*
* in-line sequence:
* +--------+-------+------+------+------+-----+------+-------+---------+
* | leadin | count | code | type | arg1 | ... | argn | count | leadout |
* +--------+-------+------+------+------+-----+------+-------+---------+
*
******************************************************************************/

/* in-line identifier code is CH_INLINE */

#define IL_OFF_LEAD     0
#define IL_OFF_COUNT    1  /* offsets - to count */
#define IL_OFF_CODE     2
#define IL_OFF_TYPE     3
#define IL_OFF_DATA     4

#define INLINE_SOVH     4U  /* leadin, count, code, type */

/*
backwards methods look for a sequence *before* the current position, hence non-zero IL_OFF_LEADB
*/

#define IL_OFF_COUNT_B  2
#define IL_OFF_LEAD_B   1

#define INLINE_EOVH     2U  /* count, leadout */

#endif

#define INLINE_OVH (INLINE_SOVH + INLINE_EOVH)

#define INLINE_MAX (U8_MAX + INLINE_OVH)
#define BUF_INLINE_MAX (INLINE_MAX + 1)

/* in-line sequence here? */
#define is_inline(ptr) ( \
    CH_INLINE == PtrGetByte(ptr) )

/* in-line sequence at given offset? */
#define is_inline_off(ptr, off) ( \
    CH_INLINE == PtrGetByteOff(ptr, off) )

/* return in-line code for an in-line sequence (or character) */
#define inline_code(ptr) ( /*IL_CODE*/ \
    inline_code_off(ptr, 0) )

/* return in-line code for an in-line sequence (or character) at given offset */
#define inline_code_off(ptr, off) ( /*IL_CODE*/ \
    is_inline_off(ptr, off) \
        ? (IL_CODE) PtrGetByteOff(ptr, (off) + IL_OFF_CODE) \
        : IL_NONE )

/* return number of bytes in an in-line sequence (or character) */
#define inline_bytecount(ptr) ( \
    is_inline(ptr) \
        ? (S32) PtrGetByteOff(ptr, IL_OFF_COUNT) \
        : uchars_bytes_of_char(((PC_UCHARS) (ptr))) )

/* return number of bytes in an in-line sequence (or character) at given offset */
#define inline_bytecount_off(ptr, off) ( \
    is_inline_off(ptr, off) \
        ? (S32) PtrGetByteOff(ptr, (off) + IL_OFF_COUNT) \
        : (S32) uchars_bytes_of_char_off(((PC_UCHARS) (ptr)), off) )

#define contains_inline(ptr, n_bytes) ( /*BOOL*/ \
    NULL != memchr(ptr, CH_INLINE, n_bytes) )

#define next_inline(__ptr_type, ptr, n_bytes) ((__ptr_type) /*ptr*/ \
    memchr(ptr, CH_INLINE, n_bytes) )

#define next_inline_off(__ptr_type, ptr, off, n_bytes) ((__ptr_type) /*ptr*/ \
    memchr(PtrAddBytes(PC_BYTE, ptr, off), CH_INLINE, n_bytes) )

/* return pointer to contained data */
#define inline_data_ptr(__ptr_type, ptr) ( /*ptr*/ \
    inline_data_ptr_off(__ptr_type, ptr, 0) )

#define inline_data_ptr_off(__ptr_type, ptr, off) ( /*ptr*/ \
    PtrAddBytes(__ptr_type, ptr, (off) + IL_OFF_DATA) )

/* return size of data in inline */
#define inline_data_size(ptr) ( \
    (S32) PtrGetByteOff(ptr, IL_OFF_COUNT) - INLINE_OVH )

/* return type of data in inline */
#define inline_data_type(ptr) ( \
    (S32) PtrGetByteOff(ptr, IL_OFF_TYPE) )

/* advance a pointer forwards to skip this in-line sequence (or character) */
#define inline_advance(__ptr_type, ptr__ref) ( \
    PtrIncBytes(__ptr_type, ptr__ref, inline_bytecount(ptr__ref)) )

/*
backwards methods
*/

/* in-line sequence going backwards here? */
#define is_inline_b(ptr) ( \
    is_inline_b_off(ptr, 0) )

/* in-line sequence going backwards from given offset? */
#define is_inline_b_off(ptr, off) ( \
    is_inline_off(ptr, (off) - IL_OFF_LEAD_B) )

/* return in-line code for an in-line sequence (or character) going backwards */
#define inline_b_code(ptr) ( /*IL_CODE*/ \
    inline_b_code_off(ptr, 0) )

/* return in-line code for an in-line sequence (or character) going backwards from given offset */
#define inline_b_code_off(ptr, off) ( /*IL_CODE*/ \
    is_inline_b_off(ptr, off) \
        ? (IL_CODE) PtrGetByteOff(ptr, (off) + IL_OFF_CODE - inline_b_bytecount_off(ptr, off)) \
        : IL_NONE )

/* return number of bytes in an in-line sequence (or character) going backwards */
#define inline_b_bytecount(ptr) ( \
    inline_b_bytecount_off(ptr, 0) )

#if defined(IL_OFF_COUNT_B)

/* return number of bytes in an in-line sequence (or character) going backwards from given offset */
#define inline_b_bytecount_off(ptr, off) ( \
    is_inline_b_off(ptr, off) \
        ? (S32) PtrGetByteOff(ptr, (off) - IL_OFF_COUNT_B) \
        : uchars_bytes_prev_of_char_NS(uchars_AddBytes(ptr, off)) )

#else /* !IL_OFF_COUNT_B */

_Check_return_
extern S32
inline_b_bytecount_off(
    _In_reads_(offset) PC_UCHARS_INLINE uchars,
    _InVal_     U32 offset);

#endif /* IL_OFF_COUNT_B */

/* inline codes */

typedef enum IL_CODE
{
    IL_NONE = 0,

    /* >>>style_encoding_index in sk_styl.c depends on the order here */

    IL_STYLE_NAME = 1,
    IL_STYLE_KEY,
    IL_STYLE_HANDLE,            /* inline style indirection */
    IL_STYLE_SEARCH,

    IL_STYLE_CS_WIDTH,
    IL_STYLE_CS_COL_NAME,

    IL_STYLE_RS_HEIGHT,
    IL_STYLE_RS_HEIGHT_FIXED,
    IL_STYLE_RS_UNBREAKABLE,
    IL_STYLE_RS_ROW_NAME,

    IL_STYLE_PS_MARGIN_LEFT,
    IL_STYLE_PS_MARGIN_RIGHT,
    IL_STYLE_PS_MARGIN_PARA,
    IL_STYLE_PS_TAB_LIST,
    IL_STYLE_PS_RGB_BACK,
    IL_STYLE_PS_PARA_START,
    IL_STYLE_PS_PARA_END,
    IL_STYLE_PS_LINE_SPACE,
    IL_STYLE_PS_JUSTIFY,
    IL_STYLE_PS_JUSTIFY_V,
    IL_STYLE_PS_NEW_OBJECT,
    IL_STYLE_PS_NUMFORM_NU,
    IL_STYLE_PS_NUMFORM_DT,
    IL_STYLE_PS_NUMFORM_SE,
    IL_STYLE_PS_RGB_BORDER,
    IL_STYLE_PS_BORDER,
    IL_STYLE_PS_RGB_GRID_LEFT,
    IL_STYLE_PS_RGB_GRID_TOP,
    IL_STYLE_PS_RGB_GRID_RIGHT,
    IL_STYLE_PS_RGB_GRID_BOTTOM,
    IL_STYLE_PS_GRID_LEFT,
    IL_STYLE_PS_GRID_TOP,
    IL_STYLE_PS_GRID_RIGHT,
    IL_STYLE_PS_GRID_BOTTOM,
    IL_STYLE_PS_PROTECT,

    IL_STYLE_FS_NAME,
    IL_STYLE_FS_SIZE_X,
    IL_STYLE_FS_SIZE_Y,
    IL_STYLE_FS_UNDERLINE,
    IL_STYLE_FS_BOLD,
    IL_STYLE_FS_ITALIC,
    IL_STYLE_FS_SUPERSCRIPT,
    IL_STYLE_FS_SUBSCRIPT,
    IL_STYLE_FS_COLOUR,

    IL_TAB = 100,
    IL_PAGE_Y,
    IL_DATE,
    IL_WHOLENAME,
    IL_LEAFNAME,
    IL_SOFT_HYPHEN,
    IL_RETURN,
    IL_PAGE_BREAK,
    IL_MS_FIELD,
    IL_SLE_ARGUMENT,
    IL_SS_NAME,
    IL_PAGE_X,
    IL_FILE_DATE,
    IL_UTF8,

    IL_COUNT
}
IL_CODE; typedef IL_CODE * P_IL_CODE;

/*
inline data types
*/

#define IL_TYPE_NONE    0
#define IL_TYPE_USTR    1
#define IL_TYPE_U8      2
#define IL_TYPE_S32     3
#define IL_TYPE_F64     4
#define IL_TYPE_ANY     5

#define IL_TYPE_MAX     6

#define IL_TYPE_PRIVATE 6 /* e.g. for IL_ARGUMENT */

typedef S32 IL_TYPE; typedef IL_TYPE * P_IL_TYPE;

typedef struct MULTIPLE_DATA
{
    P_ANY p_data;
    U32 n_bytes;
}
MULTIPLE_DATA, * P_MULTIPLE_DATA; typedef const MULTIPLE_DATA * PC_MULTIPLE_DATA;

#if RELEASED || defined(CODE_ANALYSIS)

/* using inline function can spot any bad buffer-to-pointer casts that would otherwise happen silently */
_Check_return_
_Ret_notnull_
static inline P_USTR_INLINE
ustr_inline_bptr(UCHARB_INLINE buf[])
{
    return((P_USTR_INLINE) buf);
}

#else

/* ease single-step debugging */
#define ustr_inline_bptr(buf)    buf

#endif /* RELEASED */

#define ustr_inline_from_h_ustr(pc_array_handle) \
    array_base(pc_array_handle, _UCHARS_INLINE)

/*
exported routines
*/

extern void
data_from_inline(
    _Out_writes_bytes_(bytesof_buffer) P_ANY buffer,
    _InVal_     U32 bytesof_buffer,
    _In_reads_c_(INLINE_OVH) PC_USTR_INLINE ustr_inline,
    _InVal_     S32 type_expected);

_Check_return_
extern S32
data_from_inline_s32(
    _In_reads_c_(INLINE_OVH + sizeof32(S32)) PC_USTR_INLINE ustr_inline);

_Check_return_
extern STATUS /* size out */
inline_array_from_data(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle /*appended to*/,
    _InVal_     IL_CODE inline_id,
    _InVal_     IL_TYPE inline_type,
    _In_reads_bytes_opt_(size) PC_ANY p_data,
    _InVal_     U32 size);

_Check_return_
extern U32 /* size out */
inline_uchars_buf_from_data(
    _Out_writes_(elemof_buffer) P_USTR_INLINE ustr_inline,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 inline_id,
    _InVal_     S32 inline_type,
    _In_reads_bytes_opt_(size) PC_ANY p_any,
    _InVal_     U32 size /* only for TYPE_ANY */);

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_code(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     IL_CODE inline_id);

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_data(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     S32 inline_id,
    _InVal_     S32 inline_type,
    _In_reads_bytes_opt_(size) PC_ANY p_any,
    _InVal_     U32 size /* only for TYPE_ANY */);

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_multiple_data(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     S32 inline_id,
    _InVal_     S32 inline_type,
    _In_reads_(n_elements) PC_MULTIPLE_DATA p_multiple_data,
    _InVal_     U32 n_elements);

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_from_ustr(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     IL_CODE inline_id,
    _In_z_      PC_USTR ustr);

_Check_return_
extern STATUS /* size out */
inline_quick_ublock_IL_TAB(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/);

_Check_return_
_Ret_z_
extern PCTSTR
report_ustr_inline(
    _In_opt_z_  PC_USTR_INLINE ustr_inline);

_Check_return_
extern int
uchars_inline_compare_n2(
    _In_reads_(uchars_n_1) PC_UCHARS_INLINE uchars_inline_1,
    _InVal_     U32 uchars_n_1,
    _In_reads_(uchars_n_2) PC_UCHARS_INLINE uchars_inline_2,
    _InVal_     U32 uchars_n_2);

/*ncr*/
extern U32
uchars_inline_copy_strip(
    _Out_writes_to_(elemof_buffer,return) P_UCHARS uchars_buf,
    _InVal_     U32 elemof_buffer,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n);

/*ncr*/
extern U32
ustr_inline_copy_strip(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR_INLINE ustr_inline);

_Check_return_
extern STATUS
uchars_inline_plain_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
ustr_inline_replace_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_out /*appended*/,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_in,
    _InVal_     S32 case_1,
    _InVal_     S32 case_2,
    _InVal_     S32 copy_capitals);

_Check_return_
extern STATUS
uchars_inline_search(
    _In_reads_(*p_end) PC_UCHARS_INLINE uchars_inline,
    _In_z_      PC_USTR ustr_search_for,
    _InoutRef_  P_S32 p_start,
    _InoutRef_  P_S32 p_end,
    _InVal_     BOOL ignore_capitals,
    _In_        BOOL whole_words);

_Check_return_
extern STATUS
uchars_inline_search_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n);

_Check_return_
extern U32
ustr_inline_strlen32(
    _In_z_      PC_USTR_INLINE ustr_inline);

#define ustr_inline_strlen32p1(ustr_inline) ( \
    1 /*CH_NULL*/ + ustr_inline_strlen32(ustr_inline) )

/* S32-returning compatible with most text setup fns */
#define ustr_inline_strlen(ustr_inline) ( \
    (S32) ustr_inline_strlen32(ustr_inline) )

#define uchars_inline_IncByte(uchars_inline) \
    PtrIncByte(PC_UCHARS_INLINE, uchars_inline)

#define uchars_inline_IncByte_wr(uchars_inline_wr) \
    PtrIncByte(P_UCHARS_INLINE, uchars_inline_wr)

#define uchars_inline_IncBytes(uchars_inline, add) \
    PtrIncBytes(PC_UCHARS_INLINE, uchars_inline, add)

#define uchars_inline_IncBytes_wr(uchars_inline_wr, add) \
    PtrIncBytes(P_UCHARS_INLINE, uchars_inline_wr, add)

#define uchars_inline_AddBytes(uchars_inline, add) \
    PtrAddBytes(PC_UCHARS_INLINE, uchars_inline, add)

#define uchars_inline_AddBytes_wr(uchars_inline_wr, add) \
    PtrAddBytes(P_UCHARS_INLINE, uchars_inline_wr, add)

#define ustr_inline_IncByte(ustr_inline) \
    PtrIncByte(PC_USTR_INLINE, ustr_inline)

#define ustr_inline_IncByte_wr(ustr_inline_wr) \
    PtrIncByte(P_USTR_INLINE, ustr_inline_wr)

#define ustr_inline_IncBytes(ustr_inline, add) \
    PtrIncBytes(PC_USTR_INLINE, ustr_inline, add)

#define ustr_inline_IncBytes_wr(ustr_inline_wr, add) \
    PtrIncBytes(P_USTR_INLINE, ustr_inline_wr, add)

#define ustr_inline_AddBytes(ustr_inline, add) \
    PtrAddBytes(PC_USTR_INLINE, ustr_inline, add)

#define ustr_inline_AddBytes_wr(ustr_inline_wr, add) \
    PtrAddBytes(P_USTR_INLINE, ustr_inline_wr, add)

#define quick_ublock_uchars_inline(/*const*/ p_quick_ublock) \
    (PC_UCHARS_INLINE) quick_ublock_uchars(p_quick_ublock)

#define quick_ublock_ustr_inline(/*const*/ p_quick_ublock) \
    (PC_USTR_INLINE) quick_ublock_ustr(p_quick_ublock)

#if USTR_IS_SBSTR

#define uchars_inline_validate(func, uchars_inline, n_bytes) /*nothing*/

#define ustr_inline_validate(func, ustr_inline) /*nothing*/
#define ustr_inline_validate_n(func, ustr_inline, n_bytes) /*nothing*/

#else

/* from utf8.c */

_Check_return_
extern STATUS /* STATUS_OK, error if not */
uchars_inline_validate(
    _In_z_      PCTSTR func,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline,
    _InVal_     U32 uchars_n);

/* from utf8str.c */

_Check_return_
extern STATUS /* STATUS_OK, error if not */
ustr_inline_validate(
    _In_z_      PCTSTR func,
    _In_z_      PC_USTR_INLINE ustr_inline);

_Check_return_
extern STATUS /* STATUS_OK, error if not */
ustr_inline_validate_n(
    _In_z_      PCTSTR func,
    _In_z_      PC_USTR_INLINE ustr_inline,
    _In_        U32 uchars_n /*strlen_with,without_NULLCH*/);

#endif /* USTR_IS_SBSTR */

/*
arglist.c
*/

#define ARG_TYPE_NONE           0U
#define ARG_TYPE_U8C            1U
#define ARG_TYPE_U8N            2U
#define ARG_TYPE_BOOL           3U
#define ARG_TYPE_S32            4U
#define ARG_TYPE_X32            5U
#define ARG_TYPE_COL            6U
#define ARG_TYPE_ROW            7U
#define ARG_TYPE_F64            8U
#define ARG_TYPE_RAW            9U
#define ARG_TYPE_RAW_DS         10U
#define ARG_TYPE_USTR           11U  /* NB may contain inlines */

#if !TSTR_IS_SBSTR
#define ARG_TYPE_TSTR_DISTINCT 1
#endif
#if defined(ARG_TYPE_TSTR_DISTINCT)
#define ARG_TYPE_TSTR           12U
#else
#define ARG_TYPE_TSTR           ARG_TYPE_USTR
#endif

#define ARG_TYPE_USTR_INLINES   ARG_TYPE_USTR /* alias */

#define ARG_TYPE_MASK           0x1FU

#define ARG_ALLOC               0x20U

#define ARG_OPTIONAL            0x00U
#define ARG_MANDATORY           0x80U
#define ARG_MANDATORY_OR_BLANK  (ARG_MANDATORY | 0x40U) /* if arg omitted, a blank string (not NULL) is substituted */

#define ARG_TYPE_CONTROL_FLAGS  0xE0U

typedef U8 ARG_TYPE; typedef ARG_TYPE * P_ARG_TYPE; typedef const ARG_TYPE * PC_ARG_TYPE;

typedef struct ARGLIST_ARG_DATA_WORDS
{
    U32 w0, w1; /* as big as the biggest item (currently F64) */
}
ARGLIST_ARG_DATA_WORDS;

typedef union ARGLIST_ARG_DATA
{
    ARGLIST_ARG_DATA_WORDS w;

    U8           u8c;
    U8           u8n;
    BOOL         fBool;         /* TRUE, FALSE or maybe INDETERMINATE, but without U8 masking/casting */
    S32          s32;
    U32          x32;
    COL          col;
    ROW          row;
    F64          f64;
    ARRAY_HANDLE raw;
    PC_USTR      ustr;          /* al_ustr_set() if ALLOC; NB may contain inlines */
    P_USTR       ustr_wr;
    PCTSTR       tstr;          /* al_tstr_set() if ALLOC */
    PTSTR        tstr_wr;

    PIXIT        pixit;         /*s32*/
    OBJECT_ID    object_id;     /*s32*/
    T5_MESSAGE   t5_message;    /*s32*/
    T5_FILETYPE  t5_filetype;   /*s32*/

    FP_PIXIT     fp_pixit;      /*f64*/

    PC_USTR_INLINE ustr_inline; /*ustr*/
}
ARGLIST_ARG_DATA;

typedef struct ARGLIST_ARG
{
    ARG_TYPE type;

    ARGLIST_ARG_DATA val;
}
ARGLIST_ARG, * P_ARGLIST_ARG; typedef const ARGLIST_ARG * PC_ARGLIST_ARG;

typedef ARRAY_HANDLE ARGLIST_HANDLE; typedef ARGLIST_HANDLE * P_ARGLIST_HANDLE; typedef const ARGLIST_HANDLE * PC_ARGLIST_HANDLE;

/*
exported functions
*/

_Check_return_
extern STATUS
arg_alloc_ustr(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx,
    _In_opt_z_  PC_USTR ustr); /* NB poss inline */

_Check_return_
extern STATUS
arg_alloc_tstr(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx,
    _In_opt_z_  PCTSTR tstr);

extern void
arg_dispose(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx);

extern void
arg_dispose_val(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _InVal_     U32 arg_idx);

extern void
arglist_cache_reduce(void);

extern void
arglist_dispose(
    _InoutRef_  P_ARGLIST_HANDLE p_h_args);

extern void
arglist_dispose_after(
    _InRef_     PC_ARGLIST_HANDLE p_h_args /*data modified*/,
    _In_        U32 arg_idx /* U32_MAX -> all args */);

_Check_return_
extern STATUS
arglist_duplicate(
    _OutRef_    P_ARGLIST_HANDLE p_h_dst_args,
    _InRef_     PC_ARGLIST_HANDLE p_h_src_args);

_Check_return_
extern STATUS
arglist_prepare(
    _OutRef_    P_ARGLIST_HANDLE p_h_args,
    _InRef_     PC_ARG_TYPE p_arg_type /*[], terminator is ARG_TYPE_NONE*/);

#if TRACE_ALLOWED

extern void
arglist_trace(
    _InRef_     PC_ARGLIST_HANDLE p_h_args,
    _In_z_      PCTSTR caller);

#endif

/*
functions as macros
*/

/* NB. only use if arglist is present */
#define arg_present(p_h_args, arg_idx, p_p_arg) (   \
    *(p_p_arg) = p_arglist_arg(p_h_args, arg_idx),  \
    (*(p_p_arg))->type != ARG_TYPE_NONE             )

/* ok as checks for null arglist */
#define n_arglist_args(p_h_args) \
    array_elements32(p_h_args)

/* NB. only use if arglist is present */
#define p_arglist_arg(p_h_args, arg_idx) \
    array_ptr32_no_checks(p_h_args, ARGLIST_ARG, arg_idx)

/* NB. only use if arglist is present */
#define pc_arglist_arg(p_h_args, arg_idx) \
    array_ptr32c_no_checks(p_h_args, ARGLIST_ARG, arg_idx)

_Check_return_
_Ret_writes_(n_args)
static inline P_ARGLIST_ARG
p_arglist_args(
    _InRef_     PC_ARGLIST_HANDLE pc_arglist_handle,
    _InVal_     U32 n_args)
{
    assert(n_args <= array_elements32(pc_arglist_handle)); /* NB NOT array_index_is_valid(pc_arglist_handle, n_args) */
    UNREFERENCED_PARAMETER_InVal_(n_args);
    return(array_range(pc_arglist_handle, ARGLIST_ARG, 0, n_args));
}

_Check_return_
_Ret_writes_(n_args)
static inline PC_ARGLIST_ARG
pc_arglist_args(
    _InRef_     PC_ARGLIST_HANDLE pc_arglist_handle,
    _InVal_     U32 n_args)
{
    assert(n_args <= array_elements32(pc_arglist_handle)); /* NB NOT array_index_is_valid(pc_arglist_handle, n_args) */
    UNREFERENCED_PARAMETER_InVal_(n_args);
    return(array_rangec(pc_arglist_handle, ARGLIST_ARG, 0, n_args));
}

#define arg_is_present(p_args, arg_idx) (   \
    (p_args)[(arg_idx)].type != ARG_TYPE_NONE )

/*
ma_event.c
*/

typedef STATUS MAEVE_HANDLE;

/*
master event handlers
*/

typedef struct MAEVE_BLOCK
{
    CLIENT_HANDLE client_handle;
    MAEVE_HANDLE maeve_handle;
#if defined(MAEVE_BLOCK_P_DATA)
    P_ANY mb_p_data;
#endif
}
MAEVE_BLOCK; typedef const MAEVE_BLOCK * PC_MAEVE_BLOCK;

#if defined(MAEVE_BLOCK_P_DATA)
#define P_DATA_FROM_MB(__base_type, p_maeve_block) ( \
    (__base_type *) (p_maeve_block)->mb_p_data)
#endif

/*
maeve event procedure prototypes
*/

typedef STATUS (* P_PROC_MAEVE_EVENT) (
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_MAEVE_BLOCK p_maeve_block);

#define MAEVE_EVENT_PROTO(_e_s, _proc_name) \
_e_s STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InVal_     T5_MESSAGE t5_message, \
    /*_Inout_*/ P_ANY p_data, \
    _InRef_     PC_MAEVE_BLOCK p_maeve_block)

/*
master services event handlers
*/

typedef STATUS MAEVE_SERVICES_HANDLE;

typedef struct MAEVE_SERVICES_BLOCK
{
    MAEVE_SERVICES_HANDLE maeve_services_handle;
#if defined(MAEVE_SERVICES_BLOCK_P_DATA)
    P_ANY msb_p_data;
#endif
}
MAEVE_SERVICES_BLOCK; typedef const MAEVE_SERVICES_BLOCK * PC_MAEVE_SERVICES_BLOCK;

#if defined(MAEVE_SERVICES_BLOCK_P_DATA)
#define P_DATA_FROM_MSB(__base_type, p_maeve_services_block) ( \
    (__base_type *) (p_maeve_services_block)->msb_p_data)
#endif

/*
maeve services event procedure prototypes
*/

typedef STATUS (* P_PROC_MAEVE_SERVICES_EVENT) (
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_MAEVE_SERVICES_BLOCK p_maeve_services_block);

#define MAEVE_SERVICES_EVENT_PROTO(_e_s, _proc_name) \
_e_s STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InVal_     T5_MESSAGE t5_message, \
    /*_Inout_*/ P_ANY p_data, \
    _InRef_     PC_MAEVE_SERVICES_BLOCK p_maeve_services_block)

/*
exported routines
*/

_Check_return_
extern STATUS /* MAEVE_HANDLE out */
maeve_event_handler_add(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_PROC_MAEVE_EVENT p_proc_event /* event routine */,
    _InVal_     CLIENT_HANDLE client_handle /* clients handle in */);

extern void
maeve_event_handler_del(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_PROC_MAEVE_EVENT p_proc_event /* event routine */,
    _InVal_     CLIENT_HANDLE client_handle /* clients handle in */);

extern void
maeve_event_handler_del_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     MAEVE_HANDLE maeve_handle);

PROC_EVENT_PROTO(extern, maeve_event);

extern void
maeve_event_close_thunk(
    _DocuRef_   P_DOCU p_docu);

/*
maeve service events
*/

_Check_return_
extern STATUS /* MAEVE_SERVICES_HANDLE out */
maeve_services_event_handler_add(
    _InRef_     P_PROC_MAEVE_SERVICES_EVENT p_proc_maeve_services_event);

extern void
maeve_services_event_handler_del_handle(
    _InVal_     MAEVE_SERVICES_HANDLE maeve_services_handle);

PROC_EVENT_PROTO(extern, maeve_service_event);

extern void
maeve_service_event_exit2(void);

#if RISCOS

_Check_return_
extern STATUS
ho_help_url(
    _In_z_      PCTSTR url);

_Check_return_
extern STATUS
thesaurus_loaded(void);

_Check_return_
extern STATUS
thesaurus_process_word(
    _In_z_      PC_USTR ustr);

extern void
thesaurus_startup(void);

#endif /* RISCOS */

/*
object.c
*/

/*
object:message pair
*/

typedef struct OBJECT_MESSAGE
{
    OBJECT_ID object_id;
    T5_MESSAGE t5_message;
}
OBJECT_MESSAGE;

/*
exported routines
*/

_Check_return_
extern BOOL
object_available(
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
object_call_all(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data);

_Check_return_
extern STATUS
object_call_all_accumulate(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data);

_Check_return_
extern STATUS
object_call_between(
    _InoutRef_  P_OBJECT_ID p_object_id,
    _InVal_     OBJECT_ID max_object_id,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data);

_Check_return_
extern STATUS
object_call_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern STATUS
object_call_id_reordered(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InVal_     OBJECT_ID object_id);

#define object_call_id(object_id, p_docu, t5_message, p_data) \
    object_call_id_reordered(p_docu, t5_message, p_data, object_id)

_Check_return_
extern STATUS
object_call_id_load(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InVal_     OBJECT_ID object_id);

/*ncr*/
extern STATUS
object_data_from_docu_area_br(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area);

/*ncr*/
extern STATUS
object_data_from_docu_area_tl(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_DOCU_AREA p_docu_area);

/*ncr*/
extern STATUS
object_data_from_position(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_POSITION p_position,
    _InRef_maybenone_ PC_OBJECT_POSITION p_object_position);

/*ncr*/
extern STATUS /* STATUS_DONE == cell contains data */
object_data_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_OBJECT_DATA p_object_data,
    _InRef_     PC_SLR p_slr);

extern void
object_data_init(
    _OutRef_    P_OBJECT_DATA p_object_data);

extern void
object_install(
    _InVal_     OBJECT_ID object_id,
    _InRef_     P_PROC_OBJECT p_proc_object);

_Check_return_
extern STATUS
object_instance_data_alloc(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     S32 size);

extern void
object_instance_data_dispose(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
object_load(
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
object_next(
    _InoutRef_  P_OBJECT_ID p_object_id);

_Check_return_
extern BOOL
object_present(
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
object_realloc(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_ANY p_p_any /* pointer to object data */,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id,
    _InVal_     S32 object_size);

_Check_return_
extern STATUS
object_startup(
    _In_        int phase);

_Check_return_
_Ret_maybenone_
extern P_ANY
p_object_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id);

_Check_return_
_Ret_maybenone_
static inline struct EV_CELL *
p_ev_cell_object_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    return((struct EV_CELL *) p_object_from_slr(p_docu, p_slr, OBJECT_ID_SS));
}

_Check_return_
_Ret_writes_maybenone_(bytesof_data)
extern P_BYTE
_p_object_instance_data(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id
    CODE_ANALYSIS_ONLY_ARG(_In_ U32 bytesof_data));

_Check_return_
extern P_PROC_OBJECT
t5_glued_object(
    _InVal_     OBJECT_ID object_id);

/*
ui_data.c
*/

/*
a data type used to pass references to text around
*/

typedef enum UI_TEXT_TYPE
{
    UI_TEXT_TYPE_NONE = 0,
    UI_TEXT_TYPE_RESID,

    UI_TEXT_TYPE_USTR_PERM,
    UI_TEXT_TYPE_USTR_TEMP,
    UI_TEXT_TYPE_USTR_ALLOC,
    UI_TEXT_TYPE_USTR_ARRAY,

#if !TSTR_IS_SBSTR
#define UI_TEXT_TYPE_TSTR_DISTINCT 1
#endif
#if defined(UI_TEXT_TYPE_TSTR_DISTINCT)
    UI_TEXT_TYPE_TSTR_PERM,
    UI_TEXT_TYPE_TSTR_TEMP,
    UI_TEXT_TYPE_TSTR_ALLOC,
    UI_TEXT_TYPE_TSTR_ARRAY,
#else
    UI_TEXT_TYPE_TSTR_PERM  = UI_TEXT_TYPE_USTR_PERM,
    UI_TEXT_TYPE_TSTR_TEMP  = UI_TEXT_TYPE_USTR_TEMP,
    UI_TEXT_TYPE_TSTR_ALLOC = UI_TEXT_TYPE_USTR_ALLOC,
    UI_TEXT_TYPE_TSTR_ARRAY = UI_TEXT_TYPE_USTR_ARRAY,
#endif /* UI_TEXT_TYPE_TSTR_DISTINCT */

    UI_TEXT_TYPE_MAX
}
UI_TEXT_TYPE;

typedef union UI_TEXT_DATA
{
    STATUS              resource_id;        /* handle of a resource based string (first for most union initialisers) */

    PC_USTR             ustr;               /* this CH_NULL-terminated UCHAR data is either still owned by the object or by this UI_TEXT */
    P_USTR              ustr_wr;            /* writeable variant for convenience */
    ARRAY_HANDLE_USTR   array_handle_ustr;  /* UCHAR [] - if this is passed back then it is deemed to be owned by the proc returned to */

    PCTSTR              tstr;               /* this CH_NULL-terminated TCHAR data is either still owned by the object or by this UI_TEXT */
    PTSTR               tstr_wr;            /* writeable variant for convenience */
    ARRAY_HANDLE_TSTR   array_handle_tstr;  /* TCHAR [] - if this is passed back then it is deemed to be owned by the proc returned to */
}
UI_TEXT_DATA;

typedef struct UI_TEXT
{
    UI_TEXT_TYPE type;
    UI_TEXT_DATA text;
}
UI_TEXT, * P_UI_TEXT; typedef const UI_TEXT * PC_UI_TEXT;

#define UI_TEXT_INIT_RESID(resource_id) { UI_TEXT_TYPE_RESID, { (resource_id) } }

/*
these are the known classes of UI data
*/

typedef enum UI_DATA_TYPE
{
    UI_DATA_TYPE_NONE = 0,
    UI_DATA_TYPE_spare_1,
    UI_DATA_TYPE_spare_2,
    UI_DATA_TYPE_S32,
    UI_DATA_TYPE_F64,
    UI_DATA_TYPE_ANY,
    UI_DATA_TYPE_TEXT,

    UI_DATA_TYPE_MAX
}
UI_DATA_TYPE, * P_UI_DATA_TYPE;

typedef union UI_DATA
{
    ARRAY_HANDLE any;
    F64          f64;
    S32          s32;
    UI_TEXT      text;
}
UI_DATA, * P_UI_DATA;

/*
a data type used to access UI data items
*/

typedef enum UI_SOURCE_TYPE
{
    UI_SOURCE_TYPE_NONE = 0,
    UI_SOURCE_TYPE_SINGLE,
    UI_SOURCE_TYPE_ARRAY,
    UI_SOURCE_TYPE_LIST,
    UI_SOURCE_TYPE_PROC
}
UI_SOURCE_TYPE;

#ifdef UI_SOURCE_TYPE_PROCS
/*
callback procedure required to convert an itemno from a UI data source to a UI data item
*/

typedef STATUS (* P_PROC_UI_SOURCE) (
    P_ANY ui_source_handle,
    _InVal_     S32 itemno,
    /*out*/ P_UI_DATA p_ui_data);

#define PROC_UI_SOURCE_PROTO(_proc_name) \
STATUS \
_proc_name( \
    P_ANY ui_source_handle, \
    _InVal_     S32 itemno, \
    /*out*/ P_UI_DATA p_ui_data)
#endif

#ifdef UI_SOURCE_TYPE_PROCS
typedef struct UI_SOURCE_SOURCE_PROC
{
    P_PROC_UI_SOURCE proc;
    P_ANY ui_source_handle;
}
UI_SOURCE_SOURCE_PROC;
#endif

typedef union UI_SOURCE_SOURCE
{
    P_UI_DATA single_p_ui_data;
    ARRAY_HANDLE array_handle;
    P_LIST_BLOCK p_list_block;
#ifdef UI_SOURCE_TYPE_PROCS
    UI_SOURCE_SOURCE_PROC proc;
#else
    S32 poo[2];
#endif
}
UI_SOURCE_SOURCE;

typedef struct UI_SOURCE
{
    UI_SOURCE_TYPE type;
    UI_SOURCE_SOURCE source;
}
UI_SOURCE, * P_UI_SOURCE; typedef const UI_SOURCE * PC_UI_SOURCE;

/*
callback procedure required to bound check an 'ANY' format data value
*/

typedef STATUS (* P_PROC_UI_ANY_DATA_BOUND_CHECK) (
    P_ANY bound_check_handle,
    /*inout*/ P_ARRAY_HANDLE p_data_handle);

/*
callback procedure required to convert input text into 'ANY' format data
*/

typedef STATUS (* P_PROC_UI_ANY_DATA_FROM_TEXT) (
    P_ANY data_from_text_handle,
    _InRef_     PC_UI_TEXT p_ui_text,
    /*out*/ P_ARRAY_HANDLE p_data_handle);

/*
callback procedure required to increment or decrement an 'ANY' format data value
*/

typedef STATUS (* P_PROC_UI_ANY_DATA_INC_DEC) (
    P_ANY inc_dec_handle,
    /*inout*/ P_ARRAY_HANDLE p_data_handle,
    _InVal_     S32 inc);

/*
callback procedure required to convert 'ANY' format data into text suitable for output
*/

typedef STATUS (* P_PROC_UI_ANY_TEXT_FROM_DATA) (
    P_ANY text_from_data_handle,
    _InVal_     ARRAY_HANDLE data_handle,
    /*out*/ P_UI_TEXT p_ui_text);

/*
UI data control structures for dialog manipulation
*/

typedef struct UI_CONTROL_ANY
{
    P_PROC_UI_ANY_DATA_BOUND_CHECK data_bound_check_proc;
    P_ANY                          data_bound_check_handle;

    P_PROC_UI_ANY_DATA_FROM_TEXT   data_from_text_proc;
    P_ANY                          data_from_text_handle;

    P_PROC_UI_ANY_DATA_INC_DEC     data_inc_dec_proc;
    P_ANY                          data_inc_dec_handle;

    P_PROC_UI_ANY_TEXT_FROM_DATA   text_from_data_proc;
    P_ANY                          text_from_data_handle;
}
UI_CONTROL_ANY; typedef const UI_CONTROL_ANY * PC_UI_CONTROL_ANY;

typedef struct UI_CONTROL_F64
{
    F64 min_val, max_val;

    F64 bump_val;

    P_USTR ustr_numform;
    F64  inc_dec_round;
}
UI_CONTROL_F64; typedef const UI_CONTROL_F64 * PC_UI_CONTROL_F64;

#define PC_UI_CONTROL_F64_NONE _P_DATA_NONE(PC_UI_CONTROL_F64)

typedef struct UI_CONTROL_S32
{
    S32 min_val, max_val;

    S32 bump_val;
}
UI_CONTROL_S32; typedef const UI_CONTROL_S32 * PC_UI_CONTROL_S32;

#define PC_UI_CONTROL_S32_NONE _P_DATA_NONE(PC_UI_CONTROL_S32)

typedef struct UI_CONTROL_TEXT /* also for p_u8 */
{
    S32 maxlen;

    P_BITMAP p_allowed /*NULL->all*/;
}
UI_CONTROL_TEXT;

typedef enum UI_NUMFORM_CLASS
{
    UI_NUMFORM_CLASS_DATE = 0,
    UI_NUMFORM_CLASS_PAGE = 1,
    UI_NUMFORM_CLASS_GENERAL = 2,
    UI_NUMFORM_CLASS_RULER = 3,
    UI_NUMFORM_CLASS_NUMFORM_NU = 4, /*STYLE_SW_PS_NUMFORM_NU*/
    UI_NUMFORM_CLASS_NUMFORM_DT = 5, /*STYLE_SW_PS_NUMFORM_DT*/
    UI_NUMFORM_CLASS_NUMFORM_SE = 6, /*STYLE_SW_PS_NUMFORM_SE*/
    UI_NUMFORM_CLASS_FUNCTIONS_SOME = 7,
    UI_NUMFORM_CLASS_MROFMUN = 8,
    UI_NUMFORM_CLASS_LOAD_TEMPLATE = 9,
    UI_NUMFORM_CLASS_TIME = 10,

    UI_NUMFORM_CLASS_MAX
}
UI_NUMFORM_CLASS;

typedef struct UI_NUMFORM
{
    UI_NUMFORM_CLASS numform_class;
    P_USTR ustr_numform;    /* alloc_block_ustr_set() */
    P_USTR ustr_opt;        /* points to digits, ditto */
}
UI_NUMFORM, * P_UI_NUMFORM; typedef const UI_NUMFORM * PC_UI_NUMFORM;

typedef struct UI_LIST_ENTRY_STYLE
{
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, elemof32("A reasonable style")); /* NB buffer adjacent for fixup */
}
UI_LIST_ENTRY_STYLE, * P_UI_LIST_ENTRY_STYLE;

typedef struct UI_LIST_ENTRY_TYPEFACE
{
    /* HOST typeface */
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 16); /* NB buffer adjacent for fixup */
}
UI_LIST_ENTRY_TYPEFACE, * P_UI_LIST_ENTRY_TYPEFACE;

/*
handle to a dialog box
*/

#if defined(_WIN64)
typedef U32                H_DIALOG;
#define H_DIALOG_XTFMT U32_XTFMT
#else
typedef struct _H_DIALOG * H_DIALOG; /* actually just a 32-bit abstract handle */
#define H_DIALOG_XTFMT PTR_XTFMT
#endif

typedef struct BITMAP_VALIDATION_SBCHAR
{
    BITMAP(bitmap, 256); /* enough for SBCHAR character range */
}
BITMAP_VALIDATION_SBCHAR, * P_BITMAP_VALIDATION_SBCHAR; typedef const BITMAP_VALIDATION_SBCHAR * PC_BITMAP_VALIDATION_SBCHAR;

/*
exported routines
*/

_Check_return_
extern STATUS
ui_text_copy_as_sbstr(
    _OutRef_    P_P_SBSTR p_sbstr,
    _InRef_     PC_UI_TEXT p_src_ui_text);

extern void
ui_text_copy_as_sbstr_buf(
    _Out_writes_z_(buflen) P_SBSTR l1buf,
    _InVal_     U32 buflen,
    _InRef_     PC_UI_TEXT p_src_ui_text);

_Check_return_
extern S32
ui_bytes_per_item(
    _InVal_     UI_DATA_TYPE ui_data_type);

_Check_return_
extern STATUS
ui_data_bound_check(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InoutRef_  P_UI_DATA p_ui_data,
    /*in*/      const void /*UI_CONTROL*/ * const p_ui_control);

extern void
ui_data_dispose(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InoutRef_  P_UI_DATA p_ui_data);

_Check_return_
extern STATUS
ui_data_inc_dec(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InoutRef_  P_UI_DATA p_ui_data,
    /*in*/      const void /*UI_CONTROL*/ * const p_ui_control,
    _InVal_     BOOL inc);

_Check_return_
extern S32
ui_data_n_items_query(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InRef_     PC_UI_SOURCE p_ui_source);

_Check_return_
extern STATUS
ui_data_query(
    _InoutRef_  P_UI_DATA_TYPE p_ui_data_type,
    _InRef_     PC_UI_SOURCE p_ui_source,
    _InVal_     S32 itemno,
    _OutRef_    P_UI_DATA p_ui_data);

_Check_return_
extern STATUS
ui_data_query_as_text(
    _InVal_     UI_DATA_TYPE ui_data_type,
    _InRef_     PC_UI_SOURCE p_ui_source,
    _InVal_     S32 itemno,
    /*in*/      const void /*UI_CONTROL*/ * const p_ui_control,
    _OutRef_    P_UI_TEXT p_ui_text);

extern void
ui_list_size_estimate(
    _InVal_     S32 num_elements,
    _OutRef_    P_PIXIT_SIZE p_pixit_size);

_Check_return_
extern STATUS
ui_list_create_style(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source,
    _OutRef_    P_PIXIT p_max_width);

_Check_return_
extern STATUS
ui_list_create_typeface(
    _OutRef_    P_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source);

extern void
ui_list_dispose_style(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source);

extern void
ui_lists_dispose(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source);

extern void
ui_lists_dispose_tb(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source,
    _InVal_     S32 quick_tblock_offset /*-1 -> none*/);

extern void
ui_lists_dispose_ub(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle,
    _InoutRef_  P_UI_SOURCE p_ui_source,
    _InVal_     S32 quick_ublock_offset /*-1 -> none*/);

_Check_return_
extern STATUS
ui_source_create_tb(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source,
    _InVal_     UI_TEXT_TYPE ui_text_type,
    _InVal_     S32 quick_tblock_offset /*-1 -> none, 0th is ptr*/);

_Check_return_
extern STATUS
ui_source_create_ub(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _OutRef_    P_UI_SOURCE p_ui_source,
    _InVal_     UI_TEXT_TYPE ui_text_type,
    _InVal_     S32 quick_ublock_offset /*-1 -> none, 0th is ptr*/);

extern void
ui_source_dispose(
    _InoutRef_  P_UI_SOURCE p_ui_source);

extern void
ui_source_list_fixup_tb(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 quick_tblock_offset);

extern void
ui_source_list_fixup_ub(
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     S32 quick_ublock_offset);

extern void
ui_string_to_text(
    _DocuRef_   P_DOCU p_docu,
    _Inout_updates_z_(elemof_buffer) PTSTR tstr_buf /*appended*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR src);

_Check_return_
extern STATUS
ui_text_alloc_from_tstr(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _In_opt_z_  PCTSTR tstr);

_Check_return_
extern STATUS
ui_text_alloc_from_ustr(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _In_opt_z_  PC_USTR ustr);

_Check_return_
extern STATUS
ui_text_alloc_from_p_ev_string(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _InRef_opt_ PC_EV_STRINGC p_ev_string);

_Check_return_
extern BOOL
ui_text_is_blank(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */);

_Check_return_
extern int
ui_text_compare(
    _InRef_     PC_UI_TEXT p_ui_text_1,
    _InRef_     PC_UI_TEXT p_ui_text_2,
    _InVal_     BOOL fussy,
    _InVal_     BOOL insensitive);

_Check_return_
extern STATUS
ui_text_copy(
    _OutRef_    P_UI_TEXT p_dst_ui_text,
    _InRef_     PC_UI_TEXT p_src_ui_text);

extern void
ui_text_dispose(
    _InoutRef_  P_UI_TEXT p_ui_text);

_Check_return_
extern STATUS
ui_text_from_data(
    _InVal_     UI_DATA_TYPE ui_data_type,
    /*in*/      const UI_DATA * const p_ui_data,
    /*in*/      const void /*UI_CONTROL*/ * const p_ui_control,
    _OutRef_    P_UI_TEXT p_ui_text);

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR
ui_text_tstr(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */);

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern PCTSTR
ui_text_tstr_no_default(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */);

_Check_return_
_Ret_z_ /* never NULL */
extern PC_USTR
ui_text_ustr(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */);

_Check_return_
_Ret_maybenull_z_ /* may be NULL */
extern PC_USTR
ui_text_ustr_no_default(
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */);

_Check_return_
extern STATUS
ui_text_read(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended*/,
    _InRef_opt_ PC_UI_TEXT p_ui_text /* may be NULL */);

_Check_return_
extern U32
ui_text_validate(
    _InoutRef_  P_UI_TEXT p_ui_text,
    _InRef_opt_ PC_BITMAP p_bitmap_accept);

_Check_return_
extern PIXIT
ui_width_from_ustr(
    _In_z_      PC_USTR ustr);

_Check_return_
extern PIXIT
ui_width_from_tstr(
    _In_z_      PCTSTR tstr);

_Check_return_
extern PIXIT
ui_width_from_p_ui_text(
    _InRef_     PC_UI_TEXT p_ui_text);

_Check_return_
extern PIXIT
ui_width_from_tstr_host(
    _In_z_      PCTSTR tstr); /* actually in os_dlg --- eek! */

/*
sk_prost.c
*/

typedef UINT PROCESS_STATUS_TYPE;
#define PROCESS_STATUS_PERCENT 0
#define PROCESS_STATUS_TEXT 1

typedef struct PROCESS_STATUS_FLAGS
{
    UBF foreground : 1;
    UBF dialog     : 1;

    UBF reserved   : 8*sizeof(int) -1-1 -1-1-1-1;

    /* for process_status internal use only */
    UBF hourglass_bashed : 1;
    UBF initial_run_completed : 1;
    UBF status_line_written : 1;
    UBF dialog_is_hwnd : 1; /* Windows only */
}
PROCESS_STATUS_FLAGS;

typedef struct PROCESS_STATUS_DATA_PERCENT
{
    S32 current;
    S32 cutoff;
    S32 last;
}
PROCESS_STATUS_DATA_PERCENT;

typedef union PROCESS_STATUS_DATA
{
    PROCESS_STATUS_DATA_PERCENT percent;

    UI_TEXT text;
}
PROCESS_STATUS_DATA;

#if WINDOWS
#define WINDOWS_PROCESS_DIALOG_WINDOW 1
#else
#define WINDOWS_PROCESS_DIALOG_WINDOW 0
#endif

#if (WINDOWS && !WINDOWS_PROCESS_DIALOG_WINDOW) || (RISCOS && 0)

typedef struct PROCESS_STATUS_CONTROLS
{
    DIALOG_CONTROL_DATA_STATICTEXT reason_data;
    DIALOG_CONTROL                 reason;

    DIALOG_CONTROL_DATA_STATICTEXT status_data;
    DIALOG_CONTROL                 status;
}
PROCESS_STATUS_CONTROLS;

#endif /* OS etc. */

typedef struct PROCESS_STATUS
{
    PROCESS_STATUS_TYPE     type;
    PROCESS_STATUS_FLAGS    flags;
    PROCESS_STATUS_DATA     data;

    UI_TEXT                 reason;

    DOCNO                   docno;
    U8                      reserved_byte_padding[3];

#if WINDOWS && WINDOWS_PROCESS_DIALOG_WINDOW
    HWND                    hwnd;
#elif (WINDOWS && !WINDOWS_PROCESS_DIALOG_WINDOW) || (RISCOS && 0)
    H_DIALOG                h_dialog;
    PROCESS_STATUS_CONTROLS controls;
#else
    /*nothing*/
#endif

    MONOTIME                last_time;
    MONOTIMEDIFF            initial_run;
    MONOTIMEDIFF            granularity;
}
PROCESS_STATUS, * P_PROCESS_STATUS;

#define PROCESS_STATUS_INIT { PROCESS_STATUS_PERCENT, { 0 } }

typedef enum STATUS_LINE_LEVEL
{
    STATUS_LINE_LEVEL_INFORMATION_FOCUS_BASE = -0x2000, /* add your focus_owner object_id to this */
#define STATUS_LINE_LEVEL_INFORMATION_FOCUS(object_id) ( \
    (STATUS_LINE_LEVEL) (STATUS_LINE_LEVEL_INFORMATION_FOCUS_BASE + object_id))

    STATUS_LINE_LEVEL_INFORMATION_FOCUS_2_BASE = -0x1800, /* for ob_rec use only */
#define STATUS_LINE_LEVEL_INFORMATION_FOCUS_2(object_id) ( \
    (STATUS_LINE_LEVEL) (STATUS_LINE_LEVEL_INFORMATION_FOCUS_2_BASE + object_id))

    STATUS_LINE_LEVEL_SS_RECALC = -0x1000,

    STATUS_LINE_LEVEL_PROCESS_INFO = -0x0800,

    STATUS_LINE_LEVEL_PANEWINDOW_TRACKING = 0,

    STATUS_LINE_LEVEL_AUTO_CLEAR = 0x0800, /* cleared out on most user-initiated events */

    STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS = 0x1000,

    STATUS_LINE_LEVEL_DIALOG_CONTROLS = 0x2000,
    STATUS_LINE_LEVEL_DIALOG_INFO,

    STATUS_LINE_LEVEL_SS_FORMULA_LINE = 0x4000,

    STATUS_LINE_LEVEL_DRAGGING = 0x6000,

    STATUS_LINE_LEVEL_FUNCTION_SELECTOR = 0x6100,

    STATUS_LINE_LEVEL_DEBUG = 0x7000,

    STATUS_LINE_LEVEL_END
}
STATUS_LINE_LEVEL;

/*
exported functions
*/

extern void
fill_in_help_request(
    _DocuRef_   P_DOCU p_docu,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

extern void
process_status_begin(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_PROCESS_STATUS p_process_status,
    _In_        PROCESS_STATUS_TYPE type);

extern void
process_status_end(
    _InoutRef_  P_PROCESS_STATUS p_process_status);

static inline void
process_status_init(
    _OutRef_    P_PROCESS_STATUS p_process_status)
{
    zero_struct_ptr(p_process_status);
}

extern void
process_status_reflect(
    _InoutRef_  P_PROCESS_STATUS p_process_status);

extern void
status_line_auto_clear(
    _DocuRef_   P_DOCU p_docu);

extern void
status_line_clear(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS_LINE_LEVEL level);

extern void
status_line_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS_LINE_LEVEL level,
    _InRef_     PC_UI_TEXT p_ui_text);

extern void __cdecl
status_line_setf(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS_LINE_LEVEL level,
    _InVal_     STATUS status,
    /**/        ...);

/*
sk_name.c
*/

/*
name block
*/

typedef struct DOCU_NAME_FLAGS
{
    U8 path_name_supplied; /*UBF path_name_supplied : 1;*/
    U8 _spare[3];
}
DOCU_NAME_FLAGS;

typedef struct DOCU_NAME
{
    PTSTR leaf_name;
    PTSTR path_name;
    PTSTR extension;
    DOCU_NAME_FLAGS flags;
}
DOCU_NAME, * P_DOCU_NAME; typedef const DOCU_NAME * PC_DOCU_NAME;

/*
exported routines
*/

_Check_return_
extern int
name_compare(
    _InRef_     PC_DOCU_NAME p_docu_name1,
    _InRef_     PC_DOCU_NAME p_docu_name2,
    _InVal_     BOOL add_extension);

extern void
name_dispose(
    _InoutRef_  P_DOCU_NAME p_docu_name);

extern void
name_donate(
    _InoutRef_  P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in);

_Check_return_
extern STATUS
name_dup(
    _OutRef_    P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in);

_Check_return_
extern STATUS
name_ensure_path(
    _InoutRef_  P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in);

extern void
name_init(
    _OutRef_    P_DOCU_NAME p_docu_name);

_Check_return_
extern STATUS
name_make_wholename(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/,
    _InVal_     BOOL add_extension);

_Check_return_
extern STATUS /* number of chars read including delimiter */
name_read_ustr(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PC_USTR ustr_in,
    _In_        U8 delimiter /* in; 0=auto */);

_Check_return_
extern STATUS
name_read_tstr(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PCTSTR tstr_in);

/*ncr*/
extern BOOL
name_preprocess_docu_name_flags_for_rename(
    _InoutRef_  P_DOCU_NAME p_docu_name);

_Check_return_
extern STATUS
name_set_untitled(
    _OutRef_    P_DOCU_NAME p_docu_name);

_Check_return_
extern STATUS
name_set_untitled_with(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PCTSTR leafname);

_Check_return_
extern U32 /* number of characters output */
name_write_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer,
    _InoutRef_  P_DOCU_NAME p_docu_name_to /* name output */,
    _InRef_opt_ PC_DOCU_NAME p_docu_name_from,
    _InVal_     BOOL add_extension);

_Check_return_
static inline BOOL
name_is_blank(
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    return(
        (NULL == p_docu_name->path_name) &&
        (NULL == p_docu_name->leaf_name) ); /* don't consider extension here */
}

/*
sk_styl.c
*/

typedef S32 STYLE_HANDLE; typedef STYLE_HANDLE * P_STYLE_HANDLE;

#define STYLE_HANDLE_ENUM_START     ((STYLE_HANDLE) -1)
#define STYLE_HANDLE_NONE           ((STYLE_HANDLE) 0)

/* style style_selector switches */
typedef enum STYLE_BIT_NUMBER
{
    STYLE_BIT_NUMBER_ENUM_END = -1, /* from style_selector_next_bit() */

    STYLE_BIT_NUMBER_FIRST = 0, /* for explicit for(;;) loops */

    /* bw0 b0 */    STYLE_SW_NAME               = 0,    /* style name */
    /* bw0 b1 */    STYLE_SW_KEY                = 1,
    /* bw0 b2 */    STYLE_SW_HANDLE             = 2,
    /* bw0 b3 */    STYLE_SW_SEARCH             = 3,

    /* bw0 b4 */    STYLE_SW_CS_WIDTH           = 4,    /* column style */
    /* bw0 b5 */    STYLE_SW_CS_COL_NAME        = 5,

    /* bw0 b6 */    STYLE_SW_RS_HEIGHT          = 6,    /* row style */
    /* bw0 b7 */    STYLE_SW_RS_HEIGHT_FIXED    = 7,
    /* bw1 b0 */    STYLE_SW_RS_UNBREAKABLE     = 8,
    /* bw1 b1 */    STYLE_SW_RS_ROW_NAME        = 9,

    /* bw1 b2 */    STYLE_SW_PS_MARGIN_LEFT     = 10,   /* paragraph style */
    /* bw1 b3 */    STYLE_SW_PS_MARGIN_RIGHT    = 11,
    /* bw1 b4 */    STYLE_SW_PS_MARGIN_PARA     = 12,
    /* bw1 b5 */    STYLE_SW_PS_TAB_LIST        = 13,
    /* bw1 b6 */    STYLE_SW_PS_RGB_BACK        = 14,
    /* bw1 b7 */    STYLE_SW_PS_PARA_START      = 15,
    /* bw2 b0 */    STYLE_SW_PS_PARA_END        = 16,
    /* bw2 b1 */    STYLE_SW_PS_LINE_SPACE      = 17,
    /* bw2 b2 */    STYLE_SW_PS_JUSTIFY         = 18,
    /* bw2 b3 */    STYLE_SW_PS_JUSTIFY_V       = 19,
    /* bw2 b4 */    STYLE_SW_PS_NEW_OBJECT      = 20,
    /* bw2 b5 */    STYLE_SW_PS_NUMFORM_NU      = 21,
    /* bw2 b6 */    STYLE_SW_PS_NUMFORM_DT      = 22,
    /* bw2 b7 */    STYLE_SW_PS_NUMFORM_SE      = 23,
    /* bw3 b0 */    STYLE_SW_PS_RGB_BORDER      = 24,
    /* bw3 b1 */    STYLE_SW_PS_BORDER          = 25,
    /* bw3 b2 */    STYLE_SW_PS_RGB_GRID_LEFT   = 26,
    /* bw3 b3 */    STYLE_SW_PS_RGB_GRID_TOP    = 27,
    /* bw3 b4 */    STYLE_SW_PS_RGB_GRID_RIGHT  = 28,
    /* bw3 b5 */    STYLE_SW_PS_RGB_GRID_BOTTOM = 29,
    /* bw3 b6 */    STYLE_SW_PS_GRID_LEFT       = 30,
    /* bw3 b7 */    STYLE_SW_PS_GRID_TOP        = 31,
    /* bw4 b0 */    STYLE_SW_PS_GRID_RIGHT      = 32,
    /* bw4 b1 */    STYLE_SW_PS_GRID_BOTTOM     = 33,
    /* bw4 b2 */    STYLE_SW_PS_PROTECT         = 34,

    /* bw4 b3 */    STYLE_SW_FS_NAME            = 35,   /* font spec switches */
    /* bw4 b4 */    STYLE_SW_FS_SIZE_X          = 36,
    /* bw4 b5 */    STYLE_SW_FS_SIZE_Y          = 37,
    /* bw4 b6 */    STYLE_SW_FS_UNDERLINE       = 38,
    /* bw4 b7 */    STYLE_SW_FS_BOLD            = 39,
    /* bw5 b0 */    STYLE_SW_FS_ITALIC          = 40,
    /* bw5 b1 */    STYLE_SW_FS_SUPERSCRIPT     = 41,
    /* bw5 b2 */    STYLE_SW_FS_SUBSCRIPT       = 42,
    /* bw5 b3 */    STYLE_SW_FS_COLOUR          = 43,

    STYLE_SW_COUNT,

    STYLE_SW_COUNT_FULL = 64 /* rounded out to BITMAP_WORD for FAST_BITMAP_OPS below ... */
}
STYLE_BIT_NUMBER;

#define STYLE_BIT_NUMBER_INCR(style_bit_number__ref) \
    ENUM_INCR(STYLE_BIT_NUMBER, style_bit_number__ref)

typedef struct STYLE_SELECTOR
{
    BITMAP(bitmap, STYLE_SW_COUNT_FULL);
}
STYLE_SELECTOR, * P_STYLE_SELECTOR; typedef const STYLE_SELECTOR * PC_STYLE_SELECTOR;

/* DON'T add STYLE_SELECTOR_INIT as { { 0 } } - it gives poor code on ARM */

/* NB we have now built in explicit knowledge of the size of STYLE_SELECTOR */
#if defined(FAST_BITMAP_OPS_32)
#define VOID_STYLE_SELECTOR_BINOP(p_style_selector_result, pc_style_selector_in_1, pc_style_selector_in_2, _op_) \
    (p_style_selector_result)->bitmap[0] = ((pc_style_selector_in_1)->bitmap[0]) _op_ ((pc_style_selector_in_2)->bitmap[0]), \
    (p_style_selector_result)->bitmap[1] = ((pc_style_selector_in_1)->bitmap[1]) _op_ ((pc_style_selector_in_2)->bitmap[1])
#elif defined(FAST_BITMAP_OPS_64)
#define VOID_STYLE_SELECTOR_BINOP(p_style_selector_result, pc_style_selector_in_1, pc_style_selector_in_2, _op_) \
    (p_style_selector_result)->bitmap[0] = ((pc_style_selector_in_1)->bitmap[0]) _op_ ((pc_style_selector_in_2)->bitmap[0])
#endif

/*ncr*/
static inline BOOL
style_selector_and(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_result,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
    /* NB we have now built in explicit knowledge of the size of STYLE_SELECTOR */
#if defined(FAST_BITMAP_OPS_32)
    BOOL any_bits_set;
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    any_bits_set = (0 != (p_style_selector_result->bitmap[0] = p_style_selector_1->bitmap[0] & p_style_selector_2->bitmap[0]));
    if(0 != (p_style_selector_result->bitmap[1] = p_style_selector_1->bitmap[1] & p_style_selector_2->bitmap[1]))
        any_bits_set = TRUE;
    return(any_bits_set);
#elif defined(FAST_BITMAP_OPS_64)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    return((0 != (p_style_selector_result->bitmap[0] = p_style_selector_1->bitmap[0] & p_style_selector_2->bitmap[0])));
#else
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    return(bitmap_and(p_style_selector_result->bitmap, p_style_selector_1->bitmap, p_style_selector_2->bitmap, N_BITS_ARG(STYLE_SW_COUNT_FULL)));
#endif
}

static inline void
void_style_selector_and(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_result,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
#if defined(VOID_STYLE_SELECTOR_BINOP)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    VOID_STYLE_SELECTOR_BINOP(p_style_selector_result, p_style_selector_1, p_style_selector_2, &);
#else
    consume_bool(style_selector_and(p_style_selector_result, p_style_selector_1, p_style_selector_2));
#endif
}

_Check_return_
static inline BOOL
style_selector_any(
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    /* NB we have now built in explicit knowledge of the size of STYLE_SELECTOR */
#if defined(FAST_BITMAP_OPS_32)
    return(0 != (p_style_selector->bitmap[0] | p_style_selector->bitmap[1]));
#elif defined(FAST_BITMAP_OPS_64)
    return(0 != (p_style_selector->bitmap[0]));
#else
    return(bitmap_any(p_style_selector->bitmap, N_BITS_ARG(STYLE_SW_COUNT)));
#endif
}

/*ncr*/
static inline BOOL
style_selector_bic(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_result,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
    /* NB we have now built in explicit knowledge of the size of STYLE_SELECTOR */
#if defined(FAST_BITMAP_OPS_32)
    BOOL any_bits_set;
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    any_bits_set = (0 != (p_style_selector_result->bitmap[0] = p_style_selector_1->bitmap[0] &~ p_style_selector_2->bitmap[0]));
    if(0 != (p_style_selector_result->bitmap[1] = p_style_selector_1->bitmap[1] &~ p_style_selector_2->bitmap[1]))
        any_bits_set = TRUE;
    return(any_bits_set);
#elif defined(FAST_BITMAP_OPS_64)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    return((0 != (p_style_selector_result->bitmap[0] = p_style_selector_1->bitmap[0] &~ p_style_selector_2->bitmap[0])));
#else
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    return(bitmap_bic(p_style_selector_result->bitmap, p_style_selector_1->bitmap, p_style_selector_2->bitmap, N_BITS_ARG(STYLE_SW_COUNT_FULL)));
#endif
}

static inline void
void_style_selector_bic(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_result,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
#if defined(VOID_STYLE_SELECTOR_BINOP)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    VOID_STYLE_SELECTOR_BINOP(p_style_selector_result, p_style_selector_1, p_style_selector_2, &~);
#else
    consume_bool(style_selector_bic(p_style_selector_result, p_style_selector_1, p_style_selector_2));
#endif
}

static inline void
style_selector_bit_clear(
    _InoutRef_  P_STYLE_SELECTOR p_style_selector,
    _InVal_     U32 style_bit_number)
{
    bitmap_bit_clear(p_style_selector->bitmap, style_bit_number, N_BITS_ARG(STYLE_SW_COUNT));
}

static inline void
style_selector_bit_copy(
    _InoutRef_  P_STYLE_SELECTOR p_style_selector_mod,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     U32 style_bit_number)
{
    bitmap_bit_copy(p_style_selector_mod->bitmap, p_style_selector->bitmap, style_bit_number, N_BITS_ARG(STYLE_SW_COUNT));
}

static inline void
style_selector_bit_set(
    _InoutRef_  P_STYLE_SELECTOR p_style_selector,
    _InVal_     U32 style_bit_number)
{
    bitmap_bit_set(p_style_selector->bitmap, style_bit_number, N_BITS_ARG(STYLE_SW_COUNT));
}

_Check_return_
static inline BOOL
style_selector_bit_test(
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     U32 style_bit_number)
{
    return(bitmap_bit_test(p_style_selector->bitmap, style_bit_number, N_BITS_ARG(STYLE_SW_COUNT)));
}

static inline void
style_selector_clear(
    _OutRef_    P_STYLE_SELECTOR p_style_selector)
{
    /* NB we have now built in explicit knowledge of the size of STYLE_SELECTOR */
#if defined(FAST_BITMAP_OPS_32)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector)); /* VC2008 /analyze stubborn whinge */
    p_style_selector->bitmap[0] = 0;
    p_style_selector->bitmap[1] = 0;
#elif defined(FAST_BITMAP_OPS_64)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector)); /* VC2008 /analyze stubborn whinge */
    p_style_selector->bitmap[0] = 0;
#else
    bitmap_clear(p_style_selector->bitmap, N_BITS_ARG(STYLE_SW_COUNT_FULL));
#endif
}

_Check_return_
static inline BOOL
style_selector_compare(
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
    return(bitmap_compare(p_style_selector_1->bitmap, p_style_selector_2->bitmap, N_BITS_ARG(STYLE_SW_COUNT)));
}

static inline void
style_selector_copy(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_out,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_in)
{
    /* NB we have now built in explicit knowledge of the size of STYLE_SELECTOR */
#if defined(FAST_BITMAP_OPS_32)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_out)); /* VC2008 /analyze stubborn whinge */
    p_style_selector_out->bitmap[0] = p_style_selector_in->bitmap[0];
    p_style_selector_out->bitmap[1] = p_style_selector_in->bitmap[1];
#elif defined(FAST_BITMAP_OPS_64)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_out)); /* VC2008 /analyze stubborn whinge */
    p_style_selector_out->bitmap[0] = p_style_selector_in->bitmap[0];
#else
    bitmap_copy(p_style_selector_out->bitmap, p_style_selector_in->bitmap, N_BITS_ARG(STYLE_SW_COUNT_FULL));
#endif
}

_Check_return_
static inline STYLE_BIT_NUMBER /* <0 no more bits set */
style_selector_next_bit(
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     U32 style_bit_number)
{
    return((STYLE_BIT_NUMBER) bitmap_next_bit(p_style_selector->bitmap, style_bit_number, N_BITS_ARG(STYLE_SW_COUNT)));
}

_Check_return_
static inline STYLE_BIT_NUMBER /* <0 no more bits set */
style_selector_next_bit_in_both(
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2,
    _InVal_     U32 style_bit_number)
{
    return((STYLE_BIT_NUMBER) bitmap_next_bit_in_both(p_style_selector_1->bitmap, p_style_selector_2->bitmap, style_bit_number, N_BITS_ARG(STYLE_SW_COUNT)));
}

static inline void
void_style_selector_not(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_result,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    bitmap_not(p_style_selector_result->bitmap, p_style_selector->bitmap, N_BITS_ARG(STYLE_SW_COUNT));
}

static inline void
void_style_selector_or(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_result,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
#if defined(VOID_STYLE_SELECTOR_BINOP)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    VOID_STYLE_SELECTOR_BINOP(p_style_selector_result, p_style_selector_1, p_style_selector_2, |);
#else
    bitmap_or(p_style_selector_result->bitmap, p_style_selector_1->bitmap, p_style_selector_2->bitmap, N_BITS_ARG(STYLE_SW_COUNT));
#endif
}

_Check_return_
static inline BOOL
style_selector_test(
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
    return(bitmap_test(p_style_selector_1->bitmap, p_style_selector_2->bitmap, N_BITS_ARG(STYLE_SW_COUNT)));
}

static inline void
void_style_selector_xor(
    _OutRef_    P_STYLE_SELECTOR p_style_selector_result,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_1,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_2)
{
#if defined(VOID_STYLE_SELECTOR_BINOP)
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style_selector_result)); /* VC2008 /analyze stubborn whinge */
    VOID_STYLE_SELECTOR_BINOP(p_style_selector_result, p_style_selector_1, p_style_selector_2, ^);
#else
    bitmap_xor(p_style_selector_result->bitmap, p_style_selector_1->bitmap, p_style_selector_2->bitmap, N_BITS_ARG(STYLE_SW_COUNT));
#endif
}

#define COL_CHANGE_THRESHOLD 9

/*
column style
*/

typedef struct COL_STYLE
{
    PIXIT width;
    ARRAY_HANDLE h_numform;
}
COL_STYLE, * P_COL_STYLE;

/*
row style
*/

typedef struct ROW_STYLE
{
    PIXIT height;
    U8 height_fixed;
    U8 unbreakable;
    U8 _spare[2];
    ARRAY_HANDLE h_numform;
}
ROW_STYLE, * P_ROW_STYLE;

/*
line spacing
*/

typedef struct LINE_SPACE
{
    S32 type;
    PIXIT leading;
}
LINE_SPACE, * P_LINE_SPACE;

/*
paragraph style
*/

typedef struct PARA_STYLE
{
    PIXIT margin_left;
    PIXIT margin_right;
    PIXIT margin_para;
    ARRAY_HANDLE h_tab_list;
    RGB rgb_back;
    PIXIT para_start;
    PIXIT para_end;
    LINE_SPACE line_space;
    ARRAY_HANDLE_USTR h_numform_nu;
    ARRAY_HANDLE_USTR h_numform_dt;
    ARRAY_HANDLE_USTR h_numform_se;
    RGB rgb_border;
    RGB rgb_grid_left;
    RGB rgb_grid_top;
    RGB rgb_grid_right;
    RGB rgb_grid_bottom;
    U8 border;
    U8 grid_left;                       /* left grid */
    U8 grid_top;                        /* top grid */
    U8 grid_right;                      /* right grid */
    U8 grid_bottom;                     /* bottom grid */
    U8 protect;
    U8 justify;
    U8 justify_v;
    U8 new_object;
    U8 _spare[3];
}
PARA_STYLE, * P_PARA_STYLE;

/*
style record
*/

typedef struct STYLE
{
    STYLE_SELECTOR selector;            /* selector bitmap */
    ARRAY_HANDLE_TSTR h_style_name_tstr; /* name of style: [] of TCHAR, CH_NULL-terminated  */
    S32 style_key;                      /* control key shortcut */
    STYLE_HANDLE style_handle;          /* indirection to another style */
    S32 search;                         /* order for style searches */
    COL_STYLE col_style;                /* column style */
    ROW_STYLE row_style;                /* row style */
    PARA_STYLE para_style;              /* paragraph style */
    FONT_SPEC font_spec;                /* font specification */
}
STYLE, * P_STYLE, ** P_P_STYLE; typedef const STYLE * PC_STYLE;

#define    P_STYLE_NONE             _P_DATA_NONE(P_STYLE)
#define IS_P_STYLE_NONE(p_style)     IS_PTR_NONE(p_style)

static inline void
style_bit_set(
    _InoutRef_  P_STYLE p_style,
    _InVal_     U32 style_bit_number)
{
    style_selector_bit_set(&p_style->selector, style_bit_number);
}

_Check_return_
static inline BOOL
style_bit_test(
    _InRef_     PC_STYLE p_style,
    _InVal_     U32 style_bit_number)
{
    return(style_selector_bit_test(&p_style->selector, style_bit_number));
}

/* special purposes */
enum SF_SPECIAL
{
    SF_SPECIAL_NONE = 0,
    SF_SPECIAL_BASE,
    SF_SPECIAL_CURRENT,
    SF_SPECIAL_HEFO,
    SF_SPECIAL_TEXT,

    SF_SPECIAL_COUNT
};

/* justify type */
enum SF_JUSTIFY
{
    SF_JUSTIFY_LEFT = 0,
    SF_JUSTIFY_CENTRE,
    SF_JUSTIFY_RIGHT,
    SF_JUSTIFY_BOTH,

    SF_JUSTIFY_V_TOP    = SF_JUSTIFY_LEFT,
    SF_JUSTIFY_V_CENTRE = SF_JUSTIFY_CENTRE,
    SF_JUSTIFY_V_BOTTOM = SF_JUSTIFY_RIGHT,
};

/* line_space type */
enum SF_LINE_SPACE
{
    SF_LINE_SPACE_SINGLE = 0,
    SF_LINE_SPACE_ONEP5,
    SF_LINE_SPACE_DOUBLE,
    SF_LINE_SPACE_SET
};

/* border type */

#define SF_BORDER_NONE     0U
#define SF_BORDER_THIN     1U
#define SF_BORDER_STANDARD 2U
#define SF_BORDER_BROKEN   3U
#define SF_BORDER_THICK    4U
#define SF_BORDER_COUNT    5U

/*
slr style cache entry
*/

typedef struct STYLE_CACHE_SLR_ENTRY
{
    SLR slr;
    STYLE style;
    U8 used;
}
STYLE_CACHE_SLR_ENTRY, * P_STYLE_CACHE_SLR_ENTRY;

/*
style info stored in regions
*/

enum REGION_CLASS_E
{
    REGION_INTERNAL = 0,
    REGION_BASE = 10,
    REGION_LOWER = 20,
    REGION_MAIN = 30,
    REGION_UPPER = 40,
    REGION_END
};

typedef U8 REGION_CLASS; /* packed */

typedef struct STYLE_DOCU_AREA
{
    DOCU_AREA docu_area;                /* region of style effects */
    UREF_HANDLE uref_handle;            /* UREF's handle to us */
    CLIENT_HANDLE client_handle;        /* our handle to UREF */
    STYLE_HANDLE style_handle;          /* indirection to style... */
    P_STYLE p_style_effect;             /* pointer to style for effect */
    OBJECT_MESSAGE object_message;      /* object/message pair for implied styles */
    S32 arg;                            /* argument for implied styles */
    DATA_SPACE data_space;              /* data space of docu_area */
    REGION_CLASS region_class;          /* class for region */
    U8 deleted;                         /* area is deleted and useless */
    U8 internal;                        /* area is internal; do not delete or save */
    U8 base;                            /* area is the base region; do not delete */
    U8 column_zero_width;               /* special bit to indicate the zero width column region in text documents */
    U8 caret;                           /* docu_area applied at caret */
    U8 uref_hold;                       /* temporary uref hold flag */
}
STYLE_DOCU_AREA, * P_STYLE_DOCU_AREA; typedef const STYLE_DOCU_AREA * PC_STYLE_DOCU_AREA;

/*
SKS created parameter block for style_docu_area_add[_reformat]()
*/

typedef union STYLE_DOCU_AREA_ADD_PARM_DATA
{
    P_STYLE         p_style;
    PC_USTR_INLINE  ustr_inline;
    STYLE_HANDLE    style_handle;
}
STYLE_DOCU_AREA_ADD_PARM_DATA;

typedef enum STYLE_DOCU_AREA_ADD_PARM_TYPE
{
    STYLE_DOCU_AREA_ADD_TYPE_STYLE = 0,
    STYLE_DOCU_AREA_ADD_TYPE_INLINE,
    STYLE_DOCU_AREA_ADD_TYPE_HANDLE,
    STYLE_DOCU_AREA_ADD_TYPE_IMPLIED
}
STYLE_DOCU_AREA_ADD_PARM_TYPE;

typedef struct STYLE_DOCU_AREA_ADD_PARM
{
    STYLE_DOCU_AREA_ADD_PARM_DATA data;

    STYLE_DOCU_AREA_ADD_PARM_TYPE type;

    P_ARRAY_HANDLE p_array_handle; /* NULL -> add to docu's list, otherwise another object's list */

    S32 object_id;
    T5_MESSAGE t5_message;
    S32 arg;

    U8 base;
    U8 internal;
    U8 caret;
    DATA_SPACE data_space;
    REGION_CLASS region_class;
    U8 add_without_subsume; /* SKS 31jan97 */
}
STYLE_DOCU_AREA_ADD_PARM, * P_STYLE_DOCU_AREA_ADD_PARM;

#define STYLE_DOCU_AREA_ADD_STYLE(p_style_docu_area_add_parm, p_style_i) \
{ \
    (p_style_docu_area_add_parm)->data.p_style          = (p_style_i); \
    (p_style_docu_area_add_parm)->type                  = STYLE_DOCU_AREA_ADD_TYPE_STYLE; \
    (p_style_docu_area_add_parm)->p_array_handle        = NULL; \
    (p_style_docu_area_add_parm)->base                  = 0; \
    (p_style_docu_area_add_parm)->internal              = 0; \
    (p_style_docu_area_add_parm)->caret                 = 0; \
    (p_style_docu_area_add_parm)->data_space            = DATA_SLOT; \
    (p_style_docu_area_add_parm)->object_id             = OBJECT_ID_NONE; \
    (p_style_docu_area_add_parm)->t5_message            = T5_EVENT_NONE; \
    (p_style_docu_area_add_parm)->arg                   = 0; \
    (p_style_docu_area_add_parm)->region_class          = REGION_MAIN; \
    (p_style_docu_area_add_parm)->add_without_subsume   = 0; \
}

#define STYLE_DOCU_AREA_ADD_INLINE(p_style_docu_area_add_parm, ustr_inline_i) \
{ \
    (p_style_docu_area_add_parm)->data.ustr_inline      = (ustr_inline_i); \
    (p_style_docu_area_add_parm)->type                  = STYLE_DOCU_AREA_ADD_TYPE_INLINE; \
    (p_style_docu_area_add_parm)->p_array_handle        = NULL; \
    (p_style_docu_area_add_parm)->base                  = 0; \
    (p_style_docu_area_add_parm)->internal              = 0; \
    (p_style_docu_area_add_parm)->caret                 = 0; \
    (p_style_docu_area_add_parm)->data_space            = DATA_SLOT; \
    (p_style_docu_area_add_parm)->object_id             = OBJECT_ID_NONE; \
    (p_style_docu_area_add_parm)->t5_message            = T5_EVENT_NONE; \
    (p_style_docu_area_add_parm)->arg                   = 0; \
    (p_style_docu_area_add_parm)->region_class          = REGION_MAIN; \
    (p_style_docu_area_add_parm)->add_without_subsume   = 0; \
}

#define STYLE_DOCU_AREA_ADD_HANDLE(p_style_docu_area_add_parm, style_handle_i) \
{ \
    (p_style_docu_area_add_parm)->data.style_handle     = (style_handle_i); \
    (p_style_docu_area_add_parm)->type                  = STYLE_DOCU_AREA_ADD_TYPE_HANDLE; \
    (p_style_docu_area_add_parm)->p_array_handle        = NULL; \
    (p_style_docu_area_add_parm)->base                  = 0; \
    (p_style_docu_area_add_parm)->internal              = 0; \
    (p_style_docu_area_add_parm)->caret                 = 0; \
    (p_style_docu_area_add_parm)->data_space            = DATA_SLOT; \
    (p_style_docu_area_add_parm)->object_id             = OBJECT_ID_NONE; \
    (p_style_docu_area_add_parm)->t5_message            = T5_EVENT_NONE; \
    (p_style_docu_area_add_parm)->arg                   = 0; \
    (p_style_docu_area_add_parm)->region_class          = REGION_MAIN; \
    (p_style_docu_area_add_parm)->add_without_subsume   = 0; \
}

#define STYLE_DOCU_AREA_ADD_IMPLIED(p_style_docu_area_add_parm, ustr_inline_i, object_id_i, t5_message_i, arg_i, region_class_i) \
{ \
    (p_style_docu_area_add_parm)->data.ustr_inline      = (ustr_inline_i); \
    (p_style_docu_area_add_parm)->type                  = STYLE_DOCU_AREA_ADD_TYPE_INLINE; \
    (p_style_docu_area_add_parm)->p_array_handle        = NULL; \
    (p_style_docu_area_add_parm)->base                  = 0; \
    (p_style_docu_area_add_parm)->internal              = 0; \
    (p_style_docu_area_add_parm)->caret                 = 0; \
    (p_style_docu_area_add_parm)->data_space            = DATA_SLOT; \
    (p_style_docu_area_add_parm)->object_id             = object_id_i; \
    (p_style_docu_area_add_parm)->t5_message            = t5_message_i; \
    (p_style_docu_area_add_parm)->arg                   = (U8) arg_i; \
    (p_style_docu_area_add_parm)->region_class          = region_class_i; \
    (p_style_docu_area_add_parm)->add_without_subsume   = 0; \
}

typedef struct STYLE_SUB_CHANGE
{
    POSITION position;
    STYLE style;
}
STYLE_SUB_CHANGE, * P_STYLE_SUB_CHANGE;

/*
object sub position update
*/

typedef struct OBJECT_POSITION_UPDATE
{
    DATA_REF data_ref;
    OBJECT_POSITION object_position;
    S32 data_update;
}
OBJECT_POSITION_UPDATE, * P_OBJECT_POSITION_UPDATE; typedef const OBJECT_POSITION_UPDATE * PC_OBJECT_POSITION_UPDATE;

/*
style use removal
*/

typedef struct STYLE_USE_REMOVE
{
    STYLE_HANDLE style_handle;
    ROW row;
}
STYLE_USE_REMOVE, * P_STYLE_USE_REMOVE;

/*
passed as data for T5_MSG_STYLE_USE_QUERY
*/

typedef struct STYLE_USE_QUERY
{
    DOCU_AREA docu_area;
    U8 data_class;
    REGION_CLASS region_class;
    U8 reserved_byte_padding[2];

    ARRAY_HANDLE h_style_use; /* which is an array of ... */
}
STYLE_USE_QUERY, * P_STYLE_USE_QUERY;

typedef struct STYLE_USE_QUERY_ENTRY
{
    STYLE_HANDLE style_handle;
    S32 use;
}
STYLE_USE_QUERY_ENTRY, * P_STYLE_USE_QUERY_ENTRY;

typedef BOOL (* P_PROC_POSITION_COMPARE) (
    _InRef_     PC_DOCU_AREA p_docu_area, \
    _InRef_     PC_POSITION p_position);

#define PROC_POSITION_COMPARE_PROTO(_proc_name) \
extern BOOL \
_proc_name( \
    _InRef_     PC_DOCU_AREA p_docu_area, \
    _InRef_     PC_POSITION p_position)

/*
exported routines
*/

#define p_style_from_docu_area(p_docu, p_style_docu_area) ( (P_STYLE)       \
    ((p_style_docu_area)->style_handle                                      \
        ? p_style_from_handle(p_docu, (p_style_docu_area)->style_handle)    \
        : (p_style_docu_area)->p_style_effect)                              )

#define p_style_from_handle(p_docu, handle) \
    array_ptr(&(p_docu)->h_style_list, STYLE, (handle))

_Check_return_
extern STATUS
style_apply_struct_to_source(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE p_style);

_Check_return_
extern BOOL
style_at_or_above_class(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area,
    _In_        REGION_CLASS region_class);

_Check_return_
extern ARRAY_HANDLE
style_change_between_cols(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col_s,
    _InVal_     COL col_e,
    _InVal_     ROW row,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

_Check_return_
extern ARRAY_HANDLE
style_change_between_rows(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row_start,
    _InVal_     ROW row_end,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

extern void
style_copy_defaults(
    _DocuRef_   PC_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

_Check_return_
extern PIXIT
style_default_measurement(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     STYLE_BIT_NUMBER style_bit_number);

static inline void
style_dispose(
    _InoutRef_  P_STYLE p_style)
{
    style_selector_clear(&(p_style)->selector);
}

_Check_return_
extern STATUS
style_docu_area_add(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_AREA p_docu_area,
    P_STYLE_DOCU_AREA_ADD_PARM p_style_docu_area_add_parm);

_Check_return_
extern STATUS
style_docu_area_add_base(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_array_handle,
    _InVal_     STYLE_HANDLE style_handle,
    _In_        DATA_SPACE data_space);

_Check_return_
extern STATUS
style_docu_area_add_internal(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_array_handle,
    _In_        DATA_SPACE data_space);

enum STYLE_DOCU_AREA_CHOOSE_ACTION
{
    STYLE_CHOOSE_DOWN,
    STYLE_CHOOSE_UP,
    STYLE_CHOOSE_LEAVE,
    STYLE_CHOOSE_COUNT
};

_Check_return_
extern S32 /* index of chosen region; -1 == end */
style_docu_area_choose(
    _InRef_     PC_ARRAY_HANDLE p_h_style_list,
    _InRef_opt_ PC_DOCU_AREA p_docu_area,
    _InRef_opt_ PC_POSITION p_position /* if p_docu_area == NULL*/,
    _InVal_     enum STYLE_DOCU_AREA_CHOOSE_ACTION action,
    _InVal_     S32 index /* -1 to start at either end */);

extern void
style_docu_area_clear(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InVal_     DATA_SPACE data_space);

_Check_return_
extern S32
style_docu_area_count(
    _InRef_     PC_ARRAY_HANDLE p_h_style_list,
    _InRef_     PC_DOCU_AREA p_docu_area);

extern void
style_docu_area_delete(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE_DOCU_AREA p_style_docu_area);

_Check_return_
_Ret_maybenull_
extern P_STYLE_DOCU_AREA /* NULL == didn't find one */
style_docu_area_enum_implied(
    _DocuRef_   P_DOCU p_docu,
    /*inout*/ P_ARRAY_INDEX p_array_index /* -1 to start */,
    _InVal_     OBJECT_ID object_id,
    _InRef_opt_ PC_T5_MESSAGE p_t5_message /* NULL==don't care */,
    _InRef_opt_ PC_S32 p_arg /* NULL==don't care */);

extern void
style_docu_area_delete_list(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InVal_     S32 all);

extern void
style_docu_area_position_update(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_OBJECT_POSITION_UPDATE p_object_position_update);

extern void
style_docu_area_remove(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list /*data modified*/,
    _InVal_     ARRAY_INDEX ix);

_Check_return_
extern BOOL
style_docu_area_selector_and(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE_SELECTOR p_style_selector_out,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_in);

extern void
style_docu_area_uref_hold(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_SLR p_slr);

extern void
style_docu_area_uref_release(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InRef_     PC_SLR p_slr,
    _InVal_     OBJECT_ID object_id /* new object id to use */);

_Check_return_
_Ret_maybenull_
extern P_STYLE_DOCU_AREA /* pointer to structure to modify */
style_effect_source_find(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list,
    _InVal_     BOOL implied_ok);

extern void
style_effect_source_modify(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list,
    _InRef_     PC_POSITION p_position,
    _In_reads_opt_c_(INLINE_OVH) PC_USTR_INLINE ustr_inline_in,
    _InRef_opt_ P_STYLE p_style_in);

_Check_return_
extern STYLE_HANDLE /* new index out */
style_enum_styles(
    _DocuRef_   PC_DOCU p_docu,
    _OutRef_    P_P_STYLE p_p_style,
    _InoutRef_  P_STYLE_HANDLE p_style_handle /* -1 to start enum */);

extern void
style_free_resources(
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

static inline void
style_free_resources_all(
    _InoutRef_  P_STYLE p_style)
{
    style_free_resources(p_style, &p_style->selector);
}

extern void
style_from_col(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     COL col);

extern void
style_from_docu_area_no_indirection(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style_out,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area);

extern void
style_from_position(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InRef_     P_PROC_POSITION_COMPARE p_proc_position_compare,
    _InVal_     BOOL caret_check);

extern void
style_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     ROW row);

extern void
style_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern STATUS /* style handle */
style_handle_add(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style); /* always either stolen or discarded */

_Check_return_
extern STYLE_HANDLE
style_handle_base(
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area);

/* ***DO NOT USE THIS ROUTINE*** */

_Check_return_
extern STYLE_HANDLE
style_handle_current(
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area);

_Check_return_
extern STATUS
style_handle_find_in_docu_area_list(
    _OutRef_    P_ROW p_row,
    _InRef_     PC_ARRAY_HANDLE p_h_docu_area_list,
    _InVal_     STYLE_HANDLE style_handle);

_Check_return_
extern STYLE_HANDLE
style_handle_from_name(
    _DocuRef_   PC_DOCU p_docu,
    _In_z_      PCTSTR name);

extern void
style_handle_modify(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle                     /* handle to modify */,
    _InoutRef_  P_STYLE p_style_new                           /* data for modified effects */,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_modified   /* selector for data for modified effects */,
    _InRef_     PC_STYLE_SELECTOR p_style_selector            /* modified selector */);

extern void
style_handle_remove(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle);

_Check_return_
extern ARRAY_INDEX
style_handle_use_find(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle,
    _InRef_     PC_POSITION p_position,
    _InRef_     PC_ARRAY_HANDLE p_h_style_list);

_Check_return_ _Success_(return)
extern BOOL
style_implied_query(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style_out,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area,
    _InRef_     PC_POSITION p_position);

static inline void
style_init(
    _OutRef_    P_STYLE p_style)
{   /* NB selector determines what else is valid here, so technically _Inout_ - apply CODE_ANALYSIS fudge */
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_style));
    style_selector_clear(&p_style->selector);
}

_Check_return_
extern PIXIT
style_leading_from_style(
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_FONT_SPEC p_font_spec,
    _InVal_     BOOL draft_mode);

extern void
style_of_area(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_STYLE p_style_out,
    _OutRef_    P_STYLE_SELECTOR p_style_selector_fuzzy_out,
    _InRef_     PC_STYLE_SELECTOR p_style_selector_in,
    P_DOCU_AREA p_docu_area_in);

extern void
style_region_class_limit_set(
    _DocuRef_   P_DOCU p_docu,
    _In_        REGION_CLASS region_class);

_Check_return_
extern BOOL
style_save_docu_area_save_from_index(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_DOCU_AREA p_style_docu_area,
    _InRef_     PC_DOCU_AREA p_docu_area_save,
    _In_        UBF save_index);

extern void
style_struct_from_handle(
    _DocuRef_   PC_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InVal_     STYLE_HANDLE style_handle,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

_Check_return_
extern ARRAY_HANDLE
style_sub_changes(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_OBJECT_POSITION p_object_position,
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

extern void
style_use_query(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_style_docu_area,
    P_STYLE_USE_QUERY p_style_use_query);

/*ncr*/
extern ROW
style_use_remove(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_style_docu_area /*data modified*/,
    _InVal_     STYLE_HANDLE style_handle);

#define p_style_member(__base_type, p_style, style_bit_number) ( \
    PtrAddBytes(__base_type *, (p_style), style_exported_info_table[style_bit_number].member_offset) )

/*
exported data
*/

extern STYLE_SELECTOR style_selector_all;
extern STYLE_SELECTOR style_selector_font_spec;
extern STYLE_SELECTOR style_selector_para_leading;
extern STYLE_SELECTOR style_selector_para_text;
/* extern STYLE_SELECTOR style_selector_caret; */

extern STYLE_SELECTOR style_selector_para_ruler;
extern STYLE_SELECTOR style_selector_para_format;
extern STYLE_SELECTOR style_selector_para_border;
extern STYLE_SELECTOR style_selector_para_grid;

extern STYLE_SELECTOR style_selector_col;
extern STYLE_SELECTOR style_selector_row;
extern STYLE_SELECTOR style_selector_numform;

typedef struct STYLE_EXPORTED_INFO
{
    U32 member_size;
    U32 member_offset;
}
STYLE_EXPORTED_INFO; typedef const STYLE_EXPORTED_INFO * PC_STYLE_EXPORTED_INFO;

extern const STYLE_EXPORTED_INFO style_exported_info_table[STYLE_SW_COUNT];

/*
sk_stylc.c
*/

_Check_return_
extern BOOL /*any diffs?*/
style_compare(
    _OutRef_    P_STYLE_SELECTOR p_style_selector /* bit set for different fields */,
    _InRef_     PC_STYLE p_style_1,
    _InRef_     PC_STYLE p_style_2);

extern void
style_copy(
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

static inline void
style_copy_into(
    _OutRef_    P_STYLE p_style_out,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{   /* SKS 21apr95 a very fast style_copy for cases where we are setting up a STYLE for the first time, not adding to one */
    memcpy32(p_style_out, p_style_in, sizeof32(*p_style_in));
    void_style_selector_and(&p_style_out->selector, &p_style_in->selector, p_style_selector);
}

_Check_return_
extern STATUS
style_duplicate(
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

_Check_return_
extern STATUS /* size out */
style_ustr_inline_from_struct(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InRef_     PC_STYLE p_style);

_Check_return_
extern STATUS /* length processed */
style_struct_from_ustr_inline(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _In_reads_c_(INLINE_OVH) PC_USTR_INLINE ustr_inline,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

_Check_return_
extern STATUS
style_text_convert(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

/*
of_table.h
*/

/*
Object construct table
*/

typedef struct CONSTRUCT_TABLE_BITS
{
    UBF reject_if_file_insertion        : 1;
    UBF reject_if_template_insertion    : 1;
    UBF maybe_interactive               : 1;
    UBF unrecordable                    : 1;
    UBF unrepeatable                    : 1;
    UBF exceptional                     : 1;
    UBF modify_doc                      : 1; /* modifies document if not interactive */
    UBF ensure_frothy                   : 1;
    UBF supress_newline                 : 1; /* for save */
    UBF check_protect                   : 1;
    UBF send_via_maeve_service          : 1;
    UBF wrap_with_before_after          : 1;
    UBF send_to_focus_owner             : 1;

    UBF reserved                        : 8*sizeof(int) - 13*1 - 8;

    UBF construct_name_length           : 8; /* filled at run-time (during registration) */
}
CONSTRUCT_TABLE_BITS;

typedef struct CONSTRUCT_TABLE
{
    PC_A7STR a7str_construct_name;

    PC_ARG_TYPE args;

    T5_MESSAGE t5_message;

    CONSTRUCT_TABLE_BITS bits;
}
CONSTRUCT_TABLE, * P_CONSTRUCT_TABLE; typedef const CONSTRUCT_TABLE * PC_CONSTRUCT_TABLE; typedef PC_CONSTRUCT_TABLE * P_PC_CONSTRUCT_TABLE;

/* terminated by entry with a7str_construct_name == NULL */

/*
Request to object for pointer to its construct table
*/

typedef struct CONSTRUCT_TABLE_RQ
{
    /*OUT*/
    PC_CONSTRUCT_TABLE p_construct_table;
}
CONSTRUCT_TABLE_RQ, * P_CONSTRUCT_TABLE_RQ;

/*
of_load.c
*/

#define OF_ESCAPE_CHAR                  CH_BACKWARDS_SLASH /* \ */
#define OF_ESCAPE_STR                   "\\"

#define OF_CONSTRUCT_START              CH_LEFT_CURLY_BRACKET /* { */
#define OF_CONSTRUCT_START_STR          "{"
#define OF_CONSTRUCT_END                CH_RIGHT_CURLY_BRACKET /* } */
#define OF_CONSTRUCT_END_STR            "}"

#define OF_CONSTRUCT_OBJECT_SEPARATOR   CH_HYPHEN_MINUS /* - */

#define OF_CONSTRUCT_ARGLIST_SEPARATOR  CH_COLON /* : */

#define OF_CONSTRUCT_ARG_SEPARATOR      CH_SEMICOLON /* ; */
#define OF_CONSTRUCT_ARG_SEPARATOR_STR  ";"

typedef enum IO_TYPE
{
    IO_TYPE_AUTO        = 0,    /* this is only applicable for load! */
    IO_TYPE_LATIN_1     = 1,
    IO_TYPE_LATIN_2     = 2,
    IO_TYPE_LATIN_3     = 3,
    IO_TYPE_LATIN_4     = 4,
    IO_TYPE_LATIN_9     = 9,

    IO_TYPE_UTF_32LE    = 32,
    IO_TYPE_UTF_32BE    = 33,
    IO_TYPE_UTF_16LE    = 34,
    IO_TYPE_UTF_16BE    = 35,
    IO_TYPE_UTF_8       = 36,   /* e.g. XLS XML */

    IO_TYPE_ASCII       = 64,   /* e.g. RTF */
    IO_TYPE_TRANSLATE_8 = 65    /* e.g. EBCDIC 8-bit plain text I/O */
}
IO_TYPE;

typedef enum IP_INPUT_STATE
{
    IP_INPUT_INVALID    = 0,
    IP_INPUT_FILE       = 1,
    IP_INPUT_MEM        = 2/*,
    IP_INPUT_STG        = 3*/
}
IP_INPUT_STATE;

#define OF_DATA_MAX     255
#define OF_DATA_MAX_BUF 256

typedef S32 OPTION_ID;

#define OWNFORM_DATA_TYPE_TEXT      0
#define OWNFORM_DATA_CHAR_TEXT     'X'

#define OWNFORM_DATA_TYPE_DATE      1
#define OWNFORM_DATA_CHAR_DATE     'D'

#define OWNFORM_DATA_TYPE_CONSTANT  2
#define OWNFORM_DATA_CHAR_CONSTANT 'C'

#define OWNFORM_DATA_TYPE_FORMULA   3
#define OWNFORM_DATA_CHAR_FORMULA  'F'

#define OWNFORM_DATA_TYPE_ARRAY     4
#define OWNFORM_DATA_CHAR_ARRAY    'A'

#define OWNFORM_DATA_TYPE_DRAWFILE  5
#define OWNFORM_DATA_CHAR_DRAWFILE 'W'

#define OWNFORM_DATA_TYPE_OWNER     6
#define OWNFORM_DATA_CHAR_OWNER    'O'

#define OWNFORM_DATA_TYPE_MAX       7

/* Input state - file/memory/storage */

typedef struct IP_FORMAT_FLAGS
{
    UBF is_template         : 1;
    UBF insert              : 1;
    UBF block_loaded        : 1;
    UBF base_style_same     : 1;

    UBF unknown_object      : 1;
    UBF unknown_construct   : 1;
    UBF unknown_font_spec   : 1;
    UBF unknown_named_style : 1;
    UBF unknown_data_type   : 1;

    UBF _was_dispose_array_on_finalise : 1;
    UBF table_insert        : 1; /* MRJC 9.11.93 */
    UBF incoming_base_single_col : 1;
    UBF virtual_row_table   : 1; /* SKS 03feb96 */
    UBF had_end_of_data     : 1; /* SKS 05jun96 */
    UBF full_file_load      : 1; /* SKS 31jan97 */
    UBF _spare_ : 1;

    UBF _spares_ : 16;
}
IP_FORMAT_FLAGS;

typedef struct IP_FORMAT_INPUT
{
    IP_INPUT_STATE  state;

    /* NB not a union as we need both in action when in input_force_via_memory() */
    struct IP_FORMAT_INPUT_FILE
    {
        FILE_HANDLE file_handle;
        U32 file_offset_in_bytes;
        U32 file_size_in_bytes;
    } file;

    struct IP_FORMAT_INPUT_MEM
    {
        PC_ARRAY_HANDLE p_array_handle;
        U32 array_offset;

        ARRAY_HANDLE owned_array_handle;
    } mem;

    U32             bom_bytes;  /* needed for input_rewind() */

    UCS4            ungotchar;

    SBCHAR_CODEPAGE sbchar_codepage;
}
IP_FORMAT_INPUT, * P_IP_FORMAT_INPUT;

#define IP_FORMAT_INPUT_INIT { IP_INPUT_INVALID, { NULL, 0, 0 }, { NULL, 0, 0 }, 0, 0 }

typedef struct OF_IP_FORMAT
{
    IP_FORMAT_INPUT input;

    DOCNO           docno;
    U8              reserved_byte_padding[3];

    PTSTR           input_filename; /* in case input_force_via_memory() used */

    POSITION        insert_position; /* position in document that the file is coming in at */
    DOCU_AREA       original_docu_area; /* area of data that was originally saved, set up by block construct or new position construct */

    LIST_BLOCK      object_data_list;
    ARRAY_HANDLE    renamed_styles;

    IP_FORMAT_FLAGS flags;          /* NB members may be set prior to load_initialise_load() */

    OBJECT_ID       directed_object_id;
    OBJECT_ID       stopped_object_id;

    PROCESS_STATUS  process_status; /* NB members may be set prior to load_initialise_load() */
    U32             process_status_threshold;

    BOOL            clip_data_from_cut_operation;
}
OF_IP_FORMAT, * P_OF_IP_FORMAT;

#define OF_IP_FORMAT_INIT { IP_FORMAT_INPUT_INIT, DOCNO_NONE, 0, 0, 0, NULL, { 0 }, { 0 } }

typedef struct FF_IP_FORMAT
{
    OF_IP_FORMAT    of_ip_format; /* NB at offset zero so we can trivially cast */

    IO_TYPE         io_type;

    STATUS (* read_ucs4_character_fn) (
        _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input,
        _Out_       P_UCS4 p_char_read);
}
FF_IP_FORMAT, * P_FF_IP_FORMAT;

#define FF_IP_FORMAT_INIT { OF_IP_FORMAT_INIT, IO_TYPE_AUTO, NULL }

typedef struct LOAD_CELL_OWNFORM
{
    BOOL            processed;  /*INOUT*/

    OBJECT_DATA     object_data;
    SLR             original_slr;
    S32             data_type;
    PC_USTR_INLINE  ustr_inline_contents;
    PC_USTR         ustr_formula;

    PCTSTR          tstr_mrofmun_style;
    REGION          region_saved;
    BOOL            clip_data_from_cut_operation;
}
LOAD_CELL_OWNFORM, * P_LOAD_CELL_OWNFORM;

typedef struct LOAD_CELL_FOREIGN
{
    BOOL            processed;  /*INOUT*/

    OBJECT_DATA     object_data;
    SLR             original_slr;
    S32             data_type;
    PC_USTR_INLINE  ustr_inline_contents;
    PC_USTR         ustr_formula;

    STYLE           style;
    STYLE_HANDLE    style_handle;
}
LOAD_CELL_FOREIGN, * P_LOAD_CELL_FOREIGN;

/*
loading constructs
*/

typedef struct CONSTRUCT_CONVERT
{
    ARGLIST_HANDLE      arglist_handle;
    P_OF_IP_FORMAT      p_of_ip_format;
    PC_CONSTRUCT_TABLE  p_construct;

    ARRAY_HANDLE        object_array_handle_uchars_inline;
}
CONSTRUCT_CONVERT, * P_CONSTRUCT_CONVERT;

/*
construct id requests for load/save
*/

typedef struct CONSTRUCT_ID_RQ
{
    U8 construct_id;
}
CONSTRUCT_ID_RQ, * P_CONSTRUCT_ID_RQ;

/*
T5_MSG_INSERT_OWNFORM/FOREIGN
*/

typedef struct MSG_INSERT_OWNFORM
{
    PCTSTR filename;
    ARRAY_HANDLE array_handle; /* use iff filename==NULL */
    T5_FILETYPE t5_filetype;
    BOOL insert;
    BOOL scrap_file;
    BOOL ctrl_pressed; /* really just for FOREIGN */
    POSITION position;
    SKEL_POINT skel_point;
    U32 retry_with_this_arg;

    /* common load/insert optimisation for callee - don't use in called code */
    BOOL old_virtual_row_table;
}
MSG_INSERT_OWNFORM, * P_MSG_INSERT_OWNFORM,
MSG_INSERT_FOREIGN, * P_MSG_INSERT_FOREIGN,
MSG_INSERT_FILE /* for when we're not sure! */;

/*
T5_CMD_xxx messages all get standard arg form
*/

typedef struct T5_CMD
{
    ARGLIST_HANDLE arglist_handle;
    S32            interactive;
    U32            count;
    P_OF_IP_FORMAT p_of_ip_format; /* NULL if not in load context */
}
T5_CMD, * P_T5_CMD; typedef const T5_CMD * PC_T5_CMD;

#define P_T5_CMD_NONE _P_DATA_NONE(P_T5_CMD)

#define T5_CMD_PROTO(_e_s, _proc_name) \
_e_s STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InVal_     T5_MESSAGE t5_message, \
    _InRef_     PC_T5_CMD p_t5_cmd)

/* some commands like being wrapped by cur_change_before/cur_change_after */

#define CCBA_WRAP_T5_CMD(_e_s, _cmd_name) \
T5_CMD_PROTO(_e_s, _cmd_name) \
{ \
    STATUS status; \
    cur_change_before(p_docu); \
    status = ccba_wrapped_ ## _cmd_name(p_docu, t5_message, p_t5_cmd); \
    cur_change_after(p_docu); \
    return(status); \
}

extern const PCTSTR
extension_document_tstr;

extern const PCTSTR
extension_template_tstr;

#if RISCOS
#define TEMPLATES_SUBDIR TEXT("Template")
#else
#define TEMPLATES_SUBDIR TEXT("Templates")
#endif

#if RISCOS
#define FOREIGN_TEMPLATES_SUBDIR TEXT("ForeignTem")
#else
#define FOREIGN_TEMPLATES_SUBDIR TEXT("ForeignTemplates")
#endif

#if RISCOS
#define CONFIG_FILE_EXTENSION  TEXT("")
#else
#define CONFIG_FILE_EXTENSION  FILE_EXT_SEP_TSTR TEXT("txt")
#endif
#define CONFIG_FILE_NAME TEXT("Config") CONFIG_FILE_EXTENSION

#if RISCOS
#define CHOICES_FILE_EXTENSION TEXT("")
#else
#define CHOICES_FILE_EXTENSION FILE_EXT_SEP_TSTR TEXT("txt")
#endif
#define CHOICES_DIR_NAME TEXT("Choices")
#define CHOICES_FILE_FORMAT_STR  CHOICES_DIR_NAME FILE_DIR_SEP_TSTR TEXT("Choice") TEXT("%.2") S32_TFMT_POSTFIX CHOICES_FILE_EXTENSION
#define CHOICES_FILE_EXAMPLE_STR CHOICES_DIR_NAME FILE_DIR_SEP_TSTR TEXT("Choice") TEXT("00")                   CHOICES_FILE_EXTENSION

#define CHOICES_DOCU_FILE_NAME   CHOICES_DIR_NAME FILE_DIR_SEP_TSTR TEXT("ChoicesDoc") CHOICES_FILE_EXTENSION

extern ARRAY_HANDLE
most_recently_used_file_list;

/*
raw
*/

_Check_return_
extern STATUS
file_memory_load(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_data,
    _In_z_      PCTSTR pathname,
    _OutRef_opt_ P_PROCESS_STATUS p_process_status,
    _InVal_     U32 min_file_size);

_Check_return_
extern STATUS
input_rewind(
    _InoutRef_  P_IP_FORMAT_INPUT p_ip_format_input);

/*
ownform
*/

_Check_return_
extern U8
construct_id_from_data_type(
    _InVal_     S32 data_type);

_Check_return_
extern U8
construct_id_from_object_id(
    _InVal_     OBJECT_ID object_id);

_Check_return_
_Ret_maybenull_
extern PC_CONSTRUCT_TABLE
construct_table_from_object_id(
    _InVal_     OBJECT_ID object_id);

_Check_return_
_Ret_maybenull_
extern PC_CONSTRUCT_TABLE
construct_table_lookup_message(
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message);

_Check_return_
extern STATUS
data_type_from_construct_id(
    _InVal_     U8 data_type_char,
    _OutRef_    P_S32 data_type);

_Check_return_
extern STATUS
load_ownform_config_file(void);

extern DOCNO
last_docno_loaded_query(void);

_Check_return_
extern STATUS
load_object_config_file(
    _InVal_     OBJECT_ID object_id);

extern void
load_supporting_documents(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS /*reported*/
load_and_print_this_file_rl(
    _In_z_      PCTSTR filename,
    _In_opt_z_  PCTSTR printername);

_Check_return_
extern STATUS /*reported*/
load_this_dir_rl(
    _In_z_      PCTSTR dirname);

_Check_return_
extern STATUS /*reported*/
load_this_fireworkz_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     BOOL fReadOnly);

_Check_return_
extern STATUS /*reported*/
load_this_template_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_opt_z_  PCTSTR filename);

_Check_return_
extern STATUS /*reported*/
load_this_command_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
ownform_finalise_load(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format);

_Check_return_
extern STATUS
ownform_initialise_load(
    _DocuRef_   P_DOCU p_docu,
    _InRef_maybenone_ PC_POSITION p_position,
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _In_opt_z_  PCTSTR filename,
    _InRef_opt_ PC_ARRAY_HANDLE p_array_handle);

_Check_return_
extern STATUS
load_ownform_file(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format);

_Check_return_
extern STATUS
load_ownform_command_file(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format);

_Check_return_
extern STATUS
load_ownform_from_array_handle(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InRef_maybenone_ PC_POSITION p_position,
    _InVal_     BOOL clip_data_from_cut_operation);

extern void
load_reflect_status(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format);

_Check_return_
extern STATUS
object_id_from_construct_id(
    _InVal_     U8 construct_id,
    _OutRef_    P_OBJECT_ID p_object_id);

extern void
of_load_prepare_first_template(void);

extern void
of_load_S32_arg_offset_as_COL(
    _InRef_     P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_ARGLIST_ARG p_arg);

extern void
of_load_S32_arg_offset_as_ROW(
    _InRef_     P_OF_IP_FORMAT p_of_ip_format,
    _InoutRef_  P_ARGLIST_ARG p_arg);

_Check_return_
extern STATUS /*reported*/
load_file_for_windows_startup_rl(
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
load_into_thunk_using_array_handle(
    _InVal_     DOCNO docno,
    _InRef_     PC_ARRAY_HANDLE p_array_handle);

_Check_return_
extern STATUS
register_object_construct_table(
    _InVal_     OBJECT_ID object_id,
    _InRef_     P_CONSTRUCT_TABLE p_construct_table /*data sorted*/,
    _InVal_     BOOL needs_help);

/*
foreign
*/

_Check_return_
extern STATUS
foreign_load_finalise(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format);

_Check_return_
extern STATUS
foreign_load_initialise(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_maybenone_ P_POSITION p_position,
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _In_opt_z_  PCTSTR filename,
    _InRef_opt_ PC_ARRAY_HANDLE p_array_handle);

extern void
foreign_load_set_io_type(
    _InoutRef_  P_FF_IP_FORMAT p_ff_ip_format,
    _InVal_     IO_TYPE io_type);

_Check_return_
extern STATUS
foreign_load_determine_io_type(
    _Inout_     P_FF_IP_FORMAT p_ff_ip_format);

_Check_return_
extern STATUS /*reported*/
load_foreign_file_rl(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype);

#define ENUMERATE_BOUND_FILETYPES_START -1

/* these are bits */
#define BOUND_FILETYPE_READ             0x01
#define BOUND_FILETYPE_WRITE            0x02
#define BOUND_FILETYPE_READ_PICT        0x04
#define BOUND_FILETYPE_WRITE_PICT       0x08

_Check_return_
extern T5_FILETYPE
enumerate_bound_filetypes(
    _InoutRef_  P_ARRAY_INDEX p_array_index,
    _InVal_     S32 mask);

_Check_return_
_Ret_z_
extern PC_USTR
description_ustr_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_BOOL p_found);

_Check_return_
_Ret_z_
extern PC_USTR
extension_srch_ustr_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype,
    _OutRef_    P_BOOL p_found);

_Check_return_
_Ret_maybenull_
extern PCTSTR
foreign_template_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern T5_FILETYPE
t5_filetype_from_data(
    _In_reads_bytes_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes);

_Check_return_
extern T5_FILETYPE
t5_filetype_from_extension(
    _In_opt_z_  PCTSTR extension);

_Check_return_
extern T5_FILETYPE
t5_filetype_from_file_header(
    _In_z_      PCTSTR filename);

_Check_return_
extern T5_FILETYPE
t5_filetype_from_filename(
    _In_z_      PCTSTR filename);

_Check_return_
extern OBJECT_ID
object_id_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype);

/*
sk_stylg.c
*/

#define grid_is_on(pd) \
    (pd)->page_def.grid_size

/*
output from grid accumulator
*/

typedef struct GRID_LINE_STYLE
{
    BORDER_LINE_FLAGS border_line_flags;
    RGB rgb;
}
GRID_LINE_STYLE, * P_GRID_LINE_STYLE;

typedef struct GRID_FLAGS
{
    UBF outer_edge : 1;
    UBF faint_grid : 1;
}
GRID_FLAGS, * P_GRID_FLAGS;

enum GRID_TOP_BOTTOM
{
    GRID_AT_TOP,
    GRID_AT_BOTTOM
};

enum GRID_LEFT_RIGHT
{
    GRID_AT_LEFT,
    GRID_AT_RIGHT
};

/*
grid style communication
*/

typedef struct GRID_ELEMENT
{
    U8 type;
    U8 reserved_byte_padding[3];
    S32 level;
    RGB rgb;
    S32 rgb_level;
}
GRID_ELEMENT, * P_GRID_ELEMENT;

enum GRID_INDICES
{
    IX_GRID_LEFT = 0,
    IX_GRID_TOP,
    IX_GRID_RIGHT,
    IX_GRID_BOTTOM,
    IX_GRID_COUNT
};

typedef struct GRID_SLOT
{
    STYLE_SELECTOR selector;
    SLR slr;
    GRID_ELEMENT grid_element[IX_GRID_COUNT];
}
GRID_SLOT, * P_GRID_SLOT;

typedef struct PAGE_FLAGS
{
    UBF first_col_on_page : 1;
    UBF first_row_on_page : 1;
    UBF last_col_on_page : 1;
    UBF last_row_on_page : 1;
}
PAGE_FLAGS;

/*
block of grid info
*/

typedef struct GRID_BLOCK
{
    REGION region;
    ARRAY_HANDLE h_grid_array;
}
GRID_BLOCK, * P_GRID_BLOCK;

extern void
style_grid_block_dispose(
    P_GRID_BLOCK p_grid_block);

_Check_return_
extern STATUS
style_grid_block_for_region_new(
    _DocuRef_   P_DOCU p_docu,
    P_GRID_BLOCK p_grid_block,
    _InRef_     PC_REGION p_region,
    _In_        PAGE_FLAGS page_flags);

extern void
style_grid_block_init(
    P_GRID_BLOCK p_grid_block);

extern COL
style_grid_from_grid_block_h(
    P_GRID_LINE_STYLE p_grid_line_style,
    P_GRID_BLOCK p_grid_block,
    _InVal_     COL col,
    _InVal_     ROW row,
    _In_        GRID_FLAGS grid_flags);

extern ROW
style_grid_from_grid_block_v(
    P_GRID_LINE_STYLE p_grid_line_style,
    P_GRID_BLOCK p_grid_block,
    _InVal_     COL col,
    _InVal_     ROW row,
    _In_        GRID_FLAGS grid_flags);

/*
UI style names (forever values, saved in ChoicesDoc)
*/

#define STYLE_NAME_UI_BASE          TEXT("UI.Base")
#define STYLE_NAME_UI_BORDER        TEXT("UI.Border")
#define STYLE_NAME_UI_BORDER_HORZ   TEXT("UI.Border.Horizontal")
#define STYLE_NAME_UI_BORDER_VERT   TEXT("UI.Border.Vertical")
#define STYLE_NAME_UI_FORMULA_LINE  TEXT("UI.FormulaLine")
#define STYLE_NAME_UI_RULER         TEXT("UI.Ruler")
#define STYLE_NAME_UI_RULER_HORZ    TEXT("UI.Ruler.Horizontal")
#define STYLE_NAME_UI_RULER_VERT    TEXT("UI.Ruler.Vertical")
#define STYLE_NAME_UI_STATUS_LINE   TEXT("UI.StatusLine")

extern void
font_spec_from_ui_style(
    _OutRef_    P_FONT_SPEC p_font_spec,
    _InRef_     PC_STYLE p_ui_style);

/*
of_save.c
*/

typedef enum OP_OUTPUT_STATE
{
    OP_OUTPUT_INVALID   = 0,
    OP_OUTPUT_FILE      = 1,
    OP_OUTPUT_MEM       = 2/*,
    OP_OUTPUT_STG       = 3*/
}
OP_OUTPUT_STATE;

typedef struct OP_FORMAT_OUTPUT
{
    OP_OUTPUT_STATE state;

    union OP_FORMAT_OUTPUT_U
    {
        struct OP_FORMAT_OUTPUT_FILE
        {
            FILE_HANDLE file_handle;
            PTSTR filename;
            EV_DATE ev_date;
            T5_FILETYPE t5_filetype;
        } file;

        struct OP_FORMAT_OUTPUT_MEM
        {
            P_ARRAY_HANDLE p_array_handle;
        } mem;
    } u;

    SBCHAR_CODEPAGE sbchar_codepage;
}
OP_FORMAT_OUTPUT, * P_OP_FORMAT_OUTPUT;

#define OP_FORMAT_OUTPUT_INIT { OP_OUTPUT_INVALID }

enum OF_OPTION_TYPE
{
    OF_OPTION_REGION = 0,
    OF_OPTION_STYLE_DEFINE,
    OF_OPTION_HEADER,
    OF_OPTION_FOOTER
};

typedef struct SAVE_FILETYPE
{
    OBJECT_ID   object_id;
    T5_FILETYPE t5_filetype;
    UI_TEXT     description;
    UI_TEXT     suggested_leafname;
}
SAVE_FILETYPE, * P_SAVE_FILETYPE; typedef const SAVE_FILETYPE * PC_SAVE_FILETYPE;

/*DATA_CLASS_INDEX*/

#define DATA_SAVE_CHARACTER UBF_PACK(0)
#define DATA_SAVE_MANY      UBF_PACK(1)
#define DATA_SAVE_DOC       UBF_PACK(2)
#define DATA_SAVE_WHOLE_DOC UBF_PACK(3)

typedef struct OF_TEMPLATE
{
    UBF used_styles      : 1;
    UBF unused_styles    : 1;
    UBF version          : 1;
    UBF _____unused      : 1;
    UBF key_defn         : 1;
    UBF menu_struct      : 1;
    UBF views            : 1;
    UBF data_class       : 2; /* derived during save */
    UBF ruler_scale      : 1;
    UBF note_layer       : 1;
    UBF numform_context  : 1;
    UBF paper            : 1;
    UBF _spare           : 3;

    UBF _spare_16        : 16;
    /* must be 32 bits total */
}
OF_TEMPLATE; typedef const OF_TEMPLATE * PC_OF_TEMPLATE;

typedef struct OF_OP_FORMAT
{
    OP_FORMAT_OUTPUT    output;

    DOCNO               docno;
    U8                  _spare_1[3];

    DOCU_AREA           save_docu_area;         /* originally specified save area */

    PROCESS_STATUS      process_status;         /* NB members may be set prior to ownform_initialise_save() */

    /* currently ownform-only members */
    LIST_BLOCK          object_data_list;

    OBJECT_ID           stopped_object_id;

    /* definitely ownform-only members */
    DOCU_AREA           save_docu_area_no_frags; /* area to save with no fragments */

    OF_TEMPLATE         of_template;            /* NB members may be set prior to ownform_initialise_save() */

    U8                  saved_database_constructs; /* SKS 14apr96 allows CSV saving of database to work */
    U8                  _spare_2[3];
}
OF_OP_FORMAT, * P_OF_OP_FORMAT;

#define OF_OP_FORMAT_INIT { OP_FORMAT_OUTPUT_INIT, 0, 0, 0, 0, { 0 }, PROCESS_STATUS_INIT }

/* Save construct request */

typedef struct SAVE_CONSTRUCT_OWNFORM
{
    PC_USTR_INLINE      ustr_inline;

    PC_CONSTRUCT_TABLE  p_construct;
    ARGLIST_HANDLE      arglist_handle;
    OBJECT_ID           object_id;
}
SAVE_CONSTRUCT_OWNFORM, * P_SAVE_CONSTRUCT_OWNFORM;

/* Save cell request */

typedef struct SAVE_CELL_OWNFORM
{
    OBJECT_DATA         object_data;
    P_OF_OP_FORMAT      p_of_op_format; /* tragically required by ob_rec */

    /*OUT*/
    U8                  data_type;      /* indicate to caller what type of data we have just saved */
    U8                  _spare[3];

    QUICK_UBLOCK        contents_data_quick_ublock;
    QUICK_UBLOCK        formula_data_quick_ublock;

    PCTSTR              tstr_mrofmun_style;
}
SAVE_CELL_OWNFORM, * P_SAVE_CELL_OWNFORM;

/*
passed as data for T5_MSG_SAVE with T5_MSG_(PRE,DATA,POST)_SAVE_(1,2,3) phase args
*/

typedef enum T5_MSG_SAVE_MESSAGE
{
    T5_MSG_SAVE__SAVE_STARTED,

    T5_MSG_SAVE__PRE_SAVE_1,
    T5_MSG_SAVE__PRE_SAVE_2,
    T5_MSG_SAVE__PRE_SAVE_3,

    T5_MSG_SAVE__POST_SAVE_1,
    T5_MSG_SAVE__POST_SAVE_2,
    T5_MSG_SAVE__POST_SAVE_3,

    T5_MSG_SAVE__DATA_SAVE_1,
    T5_MSG_SAVE__DATA_SAVE_2,
    T5_MSG_SAVE__DATA_SAVE_3,

    T5_MSG_SAVE__SAVE_ENDED
}
T5_MSG_SAVE_MESSAGE;

typedef struct MSG_SAVE
{
    P_OF_OP_FORMAT p_of_op_format;
    T5_MSG_SAVE_MESSAGE t5_msg_save_message; /* phase */
}
MSG_SAVE; typedef const MSG_SAVE * PC_MSG_SAVE;

/*
exported routines - ownform
*/

_Check_return_
extern STATUS
arglist_prepare_with_construct(
    _OutRef_    P_ARGLIST_HANDLE p_arglist_handle,
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message,
    _OutRef_    P_PC_CONSTRUCT_TABLE p_p_construct_table);

_Check_return_
_Ret_z_
extern PCTSTR
localise_filename(
    _In_z_      PCTSTR name_to_make_relative_to,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
ownform_finalise_save(
    _OutRef_opt_ P_P_DOCU p_p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     STATUS save_status);

_Check_return_
extern STATUS
ownform_initialise_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _In_opt_z_  PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InRef_opt_ PC_DOCU_AREA p_docu_area);

_Check_return_
extern STATUS
ownform_save_arglist(
    _InVal_     ARGLIST_HANDLE arglist_handle,
    _InVal_     OBJECT_ID object_id,
    _InRef_     PC_CONSTRUCT_TABLE p_construct_table,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

_Check_return_
extern STATUS
ownform_output_group_stt(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

_Check_return_
extern STATUS
ownform_output_group_end(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

_Check_return_
extern STATUS
ownform_output_newline(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

_Check_return_
extern STATUS
save_ownform_file(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

_Check_return_
extern STATUS
save_ownform_to_array_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _InRef_     PC_OF_TEMPLATE p_template,
    _InRef_     PC_DOCU_AREA p_docu_area);

_Check_return_
extern STATUS
save_foreign_to_array_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_opt_ P_ARRAY_HANDLE p_array_handle,
    _InVal_     T5_FILETYPE t5_filetype,
    _InRef_     PC_DOCU_AREA p_docu_area);

extern void
save_reflect_status(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     S32 percent);

_Check_return_
extern STATUS
locate_copy_of_dir_template(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename_template,
    _Out_writes_z_(elemof_dirname_buffer) PTSTR dirname_buffer,
    _InVal_     U32 elemof_dirname_buffer);

/*
foreign
*/

typedef struct FF_OP_FORMAT
{
    OF_OP_FORMAT    of_op_format; /* NB at offset zero so we can trivially cast */

    IO_TYPE         io_type;

    STATUS (* write_ucs4_character_fn) (
        _InoutRef_  P_OP_FORMAT_OUTPUT p_op_format_output,
        _InVal_     UCS4 ucs4);
}
FF_OP_FORMAT, *P_FF_OP_FORMAT;

/* Save as foreign format request */

typedef struct MSG_SAVE_FOREIGN
{
    T5_FILETYPE         t5_filetype;
    P_FF_OP_FORMAT      p_ff_op_format;
}
MSG_SAVE_FOREIGN, * P_MSG_SAVE_FOREIGN;

/* Save as picture request */

typedef struct MSG_SAVE_PICTURE
{
    T5_FILETYPE         t5_filetype;
    P_FF_OP_FORMAT      p_ff_op_format;
    uintptr_t           extra;
}
MSG_SAVE_PICTURE, * P_MSG_SAVE_PICTURE;

/* Save as picture request */

typedef struct MSG_SAVE_PICTURE_FILETYPES_REQUEST
{
    ARRAY_HANDLE        h_save_filetype; /* SAVE_FILETYPE[] */ /*appended*/
    uintptr_t           extra;
}
MSG_SAVE_PICTURE_FILETYPES_REQUEST, * P_MSG_SAVE_PICTURE_FILETYPES_REQUEST;

/*
reperr.c
*/

/*
structure
*/

#define     MAX_ERRORSTRING 255
#define BUF_MAX_ERRORSTRING 256

typedef struct MSG_ERROR_RQ
{
    PTSTR tstr_buf; /*->TCHARZ errmess[elemof_buffer]*/
    U32 elemof_buffer;
    STATUS status;
}
MSG_ERROR_RQ, * P_MSG_ERROR_RQ;

/*
whenever this error offset is returned, the object is queried to obtain the error
*/

#define ERR_OFFSET_ERROR_RQ (-1)

extern BOOL g_silent_shutdown;

/*
exported functions
*/

_Check_return_
extern STATUS
create_error_from_tstr(
    _In_z_      PCTSTR tstr);

/*ncr*/
extern STATUS __cdecl
reperr(
    _InVal_     STATUS status,
    _In_opt_z_  PCTSTR text,
    /**/        ...);

/*ncr*/
static inline STATUS
reperr_null(
    _InVal_     STATUS status)
{   /* reperr() with no 2nd parm or args */
    return(reperr(status, PTSTR_NONE));
}

extern void __cdecl
messagef(
    _In_z_      PCTSTR format,
    /**/        ...);

/*
sk_alpha.c
*/

/*
exported routines
*/

extern void
sk_alpha_startup(void);

T5_CMD_PROTO(extern, t5_cmd_ctypetable);

/* derived from RISC OS ctype.h - remember we've forced all ctype.h functions to return values from the "C" locale in Fireworkz */

#define T5__S 1U           /* whitespace           */
#define T5__P 2U           /* punctuation          */
#define T5__B 4U           /* blank                */
#define T5__L 8U           /* lower case letter    */
#define T5__U 16U          /* upper case letter    */
#define T5__N 32U          /* (decimal) digit      */
#define T5__C 64U          /* control chars        */
#define T5__X 128U         /* A-F and a-f          */

#define t5__ctype_isalnum(cte) ((int) \
    ((cte) & (T5__U+T5__L+T5__N)))
    /* non-0 iff cte is alphabetic or numeric */

#define t5__ctype_isalpha(cte) ((int) \
    ((cte) & (T5__U+T5__L)))
    /* non-0 iff cte is alphabetic */

#define t5__ctype_iscntrl(cte) ((int) \
    ((cte) & (T5__C)))
    /* non-0 iff cte is a control character - this means (c < CH_SPACE) || (c = 0x7F) */

#define t5__ctype_isdigit(cte) ((int) \
    ((cte) & (T5__N)))
    /* non-0 iff cte is a decimal digit */

#define t5__ctype_isgraph(cte) ((int) \
    ((cte) & (T5__L+T5__U+T5__N+T5__P)))
    /* non-0 iff cte is any printing character other than CH_SPACE */

#define t5__ctype_islower(cte) ((int) \
    ((cte) & (T5__L)))
    /* non-0 iff cte is a lower-case letter */

#define t5__ctype_isprint(cte) ((int) \
    ((cte) & (T5__L+T5__U+T5__N+T5__P+T5__B)))
    /* non-0 iff cte is a printing character */

#define t5__ctype_ispunct(cte) ((int) \
    ((cte) & (T5__P)))
    /* non-0 iff cte is a non-space, non-alpha-numeric, printing character */

#define t5__ctype_isspace(cte) ((int) \
    ((cte) & (T5__S)))
    /* non-0 iff cte is a white-space char: CH_SPACE, '\f', '\n', '\r', '\t', '\v'. */

#define t5__ctype_isupper(cte) ((int) \
    ((cte) & (T5__U)))
    /* non-0 iff cte is an upper-case letter */

#define t5__ctype_isxdigit(cte) ((int) \
    ((cte) & (T5__N+T5__X)))
    /* non-0 iff cte is a digit, in 'a'..'f', or in 'A'..'F' */

#define t5__ctype_tolower(cte) ((U8) \
    (((cte) >> 16) & 0xFF))
    /* return the corresponding lower-case letter for cte */

#define t5__ctype_toupper(cte) ((U8) \
    (((cte) >>  8) & 0xFF))
    /* return the corresponding upper-case letter for cte */

#define t5__ctype_sortbyte(cte) ((U8) \
    (((cte) >> 24) & 0xFF))

extern /*const-to-you*/ U32 t5__ctype_sbchar[256]; /* Latin-N locale */

#define sbchar_isalnum(c) \
    t5__ctype_isalnum(t5__ctype_sbchar[c])

#define sbchar_isalpha(c) \
    t5__ctype_isalpha(t5__ctype_sbchar[c])

/*#define sbchar_iscntrl(c) \
    t5__ctype_iscntrl(t5__ctype_sbchar[c])*/

#define sbchar_isdigit(c) \
    t5__ctype_isdigit(t5__ctype_sbchar[c])

/*#define sbchar_isgraph(c) \
    t5__ctype_isgraph(t5__ctype_sbchar[c])*/

/*#define sbchar_islower(c) \
    t5__ctype_islower(t5__ctype_sbchar[c])*/

/*#define sbchar_isprint(c) \
    t5__ctype_isprint(t5__ctype_sbchar[c])*/

#define sbchar_ispunct(c) \
    t5__ctype_ispunct(t5__ctype_sbchar[c])

/*#define sbchar_isspace(c) \
    t5__ctype_isspace(t5__ctype_sbchar[c])*/

/*#define sbchar_isupper(c) \
    t5__ctype_isupper(t5__ctype_sbchar[c])*/

/*#define sbchar_isxdigit(c) \
    t5__ctype_isxdigit(t5__ctype_sbchar[c])*/

#define sbchar_tolower(c) \
    t5__ctype_tolower(t5__ctype_sbchar[c])

#define sbchar_toupper(c) \
    t5__ctype_toupper(t5__ctype_sbchar[c])

#define sbchar_sortbyte(c) \
    t5__ctype_sortbyte(t5__ctype_sbchar[c])

extern /*const-to-you*/ U32 t5__ctype[256]; /* Copied from Latin-N locale. May be modified by Config */

/*#define t5_isalnum(c) \
    t5__ctype_isalnum(t5__ctype[c])*/

#define t5_isalpha(c) \
    t5__ctype_isalpha(t5__ctype[c])

/*#define t5_iscntrl(c) \
    t5__ctype_iscntrl(t5__ctype[c])*/

/*#define t5_isdigit(c) \
    t5__ctype_isdigit(t5__ctype[c])*/

/*#define t5_isgraph(c) \
    t5__ctype_isgraph(t5__ctype[c])*/

#define t5_islower(c) \
    t5__ctype_islower(t5__ctype[c])

#define t5_isprint(c) \
    t5__ctype_isprint(t5__ctype[c])

#define t5_ispunct(c) \
    t5__ctype_ispunct(t5__ctype[c])

/*#define t5_isspace(c) \
    t5__ctype_isspace(t5__ctype[c])*/

#define t5_isupper(c) \
    t5__ctype_isupper(t5__ctype[c])

/*#define t5_isxdigit(c) \
    t5__ctype_isxdigit(t5__ctype[c])*/

#define t5_tolower(c) \
    t5__ctype_tolower(t5__ctype[c])

#define t5_toupper(c) \
    t5__ctype_toupper(t5__ctype[c])

#define t5_sortbyte(c) \
    t5__ctype_sortbyte(t5__ctype[c])

/* transitional functions */

#if USTR_IS_SBSTR
_Check_return_
static inline BOOL
t5_ucs4_is_alphabetic(_InVal_ UCS4 ucs4)
{
    if(!ucs4_is_sbchar(ucs4))
        return(FALSE);

    return(t5_isalpha((U8) ucs4));
}
#else
#define t5_ucs4_is_alphabetic(ucs4) \
    ucs4_is_alphabetic(ucs4)
#endif

#if USTR_IS_SBSTR
_Check_return_
static inline BOOL
t5_ucs4_is_decimal_digit(_InVal_ UCS4 ucs4)
{
    if(!ucs4_is_sbchar(ucs4))
        return(FALSE);

    return(sbchar_isdigit((U8) ucs4));
}
#else
#define t5_ucs4_is_decimal_digit(ucs4) \
    ucs4_is_decimal_digit(ucs4)
#endif

#if USTR_IS_SBSTR
_Check_return_
static inline BOOL
t5_ucs4_is_lowercase(_InVal_ UCS4 ucs4)
{
    if(!ucs4_is_sbchar(ucs4))
        return(FALSE);

    return(t5_islower((U8) ucs4));
}
#else
#define t5_ucs4_is_lowercase(ucs4) \
    ucs4_is_lowercase(ucs4)
#endif

_Check_return_
static inline BOOL
t5_ucs4_ispunct(_InVal_ UCS4 ucs4)
{
    if(!ucs4_is_sbchar(ucs4))
        return(FALSE);

    return(t5_ispunct((U8) ucs4));
}

#if USTR_IS_SBSTR
_Check_return_
static inline BOOL
t5_ucs4_is_uppercase(_InVal_ UCS4 ucs4)
{
    if(!ucs4_is_sbchar(ucs4))
        return(FALSE);

    return(t5_isupper((U8) ucs4));
}
#else
#define t5_ucs4_is_uppercase(ucs4) \
    ucs4_is_uppercase(ucs4)
#endif

#if USTR_IS_SBSTR
_Check_return_
static inline UCS4
t5_ucs4_lowercase(_InVal_ UCS4 ucs4)
{
    if(!ucs4_is_sbchar(ucs4))
        return(ucs4);

    return(t5_tolower((U8) ucs4));
}
#else
#define t5_ucs4_lowercase(ucs4) \
    ucs4_lowercase(ucs4)
#endif

#if USTR_IS_SBSTR
_Check_return_
static inline UCS4
t5_ucs4_uppercase(_InVal_ UCS4 ucs4)
{
    if(!ucs4_is_sbchar(ucs4))
        return(ucs4);

    return(t5_toupper((U8) ucs4));
}
#else
#define t5_ucs4_uppercase(ucs4) \
    ucs4_uppercase(ucs4)
#endif

/*
sk_cmd.c
*/

/*
exported procs
*/

_Check_return_
extern STATUS
check_protection_selection(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
check_protection_simple(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL status_line_message);

_Check_return_
extern STATUS
command_array_handle_assign_to_key_code(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE code,
    _InVal_     ARRAY_HANDLE h_commands);

extern void
command_array_handle_dispose_from_key_code(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE code);

_Check_return_
extern STATUS
command_array_handle_execute(
    _InVal_     DOCNO docno,
    _InVal_     ARRAY_HANDLE h_commands);

extern ARRAY_HANDLE
command_array_handle_from_key_code(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE code,
    /*out*/ P_T5_MESSAGE p_t5_message);

_Check_return_
extern BOOL
command_query_interactive(void);

_Check_return_
extern STATUS
command_record(
    _InVal_     ARGLIST_HANDLE arglist_handle,
    _InVal_     OBJECT_ID object_id,
    _InRef_     PC_CONSTRUCT_TABLE p_construct_table);

_Check_return_
extern STATUS
command_set_current_docu(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
command_set_current_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
command_set_interactive(void);

extern void
docu_modify(
    _DocuRef_   P_DOCU p_docu);

extern void
docu_modify_clear(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
execute_command(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_opt_ PC_ARGLIST_HANDLE pc_arglist_handle,
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
execute_command_reperr(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_opt_ PC_ARGLIST_HANDLE pc_arglist_handle,
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
execute_loaded_command(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert,
    _InVal_     OBJECT_ID object_id);

_Check_return_
extern STATUS
execute_command_n(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     OBJECT_ID object_id,
    _InVal_     U32 count);

extern KMAP_CODE
key_code_from_key_of_name(
    _In_z_      PCTSTR key_name);

extern PCTSTR /*low-lifetime*/
key_of_name_from_key_code(
    _In_        KMAP_CODE key_code);

extern PCTSTR /*low-lifetime*/
key_ui_name_from_key_code(
    _In_        KMAP_CODE key_code,
    _InVal_     BOOL long_name);

T5_CMD_PROTO(extern, t5_cmd_current_document);
T5_CMD_PROTO(extern, t5_cmd_current_position);
T5_CMD_PROTO(extern, t5_cmd_undefine_key);

_Check_return_
extern STATUS
t5_cmd_set_interactive(void);

#if 1
#define p_command_recorder_of_op_format NULL
#else
/*
exported variables
*/

extern P_OF_OP_FORMAT
p_command_recorder_of_op_format;
#endif

/*
sk_choic.c
*/

typedef struct GLOBAL_PREFERENCES
{
    /* Choices section (used to be in config DOCU) */
    BOOL disable_dithering;
    BOOL disable_kerning;
    BOOL disable_status_line;
    BOOL disable_toolbar;
    BOOL dont_display_pictures;
    BOOL embed_inserted_files;
    BOOL update_styles_from_choices;

    BOOL ss_calc_foreground;
    BOOL ss_calc_manual;
    BOOL ss_calc_on_load;
    BOOL ss_calc_additional_rounding;
    BOOL ss_edit_in_cell;
    BOOL ss_alternate_formula_style;

    BOOL rec_dp_client;

    BOOL chart_update_manual;

    BOOL spell_auto_check;
    BOOL spell_write_user;

    BOOL ascii_load_as_delimited;
    UCS4 ascii_load_delimiter;
    BOOL ascii_load_as_paragraphs;

    S32 auto_save_period_minutes;               /* also present per-DOCU as an override */
}
GLOBAL_PREFERENCES;

extern GLOBAL_PREFERENCES global_preferences;

typedef struct MSG_CHOICE_CHANGED
{
    T5_MESSAGE t5_message; /* which specific choice */
}
MSG_CHOICE_CHANGED; typedef const MSG_CHOICE_CHANGED * PC_MSG_CHOICE_CHANGED;

extern void
issue_choice_changed(
    _InVal_     T5_MESSAGE t5_message);

/*
sk_docno.c
*/

typedef enum T5_MSG_INITCLOSE_MESSAGE
{
    T5_MSG_IC__STARTUP_SERVICES,
    T5_MSG_IC__STARTUP,                          /* request to object to allocate resources on program startup */
    T5_MSG_IC__STARTUP_CONFIG,
    T5_MSG_IC__STARTUP_CONFIG_ENDED,
    T5_MSG_IC__EXIT2,                            /* program is closing down */
    T5_MSG_IC__EXIT1,
    T5_MSG_IC__SERVICES_EXIT2,
    T5_MSG_IC__SERVICES_EXIT1,

    T5_MSG_IC__INIT_THUNK,                       /* when thunk is made (i.e. comes before INIT1) */
    T5_MSG_IC__INIT1,                            /* request to object to allocate instance data for document */
    T5_MSG_IC__INIT2,                            /* issued just after INIT1 */
    T5_MSG_IC__CLOSE1,
    T5_MSG_IC__CLOSE2,                           /* issued just after CLOSE1 */
    T5_MSG_IC__CLOSE_THUNK                       /* issued when document thunk goes away */

}
T5_MSG_INITCLOSE_MESSAGE;

typedef struct MSG_INITCLOSE
{
    T5_MSG_INITCLOSE_MESSAGE t5_msg_initclose_message;
}
MSG_INITCLOSE; typedef const MSG_INITCLOSE * PC_MSG_INITCLOSE;

/*
document errors
*/

#define DOCNO_ERR_CANTEXTREF -100000
#define DOCNO_ERR_END        -100001

/*
routines/macros
*/

extern void
sk_docno_startup(void);

extern void
docno_close(
    _InoutRef_  P_DOCNO p_docno);

extern void
docno_close_all(void);

_Check_return_
extern DOCNO /* DOCNO_NONE == end */
docno_enum_docs(
    _InVal_     DOCNO docno_in);

_Check_return_
extern DOCNO /* DOCNO_NONE == end */
docno_enum_thunks(
    _InVal_     DOCNO docno_in);

_Check_return_
extern DOCNO
docno_establish_docno_from_name(
    _InRef_     PC_DOCU_NAME p_docu_name);

_Check_return_
extern DOCNO
docno_find_name(
    _InRef_     PC_DOCU_NAME p_docu_name);

_Check_return_
extern STATUS /* n processed */
docno_from_id(
    _DocuRef_   PC_DOCU p_docu,
    _OutRef_    P_DOCNO p_docno,
    _In_z_      PC_USTR ustr_id,
    _InVal_     BOOL ensure);

_Check_return_
extern DOCNO
docno_from_p_docu(
    _DocuRef_   PC_DOCU p_docu);

_Check_return_
extern S32
docno_get_dependent_docs(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_docnos);

_Check_return_
extern S32
docno_get_linked_docs(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_docnos);

_Check_return_
extern S32
docno_get_supporting_docs(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_docnos);

_Check_return_
extern BOOL
docno_is_valid(
    _InVal_     DOCNO docno);

_Check_return_
extern U32 /* number of chars output */
docno_name_write_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     DOCNO docno_to /* name output */,
    _InVal_     DOCNO docno_from /* common path */);

_Check_return_
extern DOCNO
docno_new_create(
    _InRef_opt_ PC_DOCU_NAME p_docu_name /* NULL gets untitled */);

extern void
docno_reuse_hold(void);

extern void
docno_reuse_release(void);

#if CHECKING

_Check_return_
_Ret_maybenone_
extern P_DOCU
p_docu_from_docno(
    _InVal_     DOCNO docno);

#else

extern P_DOCU g_docu_table[256];

_Check_return_
_Ret_maybenone_
static inline P_DOCU
p_docu_from_docno(
    _InVal_     DOCNO docno)
{
    /* zeroth entry is always P_DOCU_NONE */
    /* all entries are initialised to P_DOCU_NONE */
    /* freed entries are always reset to P_DOCU_NONE */
    return(g_docu_table[docno]);
}


#endif /* CHECKING */

_Check_return_
_Ret_valid_
static inline P_DOCU
p_docu_from_docno_valid(
    _InVal_     DOCNO docno)
{
    const P_DOCU p_docu = p_docu_from_docno(docno);
    CHECKING_ONLY(if(IS_PTR_NULL_OR_NONE(p_docu)) myDebugBreak());
    __analysis_assume(p_docu);
    return(p_docu);
}

_Check_return_
_Ret_valid_
static inline P_DOCU
p_docu_from_config_wr(void)
{
    return(p_docu_from_docno_valid(DOCNO_CONFIG));
}

_Check_return_
_Ret_valid_
static inline PC_DOCU
p_docu_from_config(void)
{
    return(p_docu_from_config_wr());
}

/*
__sk_event_h
*/

typedef struct SKELEVENT_REDRAW
{
    REDRAW_TAG     work_window; /* identifies HORZBORDER, PANE_MARGIN_HEADER, PANE_WORKAREA etc. */
    SKEL_RECT      work_skel_rect;
    SKEL_RECT      clip_skel_rect;
    REDRAW_FLAGS   flags;
    REDRAW_CONTEXT redraw_context;
}
SKELEVENT_REDRAW, * P_SKELEVENT_REDRAW; typedef const SKELEVENT_REDRAW * PC_SKELEVENT_REDRAW;

typedef struct SKELEVENT_CLICK_DATA_DRAG
{
    P_ANY p_reason_data;
    PIXIT_POINT pixit_delta;
}
SKELEVENT_CLICK_DATA_DRAG;

typedef struct SKELEVENT_CLICK_DATA_FILEINSERT
{
    PCTSTR filename;
    T5_FILETYPE t5_filetype;
    S32 safesource; /* TRUE for a real file, FALSE for a scrapfile */
}
SKELEVENT_CLICK_DATA_FILEINSERT;

typedef union SKELEVENT_CLICK_DATA /* directed events other than simple click need more data */
{
    SKELEVENT_CLICK_DATA_DRAG drag;

    SKELEVENT_CLICK_DATA_FILEINSERT fileinsert;
}
SKELEVENT_CLICK_DATA;

typedef struct SKELEVENT_CLICK
{
    CLICK_CONTEXT click_context;
    SKEL_POINT    skel_point;
    REDRAW_TAG    work_window; /* identifies HORZBORDER, PANE_MARGIN_HEADER, PANE_WORKAREA etc. */
    SKEL_RECT     work_skel_rect;

    STATUS        processed; /*IN:0;OUT*/

    SKELEVENT_CLICK_DATA data;
}
SKELEVENT_CLICK, * P_SKELEVENT_CLICK; typedef const SKELEVENT_CLICK * PC_SKELEVENT_CLICK;

typedef struct SKELEVENT_CLICK_FILTER_DATA_CLICK
{
    SKELEVENT_CLICK skelevent_click;
    STATUS in_area;
    REDRAW_TAG_AND_EVENT redraw_tag_and_event;
}
SKELEVENT_CLICK_FILTER_DATA_CLICK;

typedef union SKELEVENT_CLICK_FILTER_DATA
{
    SKELEVENT_CLICK_FILTER_DATA_CLICK click[1];

    SKELEVENT_CLICK skelevent_click_drag;
}
SKELEVENT_CLICK_FILTER_DATA;

typedef struct SKELEVENT_CLICK_FILTER
{
    T5_MESSAGE t5_message;

    S32 processed_level;
#define SKELEVENT_CLICK_FILTER_UNPROCESSED -2
#define SKELEVENT_CLICK_FILTER_SUPPRESS    -1
#define SKELEVENT_CLICK_FILTER_SLOTAREA     0

    SKELEVENT_CLICK_FILTER_DATA data;
}
SKELEVENT_CLICK_FILTER, * P_SKELEVENT_CLICK_FILTER;

_Check_return_
static inline T5_MESSAGE
right_message_if_ctrl(
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     T5_MESSAGE t5_message_right,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click)
{
    if((t5_message == t5_message_right) || p_skelevent_click->click_context.ctrl_pressed)
        return(t5_message_right);

    return(t5_message);
}

_Check_return_
static inline T5_MESSAGE
right_message_if_shift(
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     T5_MESSAGE t5_message_right,
    _InRef_     P_SKELEVENT_CLICK p_skelevent_click)
{
    if((t5_message == t5_message_right) || p_skelevent_click->click_context.shift_pressed)
        return(t5_message_right);

    return(t5_message);
}

typedef struct SKELEVENT_KEY
{
    S32 keycode;
}
SKELEVENT_KEY, * P_SKELEVENT_KEY;

typedef struct SKELEVENT_KEYS
{
    PC_QUICK_UBLOCK p_quick_ublock; /* NB. this is ***NOT*** CH_NULL-terminated */
    S32 processed;
}
SKELEVENT_KEYS, * P_SKELEVENT_KEYS;

typedef struct SKELCMD_GOTO
{
    SLR        slr;
    SKEL_POINT skel_point;
    S32 keep_selection;
}
SKELCMD_GOTO, * P_SKELCMD_GOTO;

/*
sk_hefod.c
*/

/*
header/footer data
*/

typedef struct HEADFOOT
{
    ARRAY_HANDLE_USTR h_data;       /* handle to data of header/footer */
    ARRAY_HANDLE h_style_list;      /* root of header/footer style pyramid */
}
HEADFOOT, * P_HEADFOOT;

typedef struct HEADFOOT_SIZES
{
    PIXIT margin;
    PIXIT offset;
}
HEADFOOT_SIZES, * P_HEADFOOT_SIZES; typedef const HEADFOOT_SIZES * PC_HEADFOOT_SIZES;

/*
header or footer definition
*/

typedef struct HEADFOOT_DEF
{
    HEADFOOT headfoot;
    HEADFOOT_SIZES headfoot_sizes;
    DATA_SPACE data_space;
}
HEADFOOT_DEF, * P_HEADFOOT_DEF, ** P_P_HEADFOOT_DEF;

typedef struct HEADFOOT_BOTH
{
    HEADFOOT_SIZES header;
    HEADFOOT_SIZES footer;
}
HEADFOOT_BOTH, * P_HEADFOOT_BOTH;

enum PAGE_HEFO_SWITCHES
{
    PAGE_HEFO_PAGE_BREAK,
    PAGE_HEFO_PAGE_NUM,

    PAGE_HEFO_HEADER_EVEN,
    PAGE_HEFO_HEADER_ODD,

    PAGE_HEFO_FOOTER_EVEN,
    PAGE_HEFO_FOOTER_ODD,

    PAGE_HEFO_HEADER_FIRST,
    PAGE_HEFO_FOOTER_FIRST,

    PAGE_HEFO_COUNT
};

typedef struct PAGE_HEFO_SELECTOR
{
    BITMAP(bitmap, PAGE_HEFO_COUNT);
}
PAGE_HEFO_SELECTOR, * P_PAGE_HEFO_SELECTOR; typedef const PAGE_HEFO_SELECTOR * PC_PAGE_HEFO_SELECTOR;

static inline void
page_hefo_selector_bit_clear(
    _InoutRef_  P_PAGE_HEFO_SELECTOR p_page_hefo_selector,
    _InVal_     U32 page_hefo_bit_number)
{
    bitmap_bit_clear(p_page_hefo_selector->bitmap, page_hefo_bit_number, N_BITS_ARG(PAGE_HEFO_COUNT));
}

static inline void
page_hefo_selector_bit_set(
    _InoutRef_  P_PAGE_HEFO_SELECTOR p_page_hefo_selector,
    _InVal_     U32 page_hefo_bit_number)
{
    bitmap_bit_set(p_page_hefo_selector->bitmap, page_hefo_bit_number, N_BITS_ARG(PAGE_HEFO_COUNT));
}

_Check_return_
static inline BOOL
page_hefo_selector_bit_test(
    _InRef_     PC_PAGE_HEFO_SELECTOR p_page_hefo_selector,
    _InVal_     U32 page_hefo_bit_number)
{
    return(bitmap_bit_test(p_page_hefo_selector->bitmap, page_hefo_bit_number, N_BITS_ARG(PAGE_HEFO_COUNT)));
}

typedef struct PAGE_HEFO_BREAK
{
    PAGE_HEFO_SELECTOR selector;

    HEADFOOT_DEF header_even;
    HEADFOOT_DEF header_odd;
    HEADFOOT_DEF footer_even;
    HEADFOOT_DEF footer_odd;
    HEADFOOT_DEF header_first;
    HEADFOOT_DEF footer_first;

    PAGE page_y;

    CLIENT_HANDLE uref_client_handle;
    UREF_HANDLE uref_handle;
    REGION region;
    U8 deleted;
}
PAGE_HEFO_BREAK, * P_PAGE_HEFO_BREAK;

/*
exported routines
*/

extern void
headfoot_both_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_HEADFOOT_BOTH p_headfoot_both_out,
    _InVal_     PAGE page_y);

extern void
headfoot_both_from_row_and_page_y(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_HEADFOOT_BOTH p_headfoot_both_out,
    _InVal_     ROW row,
    _InVal_     PAGE page_y);

extern void
headfoot_dispose(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_HEADFOOT_DEF p_headfoot_def,
    _InVal_     BOOL all);

_Check_return_
extern STATUS
headfoot_init(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_HEADFOOT_DEF p_headfoot_def,
    _In_        DATA_SPACE data_space);

extern void
hefod_margins_min(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PIXIT p_header_margin_min,
    _OutRef_    P_PIXIT p_footer_margin_min);

_Check_return_
_Ret_maybenull_
extern P_HEADFOOT_DEF
p_headfoot_def_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ROW p_row,
    _InVal_     PAGE page_y,
    _InVal_     OBJECT_ID object_id_focus);

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK
p_page_hefo_break_from_row_below(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK
p_page_hefo_break_from_row_exact(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK
p_page_hefo_break_new_for_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row,
    _OutRef_    P_STATUS p_status);

_Check_return_
extern BOOL
page_break_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

extern void
page_hefo_break_delete_from_row(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

_Check_return_
_Ret_maybenull_
extern P_PAGE_HEFO_BREAK /* NULL at end */
page_hefo_break_enum(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_ROW p_row /* set to -1 to start */);

_Check_return_
extern PAGE
page_number_from_page_y(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     PAGE page_y);

/*
sk_mark.c
*/

extern void
anchor_to_markers_finish(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
anchor_to_markers_new(
    _DocuRef_   P_DOCU p_docu);

extern void
anchor_to_markers_update(
    _DocuRef_   P_DOCU p_docu);

#define docu_area_from_markers_first(pd, p_da) ( \
    *(p_da) = p_markers_first(pd)->docu_area )

extern void
markers_show(
    _DocuRef_   P_DOCU p_docu);

#define p_docu_area_from_markers_first(pd) \
    &p_markers_first(pd)->docu_area

#define p_markers_first(pd) \
    array_ptr(&(pd)->mark_info_cells.h_markers, MARKERS, 0)

/*
sk_mrof.c
*/

_Check_return_
extern STATUS
mrofmun_build_list(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_mrofmun);

_Check_return_
extern STATUS
mrofmun_get_list(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_HANDLE p_h_mrofmun);

_Check_return_
extern BOOL
mrofmun_style_handle_in_use(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STYLE_HANDLE style_handle);

#define mrofmun_dispose_list(p_docu) \
    al_array_dispose(&p_docu->h_mrofmun)

/*
sk_root.c
*/

/*
exported routines
*/

OBJECT_PROTO(extern, object_skel);

PROC_EVENT_PROTO(extern, docu_flow_object_call);

PROC_EVENT_PROTO(extern, docu_focus_owner_object_call);

_Check_return_
extern STATUS
auto_save(
    _DocuRef_   P_DOCU p_docu,
    _In_        S32 auto_save_period_minutes);

extern void
caret_show_claim(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     BOOL scroll);

_Check_return_
extern STATUS
clip_data_array_handle_prepare(
    _OutRef_    P_ARRAY_HANDLE p_h_clip_data,
    _OutRef_    P_BOOL p_f_local_clip_data);

_Check_return_
extern STATUS
clip_data_save(
    _In_z_      PCTSTR filename,
    _InRef_     PC_ARRAY_HANDLE p_h_clip_data);

extern void
clip_data_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL f_local_clip_data,
    _In_        ARRAY_HANDLE h_clip_data,
    _InVal_     BOOL clip_data_from_cut_operation,
    _InVal_     OBJECT_ID object_id);

extern void
global_clip_data_dispose(void);

extern void
global_clip_data_set(
    _DocuRef_   P_DOCU p_docu,
    _In_        ARRAY_HANDLE h_clip_data,
    _InVal_     BOOL clip_data_from_cut_operation,
    _InVal_     OBJECT_ID object_id);

extern void
local_clip_data_dispose(void);

_Check_return_
extern ARRAY_HANDLE
local_clip_data_query(void);

extern void
local_clip_data_set(
    _InVal_     ARRAY_HANDLE h_clip_data,
    _InVal_     BOOL clip_data_from_cut_operation);

_Check_return_
extern STATUS
create_error(
    _InVal_     STATUS status);

_Check_return_
extern STATUS
status_nomem(void);

_Check_return_
extern STATUS
status_check(void);

#if RELEASED
#define create_error(status)    (status)
#define status_nomem()          (STATUS_NOMEM)
#define status_check()          (STATUS_CHECK)
#endif

extern void
cur_change_after(
    _DocuRef_   P_DOCU p_docu);

extern void
cur_change_before(
    _DocuRef_   P_DOCU p_docu);

/* selected object grab handles */
#define PIXIT_RECT_EAR_NONE (-1)
#define PIXIT_RECT_EAR_CENTRE 0
#define PIXIT_RECT_EAR_TL 1
#define PIXIT_RECT_EAR_TC 2
#define PIXIT_RECT_EAR_LC 3
#define PIXIT_RECT_EAR_BL 4
#define PIXIT_RECT_EAR_TR 5
#define PIXIT_RECT_EAR_BC 6
#define PIXIT_RECT_EAR_RC 7
#define PIXIT_RECT_EAR_BR 8
#define PIXIT_RECT_EAR_COUNT 9

typedef struct PIXIT_RECT_EARS
{
    PIXIT_RECT ear[PIXIT_RECT_EAR_COUNT];
    BOOL ear_active[PIXIT_RECT_EAR_COUNT];
    PIXIT_RECT outer_bound;
}
PIXIT_RECT_EARS, * P_PIXIT_RECT_EARS;

extern void
pixit_rect_get_ears(
    _OutRef_    P_PIXIT_RECT_EARS p_rect_ears,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_PIXIT_POINT p_one_program_pixel);

/*
sk_uref.c
*/

typedef struct UREF_ID
{
    REGION region;
    CLIENT_HANDLE client_handle;        /* clients reference - supplied by him */
    UREF_HANDLE uref_handle;            /* clients reference - supplied by us to him */
}
UREF_ID, * P_UREF_ID;

#define UREF_HANDLE_INVALID ((UREF_HANDLE) -1)

/*
uref block
*/

typedef union UREF_PARMS_SOURCE
{
    REGION region;
}
UREF_PARMS_SOURCE;

typedef union UREF_PARMS_TARGET
{
    SLR slr;
}
UREF_PARMS_TARGET;

typedef struct UREF_PARMS
{
    UREF_PARMS_SOURCE source;
    UREF_PARMS_TARGET target;
    UBF add_col : 1;
    UBF add_row : 1;
}
UREF_PARMS, * P_UREF_PARMS;

typedef struct UREF_EVENT_BLOCK_REASON
{
    UBF code : 4;
}
UREF_EVENT_BLOCK_REASON;

typedef struct UREF_EVENT_BLOCK
{
    UREF_PARMS uref_parms;
    UREF_ID uref_id;
    UREF_EVENT_BLOCK_REASON reason;
}
UREF_EVENT_BLOCK, * P_UREF_EVENT_BLOCK;

/*
uref event procedure prototypes
*/

typedef STATUS (* P_PROC_UREF_EVENT) (
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block);

#define PROC_UREF_EVENT_PROTO(_e_s, _proc_name) \
_e_s STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InVal_     T5_MESSAGE t5_message, \
    _InoutRef_  P_UREF_EVENT_BLOCK p_uref_event_block)

typedef P_PROC_UREF_EVENT * P_P_PROC_UREF_EVENT;

/*
internal communication in uref
*/

enum UREF_COMMS
{
    DEP_DELETE, /* dependent needs deleting */
    DEP_UPDATE, /* dependent reference needs updating */
    DEP_INFORM, /* dependent needs informing */
    DEP_NONE    /* no action for dependent */
};

_Check_return_
extern STATUS
uref_add_dependency(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REGION p_region,
    _InRef_     P_PROC_UREF_EVENT p_proc_uref_event /* uref event routine */,
    _InVal_     CLIENT_HANDLE client_handle /* clients handle in */,
    _OutRef_    P_UREF_HANDLE p_uref_handle /* urefs handle out */,
    _InVal_     BOOL simply_allocate);

_Check_return_
extern STATUS
uref_change_handle(
    _InVal_     DOCNO docno,
    _InVal_     UREF_HANDLE uref_handle,
    _InVal_     CLIENT_HANDLE client_handle);

extern void
uref_del_dependency(
    _InVal_     DOCNO docno,
    _InVal_     UREF_HANDLE uref_handle);

PROC_EVENT_PROTO(extern, uref_event);

extern S32 /* reason code out */
uref_match_docu_area(
    _InoutRef_  P_DOCU_AREA p_docu_area,
    _InVal_     T5_MESSAGE t5_message,
    P_UREF_EVENT_BLOCK p_uref_event_block);

extern S32 /* reason code out */
uref_match_region(
    _InoutRef_  P_REGION p_region,
    _InVal_     T5_MESSAGE t5_message,
    P_UREF_EVENT_BLOCK p_uref_event_block);

extern S32 /* reason code out */
uref_match_slr(
    _InoutRef_  P_SLR p_slr,
    _InVal_     T5_MESSAGE t5_message,
    P_UREF_EVENT_BLOCK p_uref_event_block);

#if TRACE_ALLOWED

extern void
uref_trace_reason(
    _InVal_     S32 reason_code,
    _In_z_      PCTSTR tstr);

#endif

/*
sk_plain.c
*/

/*
plain text output
*/

enum PLAIN_EFFECT
{
    PLAIN_EFFECT_BOLD,
    PLAIN_EFFECT_UNDERLINE,
    PLAIN_EFFECT_ITALIC,
    PLAIN_EFFECT_SUPER,
    PLAIN_EFFECT_SUB,
    PLAIN_EFFECT_COUNT
};

_Check_return_
extern STATUS
plain_text_chars_out(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(count) PC_UCHARS uchars,
    _InVal_     S32 count);

_Check_return_
extern STATUS
plain_text_effect_switch(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     enum PLAIN_EFFECT effect,
    _InVal_     S32 state);

_Check_return_
extern STATUS
plain_text_effects_update(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_U8 effects,
    _InRef_     PC_FONT_SPEC p_font_spec);

_Check_return_
extern STATUS
plain_text_effects_off(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_U8 effects);

_Check_return_
extern STATUS
plain_text_segment_out(
    _InoutRef_  P_ARRAY_HANDLE p_h_plain_text /*extended*/,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*terminated*/);

_Check_return_
extern STATUS
plain_text_spaces_out(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     S32 count);

extern void
plain_text_dispose(
    _InoutRef_  P_ARRAY_HANDLE p_h_plain_text);

_Check_return_
extern STATUS
plain_text_page_to_file(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  FILE_HANDLE file_out,
    _InRef_     PC_PAGE_NUM p_page_num);

#define chars_from_pixit(pixit, ppc) ( \
    ((pixit) + (ppc) / 2)  / (ppc) )

#define lines_from_pixit(pixit, ppl) ( \
    ((pixit)) / (ppl) )

/*
startup.c
*/

#if RISCOS

_Check_return_
extern STATUS
ensure_memory_froth(void);

#else
#define ensure_memory_froth() STATUS_OK
#endif /* OS */

_Check_return_
extern STATUS
startup_t5_application_1(void);

_Check_return_
extern STATUS
startup_t5_application_2(void);

#if WINDOWS

extern void
t5_do_exit(void);

#endif /* OS */

extern void
t5_glue_objects(void);

/*
ui_data2.c
*/

/*
exported variables
*/

extern const ARRAY_INIT_BLOCK
array_init_block_ui_text;

AL_ARRAY_ALLOC_PROTO(extern, UI_TEXT);
AL_ARRAY_EXTEND_BY_PROTO(extern, UI_TEXT);
AL_ARRAY_INSERT_BEFORE_PROTO(extern, UI_TEXT);

/*
view.c
*/

/* only a couple of procs in view.c know about handle 0 avoidance */

#define view_void_entry(p_view) \
    (p_view)->flags.deleted

#define viewno_from_p_view(p_view) \
    (p_view)->viewno

/*
display modes to control how much of document the bacon slicer removes
*/

#define DISPLAY_DESK_AREA  0
#define DISPLAY_PRINT_AREA 1
#define DISPLAY_WORK_AREA  2
typedef int DISPLAY_MODE;

typedef struct VIEW_OBJECT_ID_TOOLBAR /* needs to be here unfortunately */
{
    ARRAY_HANDLE h_toolbar_view_row_desc; /*[] of T5_TOOLBAR_VIEW_ROW_DESC */
    U32 nobble_scheduled_event_id;
#if RISCOS
    PIXIT_RECT status_line_pixit_rect;
#elif WINDOWS
    /* for tooltips */
    HOST_WND hwndTT;
    UI_TEXT ui_text;
#endif
}
VIEW_OBJECT_ID_TOOLBAR;

/*
views have an array of main (non-edge-or-pane) windows
*/

/* _main_window_identifiers */
enum WIN_MAIN_ID
{
    WIN_BACK = 0,
    WIN_TOOLBAR,
#if WINDOWS || defined(RISCOS_SLE_ICON)
    WIN_SLE,
#endif
#if WINDOWS
    WIN_UPPER_STATUS_LINE,
    WIN_LOWER_STATUS_LINE,
#endif
    NUMMAIN
};

typedef struct VIEW_MAIN_WINDOW
{
    HOST_WND hwnd;
}
VIEW_MAIN_WINDOW;

/*
views have an array of edge windows
*/

/* _edge_window_identifiers */
typedef enum EDGE_ID
{
    EDGE_ID_START = 0,

    WIN_BORDER_HORZ_SPLIT = 0,
    WIN_BORDER_HORZ = 1,
    WIN_BORDER_VERT_SPLIT = 2,
    WIN_BORDER_VERT = 3,

    WIN_RULER_HORZ_SPLIT = 4,
    WIN_RULER_HORZ = 5,
    WIN_RULER_VERT_SPLIT = 6,
    WIN_RULER_VERT = 7,

#if WINDOWS
/* this has to maintain scroll controls too */
    WIN_HSCROLL_SPLIT_HORZ = 8,
    WIN_HSCROLL = 9,
    WIN_VSCROLL_SPLIT_VERT = 10,
    WIN_VSCROLL = 11,
#endif

    NUMEDGE
}
EDGE_ID;

#define EDGE_ID_INCR(edge_id__ref) \
    ENUM_INCR(EDGE_ID, edge_id__ref)

#define EDGE_ID_DECR(edge_id__ref) \
    ENUM_DECR(EDGE_ID, edge_id__ref)

typedef struct VIEW_EDGE_WINDOW
{
    HOST_WND hwnd;
}
VIEW_EDGE_WINDOW;

/*
pane definition
*/

typedef enum PANE_ID
{
    WIN_PANE_NONE = -1, /* only for Windows' host_event_desc_table */

    PANE_ID_START = 0,

    WIN_PANE_SPLIT_DIAG = 0,
    WIN_PANE_SPLIT_VERT = 1,
    WIN_PANE_SPLIT_HORZ = 2,
    WIN_PANE = 3,

    NUMPANE
}
PANE_ID;

#define PANE_ID_INCR(pane_id__ref) \
    ENUM_INCR(PANE_ID, pane_id__ref)

#define PANE_ID_DECR(pane_id__ref) \
    ENUM_DECR(PANE_ID, pane_id__ref)

_Check_return_
static inline BOOL
IS_PANE_ID_VALID(
    _InVal_     PANE_ID pane_id)
{
    return((U32) pane_id < (U32) NUMPANE);
}

typedef struct PANE_FLAGS
{
    UBF had_break : 1;              /* bit used by rfi handler: indicates break */
}
PANE_FLAGS;

#if RISCOS
typedef struct PANE_LASTOPEN
{
    GR_BOX margin;
    int width;
    int height;
    int xscroll;
    int yscroll;
}
PANE_LASTOPEN;
#endif

struct PANE
{
    HOST_WND hwnd;

    PIXIT_RECT visible_pixit_rect;
    SKEL_RECT  visible_skel_rect;
    ROW_RANGE  visible_row_range;

    PANE_FLAGS flags;

    GDI_POINT size;                 /* current pixel size of pane */
    GDI_POINT scroll;               /* current pixel scroll posn of pane */

#if RISCOS
    PANE_LASTOPEN lastopen;
#elif WINDOWS
    RECT rect;                      /* bounding rectangle of the pane */
    GDI_POINT extent;               /* current pixel extent of pane */
#endif
};

/* _xform_identifiers */
#define XFORM_BACK 0
#define XFORM_HORZ 1
#define XFORM_VERT 2
#define XFORM_PANE 3
#define NUMXFORM 4

#define XFORM_TOOLBAR XFORM_BACK

typedef struct VIEW_FLAGS
{
    UBF horz_split_on   : 1;     /* window is split along horizontal axis */
    UBF vert_split_on   : 1;     /* window is split along vertical axis */
    UBF spare_0         : 1;
    UBF paper_off       : 1;     /* paper mode disabled for window */

    UBF horz_ruler_on   : 1;     /* horizontal ruler on */
    UBF vert_ruler_on   : 1;     /* vertical ruler on */
    UBF horz_border_on  : 1;     /* horizontal border on */
    UBF vert_border_on  : 1;     /* vertical border on */

    UBF spare_1         : 1;

    UBF h_v_b_r_change  : 1;     /* horz/vert border/ruler changed in open */

    UBF maximized       : 1;     /* window opened at maximum */
    UBF minimized       : 1;     /* window opened as icon on backdrop */

    UBF deleted         : 1;

    UBF pseudo_horz_split_on : 1; /* Windows only */
    UBF pseudo_vert_split_on : 1; /* Windows only */

    UBF view_not_yet_open : 1;

    UBF spare_2 : 16;
}
VIEW_FLAGS;

struct VIEW
{
    DOCNO  docno;  /* link to relevant document */
    VIEWNO viewno; /* view number               */
    U8 spare_1;
    U8 spare_2;

    VIEW_MAIN_WINDOW main[NUMMAIN];

    VIEW_EDGE_WINDOW edge[NUMEDGE];

    PANE pane[NUMPANE];

    PANE_ID cur_pane;               /* current pane */

#if RISCOS
    PTSTR   window_title;
    GDI_BOX margin;                 /* pane area open dimensions relative to those of backwindow */
    GDI_BOX scaled_paneextent;      /* scaled panewindow extent */
    GDI_BOX scaled_backextent;      /* scaled backwindow extent */
#endif

#if WINDOWS
    struct VIEW_PANE_POSN
    {
        POINT start;                /* top left of pane area */
        POINT split;                /* top left of split frame area */
        POINT grab;                 /* top left of split grab widget */
        POINT max;                  /* max point in working area */

        POINT inner_size;           /* width and height of inner pane set */
        POINT outer_size;           /* width and height of outer pane set */
        POINT grab_size;            /* width and height of grab area */
    }
    pane_posn;

    GDI_RECT outer_frame_gdi_rect;  /* container frame for all objects */
    GDI_RECT inner_frame_gdi_rect;  /* container frame for all objects */
#endif

    /* some display details */

    PAGE_NUM tl_visible_page;
    PAGE_NUM br_visible_page;

    DISPLAY_MODE display_mode;      /* 0=show desk,paper style,page_style & cells / 1=show page_style & cells / 2=show cells */

    PIXIT horz_split_pos;           /* diz: 21/2/94: Richard sez they be WMUNIT's - "lair, lair, pants on fire" */
    PIXIT vert_split_pos;

    PIXIT width;                    /* see comment above about WMUNIT's */
    PIXIT height;

    S32 scalet;                     /* top of scale factor */
    S32 scaleb;                     /* bottom of scale factor */

    VIEW_FLAGS flags;

    PIXIT_POINT paneextent;         /* number of pages * (paper_on ? (page size+desk strip) : printable area) */

    HOST_XFORM host_xform[NUMXFORM];

    VIEW_OBJECT_ID_TOOLBAR toolbar;

    GDI_COORD horz_ruler_gdi_height; /* h/v ruler width and height (in device units: OS units on RISC OS) */
    GDI_COORD vert_ruler_gdi_width;
    GDI_COORD horz_border_gdi_height; /* h/v border width and height (in device units: OS units on RISC OS) */
    GDI_COORD vert_border_gdi_width;

    PIXIT_RECT backwindow_pixit_rect;
};

/******************************************************************************
*
* These structures control the cutting of view-space
* coordinates into page based skel coordinates
* Both ordinates in a pane window are cut into pages,
* whereas 'edge' windows only have one ordinate cut
* Back window coordinates are totally uncut
*
* These are completely private to view but are
* stored per docu - do NOT access them!
*
******************************************************************************/

typedef struct CTRL_VIEW_SLICE
{
    BOOL x; /* TRUE for pane, horz border|ruler */
    BOOL y; /* TRUE for pane, vert border|ruler */
}
CTRL_VIEW_SLICE;

typedef struct CTRL_VIEW_SKEL
{
    CTRL_VIEW_SLICE slice;

    REDRAW_TAG_AND_EVENT paper_below;
    REDRAW_TAG_AND_EVENT print_area_below;
    REDRAW_TAG_AND_EVENT cells_area_below;
    REDRAW_TAG_AND_EVENT cells_area;
    REDRAW_TAG_AND_EVENT paper_above;
    REDRAW_TAG_AND_EVENT print_area_above;
    REDRAW_TAG_AND_EVENT cells_area_above;

    REDRAW_TAG_AND_EVENT margin_header;
    REDRAW_TAG_AND_EVENT margin_footer;
    REDRAW_TAG_AND_EVENT margin_col;
    REDRAW_TAG_AND_EVENT margin_row;
}
CTRL_VIEW_SKEL, * P_CTRL_VIEW_SKEL;

typedef struct VIEWEVENT_VISIBLEAREA_CHANGED
{
    P_VIEW p_view;
    VIEW_RECT visible_area;
}
VIEWEVENT_VISIBLEAREA_CHANGED, * P_VIEWEVENT_VISIBLEAREA_CHANGED;

typedef struct VIEWEVENT_CLICK_DATA_DRAG
{
    P_ANY p_reason_data;
    PIXIT_POINT pixit_delta;
}
VIEWEVENT_CLICK_DATA_DRAG;

typedef struct VIEWEVENT_CLICK_DATA_FILEINSERT
{
    PCTSTR filename;
    T5_FILETYPE t5_filetype;
    S32 safesource; /* TRUE for a real file, FALSE for a scrapfile */
}
VIEWEVENT_CLICK_DATA_FILEINSERT;

typedef union VIEWEVENT_CLICK_DATA /* directed events other than simple click need more data */
{
    VIEWEVENT_CLICK_DATA_DRAG drag;
    VIEWEVENT_CLICK_DATA_FILEINSERT fileinsert;
}
VIEWEVENT_CLICK_DATA;

typedef struct VIEWEVENT_CLICK
{
    CLICK_CONTEXT click_context;
    VIEW_POINT view_point;

    VIEWEVENT_CLICK_DATA data;
}
VIEWEVENT_CLICK, * P_VIEWEVENT_CLICK; typedef const VIEWEVENT_CLICK * PC_VIEWEVENT_CLICK;

typedef struct VIEWEVENT_KEY
{
    S32 keycode;
}
VIEWEVENT_KEY, * P_VIEWEVENT_KEY;

typedef struct SKELCMD_CLOSE_VIEW
{
    P_VIEW p_view;
}
SKELCMD_CLOSE_VIEW, * P_SKELCMD_CLOSE_VIEW;

/*
T5_MSG_VIEW_NEW
*/

typedef struct T5_MSG_VIEW_NEW_BLOCK
{
    P_VIEW p_view;
    STATUS status; /* stick error in here and view will be destroyed */
}
T5_MSG_VIEW_NEW_BLOCK, * P_T5_MSG_VIEW_NEW_BLOCK;

/*
T5_MSG_VIEW_DESTROY
*/

typedef struct T5_MSG_VIEW_DESTROY_BLOCK
{
    P_VIEW p_view;
}
T5_MSG_VIEW_DESTROY_BLOCK, * P_T5_MSG_VIEW_DESTROY_BLOCK;

_Check_return_
extern S32
cur_page_y(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
_Ret_maybenone_
extern P_VIEW
docu_enum_views(
    _DocuRef_   PC_DOCU p_docu,
    _InoutRef_  P_VIEWNO p_viewno /* start me with -1 */);

_Check_return_
_Ret_maybenone_
extern P_VIEW
p_view_from_viewno(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     VIEWNO viewno);

_Check_return_
_Ret_maybenone_
extern P_VIEW
p_view_from_viewno_caret(
    _DocuRef_   PC_DOCU p_docu);

_Check_return_
extern VIEWNO
viewno_from_p_view_fn(
    _ViewRef_maybenone_ PC_VIEW p_view);

extern void
view_destroy(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern void
view_cache_info(
    _ViewRef_   P_VIEW p_view);

extern void
view_grid_step(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PIXIT_POINT p_pixit_step,
    _OutRef_    P_PIXIT_POINT p_desk_margin,
    _InVal_     DISPLAY_MODE display_mode,
    _InVal_     BOOL do_x_slice,
    _InVal_     BOOL do_y_slice);

extern void
view_install_edge_window_handler(
    _InVal_     EDGE_ID edge_id,
    _InRef_     P_PROC_EVENT p_proc_event);

extern void
view_install_main_window_handler(
    _InVal_     U32 win_id,
    _InRef_     P_PROC_EVENT p_proc_event);

extern void
view_install_pane_window_handler(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     LAYER layer,
    _InRef_     P_PROC_EVENT p_proc_event);

extern void
view_install_pane_window_hefo_handlers(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_PROC_EVENT p_proc_event_header,
    _InRef_     P_PROC_EVENT p_proc_event_footer);

extern void
view_show_caret(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    P_SKEL_POINT p_skel_point,
    _InVal_     PIXIT height);

extern void
view_scroll_caret(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    P_SKEL_POINT p_skel_point,
    _InVal_     PIXIT left,
    _InVal_     PIXIT right,
    _InVal_     PIXIT top,
    _InVal_     PIXIT bottom);

extern void
view_scroll_pane(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKEL_POINT p_skel_point);

extern void
view_visible_skel_rect(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    /*out*/ P_SKEL_RECT p_skel_rect);

extern void
view_update_all(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag);

extern void
view_update_later(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags);

extern void
view_update_later_single(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags);

extern void
view_update_now(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags,
    _In_        REDRAW_FLAGS redraw_flags,
    _InVal_     LAYER layer);

extern void
view_update_now_single(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags,
    _In_        REDRAW_FLAGS redraw_flags,
    _InVal_     LAYER layer);

_Check_return_
extern BOOL
view_update_fast_start(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_REDRAW_CONTEXT p_redraw_context,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags,
    _InVal_     LAYER layer);

_Check_return_
extern BOOL
view_update_fast_continue(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context);

extern void
page_limits_from_page(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PIXIT_POINT p_pixit_point,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_PAGE_NUM p_page_num);

/*ncr*/
extern BOOL
pixit_rect_from_page(
    _DocuRef_   P_DOCU p_docu,
    P_PIXIT_RECT p_work_rect,
    P_PIXIT_RECT p_clip_rect,
    _InVal_     S32 display_mode,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_PAGE_NUM p_page_num);

extern void
view_one_page_display_size_query(
    _DocuRef_   P_DOCU p_docu,
    _In_        DISPLAY_MODE display_mode,
    /*out*/ P_PIXIT_POINT p_size);

extern void
view_back_update_later(
    _DocuRef_   P_DOCU p_docu /*, and some icon ident???*/);

extern void
view_back_update_now(
    _DocuRef_   P_DOCU p_docu /*, and some icon ident???, plus bitmap*/);

extern void
docu_set_and_show_extent_all_views(
    _DocuRef_   P_DOCU p_docu);

extern void
view_cmd_showhide_border(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 show_horz,
    _InVal_     S32 show_vert);

extern void
view_cmd_showhide_ruler(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 show_horz,
    _InVal_     S32 show_vert);

extern void
view_cmd_showhide_paper(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 show);

extern void
view_set_zoom_from_wheel_delta(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 delta);

_Check_return_
extern S32
view_return_extid(
    _DocuRef_   PC_DOCU p_docu,
    _ViewRef_   PC_VIEW p_view);

extern void
view_redraw_page(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKELEVENT_REDRAW p_skel_event_info,
    P_PIXIT_RECT p_print_area);

_Check_return_
extern PIXIT
margin_left_from(
    P_PAGE_DEF p_page_def,
    _InVal_     PAGE page_num_y);

_Check_return_
extern PIXIT
margin_right_from(
    P_PAGE_DEF p_page_def,
    _InVal_     PAGE page_num_y);

extern void
view__update__later(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

_Check_return_
extern U32
viewid_pack(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     U16 event_handler_id);

_Check_return_
_Ret_maybenone_
extern P_DOCU
viewid_unpack(
    _InVal_     U32 packed_view_id,
    _OutRef_    P_P_VIEW p_p_view,
    _OutRef_opt_ P_U16 p_event_handler_id);

/*
vi_edge.c
*/

typedef enum DISPLAY_UNIT
{
    DISPLAY_UNIT_STT    = 0,

    DISPLAY_UNIT_CM     = DISPLAY_UNIT_STT,
    DISPLAY_UNIT_MM     = 1,
    DISPLAY_UNIT_INCHES = 2,
    DISPLAY_UNIT_POINTS = 3,

    DISPLAY_UNIT_COUNT
}
DISPLAY_UNIT;

typedef struct DISPLAY_UNIT_INFO
{
    FP_PIXIT    fp_pixits_per_unit;
    STATUS      unit_message;
}
DISPLAY_UNIT_INFO, * P_DISPLAY_UNIT_INFO;

typedef struct SCALE_INFO
{
    DISPLAY_UNIT display_unit;
    S32          coarse_div;
    S32          fine_div;
    S32          numbered_units_multiplier;
    BOOL         loaded; /* can be smaller if needed */
}
SCALE_INFO, * P_SCALE_INFO; typedef const SCALE_INFO * PC_SCALE_INFO;

extern void
click_stop_initialise(
    _OutRef_    P_FP_PIXIT p_click_stop_step,
    _InRef_     PC_SCALE_INFO p_scale_info,
    _InVal_     BOOL fine);

_Check_return_
extern PIXIT
click_stop_limited(
    _InVal_     PIXIT try_value,
    _InVal_     PIXIT min_value,
    _InVal_     PIXIT max_value,
    _InVal_     PIXIT click_stop_origin,
    _InRef_     PC_FP_PIXIT p_click_stop_step);

extern void
display_unit_info_from_display_unit(
    _OutRef_    P_DISPLAY_UNIT_INFO p_display_unit_info,
    _InVal_     DISPLAY_UNIT display_unit);

extern void
scale_info_from_docu(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     BOOL horizontal,
    _OutRef_    P_SCALE_INFO p_scale_info);

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_border_horz(void);

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_border_vert(void);

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_ruler_horz(void);

_Check_return_
_Ret_valid_
extern PC_STYLE
p_style_for_ruler_vert(void);

_Check_return_
extern PIXIT
view_border_pixit_size(
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL horizontal_border);

extern void
view_border_reset_pixit_size(
    _InVal_     BOOL horizontal_border);

_Check_return_
extern PIXIT
view_ruler_pixit_size(
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL horizontal_ruler);

extern void
view_ruler_reset_pixit_size(
    _InVal_     BOOL horizontal_ruler);

/*
exports from sk_root.c
*/

_Check_return_
_Ret_valid_
extern P_NUMFORM_CONTEXT
get_p_numform_context(
    _DocuRef_   PC_DOCU p_docu);

extern void
skel_numform_context_ensure_valid(
    _DocuRef_   P_DOCU p_docu);

/*
exports from sk_menu.c
*/

typedef enum MENU_ROOT_ID
{
    MENU_ROOT_ID_FIRST = 0,

    MENU_ROOT_DOCU = MENU_ROOT_ID_FIRST,
    MENU_ROOT_ICON_BAR = 1, /* RISC OS */
    MENU_ROOT_FUNCTION_SELECTOR = 2,
    MENU_ROOT_CHART = 3,
    MENU_ROOT_CHART_EDIT = 4,

    MENU_ROOT_MAX
}
MENU_ROOT_ID;

typedef struct MENU_OTHER_INFO
{
    S32 note_behind_count;
    S32 note_picture_count;
    S32 note_selection_count;
    BOOL cells_selection;
    BOOL focus_in_database;
    BOOL focus_in_database_template;
}
MENU_OTHER_INFO, * P_MENU_OTHER_INFO;

#if WINDOWS

extern void
ho_prefer_menu_bar(
    _DocuRef_   P_DOCU p_docu_in,
    _InVal_     S32 menu_id);

#endif

/*
exports from sk_null.c
*/

/* comment out '| (TRACE_OUT | TRACE_ANY)' when not needed */
#define TRACE__NULL         (0 /*| (TRACE_OUT | TRACE_ANY)*/)
#define TRACE__SCHEDULED    (0 /*| (TRACE_OUT | TRACE_ANY)*/)

_Check_return_
extern STATUS
null_events_start(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle);

extern void
null_events_stop(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle);

_Check_return_
extern STATUS
scheduled_event_after(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle,
    _InVal_     MONOTIMEDIFF event_timediff);

_Check_return_
extern STATUS
scheduled_event_at(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle,
    _InVal_     MONOTIME event_time);

extern void
scheduled_event_remove(
    _InVal_     DOCNO docno,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     P_PROC_EVENT p_proc_event,
    _InVal_     CLIENT_HANDLE client_handle);

/*
structure containing marker information
*/

typedef struct MARKERS
{
    DOCU_AREA docu_area;
}
MARKERS, * P_MARKERS; typedef const MARKERS * PC_MARKERS;

/*
info describing markers and markers on screen
*/

typedef struct MARK_INFO
{
    ARRAY_HANDLE h_markers;
    ARRAY_HANDLE h_markers_screen;
}
MARK_INFO, * P_MARK_INFO;

/*
object communication structures
*/

typedef struct RECORDZ_IMPORT
{
    MSG_INSERT_FOREIGN msg_insert_foreign;

    PCTSTR input_filename;
    PCTSTR output_filename;
}
RECORDZ_IMPORT, * P_RECORDZ_IMPORT;

typedef struct NAME_UREF
{
    DOCNO docno;
    DATA_REF data_ref;
}
NAME_UREF, * P_NAME_UREF;

typedef struct DATA_REF_NAME_COMPARE
{
    DOCNO docno;
    DATA_REF data_ref;
    P_U8 p_compound_name;
    S32 result;
}
DATA_REF_NAME_COMPARE, * P_DATA_REF_NAME_COMPARE;

typedef struct OBJECT_HOW_BIG
{
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect;        /* T5_MSG_OBJECT_HOW_BIG supplies tl coordinate */
    PIXIT height_max;           /* maximum height available for object */
}
OBJECT_HOW_BIG, * P_OBJECT_HOW_BIG;

/*
object redraw information
*/

typedef struct OBJECT_REDRAW_FLAGS
{
    UBF show_content   : 1;                             /* copied from skelevent_redraw */
    UBF show_selection : 1;                             /* copied from skelevent_redraw */
    UBF marked_now     : 1;                             /* object is now marked (whole or part) */
    UBF marked_screen  : 1;                             /* object is marked (whole or part) on screen */
    UBF spare          : sizeof(int)*8 - 4*1;
}
OBJECT_REDRAW_FLAGS;

typedef struct OBJECT_REDRAW
{
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect_object;                         /* skel rect covering object */
    SKEL_RECT skel_rect_clip;                           /* bit to be redrawn */
    SKEL_RECT skel_rect_work;                           /* size of current work area */
    PIXIT_RECT pixit_rect_object;                       /* pixit rect covering object */
    RGB rgb_fore;                                       /* foreground colour of whole object (sub-level may change this) */
    RGB rgb_back;                                       /* background colour of object */
    OBJECT_REDRAW_FLAGS flags;
    OBJECT_POSITION start_now;                          /* marker start/end positions: use together with */
    OBJECT_POSITION end_now;                            /* marked_now/marked_screen flags to work out how */
    OBJECT_POSITION start_screen;                       /* much of object is marked */
    OBJECT_POSITION end_screen;
    REDRAW_CONTEXT redraw_context;                      /* host supplied redraw context */
}
OBJECT_REDRAW, * P_OBJECT_REDRAW;

/*
object position information
*/

typedef struct OBJECT_POSITION_FIND
{
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect;                /* object skel_rect */
    SKEL_POINT pos;                     /* IN - position required; OUT - actual position */
    PIXIT caret_height;                 /* OUT - height of caret */
}
OBJECT_POSITION_FIND, * P_OBJECT_POSITION_FIND;

/*
sub-object delete
*/

typedef struct OBJECT_DELETE_SUB
{
    OBJECT_DATA object_data;
    ARRAY_HANDLE h_data_del;    /* OUT - buffer holding deleted data in ownform */
    U8 save_data;               /* do we want saved data ? */
}
OBJECT_DELETE_SUB, * P_OBJECT_DELETE_SUB;

/*
sub-object insert
*/

typedef struct OBJECT_INSERT_SUB
{
    OBJECT_DATA object_data;
    PC_QUICK_UBLOCK p_quick_ublock;
}
OBJECT_INSERT_SUB, * P_OBJECT_INSERT_SUB;

/*
object sent a key press
*/

typedef struct OBJECT_KEY
{
    P_SKELEVENT_KEY p_skelevent_key;
    OBJECT_DATA object_data;
}
OBJECT_KEY, * P_OBJECT_KEY;

/*
object sent several keys at once
*/

typedef struct OBJECT_KEYS
{
    P_SKELEVENT_KEYS p_skelevent_keys;
    OBJECT_DATA object_data;
}
OBJECT_KEYS, * P_OBJECT_KEYS;

/*
object position set
*/

enum OBJECT_POSITION_SET_ACTION
{
    OBJECT_POSITION_SET_START,          /* set object position to start of object */
    OBJECT_POSITION_SET_END,            /* set object position to end of object */
    OBJECT_POSITION_SET_BACK,           /* back one (cursor left) */
    OBJECT_POSITION_SET_FORWARD,        /* forward one (cursor right) */
    OBJECT_POSITION_SET_START_WORD,     /* start of word */
    OBJECT_POSITION_SET_END_WORD,       /* end of word (+1) */
    OBJECT_POSITION_SET_PREV_WORD,      /* previous word */
    OBJECT_POSITION_SET_NEXT_WORD,      /* next word */
    OBJECT_POSITION_SET_START_CLEAR,    /* clear object position if at start of object */
    OBJECT_POSITION_SET_END_CLEAR,      /* clear object position if at end of object */
    OBJECT_POSITION_SET_AT_CHECK,       /* re-set current object position */
    OBJECT_POSITION_SET_NUM
};

typedef struct OBJECT_POSITION_SET
{
    OBJECT_DATA object_data;
    enum OBJECT_POSITION_SET_ACTION action; /* action required */
}
OBJECT_POSITION_SET, * P_OBJECT_POSITION_SET;

/*
object logical movement
*/

enum OBJECT_LOGICAL_MOVE_ACTION
{
    OBJECT_LOGICAL_MOVE_UP,
    OBJECT_LOGICAL_MOVE_DOWN,
    OBJECT_LOGICAL_MOVE_LEFT,
    OBJECT_LOGICAL_MOVE_RIGHT,
    OBJECT_LOGICAL_MOVE_NUM
};

typedef struct OBJECT_LOGICAL_MOVE
{
    OBJECT_DATA object_data;
    enum OBJECT_LOGICAL_MOVE_ACTION action; /* action required */
    SKEL_POINT skel_point_out;              /* output position */
    BOOL use_both_xy;                       /* must use both x and y from point - can't keep caret in line */
}
OBJECT_LOGICAL_MOVE, * P_OBJECT_LOGICAL_MOVE;

/*
object copying
*/

typedef struct OBJECT_COPY
{
    SLR slr_from;
    SLR slr_to;
}
OBJECT_COPY, * P_OBJECT_COPY;

/*
object free resources
*/

typedef struct OBJECT_FREE_RESOURCES
{
    OBJECT_DATA object_data;
}
OBJECT_FREE_RESOURCES, * P_OBJECT_FREE_RESOURCES;

/*
object case setting
*/

typedef struct OBJECT_SET_CASE
{
    OBJECT_DATA object_data;
    BOOL do_redraw;
}
OBJECT_SET_CASE, * P_OBJECT_SET_CASE;

/*
object word counting
*/

typedef struct OBJECT_WORD_COUNT
{
    OBJECT_DATA object_data;
    S32 words_counted;
}
OBJECT_WORD_COUNT, * P_OBJECT_WORD_COUNT;

/*
object how wide call
*/

typedef struct OBJECT_HOW_WIDE
{
    OBJECT_DATA object_data;
    PIXIT width;                        /* if I were to give you all the space in the world, how wide would you be ? */
}
OBJECT_HOW_WIDE, * P_OBJECT_HOW_WIDE;

/*
object plain text non-para call
*/

typedef struct OBJECT_READ_TEXT_DRAFT
{
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect_object;         /* skel rect for object */
    ARRAY_HANDLE h_plain_text;          /* handle to array of strings containing plain text */
}
OBJECT_READ_TEXT_DRAFT, * P_OBJECT_READ_TEXT_DRAFT;

/*
object string search call
*/

typedef struct OBJECT_STRING_SEARCH
{
    OBJECT_DATA object_data;
    PC_USTR ustr_search_for;
    OBJECT_POSITION object_position_found_start;
    OBJECT_POSITION object_position_found_end;
    BOOL ignore_capitals;
    BOOL whole_words;
}
OBJECT_STRING_SEARCH, * P_OBJECT_STRING_SEARCH;

/*
caret movement
*/

typedef struct CARET_SHOW_CLAIM
{
    OBJECT_ID focus;
    BOOL scroll;
}
CARET_SHOW_CLAIM, * P_CARET_SHOW_CLAIM;

/*
selection style enquiry
*/

typedef struct SELECTION_STYLE
{
    STYLE_SELECTOR selector_in;
    STYLE_SELECTOR selector_fuzzy_out;
    STYLE style_out;
}
SELECTION_STYLE, * P_SELECTION_STYLE;

/*
ruler info enquiry
*/

typedef struct RULER_INFO
{
    U8 valid;                   /* set by responder if data is OK */
    COL col;                    /* IN - info for column.. */
    COL_INFO col_info;          /* OUT - info */
}
RULER_INFO, * P_RULER_INFO; typedef const RULER_INFO * PC_RULER_INFO;

/*
column width adjustment
(used for right drag only)
(normal mechanism is STYLE_APPLY)
*/

typedef struct COL_WIDTH_ADJUST
{
    COL col;
    PIXIT width_left;
    PIXIT width_right;
}
COL_WIDTH_ADJUST, * P_COL_WIDTH_ADJUST;

/*
replace selection with supplied inline
*/

typedef struct SELECTION_REPLACE
{
    PC_QUICK_UBLOCK p_quick_ublock;         /* data that is to replace selection */
    OBJECT_POSITION object_position_after;
    BOOL copy_capitals;
}
SELECTION_REPLACE, * P_SELECTION_REPLACE;

/*
object level replace
*/

typedef struct OBJECT_STRING_REPLACE
{
    OBJECT_DATA object_data;
    PC_QUICK_UBLOCK p_quick_ublock;         /* incoming data (inline string) */
    OBJECT_POSITION object_position_after;  /* OUT - object position after the replace */
    BOOL copy_capitals;
}
OBJECT_STRING_REPLACE, * P_OBJECT_STRING_REPLACE;

/*
read contents of object as string
*/

enum OBJECT_READ_TEXT_TYPE
{
    OBJECT_READ_TEXT_PLAIN,         /* read contents text - inlines stripped */
    OBJECT_READ_TEXT_SEARCH,        /* read contents for searching */
    OBJECT_READ_TEXT_RESULT,        /* read result text */
    OBJECT_READ_TEXT_EDIT,          /* read content text for editing */
    OBJECT_READ_TEXT_COUNT
};

typedef struct OBJECT_READ_TEXT
{
    OBJECT_DATA object_data;
    P_QUICK_UBLOCK p_quick_ublock; /*appended*/
    enum OBJECT_READ_TEXT_TYPE type;
}
OBJECT_READ_TEXT, * P_OBJECT_READ_TEXT;

/*
style list change
*/

typedef struct STYLE_DOCU_AREA_CHANGED
{
    DATA_SPACE data_space;
    DOCU_AREA docu_area;
}
STYLE_DOCU_AREA_CHANGED, * P_STYLE_DOCU_AREA_CHANGED;

/*
style change
*/

typedef struct STYLE_CHANGED
{
    STYLE_HANDLE style_handle;
    STYLE_SELECTOR selector;
}
STYLE_CHANGED, * P_STYLE_CHANGED;

/*
dispose of any caches associated with an area
*/

typedef struct CACHES_DISPOSE
{
    REGION region;
    DATA_SPACE data_space;
}
CACHES_DISPOSE, * P_CACHES_DISPOSE;

/*
implied style query
*/

typedef struct IMPLIED_STYLE_QUERY
{
    S32 arg;
    POSITION position;
    PC_STYLE_DOCU_AREA p_style_docu_area;
    P_STYLE p_style; /* style returned by query */
}
IMPLIED_STYLE_QUERY, * P_IMPLIED_STYLE_QUERY;

/*
implied style argument request
*/

typedef struct IMPLIED_STYLE_ARG
{
    S32 arg; /*IN/OUT*/
    T5_MESSAGE t5_message; /*IN*/
}
IMPLIED_STYLE_ARG, * P_IMPLIED_STYLE_ARG;

/*
read mailshot information
*/

typedef struct READ_MAIL_TEXT
{
    S32 field_no;
    P_QUICK_UBLOCK p_quick_ublock;              /*appended*/ /* receives data */
    U8 responded;                               /* set to 1 when responding */
}
READ_MAIL_TEXT, * P_READ_MAIL_TEXT;

/*
document dependent/supporter info
*/

typedef struct DOCU_DEP_SUP * P_DOCU_DEP_SUP;

typedef STATUS (* P_PROC_ENSURE_DOCNO) (
    P_DOCU_DEP_SUP p_docu_dep_sup,
    _InVal_     EV_DOCNO ev_docno);

#define PROC_ENSURE_DOCNO_PROTO(_e_s, _proc_name) \
_e_s STATUS \
_proc_name( \
    P_DOCU_DEP_SUP p_docu_dep_sup, \
    _InVal_     EV_DOCNO ev_docno)

typedef struct DOCU_DEP_SUP
{
    ARRAY_HANDLE h_docnos;
    P_PROC_ENSURE_DOCNO p_proc_ensure_docno;
}
DOCU_DEP_SUP;

/*
column auto width
*/

typedef struct COL_AUTO_WIDTH
{
    COL col;
    ROW row_s;
    ROW row_e;
    BOOL allow_special;
    PIXIT width; /*OUT*/
}
COL_AUTO_WIDTH, * P_COL_AUTO_WIDTH;

/*
compare objects of same type
*/

typedef struct OBJECT_COMPARE
{
    P_ANY p_object_1;
    P_ANY p_object_2;
    S32 res;
}
OBJECT_COMPARE, * P_OBJECT_COMPARE;

/*
make a new object from plain text
*/

typedef struct NEW_OBJECT_FROM_TEXT
{
    DATA_REF data_ref;                          /* place to put object */
    PC_QUICK_UBLOCK p_quick_ublock;             /* text to make object from */
    STATUS status;                              /* soft error from making object */
    S32 pos;                                    /* error position */
    BOOL please_redraw;                         /* do redraw result (or not) */
    BOOL please_uref_overwrite;                 /* do call uref_overwrite (or not) */
    BOOL please_mrofmun;                        /* attempt to autoformat the entry */
}
NEW_OBJECT_FROM_TEXT, * P_NEW_OBJECT_FROM_TEXT;

/*
snapshot data
*/

typedef struct OBJECT_SNAPSHOT
{
    OBJECT_DATA object_data;
    BOOL do_uref_overwrite;
}
OBJECT_SNAPSHOT, * P_OBJECT_SNAPSHOT;

/*
read object data
*/

typedef struct OBJECT_DATA_READ
{
    OBJECT_DATA object_data;
    EV_DATA ev_data;
    BOOL constant;
}
OBJECT_DATA_READ, * P_OBJECT_DATA_READ;

/*
start note object edit
*/

typedef struct NOTE_OBJECT_EDIT_START
{
    STATUS processed;
    P_ANY object_data_ref;
    P_ANY p_note_info;
}
NOTE_OBJECT_EDIT_START, * P_NOTE_OBJECT_EDIT_START;

/*
read name definition from spreadsheet
reads as text for NAME_READ_TEXT message
or data for NAME_READ_DATA message
*/

typedef struct SS_NAME_READ
{
    EV_HANDLE ev_handle;
    EV_DATA ev_data;
    P_USTR ustr_description; /* NB only on loan to caller */
    U8 follow_indirection;
    U8 _spare[3];
}
SS_NAME_READ, * P_SS_NAME_READ;

/*
make spreadsheet name definition
*/

typedef struct SS_NAME_MAKE
{
    PC_USTR ustr_name_id;
    PC_USTR ustr_name_def;
    U8 undefine;
    U8 _spare[3];
    PC_USTR ustr_description;
}
SS_NAME_MAKE, * P_SS_NAME_MAKE;

/*
ensure name thunk exists
*/

typedef struct SS_NAME_ENSURE
{
    PC_USTR ustr_name_id;
    EV_HANDLE ev_handle;
}
SS_NAME_ENSURE, * P_SS_NAME_ENSURE;

/*
given handle, return name id
*/

typedef struct SS_NAME_ID_FROM_HANDLE
{
    /*IN*/
    EV_HANDLE ev_handle;
    DOCNO docno;

    /*OUT*/
    P_QUICK_UBLOCK p_quick_ublock; /*appended*/
}
SS_NAME_ID_FROM_HANDLE, * P_SS_NAME_ID_FROM_HANDLE;

/*
object double click message
*/

typedef struct OBJECT_DOUBLE_CLICK
{
    DATA_REF data_ref;
    U8 processed;
}
OBJECT_DOUBLE_CLICK, * P_OBJECT_DOUBLE_CLICK;

/*
ask people if it's OK to quit (only occurs in low memory conditions)
*/

typedef struct QUIT_OBJECTION
{
    S32 objections;
}
QUIT_OBJECTION, * P_QUIT_OBJECTION;

/*
TAB processed message
*/

typedef struct TAB_WANTED
{
    OBJECT_DATA object_data;
    T5_MESSAGE t5_message;
    BOOL want_inline_insert;
    BOOL processed;
}
TAB_WANTED, * P_TAB_WANTED;

/*
ss telling whoever that name has changed
*/

typedef struct SS_NAME_CHANGE
{
    EV_HANDLE ev_handle;
    BOOL found_use;
    BOOL name_changed;
}
SS_NAME_CHANGE, * P_SS_NAME_CHANGE;

/*
query about a database field
*/

typedef struct FIELD_DATA_QUERY
{
    P_U8 p_compound_name;
    S32 record_no;
    DOCNO docno;
    EV_DATA ev_data;
}
FIELD_DATA_QUERY, * P_FIELD_DATA_QUERY;

/*
query about change of docu focus
*/

typedef struct DOCU_FOCUS_QUERY
{
    DOCNO docno;
    BOOL processed;
}
DOCU_FOCUS_QUERY, * P_DOCU_FOCUS_QUERY;

typedef struct BOX_APPLY
{
    P_ARRAY_HANDLE p_array_handle;
    DATA_SPACE data_space;
    DOCU_AREA docu_area;
    ARGLIST_HANDLE arglist_handle;
}
BOX_APPLY, * P_BOX_APPLY;

typedef struct CSV_READ_RECORD
{
    P_FF_IP_FORMAT p_ff_ip_format;
    ARRAY_HANDLE array_handle; /* [] of EV_DATA */

    QUICK_UBLOCK temp_contents_quick_ublock;
    ARRAY_HANDLE temp_contents_array_handle;

    QUICK_UBLOCK temp_decode_quick_ublock;
    ARRAY_HANDLE temp_decode_array_handle;
}
CSV_READ_RECORD, * P_CSV_READ_RECORD;

#if WINDOWS

/* Structures used for Fireworkz messages etc. on Windows */

typedef struct MSG_FROM_WINDOWS
{
    HWND    hwnd;
    UINT    uiMsg;
    WPARAM  wParam;
    LPARAM  lParam;
}
MSG_FROM_WINDOWS, * P_MSG_FROM_WINDOWS;

typedef struct MSG_FRAME_WINDOW
{
    P_VIEW p_view;
    GDI_RECT gdi_rect;
}
MSG_FRAME_WINDOW, * P_MSG_FRAME_WINDOW;

#endif

/*
reformat data - structure for T5_MSG_REFORMAT
*/

enum DOCU_REFORMAT_ACTION
{
    REFORMAT_XY,                /* cell or hefo area x and y extents */
    REFORMAT_Y,                 /* cell or hefo area y extents only */
    REFORMAT_HEFO_Y             /* hefo margin adjustment */
};

enum DOCU_REFORMAT_DATA_TYPE
{
    REFORMAT_ALL,
    REFORMAT_DOCU_AREA,
    REFORMAT_SLR,
    REFORMAT_ROW
};

typedef union DOCU_REFORMAT_DATA
{
    DOCU_AREA docu_area;
    SLR slr;
    ROW row;
}
DOCU_REFORMAT_DATA;

typedef struct DOCU_REFORMAT
{
    enum DOCU_REFORMAT_ACTION action;
    enum DOCU_REFORMAT_DATA_TYPE data_type;
    DATA_SPACE data_space;
    DOCU_REFORMAT_DATA data;
}
DOCU_REFORMAT; typedef const DOCU_REFORMAT * PC_DOCU_REFORMAT;

typedef struct VIRTUAL_PAGE_INFO
{
    PIXIT row_height;
    PIXIT page_height;
    S32 rows_per_page;
}
VIRTUAL_PAGE_INFO, * P_VIRTUAL_PAGE_INFO;

/*
call from object to checker to check word
*/

typedef struct WORD_CHECK
{
    PC_USTR ustr_word;                          /* word to be checked */
    OBJECT_DATA object_data;
    STATUS status;                              /* did it check out ? */
    BOOL mistake_query;                         /* query the user about the mistake ? */
}
WORD_CHECK, * P_WORD_CHECK;

enum WORD_CHECK_RESULT_CODE
{
    CHECK_CONTINUE,
    CHECK_CONTINUE_RECHECK,
    CHECK_CANCEL
};

/*
call from checker to object to check object
*/

typedef struct OBJECT_CHECK
{
    OBJECT_DATA object_data;
    STATUS status;
}
OBJECT_CHECK, * P_OBJECT_CHECK;

/*
auto spell-check
*/

typedef struct SPELL_AUTO_CHECK
{
    POSITION position_now;
    POSITION position_was;
    OBJECT_DATA object_data;
}
SPELL_AUTO_CHECK, * P_SPELL_AUTO_CHECK;

/*
check protection
*/

typedef struct CHECK_PROTECTION
{
    DOCU_AREA docu_area;
    BOOL status_line_message;
    BOOL use_docu_area;
}
CHECK_PROTECTION, * P_CHECK_PROTECTION;

/*
is object editable ?
*/

typedef struct OBJECT_EDITABLE
{
    OBJECT_DATA object_data;
    BOOL editable;
}
OBJECT_EDITABLE, * P_OBJECT_EDITABLE;

/*
is editing in cell allowed ?
*/

typedef struct OBJECT_IN_CELL_ALLOWED
{
    BOOL in_cell_allowed;
}
OBJECT_IN_CELL_ALLOWED, * P_OBJECT_IN_CELL_ALLOWED;

/*
how big is an object's cells extent?
*/

typedef struct CELLS_EXTENT
{
    REGION region;
}
CELLS_EXTENT, * P_CELLS_EXTENT;

/*
establish what is object at current position:
this message is sent to the focus owner
*/

typedef struct OBJECT_AT_CURRENT_POSITION
{
    OBJECT_ID object_id;
}
OBJECT_AT_CURRENT_POSITION, * P_OBJECT_AT_CURRENT_POSITION;

typedef struct OBJECT_WANTS_LOAD
{
    BOOL object_wants_load;
}
OBJECT_WANTS_LOAD, * P_OBJECT_WANTS_LOAD;

/*
structure of a document
*/

typedef struct DOCU_FLAGS
{
    BOOL has_data;                  /* document has valid data (otherwise it is just a reference thunk) */
    BOOL document_active;           /* set to 1 before T5_MSG_INIT1, set to 0 before T5_MSG_CLOSE1 */
    BOOL init_close;                /* init/close interlock flags */
    BOOL init_close_thunk;          /* for init/close messages */
    BOOL allow_modified_change;
    BOOL read_only;

    BOOL is_current;                /* should have caret */
    BOOL has_input_focus;           /* caret is in one of pane windows */

    BOOL new_extent;                /* set new extent */
    BOOL x_extent_changed;          /* x extent needs recalculating */

    BOOL no_memory_for_bg_format;   /* document ran out of memory in background formatting (RISC OS) */
    BOOL print_margin_warning_issued;

    BOOL caret_position_after_command;
    BOOL caret_position_later;
    BOOL caret_scroll_later;
    BOOL caret_style_on;

    BOOL next_chart_unpinned;       /* one-shot for ob_rec to control charts */

    /* Options section (per DOCU) */
    BOOL base_single_col;           /* set to indicate base document is a single col; 23.10.93 set by incoming
                                        base region spanning whole document with zero column width set */
    BOOL faint_grid;
    BOOL flow_installed;            /* flow object installed */
    BOOL draft_mode;                /* document to be printed in draft mode - formatted and rendered on screen with 'draft' characteristics */

    BOOL virtual_row_table;         /* also used during bodgey loading */
    BOOL null_event_rowtab_format_started;
}
DOCU_FLAGS;

typedef struct RULER_HORZ_INFO
{
    S32 tabbar_highlight;
    RULER_INFO ruler_info;
}
RULER_HORZ_INFO;

/*
instance data for the spreadsheet object
*/

typedef struct DEF_FLAGS
{
    UBF lock : 1; /* global only */
    UBF tobedel : 1;
    UBF blown : 1; /* global only */
    UBF delhold : 1; /* names/custom only */
    UBF undefined : 1; /* names/custom only */
    UBF checkuse : 1; /* names/custom only */
    UBF circ : 1;
}
DEF_FLAGS;

/*
tree structures
*/

typedef struct DEPTABLE
{
    ARRAY_HANDLE h_table;           /* handle of table e.g. [] of CUSTOM_USE, [] of NAME_USE */
    ARRAY_INDEX sorted;             /* sorted limit */
    ARRAY_INDEX mindel;             /* minimum deleted entry */
    DEF_FLAGS flags;
}
DEPTABLE, * P_DEPTABLE;

typedef struct SS_DOC
{
    UREF_HANDLE uref_handle;

    DEPTABLE slr_table;             /* slr dependent table */
    DEPTABLE range_table;           /* range dependent table */
    ARRAY_HANDLE h_range_cols;
    ARRAY_HANDLE h_range_rows;

    S32 nam_ref_count;              /* ref count of names (to this doc) */
    S32 custom_ref_count;           /* ref count of custom functions (to this doc) */

    BOOL custom;                    /* document is a custom function document */
}
SS_DOC, * P_SS_DOC;

#define P_SS_DOC_NONE _P_DATA_NONE(P_SS_DOC)

typedef struct SS_INSTANCE_DATA
{
    SS_DOC ss_doc;
    EV_SLR ev_slr_double_click;
}
SS_INSTANCE_DATA, * P_SS_INSTANCE_DATA;

#if RISCOS

typedef struct REC_INSTANCE_DATA
{
    ARRAY_HANDLE h_cache; /* currently cached information for a record */

    struct REC_INSTANCE_DATA_FRAME
    {
        BOOL on;
        DATA_REF data_ref;
        SKEL_RECT skel_rect;
    }
    frame;

    S32 maeve_click_filter_handle;

    ARRAY_HANDLE array_handle; /* REC_PROJECTOR[] */

    BOOL buttons_enabled;

    BOOL focus_in_database;
    BOOL focus_in_database_template;
}
REC_INSTANCE_DATA, * P_REC_INSTANCE_DATA;

#endif /* OS */

typedef struct DRAW_INSTANCE_DATA
{
    ARRAY_HANDLE h_drawing_list;
}
DRAW_INSTANCE_DATA, * P_DRAW_INSTANCE_DATA;

typedef struct CELLS_INSTANCE_DATA
{
    ARRAY_HANDLE h_text;            /* handle to text being edited in cell */
    DATA_REF data_ref_text;         /* data_ref of text being edited in cell */
    OBJECT_ID object_id;            /* object id of 'new' object*/
    BOOL text_modified;             /* text in buffer been altered ? */
}
CELLS_INSTANCE_DATA, * P_CELLS_INSTANCE_DATA;

#if RISCOS

typedef struct RECB_INSTANCE_DATA
{
    RECORDZ_DEFAULTS recordz_defaults;
    BOOL recordz_defaults_valid;
}
RECB_INSTANCE_DATA, * P_RECB_INSTANCE_DATA;

#endif /* OS */

struct DOCU
{
    DOCU_FLAGS flags;

    DOCU_NAME docu_name;

    EV_DATE file_date;

    DOCNO docno;
    VIEWNO viewno_caret;                /* the view that would like the caret and input focus */
    U8 region_class_limit;
    U8 _spare_u8_here;

    OBJECT_ID object_id_flow;           /* flow structure object id */
    OBJECT_ID focus_owner;              /* owner of input focus: head/foot/cells */

    S32 modified;                       /* modified index number (0=unmodified) */

    ARRAY_HANDLE h_maeve_table;         /* handle to master event table */

    ARRAY_HANDLE h_uref_table;          /* document region dependents (UREF) */

    ARRAY_HANDLE h_view_table;
    S32 n_views;

    /* stylistics */
    ARRAY_HANDLE h_style_list;
    ARRAY_HANDLE h_style_docu_area;     /* style hierarchy */
    ARRAY_HANDLE h_style_cache_slr;     /* cache of styles */
    ARRAY_HANDLE h_style_cache_sub;     /* cache of styles */

    ARRAY_HANDLE h_col_list;            /* array of columns */
    COL cols_logical;                   /* logical number of columns */

    ARRAY_HANDLE h_rowtab;              /* array of ROWTAB */
    ROW rows_logical;                   /* logical number of rows */
    ROW format_start_row;               /* first row that's not formatted */
    ROW last_used_row;                  /* last row accessed in table */

    /* document extents */
    PAGE _last_page_y;                  /* >>USE procedure last_page_y(p_docu) */
    PAGE _last_page_x;                  /* >>USE procedure last_page_x(p_docu) */

    /* position info */
    POSITION old;                       /* old position */
    POSITION cur;                       /* current position */
    PIXIT caret_height;                 /* caret height */
    SKEL_POINT caret;                   /* caret position */
    SKEL_EDGE caret_x;                  /* anti-drift caret x position */
    PAGE cur_page_y;

    /* markers */
    MARKERS anchor_mark;                /* physical markers in progress */
    MARK_INFO mark_info_cells;           /* markers for cells area */

    PHYS_PAGE_DEF phys_page_def;
    ARRAY_HANDLE loaded_phys_page_defs;
    S32 paper_scale;
    PAGE_DEF page_def;                  /* current page definition (after scaling) */

    ARRAY_HANDLE h_page_hefo;           /* header/footer/page breaks */
    MARK_INFO mark_info_hefo;           /* hefo markers */
    DATA_REF mark_focus_hefo;           /* hefo marker focus indicator */
    HEFO_POSITION hefo_position;        /* current position in header/footer */

    TAB_TYPE insert_tab_type;
    U8 _spare_u8[3];

    S32 auto_save_period_minutes;

    ARRAY_HANDLE h_arglist_search;      /* arglist for search */

    LIST_BLOCK defined_keys;

    struct MENU_ROOT * p_menu_root;     /* NULL -> no menus of our own */
    ARRAY_HANDLE ho_menu_data;          /* windows version has menu information hanging from this array */

    P_NUMFORM_CONTEXT p_numform_context;
    ARRAY_HANDLE numforms;              /* for UI */

    ARRAY_HANDLE h_mrofmun;             /* array of mrofmun strings */

    SCALE_INFO scale_info;              /* ruler scale */
    SCALE_INFO vscale_info;
    RULER_HORZ_INFO ruler_horz_info;

    VIRTUAL_PAGE_INFO virtual_page_info;

    CTRL_VIEW_SKEL ctrl_view_skel_pane_window; /* whacked out speed up means having one note-laden document doesn't slow 'em all down */

    OBJECT_ID focus_owner_old;

    FONT_SPEC font_spec_at_caret;       /* record for caret styles of style before current caret region */

    ARRAY_HANDLE h_toolbar;

    UI_TEXT status_line_ui_text;
    ARRAY_HANDLE status_line_info;

    P_ALLOCBLOCK general_string_alloc_block;

    /* object instance data */
    LIST_BLOCK object_data_list;            /* data that belongs to objects */

    ARRAY_HANDLE h_text_cache;              /* formatted text cache */

    S32 modified_text;                      /* modified index (belongs to text object) */

    S32 spelling_mistakes;                  /* belongs to spell object (save lookup time) */

    ARRAY_HANDLE chart_automapper;          /* handle of locally loaded picture/marker mappings (belongs to chart object) */
    LIST_ITEMNO last_chart_edited;          /* to add to chart stuff */

    ARRAY_HANDLE h_note_list;               /* array of type NOTE_INFO (belongs to note object) */

    SS_INSTANCE_DATA ss_instance_data;      /* SKS 14apr96 stick this in here to save lookup time */

#if RISCOS
    REC_INSTANCE_DATA rec_instance_data;    /* SKS 12apr96 stick this in here to save lookup time */
#endif /* OS */

    DRAW_INSTANCE_DATA draw_instance_data;  /* SKS 14apr96 stick this in here to save lookup time */

    CELLS_INSTANCE_DATA cells_instance_data; /* SKS 24feb14 stick this in here to save lookup time */

#if RISCOS
    RECB_INSTANCE_DATA recb_instance_data;  /* SKS 24feb14 stick this in here to save lookup time */
#endif /* OS */
};

/* no longer rounded out in size as it's referenced by a pointer in the doc table (which is now private to sk_docno.c) */

#define  P_DOCU_NONE                    PTR_NONE_X(P_DOCU, 2)
#define IS_DOCU_NONE(p_docu)         IS_PTR_NONE_X(P_DOCU, 2, p_docu)
#define    DOCU_ASSERT(p_docu) \
    assert(!IS_DOCU_NONE(p_docu))

#define  P_VIEW_NONE                    PTR_NONE_X(P_VIEW, 3)
#define IS_VIEW_NONE(p_view)         IS_PTR_NONE_X(P_VIEW, 3, p_view)
#define    VIEW_ASSERT(p_view) \
    assert(!IS_VIEW_NONE(p_view))

_Check_return_
_Ret_valid_
static inline P_SS_INSTANCE_DATA
p_object_instance_data_SS(
    _InRef_     P_DOCU p_docu)
{
    P_SS_INSTANCE_DATA p_ss_instance_data = &p_docu->ss_instance_data;
    return(p_ss_instance_data);
}

#if RISCOS

_Check_return_
_Ret_valid_
static inline P_REC_INSTANCE_DATA
p_object_instance_data_REC(
    _InRef_     P_DOCU p_docu)
{
    P_REC_INSTANCE_DATA p_rec_instance_data = &p_docu->rec_instance_data;
    return(p_rec_instance_data);
}

#endif /* OS */

_Check_return_
_Ret_valid_
static inline P_DRAW_INSTANCE_DATA
p_object_instance_data_DRAW(
    _InRef_     P_DOCU p_docu)
{
    P_DRAW_INSTANCE_DATA p_draw_instance_data = &p_docu->draw_instance_data;
    return(p_draw_instance_data);
}

_Check_return_
_Ret_valid_
static inline P_CELLS_INSTANCE_DATA
p_object_instance_data_CELLS(
    _InRef_     P_DOCU p_docu)
{
    P_CELLS_INSTANCE_DATA p_cells_instance_data = &p_docu->cells_instance_data;
    return(p_cells_instance_data);
}

#if RISCOS

_Check_return_
_Ret_valid_
static inline P_RECB_INSTANCE_DATA
p_object_instance_data_RECB(
    _InRef_     P_DOCU p_docu)
{
    P_RECB_INSTANCE_DATA p_recb_instance_data = &p_docu->recb_instance_data;
    return(p_recb_instance_data);
}

#endif /* OS */

/*
macros
*/

#define all_cols(p_docu) ( \
    MAX_COL - 1 )

#define all_rows(p_docu) ( \
    MAX_ROW - 1 )

#define n_cols_logical(p_docu) ( \
    (p_docu)->cols_logical )

#define n_cols_physical(p_docu) ( \
    (COL) array_elements(&(p_docu)->h_col_list) )

/*
sk_area.c
*/

#define col_in_docu_area(p_docu_area, _col) (   \
    (p_docu_area)->whole_row ||                 \
    ((_col) >= (p_docu_area)->tl.slr.col &&     \
     (_col) <  (p_docu_area)->br.slr.col)       )

#define col_in_region(p_region, _col) ( \
    (p_region)->whole_row ||            \
    ((_col) >= (p_region)->tl.col &&    \
     (_col) <  (p_region)->br.col)      )

_Check_return_
extern BOOL
data_refs_equal(
    _InRef_     PC_DATA_REF p_data_ref_1,
    _InRef_     PC_DATA_REF p_data_ref_2);

_Check_return_
extern BOOL
data_ref_in_region(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     DATA_SPACE data_space,
    _InRef_     PC_REGION p_region,
    _InRef_     PC_DATA_REF p_data_ref);

extern void
docu_area_clean_up(
    _InoutRef_  P_DOCU_AREA p_docu_area);

typedef struct COALESCE_FLAGS
{
    UBF cols : 1;
    UBF rows : 1;
    UBF frags : 1;
}
COALESCE_FLAGS;

/*ncr*/
extern BOOL
docu_area_coalesce_docu_area_out(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2,
    _In_        COALESCE_FLAGS coalesce_flags);

_Check_return_
extern BOOL
docu_area_is_empty(
    _InRef_     PC_DOCU_AREA p_docu_area);

_Check_return_
extern BOOL
docu_area_equal(
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2);

#define docu_area_frag_end(p_da) ( \
    !(p_da)->whole_col &&                                    \
    !(p_da)->whole_row &&                                    \
    (OBJECT_ID_NONE != (p_da)->br.object_position.object_id) )

#define docu_area_frag_start(p_da) ( \
    !(p_da)->whole_col &&                                    \
    !(p_da)->whole_row &&                                    \
    (OBJECT_ID_NONE != (p_da)->tl.object_position.object_id) )

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_object_data(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_OBJECT_DATA p_object_data);

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_position(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position);

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_position_max(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position);

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_region(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_REGION p_region);

/*ncr*/
_Ret_valid_
extern P_DOCU_AREA
docu_area_from_slr(
    _OutRef_    P_DOCU_AREA p_docu_area,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern BOOL
docu_area_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2);

_Check_return_
extern BOOL /* do they intersect ? */
docu_area_intersect_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2);

/*ncr*/
extern BOOL /* do they intersect ? */
docu_area_intersect_docu_area_out(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2);

_Check_return_
extern BOOL
docu_area_is_frag(
    _InRef_     PC_DOCU_AREA p_docu_area);

_Check_return_
extern BOOL
docu_area_is_cell_or_less(
    _InRef_     PC_DOCU_AREA p_docu_area);

extern void
docu_area_normalise(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in);

extern void
docu_area_normalise_phys(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_in);

extern void
docu_area_offset_to_position(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position_now,
    _InRef_     PC_POSITION p_position_was);

extern void
docu_area_union_docu_area_out(
    _OutRef_    P_DOCU_AREA p_docu_area_out,
    _InRef_     PC_DOCU_AREA p_docu_area_1,
    _InRef_     PC_DOCU_AREA p_docu_area_2);

extern void
limits_from_docu_area(
    _DocuRef_   P_DOCU p_docu,
    _Out_opt_   P_COL p_col_s,
    _Out_opt_   P_COL p_col_e,
    _Out_opt_   P_ROW p_row_s,
    _Out_opt_   P_ROW p_row_e,
    _InRef_     PC_DOCU_AREA p_docu_area);

extern void
limits_from_region(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_COL p_col_s,
    _OutRef_    P_COL p_col_e,
    _OutRef_    P_ROW p_row_s,
    _OutRef_    P_ROW p_row_e,
    _InRef_     PC_REGION p_region);

_Check_return_
extern BOOL
object_position_at_start(
    _InRef_     PC_OBJECT_POSITION p_object_position);

extern void
object_position_set_start(
    _OutRef_    P_OBJECT_POSITION p_object_position,
    _InVal_     OBJECT_ID object_id);

#define OBJECT_POSITION_COMPARE_PP 0    /* two arbitrary points */
#define OBJECT_POSITION_COMPARE_SE 1    /* inclusive/exclusive points from (e.g.) a docu_area */

_Check_return_
extern S32 /* strcmp like result */
object_position_compare(
    _InRef_     PC_OBJECT_POSITION p_object_position_s,
    _InRef_     PC_OBJECT_POSITION p_object_position_e,
    _InVal_     BOOL end_point);

extern void
object_position_max(
    _OutRef_    P_OBJECT_POSITION p_object_position_out,
    _InRef_     PC_OBJECT_POSITION p_object_position_1,
    _InRef_     PC_OBJECT_POSITION p_object_position_2);

extern void
object_position_min(
    _OutRef_    P_OBJECT_POSITION p_object_position_out,
    _InRef_     PC_OBJECT_POSITION p_object_position_1,
    _InRef_     PC_OBJECT_POSITION p_object_position_2);

extern void
offsets_from_object_data(
    _OutRef_    P_S32 p_start,
    _OutRef_    P_S32 p_end,
    _InRef_     PC_OBJECT_DATA p_object_data,
    _InVal_     S32 len);

_Check_return_
extern BOOL
position_clear_object_positions(
    _InoutRef_  P_POSITION p_position,
    _InVal_     S32 end /* position is end-exclusive */);

_Check_return_
extern S32 /* strcmp like result */
position_compare(
    _InRef_     PC_POSITION p_position_1,
    _InRef_     PC_POSITION p_position_2);

_Check_return_
extern BOOL
position_col_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position);

extern void
position_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_POSITION p_position,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_OBJECT_POSITION p_object_position);

_Check_return_
extern BOOL
position_row_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position);

_Check_return_
extern BOOL
position_slr_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position);

_Check_return_
extern BOOL
position_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_POSITION p_position);

_Check_return_
extern BOOL
region_empty(
    _InRef_     PC_REGION p_region);

/*ncr*/
_Ret_valid_
extern P_REGION
region_from_docu_area_max(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_DOCU_AREA p_docu_area);

/*ncr*/
extern BOOL
region_from_docu_area_min(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_DOCU_AREA p_docu_area);

/*ncr*/
_Ret_maybenull_ _Success_(NULL != return)
extern P_REGION
region_from_docu_reformat(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat);

/*ncr*/
_Ret_valid_
extern P_REGION
region_from_two_slrs(
    _OutRef_    P_REGION p_region,
    _InRef_     PC_SLR p_slr_1,
    _InRef_     PC_SLR p_slr_2,
    _InVal_     BOOL add_one_to_br);

_Check_return_
extern BOOL
region_in_region(
    _InRef_     PC_REGION p_region_1,
    _InRef_     PC_REGION p_region_2);

_Check_return_
extern BOOL /* do they intersect ? */
region_intersect_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_REGION p_region);

_Check_return_
extern BOOL
region_intersect_region(
    _InRef_     PC_REGION p_region_1,
    _InRef_     PC_REGION p_region_2);

_Check_return_
extern BOOL
region_intersect_region_out(
    _OutRef_    P_REGION p_region_out,
    _InRef_     PC_REGION p_region_1,
    _InRef_     PC_REGION p_region_2);

extern void
region_span(
    _OutRef_    P_BOOL p_colspan,
    _OutRef_    P_BOOL p_rowspan,
    _InRef_     PC_REGION p_region1,
    _InRef_     PC_REGION p_region2);

#define row_in_docu_area(p_docu_area, _row) (   \
    (p_docu_area)->whole_col ||                 \
    ((_row) >= (p_docu_area)->tl.slr.row &&     \
     (_row) <  (p_docu_area)->br.slr.row)       )

#define row_in_region(p_region, _row) ( \
    (p_region)->whole_col ||            \
    ((_row) >= (p_region)->tl.row &&    \
     (_row) <  (p_region)->br.row)      )

_Check_return_
extern BOOL
skel_rect_empty(
    _InRef_     PC_SKEL_RECT p_skel_rect);

extern void
skel_rect_empty_set(
    _OutRef_    P_SKEL_RECT p_skel_rect);

/*ncr*/
extern BOOL
skel_rect_intersect(
    _Out_opt_   P_SKEL_RECT p_intersect,
    _InRef_     PC_SKEL_RECT p_skel_rect1,
    _InRef_     PC_SKEL_RECT p_skel_rect2);

extern void
skel_rect_move_to_next_page(
    _InoutRef_  P_SKEL_RECT p_skel_rect);

extern void
skel_rect_union(
    _OutRef_    P_SKEL_RECT p_union,
    _InRef_     PC_SKEL_RECT p_skel_rect1,
    _InRef_     PC_SKEL_RECT p_skel_rect2);

_Check_return_
extern BOOL
slr_first_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern BOOL
slr_last_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_SLR p_slr);

/* is the cell in the docu_area ? */

_Check_return_
static inline BOOL
slr_in_docu_area(
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     PC_SLR p_slr)
{
    return( (p_docu_area->whole_row || ((p_slr->col >= p_docu_area->tl.slr.col) && (p_slr->col < p_docu_area->br.slr.col))) &&
            (p_docu_area->whole_col || ((p_slr->row >= p_docu_area->tl.slr.row) && (p_slr->row < p_docu_area->br.slr.row))) );
}

/* is the cell in the region ? */

_Check_return_
static inline BOOL
slr_in_region(
    _InRef_     PC_REGION p_region,
    _InRef_     PC_SLR p_slr)
{
    return( (p_region->whole_row || ((p_slr->col >= p_region->tl.col) && (p_slr->col < p_region->br.col))) &&
            (p_region->whole_col || ((p_slr->row >= p_region->tl.row) && (p_slr->row < p_region->br.row))) );
}

/*
sk_bord.c
*/

_Check_return_
extern STATUS
skel_get_visible_row_ranges(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_row_range_list);

extern void
skel_visible_row_range(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _OutRef_    P_ROW_RANGE p_row_range);

_Check_return_
extern STATUS
style_text_for_marker(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InVal_     STYLE_BIT_NUMBER style_bit_number);

/*
sk_print.c
*/

/*
Exported types
*/

typedef struct PRINT_CTRL_FLAGS
{
    UBF two_up    : 1;
    UBF collate   : 1; /* 0=sorted/1=collated */
    UBF reverse   : 1;
    UBF landscape : 1;
}
PRINT_CTRL_FLAGS;

typedef struct PRINT_CTRL
{
    S32              copies;
    ARRAY_HANDLE     h_page_list;
    PRINT_CTRL_FLAGS flags;
}
PRINT_CTRL, * P_PRINT_CTRL, ** P_P_PRINT_CTRL;

/*
draft print plain text to file
*/

typedef struct DRAFT_PRINT_TO_FILE
{
    P_PRINT_CTRL p_print_ctrl;
    PCTSTR filename;
}
DRAFT_PRINT_TO_FILE, * P_DRAFT_PRINT_TO_FILE;

typedef struct PAGE_ENTRY
{
    PAGE_NUM page;
}
PAGE_ENTRY, * P_PAGE_ENTRY, ** P_P_PAGE_ENTRY; typedef const PAGE_ENTRY * PC_PAGE_ENTRY;

typedef struct PRINTER_PERCENTAGE
{
    S32 final_page_count;
    S32 current_page_count;
    S32 percent_per_page;

    PROCESS_STATUS process_status;
}
PRINTER_PERCENTAGE, * P_PRINTER_PERCENTAGE;

/*
exported routines
*/

extern void
page_def_from_phys_page_def(
    _InoutRef_  P_PAGE_DEF p_page_def,
    _InRef_     PC_PHYS_PAGE_DEF p_phys_page_def,
    _InVal_     S32 paper_scale);

extern void
page_def_validate(
    _InoutRef_  P_PAGE_DEF p_page_def);

extern void
print_percentage_finalise(
    _InoutRef_  P_PRINTER_PERCENTAGE p_printer_percentage);

extern void
print_percentage_initialise(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PRINTER_PERCENTAGE p_printer_percentage,
    _In_        S32 page_count);

extern void
print_percentage_page_inc(
    _InoutRef_  P_PRINTER_PERCENTAGE p_printer_percentage);

extern void
print_percentage_reflect(
    _InoutRef_  P_PRINTER_PERCENTAGE p_printer_percentage,
    _InVal_     S32 sub_percentage);

extern U32 /* used */
get_pagenum(
    _In_z_      PCTSTR tstr_in,
    _OutRef_    P_PAGE_NUM p_page_num);

_Check_return_
extern  STATUS
pagelist_add_blank(
    _InoutRef_  P_ARRAY_HANDLE p_h_page_list);

_Check_return_
extern STATUS
pagelist_add_page(
    _InoutRef_  P_ARRAY_HANDLE p_h_page_list,
    _InVal_     S32 x,
    _InVal_     S32 y);

_Check_return_
extern STATUS
pagelist_add_range(
    _InoutRef_  P_ARRAY_HANDLE p_h_page_list,
    _InVal_     S32 x0,
    _InVal_     S32 y0,
    _InVal_     S32 x1,
    _InVal_     S32 y1);

/* >>>>>>>>>>>>this wants removing soon 18.2.94 */
#ifndef          __sk_col_h
#include "ob_cells/sk_col.h"
#endif

/* >>>>>>>>>>>>this wants removing soon 18.2.94 */
#ifndef          __sk_form_h
#include "ob_cells/sk_form.h"
#endif

_Check_return_
extern STATUS
t5_cmd_activate_menu(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message);

/*
sk_find.c
*/

T5_CMD_PROTO(extern, t5_cmd_search_button_poss_db_queries);
T5_CMD_PROTO(extern, t5_cmd_search_button_poss_db_query);
T5_CMD_PROTO(extern, t5_cmd_search_intro);
T5_CMD_PROTO(extern, t5_cmd_search_do);

/*
sk_table.c
*/

typedef struct STYLE_LOAD_MAP
{
    ARRAY_HANDLE_TSTR h_new_name_tstr;
    ARRAY_HANDLE_TSTR h_old_name_tstr;
}
STYLE_LOAD_MAP, * P_STYLE_LOAD_MAP;

/* KEEP THIS CORRESPONDING WITH CONSTRUCT_TABLE IN sk_table.c */

#define OF_CONSTRUCT_MAX 16
#define BUF_OF_CONSTRUCT_MAX (OF_CONSTRUCT_MAX + 1)

/* KEEP THESE CORRESPONDING WITH ARG TABLES IN sk_table.c */

/*
offsets in args_cell[] (exported for ob_rec)
*/

enum ARG_CELL
{
    ARG_CELL_OWNER = 0,
    ARG_CELL_DATA_TYPE,

    ARG_CELL_COL,
    ARG_CELL_ROW,

    ARG_CELL_CONTENTS,
    ARG_CELL_FORMULA,
    ARG_CELL_MROFMUN,

    ARG_CELL_N_ARGS
};

/*
offsets in args_block[] (exported for ob_rec)
*/

enum ARG_BLOCK
{
    ARG_BLOCK_TL_COL = 0,
    ARG_BLOCK_TL_ROW,
    ARG_BLOCK_TL_DATA,
    ARG_BLOCK_TL_OBJECT_ID,

    ARG_BLOCK_BR_COL,
    ARG_BLOCK_BR_ROW,
    ARG_BLOCK_BR_DATA,
    ARG_BLOCK_BR_OBJECT_ID,

    ARG_BLOCK_DOC_COLS,
    ARG_BLOCK_DOC_ROWS,

    ARG_BLOCK_FA_CS1,
    ARG_BLOCK_FA_CS2,
    ARG_BLOCK_FA,

    ARG_BLOCK_FB_CS1,
    ARG_BLOCK_FB_CS2,
    ARG_BLOCK_FB,

    ARG_BLOCK_N_ARGS
};

/*
exported routine
*/

_Check_return_
extern STATUS
insert_cell_contents_ownform(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_LOAD_CELL_OWNFORM p_load_cell_ownform,
    _InRef_     PC_POSITION p_position);

_Check_return_
extern STATUS
insert_cell_contents_foreign(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     T5_MESSAGE t5_message,
    _InoutRef_  P_LOAD_CELL_FOREIGN p_load_cell_foreign);

_Check_return_
extern STATUS
insert_cell_style_for_foreign(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_LOAD_CELL_FOREIGN p_load_cell_foreign);

_Check_return_
extern STATUS
t5_cmd_of_end_of_data(
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format);

_Check_return_
extern STATUS
skeleton_load_construct(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
extern STATUS
skeleton_table_startup(void);

/*
exports from prodinfo.c
*/

_Check_return_
_Ret_z_
extern PCTSTR
product_id(void); /* e.g. FireworkzPro */

_Check_return_
_Ret_z_
extern PCTSTR
product_family_id(void); /* e.g. Fireworkz */

_Check_return_
_Ret_z_
extern PCTSTR
product_ui_id(void); /* e.g. Fireworkz<nbsp>Pro */

_Check_return_
_Ret_z_
extern PCTSTR
registration_number(void);

_Check_return_
_Ret_z_
extern PCTSTR
user_id(void);

_Check_return_
_Ret_z_
extern PCTSTR
user_organ_id(void);

/*
ui_misc.h
*/

#define QUERY_COMPLETION_SAVE 1 /* must be the same as DIALOG_COMPLETION_OK but we get into order problems */
#define QUERY_COMPLETION_DISCARD 2

#define LINKED_QUERY_COMPLETION_CLOSE_ALL 1
#define LINKED_QUERY_COMPLETION_CLOSE_DOC 2
#define LINKED_QUERY_COMPLETION_SAVE_ALL 3
#define LINKED_QUERY_COMPLETION_DISCARD_ALL 4

extern S32
docs_modified(void);

extern void
process_close_request(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        BOOL closing_a_doc,
    _In_        BOOL closing_a_view,
    _InVal_     BOOL opening_a_directory);

_Check_return_
extern STATUS
query_quit(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     S32 n_modified);

_Check_return_
extern STATUS
query_save_linked(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
query_save_modified(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
save_all_modified_docs_for_shutdown(void);

_Check_return_
extern STATUS
t5_cmd_info(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
t5_cmd_quit(
    _DocuRef_   P_DOCU p_docu);

/*
exports from ui_save.c
*/

typedef struct INSTALLED_SAVE_OBJECT
{
    OBJECT_ID object_id;
    T5_FILETYPE t5_filetype;
#if WINDOWS
    ARRAY_HANDLE_TSTR h_tstr_ClipboardFormat;
    UINT uClipboardFormat;
#endif
}
INSTALLED_SAVE_OBJECT, * P_INSTALLED_SAVE_OBJECT;

extern ARRAY_HANDLE
installed_save_objects_handle; /* [] of INSTALLED_SAVE_OBJECT */

extern void
style_name_from_marked_area(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_UI_TEXT p_ui_text);

#endif /* __xp_skel_h */

/* end of xp_skel.h */
