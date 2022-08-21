/* ob_rec.h */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __ob_rec_h
#define __ob_rec_h

#include "ob_story/ob_story.h"

#include "ob_rec/resource/resource.h"

#define TITLES_TOO 0

/* Do you want the add record command to
   add a blank record (1)
   or
   a copy of the current record (0)
*/
#define RECORDZ_BAD_FIELD_TO_NULL 1

#define FILETYPE_LINKED_FILE ((T5_FILETYPE) 0x42F86) /* filetype_Link */

/* Which Engine are we using */
#undef DPLIB
#if RISCOS && !defined(DPLIB)
#define DPLIB 1
#else
#define DPLIB 0
#endif

#define MINIMUM_CARD_WIDTH  (640)
#define MINIMUM_CARD_HEIGHT (320)

typedef FIELD_ID * P_FIELD_ID;

typedef U32 TABLE_ID;
#define INVENT_TABLE_ID ((TABLE_ID) 0x00000001)

/* Our own names for the access control functions as an emumeration not a bit set cf datapower.dplib.h.access */
typedef enum ACCESS_FUNCTION
{
    ACCESS_FUNCTION_FULL = 0,
    ACCESS_FUNCTION_READ,
    ACCESS_FUNCTION_EDIT,

    ACCESS_FUNCTION_ADD,
    ACCESS_FUNCTION_DELETE,
    ACCESS_FUNCTION_SEARCH,
    ACCESS_FUNCTION_SORT,

    ACCESS_FUNCTION_LAYOUT,
    ACCESS_FUNCTION_SAVE,
    ACCESS_FUNCTION_PRINT,

    ACCESS_FUNCTION_PASSWORD,

    ACCESS_FUNCTION_NONE,
    ACCESS_FUNCTION_ALL
}
ACCESS_FUNCTION;

/* Field types */

#define FIELD_TEXT      1
#define FIELD_INTEGER   2
#define FIELD_DATE      3
#define FIELD_REAL      4
#define FIELD_BOOL      5
#define FIELD_FILE      6
#define FIELD_FORMULA   7
#define FIELD_INTERVAL  8
#define FIELD_PICTURE   9

typedef U32 FIELD_TYPE; typedef FIELD_TYPE * P_FIELD_TYPE;

#define SEARCH_TYPE_FILE    1
#define SEARCH_TYPE_EXTEND  2
#define SEARCH_TYPE_REFINE  3
#define SEARCH_TYPE_CHANGE  4

/* Stuff for identifying active queries */

typedef struct _illegal_query * QUERY_ID;

#define RECORDZ_WHOLE_FILE ((QUERY_ID) -1)
#define BAD_QUERY_ID       ((QUERY_ID) -1) /* same thing, so that duff queries map to whole file */

/* Record Control */

#define rec_First    (-11) /* go to first card                             */
#define rec_Last     (-12) /* go to last card                              */
#define rec_Next     (-13) /* go to next card                              */
#define rec_Previous (-14) /* go to previous card                          */
#define rec_Current  (-15) /* read current card                            */
#define rec_Dunno    (-16) /* card number not known (+ve => card number n) */
#define rec_Finished (-17) /* on return => no more cards available         */

#if 0
typedef struct RESOURCE
{
    T5_FILETYPE t5_filetype; /* filetype of data in frame */
    ARRAY_HANDLE handle; /* handle of buffer containing data */
    S32 size;            /* current size of data */
    S32 offset;
    S32 buffsize;        /* current space allocated for buffer */
}
RESOURCE, * P_RESOURCE;
#endif

typedef struct REC_RESOURCE
{
    T5_FILETYPE t5_filetype;
    EV_DATA ev_data;
}
REC_RESOURCE, * P_REC_RESOURCE;

typedef struct EDITREC
{
    P_U8 p_u8;
    S32 length;
    T5_FILETYPE t5_filetype;
}
EDITREC, * P_EDITREC;

typedef struct RECORDSPEC
{
    S32 recno;  /* current record number (0..ncards-1) */
    S32 ncards; /* number of records in file */

    ARRAY_HANDLE h_editrecs; /* handle of possible array of pointers to field text with length and type info */

    /* Private to dplib */
    S32 lastvalid;
    S32 dplib_surrogate_key;
}
RECORDSPEC, * P_RECORDSPEC;

/*
Idealised structure repreenting a single field. Don't make these too big or your stack will die
*/

#define FDEFSTRLEN    64 /* no point being bigger, Neil would just crop it */
#define FDEFFORMLEN  256 /* Neil has these as 1024 - eek! If we can stop having these on the stack ... */
#define FDEFCHECKLEN 256
#define FDEFVLISTLEN 256 /* Neil has this as 1024 too */

