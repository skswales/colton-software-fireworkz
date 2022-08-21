/* dplib.h */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#if !RISCOS
#error Wrong OS
#endif

#define EXPOSE_RISCOS_FONT 1

#ifndef         __xp_skelr_h
#include "ob_skel/xp_skelr.h" /* It's all very RISC OS at heart */
#endif

#ifndef wimp_bbits
#define wimp_bbits int
#endif

#define wimp_paletteword unsigned int

typedef struct _wimp_box { int x0, y0, x1, y1; } wimp_box;

/*
From cs-nonfree/DataPower/DPlib
*/

#include "client_rpc.h"
#include "client.h"

#include "date.h"
#include "myos.h"
#include "srchexpr.h"
#include "query.h"

#include "packed.h"     /* definition of packed frames chunks */
#include "framesflds.h" /* reduces number of function prototypes that we would otherwise have to invent */

#include "winderror.h" /* from WindLibC */

typedef fieldptr P_FIELD;

typedef void * winptr;

/*
functions provided by DPlib that we access
*/

/* These definitions were cooked up by PMF */

_Check_return_
extern BOOL
field_check(
    fieldptr p_field);

_Check_return_
extern itemaccess
fields_getaccess(
    cached_layout ccl);

_Check_return_
extern BOOL
fields_setvalues_sort(
    fieldsptr fields);

_Check_return_
extern BOOL
fields_matchserverfile(
    fieldsptr fields,
    serverptr server,
    file_desc * handle);

/* These defs come from Neil */

extern file_desc *
file_find(
    char * filename);

extern file_desc *
file_findserver(
    serverptr server,
    file_desc * handle);

/*
functions provided by us that DPlib accesses
*/

/* These defs come from Neil */

#define server_complain wind_complain

extern os_error *
badfield_replace(
    char *fname,
    int cardn,
    char *errmess,
    BOOL *quietp,
    remapstash stash,
    char **anchor,
    int offset,
    int size,
    char ***anchorp,
    int *offsetp,
    int *sizep);

extern fieldptr
frames_nextfield_inlayout(
    framesptr frames,
    frameslink * flinkp);

extern os_error *
wind_suspendfile(
    serverptr server,
    file_desc * handle);

extern os_error *
wind_resumefile(
    serverptr server,
    file_desc * file);

extern os_error *
wind_suspend(
    char * filename);

extern os_error *
wind_resume(
    winptr win,
    char * filename,
    os_error * e);

extern os_error *
wind_suspend_allbut(
    serverptr client,
    char * filename);

_Check_return_
extern BOOL
wind_isresumefile_onserver(
    char * filename);

_Check_return_
extern BOOL
wind_getresumefile(
    char *filename,
    serverptr * serverp,
    file_desc ** filep);

extern char *
wind_getresumefilename(
    serverptr server,
    file_desc * file);

extern os_error *
wind_setresumefile(
    char * filename,
    serverptr server,
    file_desc * file);

extern os_error *
wind__suspend(
    char * filename);

extern os_error *
wind__unsuspend(
    char * filename,
    os_error * e);

extern os_error *
wind__resume(
    winptr win,
    char * filename,
    os_error * origerr);

extern os_error *
wind_loseresumefile(
    char * filename,
    os_error * e);

/*
DPlib exports but does not otherwise declare:
*/

extern os_error *
ClientServer_Initialise(void);

/*
DPlib requires these functions
*/

extern os_error *
import_copyimports(
    fields_file fp1,
    fields_file fp2,
    recflags flags);

extern os_filetype
import_filetype(
    os_filetype filetype);

extern int
strcmp_nocase(
    /*const*/ char * ptr1,
    /*const*/ char * ptr2);

extern int
strncmp_nocase(
    /*const*/ char * ptr1,
    /*const*/ char * ptr2,
    int maxlen);

extern os_error *
windquery2_doit(
    int x,
    int y,
    WimpWindowWithBitset * wind,
    char * message,
    int * result);

/*
exported from dblow.c
*/