typedef struct FIELDDEF
{
    TCHARZ name[FDEFSTRLEN];
    FIELD_ID id;
    TABLE_ID parentid;
    S32 length;
    FIELD_TYPE type;

#if DPLIB
    P_ANY p_field; /* DataPower related */
#endif

    S32 keyorder;
    BOOL hidden;
    BOOL compulsory;
    BOOL readable;
    BOOL writeable;

    U8Z default_formula[FDEFFORMLEN];
    U8Z check_formula[FDEFCHECKLEN];
    U8Z value_list[FDEFVLISTLEN];
}
FIELDDEF, * P_FIELDDEF; typedef const FIELDDEF * PC_FIELDDEF;

/*
Structure for an incarnation of a table from the table defs collection TD(i)
*/

typedef struct TABLEDEF
{
    TABLE_ID id;
    ARRAY_HANDLE h_fielddefs;
    P_ANY h_fields;
    TCHARZ name[64];

    /* Other table properties */
    P_ANY private_dp_layout;
    P_ANY private_dp_cursor;
}
TABLEDEF, * P_TABLEDEF; typedef const TABLEDEF * PC_TABLEDEF;

#define SORT_AZ   (+1)
#define SORT_ZA   (-1)
#define SORT_NULL (0)

/* Per-Field remap control */

typedef struct REMAP_ENTRY
{
    P_TABLEDEF p_table;
    FIELD_ID field_id;
    FIELD_ID new_field_id;
    S32 sort_order;    /* -1 for Z..A, +1 for A..Z or 0 for don't care */
}
REMAP_ENTRY, * P_REMAP_ENTRY;

/* Per-Database remap control options */

#define REMAP_OPT_INS_SEQ (1)
#define REMAP_OPT_INS_NUL (0)

typedef struct REMAP_OPTIONS
{
    S32 insert_order;
}
REMAP_OPTIONS, * P_REMAP_OPTIONS;

typedef struct DB
{
    DB_ID id;
    TCHARZ name[BUF_MAX_PATHSTRING];
    S32 n_tables;
    ARRAY_HANDLE h_tabledefs;

    /* Other DB properties*/
    P_ANY private_dp_handle;
}
DB, * P_DB;

typedef struct SEARCH_OPERATOR_INFO
{
    S32 search_operator; /* SKS: operator is a C++ keyword */
    S32 prefix_length;
    S32 suffix_length;
}
SEARCH_OPERATOR_INFO;

typedef struct SEARCH_FIELD_PATTERN
{
    ARRAY_HANDLE_USTR h_text_ustr; /* the textual string of the match pattern */
    FIELD_ID field_id;
    SEARCH_OPERATOR_INFO sop_info;
}
SEARCH_FIELD_PATTERN, * P_SEARCH_FIELD_PATTERN;

typedef struct QUERY
{
    /* Generic items */
    ARRAY_HANDLE_USTR h_name_ustr;
    QUERY_ID query_id;
    QUERY_ID parent_id; /* -1 if a root query otherwise a query id for a compound query */

    S32  search_type;
    BOOL search_andor;
    BOOL search_exclude;
    ARRAY_HANDLE h_search_pattern; /* handle of an array of SEARCH_FIELD_PATTERNs */

    /* Internal DPlib items */
    P_ANY p_qfields;
    P_ANY p_query;
    P_ANY p_expr;
}
QUERY, * P_QUERY;

typedef struct SEARCH
{
    QUERY_ID query_id; /* -1 == RECORDZ_WHOLE_FILE => no query otherwise indicates which query 0..N t */

    ARRAY_HANDLE h_query; /* handle to an array of active querys */

    /* The suggested options for the next search (not the options for a given search) */
    S32  suggest_type;
    BOOL suggest_andor;
    BOOL suggest_exclude;
    ARRAY_HANDLE h_suggest_pattern; /* The suggested search pattern text */
}
SEARCH, * P_SEARCH;

typedef struct OPENDB
{
    DB db;
    TABLEDEF table;
    BOOL dbok;
    BOOL tableok;
    SEARCH search;
    RECORDSPEC recordspec;
}
OPENDB, * P_OPENDB;

typedef struct RCBUFFER
{
    ARRAY_HANDLE_USTR h_text_ustr;
    DATA_REF data_ref;
    BOOL modified;
}
RCBUFFER, * P_RCBUFFER;

/* A nasty structure to enable rapid redraw of dragable selections
*/

typedef struct DRAG_STATE
{
    S32 flag; /* True while dragging */
    PIXIT_RECT pixit_rect_deltas;
    BOOL rec_static_flag; /* SKS bodge for update_now */
}
DRAG_STATE;

#define REC_DEFAULT_TITLE_FRAME_X 32
#define REC_DEFAULT_TITLE_FRAME_W (PIXITS_PER_INCH)
#define REC_DEFAULT_TITLE_FRAME_H REC_DEFAULT_TEXT_FRAME_V

#define REC_DEFAULT_FRAME_X (PIXITS_PER_CM * 3)
#define REC_DEFAULT_FRAME_Y (PIXITS_PER_MM * 3)

#define REC_DEFAULT_TEXT_FRAME_H (PIXITS_PER_INCH * 5 / 2)
#define REC_DEFAULT_TEXT_FRAME_V (PIXITS_PER_INCH / 4)
#define REC_DEFAULT_BOOL_FRAME_H (PIXITS_PER_INCH / 2)
#define REC_DEFAULT_PICT_FRAME_H 3200
#define REC_DEFAULT_PICT_FRAME_V 3200

#define REC_DEFAULT_FRAME_GAP_Y 64

#define REC_DEFAULT_FRAME_RM 32
#define REC_DEFAULT_FRAME_BM 128

#define DEFAULT_GRIGHT_SIZE (44 * PIXITS_PER_RISCOS)

/******************************************************************************
*
* A REC_PROJECTOR represents a database file attached to an area of a document.
* It holds the "opendb" structure with positional and display format information.
* There will be an array of these attached to a DOCU via the REC instance data.
*
******************************************************************************/

typedef struct REC_PROJECTOR
{
    OPENDB opendb;           /* A structure as passed to/from the dbread layer */

    DOCU_AREA rec_docu_area; /* Controls the in-document placement of the things extracted from the data */
    S32 start_offset;        /* scroll offset of the tl */

    PROJECTOR_TYPE projector_type;
    ARRAY_HANDLE h_rec_frames; /* REC_FRAME[] */

    BOOL adaptive_rows;
    BOOL lock; /* This is used to prevent the projector being closed due to uref_delete events */

    DOCNO docno; /* key back to containing document */

    UREF_HANDLE uref_handle;

    PIXIT_POINT card_size;
}
REC_PROJECTOR, * P_REC_PROJECTOR, ** P_P_REC_PROJECTOR;

/******************************************************************************
*
* A REC_FRAME structure describes the on-screen layout of a card
*
******************************************************************************/

typedef struct REC_FRAME
{
    FIELD_ID field_id;

    PIXIT_RECT pixit_rect_field;
    S32 field_width;

    PIXIT_RECT pixit_rect_title;
    BOOL title_show;
    ARRAY_HANDLE_USTR h_title_text_ustr;

    /* Any other style stuff for per-field storage of layout info */
}
REC_FRAME, * P_REC_FRAME, ** P_P_REC_FRAME; typedef const REC_FRAME * PC_REC_FRAME;

enum DB_IMPLIED_ARG_TYPES
{
    DB_IMPLIED_ARG_NONE = 0,
    DB_IMPLIED_ARG_DATABASE,
    DB_IMPLIED_ARG_ALL_FIELDS,
    DB_IMPLIED_ARG_ALL_TITLES,
    DB_IMPLIED_ARG_FIELD
};

_Check_return_
extern STATUS
rec_fields_ui_source_create(
    /*out*/ P_UI_SOURCE p_ui_source,
    /*const*/ P_TABLEDEF p_tabledef,
    _InoutRef_  P_PIXIT p_max_width,
    _OutRef_    P_S32 p_n_fields);

extern void
rec_fields_ui_source_dispose(
    /*inout*/ P_UI_SOURCE p_ui_source);

/*
object
*/

OBJECT_PROTO(extern, object_rec);

/*
exported routines
*/

_Check_return_
extern STATUS
rec_drag_update_by_offsets(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_deltas,
    _InRef_     PC_SKEL_RECT p_skel_rect);

_Check_return_
extern STATUS
rec_oserror_set(
    P_ANY /*really _kernel_oserror **/ p_error);

_Check_return_
extern STATUS
rec_revert_whole(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
reconstruct_frames_from_fields(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
set_default_title_rect(
    P_REC_PROJECTOR p_rec_projector,
    P_REC_FRAME p_rec_frame);

/*
exported from ob_rec2.c
*/

_Check_return_
extern STATUS
rec_data_ref_from_name(
    _DocuRef_   P_DOCU p_docu,
    P_U8 p_u8_name,
    _In_        S32 recno,
    _OutRef_    P_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_msg_style_recordz(
    _DocuRef_   P_DOCU p_docu,
    /*_Inout_*/ P_ANY p_data);

_Check_return_
extern STATUS
rec_table_attach_file(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    P_DOCU_AREA p_where);

extern P_REC_PROJECTOR /*NULL if SLR not in DB*/
rec_data_ref_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _OutRef_    P_DATA_REF p_data_ref);

extern P_REC_PROJECTOR /*NULL if SLR not in DB*/
rec_data_ref_from_slr_and_fn(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _In_        S32 fieldnumber,
    _OutRef_    P_DATA_REF p_data_ref);

extern P_REC_PROJECTOR /*NULL if SLR not in DB*/
rec_data_ref_from_slr_and_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _OutRef_    P_DATA_REF p_data_ref);

_Check_return_ _Success_(status_ok(return))
extern STATUS
rec_slr_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _OutRef_    P_SLR p_slr);