_Check_return_
extern STATUS
dplib_read_database(
    P_DB p_db,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
dplib_read_fields_for_table(
    P_DB p_db,
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
dplib_check_access_function(
    P_TABLEDEF p_table,
    _InVal_     ACCESS_FUNCTION access_function);

_Check_return_
extern STATUS
dplib_clone_structure(
    _In_z_      PCTSTR filename,
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
dplib_remap_file(
    _In_z_      PCTSTR filename,
    P_ARRAY_HANDLE p_handle,
    P_ANY priavte_dp_layout);

_Check_return_
extern STATUS
dplib_copy_fields(
    P_TABLEDEF p_table_dst,
    P_TABLEDEF p_table_src);

_Check_return_
extern STATUS
dplib_export(
    _In_z_      PCTSTR filename,
    P_OPENDB p_opendbe);

_Check_return_
extern STATUS
dplib_create_query(
    P_OPENDB p_opendb,
    QUERY_ID query_id);

_Check_return_
extern STATUS
dplib_remove_query(
    P_QUERY,
    _InVal_     BOOL delete_search_expr);

_Check_return_
extern STATUS
dplib_record_add_commence(
    P_OPENDB p_opendb,
    _InVal_     BOOL clear);

_Check_return_
extern STATUS
dplib_record_edit_commence(
    P_OPENDB p_opendb);

_Check_return_
extern STATUS
dplib_record_edit_confirm(
    P_OPENDB p_opendb,
    _In_        BOOL blank,
    _InVal_     BOOL adding);

_Check_return_
extern STATUS
dplib_record_edit_cancel(
    P_OPENDB p_opendb,
    _InVal_     BOOL adding);

_Check_return_
extern STATUS
dplib_record_edit_finished(
    P_OPENDB p_opendb,
    _InVal_     BOOL adding);

_Check_return_
extern STATUS
dplib_remap_records(
    P_OPENDB p_opendb_src,
    P_OPENDB p_opendb_dst,
    P_ARRAY_HANDLE p_handle,
    P_REMAP_OPTIONS p_options);

_Check_return_
extern STATUS
dplib_formula_type(
    _InRef_     PC_FIELDDEF p_fielddef,
    P_FIELD_TYPE p_field_type_result);

_Check_return_
extern STATUS
dplib_create_field(
    P_TABLEDEF p_table,
    P_FIELDDEF p_fielddef);

_Check_return_
extern STATUS
dplib_drop_fields_for_table(
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
dplib_fields_delete_for_table(
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
dplib_create_cursor(
    P_TABLEDEF p_table,
    P_ANY p_q17);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
dplib_remove_cursor(
    P_TABLEDEF p_table);

_Check_return_
extern STATUS
dplib_locate_record(
    P_TABLEDEF p_table,
    P_S32 p_recno,
    P_S32 p_ncards,
    P_S32 p_lastvalid);

_Check_return_
extern STATUS
dplib_current_record(
    P_TABLEDEF p_table,
    P_S32 p_recno,
    P_S32 p_ncards);

_Check_return_
extern STATUS
dplib_field_get_value(
    /*out*/ P_REC_RESOURCE p_rec_resource,
    _InRef_     PC_FIELDDEF p_fielddef,
    _InVal_     BOOL plain);

/* DataPower field types */

enum _dp_field_types
{
    FT_COMMENT = 0,
    FT_TEXT,
    FT_BOOL,
    FT_INTEGER,
    FT_REAL,
    FT_GRAPHIC,
    FT_FILE,
    FT_DATE,
    FT_FORMULA,
    FT_INTERVAL
};

/*
exported from dppack.c
*/

#define TITS_OUT 1

#define DP_FONT_MEDIUM  "Homerton.Medium"
#define DP_FONT_OBLIQUE "Homerton.Medium.Oblique"

#define FRAME_ID_START 1

#define DP_DEFAULT_FRAME_LM (-1600)
#define DP_DEFAULT_FRAME_BM (-1600)
#define DP_DEFAULT_FRAME_RM ( 1600)
#define DP_DEFAULT_FRAME_TM ( 1600)

#define DP_DEFAULT_TIT_LHS (7200)
#define DP_DEFAULT_TIT_WIDTH (72000)

#define DP_DEFAULT_FONT_WIDTH (12)
#define DP_DEFAULT_FONT_HEIGHT (12)
#define DP_DEFAULT_BIG_FONT_WIDTH (16)
#define DP_DEFAULT_BIG_FONT_HEIGHT (16)

#define DP_DEFAULT_LINE_SPACE (1200*DP_DEFAULT_FONT_HEIGHT)

#define DP_DEFAULT_FRAME_WIDTH (2*72000)
#define DP_DEFAULT_FRAME_LHS (72000*12 / 10)
#define DP_DEFAULT_FRAME_RHS (72000)
#define DP_DEFAULT_FRAME_TOP (72000*2/3)
#define DP_DEFAULT_FRAME_BOTTOM (-72000/2)
#define DP_DEFAULT_FRAME_SPACING (((72000/2) * 3) / 2)

#define DP_DEFAULT_TEXT_FORE_COLOUR (0)           /* Black */
#define DP_DEFAULT_TEXT_BACK_COLOUR (0xFFFFFF00) /* white */
#define DP_DEFAULT_BORDER_COLOUR (0x00000000) /* Black says Neil */
#define DP_DEFAULT_BACKGROUND_COLOUR (0xCCCCCC00) /* White says Neil L.Grey says PMF*/

/* Styletype values */

#define stype_None -1
#define stype_FText 0
#define stype_BText 1
#define stype_Field 2
#define stype_Title 3
#define stype_Max   4

_Check_return_
extern STATUS
write_frames_chunky(
    fields_file fp,
    _In_        int rootno,
    fieldsptr p_fields,
    _In_        S32 n_flds);

/*
DateFormat file format

; This file specifies the possible formats of date fields

; The format strings have the following possible qualifiers:
;
;       d       day of month as a 1 or 2 digit number
;       m       month as a 1 or 2 digit number
;       y       year number as a 1 or 2 digit number(1900 added)
;       Y       year number as an unsigned integer
;       a       optional AD / BC qualifier(BC means make year negative)
;       D       name of day(1st, 2nd, 3rd .. 31st or 1..31 followed by a non-digit)
;       M       month name as specified in Months field
;     space     0 or more spaces are allowed here
;   hard space  exactly one space must appear here
;   any other   the specified character must appear here
;
; The day, month or year can be missed out if there is a corresponding
; format string without a day, month or year specifier.
;
; NB: Spaces in the format are normally optional in the date, with one exception:
;
;         When reading a decimal number, the next character must not be
;         another digit if there is a space in the format.
;
;      Thus:    251090    is equivalent to    25th October 1990
;      But:     Oct2590   is equivalent to         October 2590
;               Oct25 90  is equivalent to    25th October 1990

; [Days]
; Each line specifies the name of days 1..31
; Note that the first match is accepted, so no id must be a leading substring of any later id

; [Months]
; Each line specifies the names of months 1..12
; Note that the longer versions of each alias must come before any shorter ones

; [AD/BC]
; Each line specifies the names for:
;       AD(Anno Domini) - positive year number
;       BC(Before Christ) - negative year number
*/

/* end of dplib.h */