_Check_return_ _Success_(status_ok(return))
extern STATUS
rec_skel_rect_from_data_ref(
    P_REC_PROJECTOR p_rec_projector,
    _OutRef_    P_SKEL_RECT p_skel_rect_out,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
get_frame_by_field_id(
    P_REC_PROJECTOR p_rec_projector,
    _In_        FIELD_ID fid,
    P_REC_FRAME p_rec_output);

_Check_return_
extern P_FIELDDEF
p_fielddef_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern P_FIELDDEF
p_fielddef_from_field_id(
    _InRef_     PC_ARRAY_HANDLE p_array_handle /*FIELDDEF[]*/,
    _In_        FIELD_ID f_id);

_Check_return_
extern P_FIELDDEF
p_fielddef_from_name(
    _InRef_     PC_ARRAY_HANDLE p_array_handle /*FIELDDEF[]*/,
    _In_z_      PC_U8Z p_field_name);

_Check_return_
_Ret_maybenone_
extern P_FIELDDEF
p_fielddef_from_number(
    _InRef_     PC_ARRAY_HANDLE p_array_handle /*FIELDDEF[]*/,
    _InVal_     S32 fieldnumber);

extern P_REC_FRAME
p_rec_frame_from_field_id(
    P_ARRAY_HANDLE p_array_handle /*REC_FRAME[]*/,
    _In_        FIELD_ID field_id);

_Check_return_
extern STATUS
get_card_size(
    P_REC_PROJECTOR p_rec_projector,
    /*out*/ P_PIXIT_POINT p_pixit_point);

/* Various routines forming the load/open mechanism */

_Check_return_
extern STATUS
close_rec_projector(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector);

extern void
rec_style_for_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _OutRef_    P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

_Check_return_
extern BOOL
rec_pixit_point_in_pixit_rect(
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_PIXIT_RECT p_pixit_rect);

_Check_return_
extern FIELD_ID
field_id_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref);

extern void
set_field_id_for_rec_data_ref(
    _InoutRef_  P_DATA_REF p_data_ref,
    _In_        FIELD_ID field_id);

_Check_return_
extern DB_ID
db_id_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref);

extern void
set_db_id_for_rec_data_ref(
    _InoutRef_  P_DATA_REF p_data_ref,
    _InVal_     DB_ID db_id);

_Check_return_
extern PROJECTOR_TYPE
projector_type_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern S32
get_record_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref);

extern void
set_record_for_rec_data_ref(
    _InoutRef_  P_DATA_REF p_data_ref,
    _InVal_     S32 record);

extern void
title_data_ref_from_rec_data_ref(
    _OutRef_    P_DATA_REF p_data_ref_title,
    _InRef_     PC_DATA_REF p_data_ref);

extern void
field_data_ref_from_rec_data_ref(
    _OutRef_    P_DATA_REF p_data_ref_field,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
set_rec_data_ref_field_by_number(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DATA_REF p_data_ref,
    _In_        S32 fieldnumber);

_Check_return_
extern S32
fieldnumber_from_rec_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

extern void
set_rec_object_position_data_space(
    P_OBJECT_POSITION p_object_position,
    _In_        DATA_SPACE data_space);

extern DATA_SPACE
get_rec_object_position_data_space(
    _InRef_     PC_OBJECT_POSITION p_object_position);

extern void
set_rec_object_position_field_number(
    P_OBJECT_POSITION p_object_position,
    _In_        S32 fieldnumber);

extern S32
get_rec_object_position_field_number(
    _InRef_     PC_OBJECT_POSITION p_object_position);

extern void
set_rec_object_position_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_POSITION p_object_position,
    _InRef_     PC_DATA_REF p_data_ref);

extern void
set_rec_data_ref_from_object_position(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DATA_REF p_data_ref,
    _InRef_     PC_OBJECT_POSITION p_object_position);

extern void
rec_object_position_copy(
    _InoutRef_  P_OBJECT_POSITION p_object_position_dst,
    _InRef_     PC_OBJECT_POSITION p_object_position_src);

_Check_return_
extern S32
rec_object_position_compare(
    _InRef_     PC_OBJECT_POSITION p_object_position_first,
    _InRef_     PC_OBJECT_POSITION p_object_position_second);

extern S32
rec_object_position_at_start(
    _InRef_     PC_OBJECT_POSITION p_object_position);

extern void
rec_object_position_set_start(
    P_OBJECT_POSITION p_object_position);

_Check_return_
extern STATUS
rec_object_pixit_size(
    _DocuRef_   P_DOCU p_docu,
    P_PIXIT_POINT p_pixit_point,
    P_SLR p_slr,
    _InRef_opt_ PC_STYLE p_style_in);

extern void
rec_closedown(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
rec_cmd_return(
    _DocuRef_   P_DOCU p_docu);

extern void
rec_ext_style(
    _DocuRef_   P_DOCU p_docu,
    P_IMPLIED_STYLE_QUERY p_implied_style_query);

_Check_return_
extern STATUS
rec_ext_style_fields(
    _DocuRef_   P_DOCU p_docu,
    P_IMPLIED_STYLE_QUERY p_implied_style_query);

_Check_return_
extern STATUS
rec_object_editable(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_EDITABLE p_object_editable);

/*
from ui_*.c
*/

extern S32
ensure_some_field_visible(
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
t5_cmd_db_goto_record_intro(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_cmd_db_style(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_cmd_db_search(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_cmd_db_sort(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_cmd_db_view(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_cmd_db_layout(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_cmd_db_open_default(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
t5_cmd_db_open(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
t5_cmd_db_properties(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
t5_cmd_db_create(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
t5_cmd_db_export(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
get_db_password(
    _DocuRef_   P_DOCU p_docu,
    P_U8 buffer,
    _In_        S32 buffsize,
    _InVal_     BOOL blankok);

_Check_return_
extern STATUS
rec_buttons_encode(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
rec_style_all_fields_create(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_style_all_titles_create(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_style_database_create(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_style_styles_apply(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector);

#define POINT_INSIDE_RECTANGLE(p_click_point, obj_rect) ( \
    (p_click_point->x >= obj_rect.tl.x) && \
    (p_click_point->x <  obj_rect.br.x) && \
    (p_click_point->y >= obj_rect.tl.y) && \
    (p_click_point->y <  obj_rect.br.y)    )

_Check_return_
extern STATUS
dplib_ustr_inline_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock,
    _In_reads_(uchars_n) PC_USTR_INLINE ustr_inline /*appended*/,
    _InVal_     U32 uchars_n);

_Check_return_
extern T5_FILETYPE
rec_resource_filetype(
    P_REC_RESOURCE p_rec_resource);

/*
goto record
*/

#define RECORDZ_GOTO_LAST (-1)
#define RECORDZ_PAGE_DOWN (-2)
#define RECORDZ_PAGE_UP   (-3)

_Check_return_
extern STATUS
rec_goto_record(
    _DocuRef_   P_DOCU p_docu,
    _In_        BOOL abs,
    _In_        S32 rn);

_Check_return_
extern STATUS
rec_add_record(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL add_blank);

_Check_return_
extern STATUS
rec_delete_record(
    _DocuRef_   P_DOCU p_docu);

/*
from callback.c
*/

extern void
set_p_opendb_for_getting_at_things(
    P_OPENDB p_opendb);

/*
from dblow.c
*/

_Check_return_
extern STATUS
dplib_init(void);

/*
from ed_card.c
*/

_Check_return_
extern STATUS
card_mode_pixit(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_PIXIT_POINT p_one_program_pixel);

_Check_return_
extern STATUS
rec_card_design_dragging_started(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

_Check_return_
extern STATUS
rec_card_design_dragging_movement(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

_Check_return_
extern STATUS
rec_card_design_dragging_finished(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

_Check_return_
extern STATUS
rec_card_design_dragging_aborted(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

_Check_return_
extern STATUS
rec_card_design_drag(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

_Check_return_
extern STATUS
rec_card_design_double_click(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

_Check_return_
extern STATUS
rec_card_design_single_click(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

/*
from ed_rec.c
*/

PROC_EVENT_PROTO(extern, proc_event_ed_rec_direct);

_Check_return_
extern STATUS
rec_text_from_card(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data_out,
    P_OBJECT_DATA p_object_data_in);

extern void
rec_text_message_block_init(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_MESSAGE_BLOCK p_text_message_block,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    P_OBJECT_DATA p_object_data);

_Check_return_
extern STATUS
rec_object_read_card(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/);

/*
from import.c
*/

extern NUMFORM_PARMS
numform_parms_for_dplib;

_Check_return_
extern STATUS
rec_import_file(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
rec_link_file(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
extern STATUS
create_record(
    P_OPENDB p_opendb,
    /*_Inout_*/ P_ANY p_data);

_Check_return_
extern STATUS
rec_import_csv(
    _DocuRef_   P_DOCU p_docu,
    P_RECORDZ_IMPORT p_recordz_import);

/*
from project.c
*/

extern void
rec_event_null(
    /*_Inout_*/ P_ANY p_data);

extern void
rec_inform_documents_of_changes(
    P_ANY server,
    P_ANY handle);

_Check_return_
extern STATUS
rec_check_for_file_attachment(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
rec_instance_add_alloc(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_REC_PROJECTOR p_p_rec_projector);

_Check_return_
extern STATUS
drop_projector_area(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_recompute_card_size(
    P_REC_PROJECTOR p_rec_projector);

extern void
lock_rec_projector(
    P_REC_PROJECTOR p_rec_projector);

extern void
unlock_rec_projector(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern BOOL
is_rec_projector_locked(
    P_REC_PROJECTOR p_rec_projector);

extern void
rec_kill_projector(
    _InoutRef_  P_P_REC_PROJECTOR p_p_rec_projector);

_Check_return_
extern STATUS
rec_insert_projector_and_attach_with_styles(
    P_REC_PROJECTOR p_rec_projector,
    _InVal_     BOOL loading);

_Check_return_
extern P_REC_PROJECTOR
p_rec_projector_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr);

_Check_return_
extern P_REC_PROJECTOR
p_rec_projector_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern P_REC_PROJECTOR
p_rec_projector_from_db_id(
    _DocuRef_   P_DOCU p_docu,
    _In_        DB_ID db_id);

_Check_return_
extern P_REC_PROJECTOR
which_rec_projector_from_name(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      P_U8Z p_u8_name);

_Check_return_
extern P_REC_PROJECTOR
which_rec_projector_from_field_name(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      P_U8Z p_u8_field_name);

_Check_return_
extern STATUS
max_rec_projector(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_REGION p_region);

extern void
rec_sheet_stash_field_widths(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_update_docu(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
rec_update_projector(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_update_projector_adjust(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_update_projector_adjust_goto(
    P_REC_PROJECTOR p_rec_projector,
    _In_        S32 recno);

_Check_return_
extern STATUS
rec_update_cell(
    _DocuRef_   P_DOCU p_docu,
    P_SLR p_slr);

_Check_return_
extern STATUS
rec_update_data_ref(
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
rec_update_card_contents(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
extern STATUS
rec_uref_add_dependency(
    P_REC_PROJECTOR p_rec_projector);

extern void
trash_caches_for_projector(
    P_REC_PROJECTOR p_rec_projector);

/*
from rcbuf.c
*/

extern void
rec_cache_dispose(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
rec_cache_enquire(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _OutRef_    P_ARRAY_HANDLE_USTR p_h_text_ustr,
    _OutRef_    P_S32 p_s32);

_Check_return_
extern STATUS
rec_cache_modified(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF data_ref,
    _InVal_     BOOL modflag);

_Check_return_
extern STATUS
rec_cache_purge(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
rec_cache_update(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_ARRAY_HANDLE_USTR p_h_text_ustr,
    _InVal_     BOOL modflag);

_Check_return_
extern STATUS
rec_object_write_text(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     T5_FILETYPE t5_filetype);

/*
from rcio.c
*/

T5_CMD_PROTO(extern, rec_io_table);
T5_CMD_PROTO(extern, rec_io_frame);
T5_CMD_PROTO(extern, rec_io_title);
T5_CMD_PROTO(extern, rec_io_query);
T5_CMD_PROTO(extern, rec_io_pattern);

_Check_return_
extern STATUS
ob_rec_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

_Check_return_
extern STATUS
rec_event_fileinsert_doinsert_1(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click);

_Check_return_
extern STATUS
rec_load_construct_ownform(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert);

_Check_return_
extern STATUS
rec_msg_insert_foreign(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_MSG_INSERT_FOREIGN p_msg_insert_foreign);

_Check_return_
extern STATUS
rec_table_load(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename,
    _In_        PROJECTOR_TYPE projector_type,
    P_SLR p_slr,
    P_SLR p_size,
    _InVal_     BOOL adaptive_rows,
    P_REC_PROJECTOR * p_p_rec_projector);

_Check_return_
extern STATUS
rec_open_database(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename);

/*
from rcread.c
*/

_Check_return_
extern STATUS
rec_read_resource(
    /*out*/ P_REC_RESOURCE p_rec_resource,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref,
    _InVal_     BOOL plain);

_Check_return_ _Success_(status_ok(return))
extern STATUS
rec_object_convert_to_output_text(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock               /*appended,WRONGLY terminated*/ /* text out */,
    _OutRef_    P_PIXIT p_pixit_text_width                  /* width of text out */,
    _OutRef_    P_PIXIT p_pixit_text_height                 /* height of text out */,
    _OutRef_    P_PIXIT p_pixit_base_line                   /* base line position out */,
    _OutRef_opt_ P_FONTY_HANDLE p_fonty_handle              /* fonty handle out */,
    _InRef_     PC_STYLE p_style                            /* style in */,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref                      /* Who am it really in */);

_Check_return_
extern STATUS
rec_text_from_data_ref(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,WRONGLY terminated*/ /* text out */,
    _InRef_     PC_STYLE p_style,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref);

/*
from redraw.c
*/

_Check_return_
extern STATUS
rec_card_redraw(
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref);

_Check_return_
extern STATUS
rec_sheet_redraw_field(
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref);

/*
from remap.c
*/

_Check_return_
extern STATUS
remap_sort(
    P_OPENDB p_opendb,
    P_ARRAY_HANDLE p_remap_array_handle,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
remap_remap(
    P_OPENDB p_opendb,
    P_TABLEDEF p_table,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
remap_similar(
    P_OPENDB p_opendb_1,
    P_OPENDB p_opendb_2,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
remap_append(
    P_OPENDB p_opendb_1,
    P_OPENDB p_opendb_2,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
remap_sim(
    P_TABLEDEF p_table_a,
    P_TABLEDEF p_table_b,
    P_ARRAY_HANDLE p_handle_a,
    P_ARRAY_HANDLE p_handle_b);

_Check_return_
extern STATUS
remap_copy_fireworkz_attributes(
    P_TABLEDEF p_table_dst,
    P_TABLEDEF p_table_src);

_Check_return_
extern STATUS
remap_insert(
    P_TABLEDEF p_table,
    P_ARRAY_HANDLE p_handle,
    _In_        S32 insert_position,
    P_TABLEDEF p_table2);

_Check_return_
extern STATUS
remap_hidden_to_end(
    P_TABLEDEF p_table,
    P_ARRAY_HANDLE p_handle);

/*
from dbquery.c
*/

_Check_return_
extern STATUS
close_query(
    P_OPENDB p_opendb,
    P_QUERY p_query);

_Check_return_
extern STATUS
open_query(
    P_OPENDB p_opendb,
    QUERY_ID query_id);

_Check_return_
_Ret_maybenull_
extern P_QUERY
p_query_from_p_opendb(
    P_OPENDB p_opendb,
    QUERY_ID query_id);

extern QUERY_ID
unique_query_id(
    P_ARRAY_HANDLE p_h_query);

extern P_SEARCH_FIELD_PATTERN
get_query_pattern(
    P_OPENDB p_opendb,
    QUERY_ID query_id);

extern QUERY_ID
get_query_parent(
    P_OPENDB p_opendb,
    QUERY_ID query_id);

extern P_U8
get_query_name(
    P_OPENDB p_opendb,
    QUERY_ID query_id);

_Check_return_
extern BOOL
has_query_got_offspring(
    P_OPENDB p_opendb,
    QUERY_ID query_id);

extern void
drop_search_suggestions(
    P_SEARCH p_search);

_Check_return_
extern STATUS
copy_search_pattern(
    P_ARRAY_HANDLE p_h_target,
    P_ARRAY_HANDLE p_h_source);

extern void
wipe_query(
    P_QUERY p_query);

/* Database functions */

_Check_return_
extern STATUS
open_database(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
close_database(
    P_OPENDB p_opendb,
    _InVal_     STATUS status_in);

_Check_return_
extern STATUS
switch_database(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR tempfile);

_Check_return_
extern STATUS
export_database_structure(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename2);

_Check_return_
extern STATUS
export_current_subset(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename2);

_Check_return_
extern STATUS
clone_database_structure(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename2);

_Check_return_
extern STATUS
clone_database(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename2);

_Check_return_
extern STATUS
clone_current_subset(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename2);

_Check_return_
extern STATUS
remap_database_structure(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename,
    P_ARRAY_HANDLE p_handle);

_Check_return_
extern STATUS
remap_records(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename,
    P_ARRAY_HANDLE p_handle,
    P_REMAP_OPTIONS p_options);

/* Table functions */

_Check_return_
extern STATUS
open_table(
    P_DB p_db,
    P_TABLEDEF p_table,
    P_U8 tablename);

_Check_return_
extern STATUS
drop_table(
    P_TABLEDEF p_table,
    _InVal_     STATUS status_in);

extern void
drop_fake_table(
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
drop_fields_for_table(
    P_TABLEDEF p_table);

_Check_return_
extern BOOL
is_table_contained_by_table(
    _InRef_     PC_TABLEDEF p_table1,
    _InRef_     PC_TABLEDEF p_table2);

_Check_return_
extern BOOL
is_table_same_as_table(
    _InRef_     PC_TABLEDEF p_table1,
    _InRef_     PC_TABLEDEF p_table2);

_Check_return_
extern STATUS
make_fake_table(
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
table_add_field(
    P_TABLEDEF p_table,
    P_FIELDDEF p_field);

_Check_return_
extern STATUS
add_new_field_to_table(
    P_TABLEDEF p_table,
    _In_z_      PC_U8Z p_u8_name,
    _In_        FIELD_TYPE ft);

_Check_return_
extern STATUS
remove_field_from_table(
    P_TABLEDEF p_table,
    _In_        ARRAY_INDEX delete_at);

_Check_return_
extern STATUS
table_copy_fields(
    P_TABLEDEF p_table_dst,
    P_TABLEDEF p_table_src);

_Check_return_
extern STATUS
table_check_access(
    P_TABLEDEF p_table,
    _InVal_     ACCESS_FUNCTION access_function);

/* Routines for editing/adding records */

_Check_return_
extern STATUS
record_add_commence(
    P_OPENDB p_opendb ,
    _InVal_     BOOL clear);

_Check_return_
extern STATUS
record_edit_commence(
    P_OPENDB p_opendb);

_Check_return_
extern STATUS
record_edit_confirm(
    P_OPENDB p_opendb ,
    _InVal_     BOOL blank,
    _InVal_     BOOL adding);

_Check_return_
extern STATUS
record_edit_cancel(
    P_OPENDB p_opendb ,
    _InVal_     BOOL adding);

_Check_return_
extern STATUS
record_current(
    P_OPENDB p_opendb,
    P_S32 p_s32_out,
    P_S32 p_ncards);

_Check_return_
extern STATUS
record_view(
    P_OPENDB p_opendb);

_Check_return_
extern STATUS
record_view_recnum(
    P_OPENDB p_opendb,
    _In_        S32 recnum);

/* recordspec functions */

_Check_return_
extern STATUS
new_recordspec_handle_table(
    P_RECORDSPEC p_recordspec,
    _In_        S32 n_required);

_Check_return_
extern STATUS
free_recordspec_handle_table(
    P_RECORDSPEC p_recordspec);

/* field functions */

_Check_return_
extern STATUS
get_field_by_number(
    P_TABLEDEF p_table,
    P_FIELDDEF p_fielddef,
    _In_        S32 n);

_Check_return_
extern STATUS
set_field_by_number(
    P_TABLEDEF p_table,
    P_FIELDDEF p_fielddef,
    _In_        S32 n);

_Check_return_
extern STATUS
get_field_by_name(
    P_TABLEDEF p_table,
    P_FIELDDEF p_fielddef,
    P_U8 p_field_name);

_Check_return_
extern STATUS
set_field_by_id(
    P_TABLEDEF p_table,
    P_FIELDDEF p_fielddef,
    _In_        FIELD_ID f_id);

_Check_return_
extern STATUS
fieldnumber_from_field_id(
    P_TABLEDEF p_table,
    _In_        FIELD_ID f_id);

_Check_return_
extern STATUS
get_formula_type(
    _InRef_     PC_FIELDDEF p_fielddef,
    P_FIELD_TYPE p_field_type_result);

extern void
wipe_fielddef(
    P_FIELDDEF p_fielddef);

_Check_return_
extern STATUS
field_get_value(
    /*out*/ P_REC_RESOURCE p_rec_resource,
    _InRef_     PC_FIELDDEF p_fielddef,
    _InVal_     BOOL plain);

/* cursor functions */

_Check_return_
extern STATUS
ensure_cursor_whole(
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
ensure_cursor_current(
    P_OPENDB p_opendb,
    _In_        S32 recnum);

_Check_return_
extern STATUS
close_cursor(
    P_TABLEDEF p_table);

#if DPLIB

_Check_return_
extern STATUS
dplib_changes_in_p_opendb(
    P_OPENDB p_opendb,
    P_ANY server,
    P_ANY handle);

_Check_return_
extern STATUS
dplib_client_updates(void);

_Check_return_
extern STATUS
dplib_create_from_prototype(
    _In_z_      PCTSTR filename,
    P_TABLEDEF p_table);

_Check_return_
extern BOOL
dplib_is_db_on_server(
    P_OPENDB p_opendb);

_Check_return_
extern STATUS
dplib_record_delete(
    P_OPENDB p_opendb);

#endif

#if DPLIB && defined(DPLIB_LOW)
#include "dplib.h"
#endif

#endif /* __ob_rec_h */

/* end of ob_rec.h */
