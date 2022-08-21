/* ob_chart.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "ob_chart/resource/resource.h"

#ifndef __gr_chart_h
#include "cmodules/gr_chart.h"
#endif

#ifndef __gr_chari_h
#include "cmodules/gr_chari.h"
#endif

#ifndef         __xp_note_h
#include "ob_note/xp_note.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#define CHART_NOTE_SIZE_FUDGE_X (2 * PIXITS_PER_RISCOS)
#define CHART_NOTE_SIZE_FUDGE_Y (2 * PIXITS_PER_RISCOS)

/*
internal structure
*/

typedef struct CHART_LISTED_DATA * P_CHART_LISTED_DATA;

typedef enum CHART_RANGE_TYPE
{
    CHART_RANGE_NONE = 0, /* unallocated hole */

    CHART_RANGE_COL,      /* rows in a column */
    CHART_RANGE_ROW,      /* columns in a row */

    CHART_RANGE_TXT       /* text objects in chart dependent on document data */
}
CHART_RANGE_TYPE;

/*
elements in an array belonging to a chart decriptor
*/

typedef struct CHART_ELEMENT
{
    LIST_ITEMNO itdepkey; /* which dep block this element refers to */

    struct CHART_ELEMENT_BITS
    {
        UBF label_first_range : 1; /* entire range labels? */
        UBF label_first_item  : 1; /* is first item (of first data set) a label? */

        UBF range_over_columns : 1; /* which orientation first data set added in */

        UBF reserved  : sizeof(U16)*8 -8 -2*1;

        UBF type : 8; /* whether COL or ROW or whatever */
    } bits;

    DOCNO docno;
    REGION region;

    UREF_HANDLE uref_handle; /* so the uref dependency can be deleted */

    struct CHART_HEADER * p_chart_header; /* have to be able to get back, pointer is ok */

    GR_INT_HANDLE gr_int_handle; /* something to delete me by */
}
CHART_ELEMENT, * P_CHART_ELEMENT, * P_P_CHART_ELEMENT;

typedef struct CHART_PROCESS
{
    UBF initial   : 1;
    UBF force_add : 1;

    UBF reserved  : sizeof(U16)*8 - 2*1;
}
CHART_PROCESS;

typedef enum CHART_RECALC_STATES
{
    CHART_UNMODIFIED = 0,
    CHART_MODIFIED_AWAITING_REBUILD,
    CHART_MODIFIED_AGAIN
}
CHART_RECALC_STATES;

typedef struct CHART_HEADER
{
    DOCNO docno;
    U8 _spare[3];

    LIST_ITEMNO chartdatakey; /* our internal handle on this chart */

    GR_CHART_HANDLE ch; /* which chart we have made */

    LIST_BLOCK listed_deps; /* which dependencies we have added */

    struct CHART_HEADER_RECALC
    {
        CHART_RECALC_STATES state;
        MONOTIME last_mod_time;
    } recalc;

    ARRAY_HANDLE h_elem;

    U8 label_first_item;   /* whether first item of first data set added was labels */
    U8 range_over_columns; /* which orientation first data set added in */

    struct CHART_HEADER_SELECTION
    {
        GR_CHART_OBJID id;
        P_GR_RISCDIAG p_gr_riscdiag; /* the Draw diagram representing the current selection */
        GR_DIAG_OFFSET hitObject; /* offset of selection in diagram */
        GR_BOX box; /* a box which can be used to represent the selection */
    } selection;

    U8 written_out_this_save;
}
CHART_HEADER, * P_CHART_HEADER, ** P_P_CHART_HEADER;

/*
a list of charts (so they can be selected from)
*/

typedef struct CHART_LISTED_DATA
{
    P_CHART_HEADER p_chart_header; /* only need this to find the chart */
}
CHART_LISTED_DATA;

/*
shape descriptor for setting up a range to chart (only one at once so don't bitfield the bits)
*/

typedef struct CHART_SHAPEDESC_BITS
{
    GR_CHART_TYPE chart_type;

    U8 number_top_left;
    U8 label_top_left;

    U8 number_left_col;
    U8 label_left_col;

    U8 number_top_row;
    U8 label_top_row;

    U8 label_first_range;
    U8 label_first_item;

    U8 range_over_columns;
    U8 range_over_manual;

    U8 some_number_cells;
}
CHART_SHAPEDESC_BITS;

typedef struct CHART_SHAPEDESC
{
    DOCNO docno;
    REGION region;

    U32 n_ranges;

    SLR min, max, n, nz_n;

    CHART_SHAPEDESC_BITS bits;

    S32 allowed_numbers, allowed_strings;

    LIST_BLOCK nz_cols; /* list of non-empty columns in range */
    LIST_BLOCK nz_rows; /* list of non-empty rows in range */
}
CHART_SHAPEDESC, * P_CHART_SHAPEDESC;

#define CELL_TYPE_BLANK  1
#define CELL_TYPE_NUMBER 2
#define CELL_TYPE_STRING 4

enum BL_GALLERY_CONTROL_IDS
{
#if RISCOS
#define BAR_GALLERY_PICT_H (((2*60) * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_H)
#define BAR_GALLERY_PICT_V (((4*29) * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_V)
#else
#define BAR_GALLERY_PICT_H (32 * PIXITS_PER_WDU_H + DIALOG_PUSHPICTUREOVH_H)
#define BAR_GALLERY_PICT_V (30 * PIXITS_PER_WDU_V + DIALOG_PUSHPICTUREOVH_V)
#endif
    BL_GALLERY_ID_PICT_GROUP = 5,
    BL_GALLERY_ID_PICT_0,
    BL_GALLERY_ID_PICT_1,
    BL_GALLERY_ID_PICT_2,
    BL_GALLERY_ID_PICT_3,
    BL_GALLERY_ID_PICT_4,
    BL_GALLERY_ID_PICT_5,
    BL_GALLERY_ID_PICT_6,
    BL_GALLERY_ID_PICT_7,
    BL_GALLERY_ID_PICT_8,

    BL_GALLERY_ID_ARRANGE_GROUP = 25,

    BL_GALLERY_ID_SERIES_GROUP,
    BL_GALLERY_ID_SERIES_TEXT,
    BL_GALLERY_ID_SERIES_COLS,
    BL_GALLERY_ID_SERIES_ROWS,

    BL_GALLERY_ID_FIRST_SERIES_GROUP,
    BL_GALLERY_ID_FIRST_SERIES_TEXT,
    BL_GALLERY_ID_FIRST_SERIES_CATEGORY_LABELS,
    BL_GALLERY_ID_FIRST_SERIES_SERIES_DATA,

    BL_GALLERY_ID_FIRST_CATEGORY_GROUP,
    BL_GALLERY_ID_FIRST_CATEGORY_TEXT,
    BL_GALLERY_ID_FIRST_CATEGORY_SERIES_LABELS,
    BL_GALLERY_ID_FIRST_CATEGORY_CATEGORY_DATA,

    BL_GALLERY_ID_3D = 40,

    BL_GALLERY_ID_CONTINUE_HERE
};

enum PIE_GALLERY_CONTROL_IDS
{
#define PIE_GALLERY_LABELPICT_H BAR_GALLERY_PICT_H
#define PIE_GALLERY_LABELPICT_V BAR_GALLERY_PICT_V
    PIE_GALLERY_ID_LABEL_GROUP = BL_GALLERY_ID_PICT_GROUP,
    PIE_GALLERY_ID_LABEL_NONE = BL_GALLERY_ID_PICT_0,
    PIE_GALLERY_ID_LABEL_LABEL,
    PIE_GALLERY_ID_LABEL_VALUE,
    PIE_GALLERY_ID_LABEL_V_PCT,

#define PIE_GALLERY_EXPLODEPICT_H PIE_GALLERY_LABELPICT_H
#define PIE_GALLERY_EXPLODEPICT_V PIE_GALLERY_LABELPICT_V
    PIE_GALLERY_ID_EXPLODE_GROUP = BL_GALLERY_ID_CONTINUE_HERE,
    PIE_GALLERY_ID_EXPLODE_NONE,
    PIE_GALLERY_ID_EXPLODE_FIRST,
    PIE_GALLERY_ID_EXPLODE_ALL,
    PIE_GALLERY_ID_EXPLODE_BY_VALUE,
    PIE_GALLERY_ID_EXPLODE_BY_TEXT,

    PIE_GALLERY_ID_START_POSITION_GROUP,
    PIE_GALLERY_ID_START_POSITION_CIRCLE,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_000,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_045,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_090,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_135,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_180,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_225,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_270,
    PIE_GALLERY_ID_START_POSITION_CIRCLE_315,
    PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE,
    PIE_GALLERY_ID_START_POSITION_ANGLE_TEXT,
    PIE_GALLERY_ID_ANTICLOCKWISE
};

/*
retrieve data to display a chart gallery
*/

typedef struct T5_MSG_CHART_GALLERY_DATA
{
    /*IN*/
    S32 chart_type;

    /*OUT*/
    P_S32 p_selected_pict;
    STATUS resource_id;
    P_DIALOG_CTL_CREATE p_ctl_create;
    U32 n_ctls;
    STATUS help_topic_resource_id;
}
T5_MSG_CHART_GALLERY_DATA, * P_T5_MSG_CHART_GALLERY_DATA;

typedef struct GR_CHART_AXIS_STATE_AXIS
{
    int /*GR_AXIS_POSITION_LZR*/ lzr;
    int /*GR_AXIS_POSITION_ARF*/ arf;
}
GR_CHART_AXIS_STATE_AXIS;

typedef struct GR_CHART_AXIS_STATE_TICKS
{
    S32 cat_value;
    F64 val_value;
    BOOL automatic;
    BOOL grid;
    int /*GR_AXIS_TICK_SHAPE*/ tick;
}
GR_CHART_AXIS_STATE_TICKS;

typedef struct GR_CHART_VAL_AXIS_STATE_AXIS
{
    F64 minimum, maximum;

    BOOL automatic;
    BOOL include_zero;
    BOOL logarithmic;
    BOOL logarithmic_modified;
    BOOL log_labels;
}
GR_CHART_VAL_AXIS_STATE_AXIS;

typedef struct GR_CHART_VAL_AXIS_STATE_SERIES
{
    BOOL cumulative;
    BOOL cumulative_modified;

    BOOL vary_by_point;
    BOOL best_fit;
    BOOL fill_to_axis;

    BOOL stacked;
    BOOL stacked_modified;
}
GR_CHART_VAL_AXIS_STATE_SERIES;

typedef struct GR_CHART_AXIS_STATE
{
    P_CHART_HEADER p_chart_header;
    GR_CHART_OBJID id;

    GR_AXES_IDX modifying_axes_idx;
    GR_AXIS_IDX modifying_axis_idx;

    BOOL processing_cat;

    STATUS level; /* stops killer recursion */

    GR_CHART_AXIS_STATE_AXIS axis;
    GR_CHART_AXIS_STATE_TICKS major, minor;
    GR_CHART_VAL_AXIS_STATE_AXIS val_axis;
    GR_CHART_VAL_AXIS_STATE_SERIES val_series;
}
GR_CHART_AXIS_STATE, * P_GR_CHART_AXIS_STATE;

enum CHART_AXIS_CONTROL_IDS
{
    GEN_AXIS_ID_POSITION_GROUP = 917,
    GEN_AXIS_ID_POSITION_LZR_GROUP,
    GEN_AXIS_ID_POSITION_LZR_LB,
    GEN_AXIS_ID_POSITION_LZR_ZERO,
    GEN_AXIS_ID_POSITION_LZR_RT,
    GEN_AXIS_ID_POSITION_ARF_GROUP,
    GEN_AXIS_ID_POSITION_ARF_FRONT,
    GEN_AXIS_ID_POSITION_ARF_AUTO,
    GEN_AXIS_ID_POSITION_ARF_REAR,

    GEN_AXIS_ID_MAJOR_GROUP,
    GEN_AXIS_ID_MAJOR_AUTO,
    GEN_AXIS_ID_MAJOR_SPACING_TEXT,
    GEN_AXIS_ID_MAJOR_SPACING,
    GEN_AXIS_ID_MAJOR_GRID,
    GEN_AXIS_ID_MAJOR_TICKS_TEXT,
    GEN_AXIS_ID_MAJOR_TICKS_NONE,
    GEN_AXIS_ID_MAJOR_TICKS_FULL,
    GEN_AXIS_ID_MAJOR_TICKS_IN,
    GEN_AXIS_ID_MAJOR_TICKS_OUT,

    GEN_AXIS_ID_MINOR_GROUP,
    GEN_AXIS_ID_MINOR_AUTO,
    GEN_AXIS_ID_MINOR_SPACING_TEXT,
    GEN_AXIS_ID_MINOR_SPACING,
    GEN_AXIS_ID_MINOR_GRID,
    GEN_AXIS_ID_MINOR_TICKS_TEXT,
    GEN_AXIS_ID_MINOR_TICKS_NONE,
    GEN_AXIS_ID_MINOR_TICKS_FULL,
    GEN_AXIS_ID_MINOR_TICKS_IN,
    GEN_AXIS_ID_MINOR_TICKS_OUT,

    VAL_AXIS_ID_SCALING_GROUP,
    VAL_AXIS_ID_SCALING_AUTO,
    VAL_AXIS_ID_SCALING_MINIMUM_TEXT,
    VAL_AXIS_ID_SCALING_MINIMUM,
    VAL_AXIS_ID_SCALING_MAXIMUM_TEXT,
    VAL_AXIS_ID_SCALING_MAXIMUM,
    VAL_AXIS_ID_SCALING_INCLUDE_ZERO,
    VAL_AXIS_ID_SCALING_LOGARITHMIC,
    VAL_AXIS_ID_SCALING_LOG_LABELS,

    VAL_AXIS_ID_SERIES_GROUP,
    VAL_AXIS_ID_SERIES_CUMULATIVE,
    VAL_AXIS_ID_SERIES_VARY_BY_POINT,
    VAL_AXIS_ID_SERIES_BEST_FIT,
    VAL_AXIS_ID_SERIES_FILL_TO_AXIS,
    VAL_AXIS_ID_SERIES_STACK,

    SERIES_ID_IN_OVERLAY,

    THATS_ALL_FOLKS
};

typedef struct T5_MSG_CHART_DIALOG_DATA
{
    /*IN*/
#define CHART_DIALOG_SERIES   0
#define CHART_DIALOG_AXIS_CAT 1
#define CHART_DIALOG_AXIS_VAL 2
    S32 reason;
    GR_AXIS_IDX modifying_axis_idx;

    /*OUT*/
    P_DIALOG_CTL_CREATE p_ctl_create;
    U32 n_ctls;
    STATUS help_topic_resource_id;
}
T5_MSG_CHART_DIALOG_DATA, * P_T5_MSG_CHART_DIALOG_DATA;

/*
internal exports from ob_chart.c
*/

_Check_return_
extern STATUS
chart_add(
    P_CHART_HEADER p_chart_header,
    P_CHART_SHAPEDESC p_chart_shapedesc);

extern void
chart_dispose(
    /*inout*/ P_P_CHART_HEADER p_p_chart_header);

_Check_return_
extern STATUS
chart_element(
    P_CHART_HEADER p_chart_header,
    P_CHART_SHAPEDESC p_chart_shapedesc);

_Check_return_
extern STATUS
chart_element_ensure(
    P_CHART_HEADER p_chart_header,
    _In_        U32 n_ranges);

extern void
chart_modify(
    _InoutRef_  P_CHART_HEADER p_chart_header);

extern void
chart_modify_docu(
    _InoutRef_  P_CHART_HEADER p_chart_header);

extern ARRAY_HANDLE
chart_host_font_name_from_textstyle(
    _InRef_     PC_GR_TEXTSTYLE textstyle);

_Check_return_
extern STATUS
chart_new(
    /*out*/ P_P_CHART_HEADER p_p_chart_header,
    _InVal_     S32 use_preferred,
    _InVal_     S32 new_untitled);

extern void
chart_rebuild_after_modify(
    _InoutRef_  P_CHART_HEADER p_chart_header);

extern void
chart_shape_continue(
    _InoutRef_  P_CHART_SHAPEDESC p_chart_shapedesc);

extern void
chart_shape_end(
    P_CHART_SHAPEDESC p_chart_shapedesc);

extern void
chart_shape_labels(
    P_CHART_SHAPEDESC p_chart_shapedesc);

_Check_return_
extern STATUS
chart_shape_start(
    _DocuRef_   P_DOCU p_docu,
    /*inout*/ P_CHART_SHAPEDESC p,
    P_DOCU_AREA p_docu_area);

extern P_CHART_HEADER
p_chart_header_from_docu_last(
    _DocuRef_   P_DOCU p_docu);

/*
internal exports from gr_chtio.c
*/

OBJECT_PROTO(extern, object_chart_io_sideways);

T5_CMD_PROTO(extern, chart_io_gr_construct_load);

_Check_return_
extern STATUS
gr_fillstyleb_make_key_for_save(
    _InRef_     PC_GR_FILLSTYLEB fillstyleb,
    P_ANY client_handle);

/*
end of internal exports from gr_chtio.c
*/

/*
internal exports from gr_edit.c
*/

OBJECT_PROTO(extern, object_chart_edit_sideways);

extern void
chart_selection_clear(
    P_CHART_HEADER p_chart_header,
    P_NOTE_INFO p_note_info);

/*
internal exports from gr_blgal.c
*/

_Check_return_
extern STATUS
gr_chart_add_more(
    _DocuRef_   P_DOCU p_docu,
    P_CHART_HEADER p_chart_header);

_Check_return_
extern STATUS
gr_chart_bl_gallery(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     GR_CHART_TYPE chart_type);

_Check_return_
extern STATUS
gr_chart_process(
    P_CHART_HEADER p_chart_header,
    _In_        GR_CHART_OBJID id);

_Check_return_
extern STATUS
gr_chart_scat_gallery(
    _DocuRef_   P_DOCU p_docu);

/*
end of internal exports from gr_blgal.c
*/

/*
internal exports from gr_uiaxi.c -- axis processing UI
*/

_Check_return_
extern STATUS
gr_chart_axis_process(
    P_CHART_HEADER p_chart_header,
    _In_        GR_CHART_OBJID id);

_Check_return_
extern STATUS
gr_chart_series_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id);

/*
end of internal exports from gr_uiaxi.c
*/

/*
internal exports from gr_uidlg.c -- glue to UI
*/

/*
end of internal exports from gr_uidlg.c
*/

/*
exports from ob_chart object to gr_chart modules
*/

extern void
gr_chart_warning(
    _ChartRef_  P_GR_CHART p_gr_chart,
    _InVal_     STATUS err);

typedef struct CHART_AUTOMAP_HELP
{
    TCHARZ wholename[BUF_MAX_PATHSTRING];
}
CHART_AUTOMAP_HELP, * P_CHART_AUTOMAP_HELP;

extern P_ARRAY_HANDLE
chart_automapper_help_handle(
    P_ANY client_handle);

extern void
chart_automapper_help_name(
    P_ANY client_handle,
    P_CHART_AUTOMAP_HELP p_chart_automap_help);

/*
internal exports from gr_uigal.c
*/

T5_MSG_PROTO(extern, chart_msg_chart_dialog, _InoutRef_ P_T5_MSG_CHART_DIALOG_DATA p_t5_msg_chart_dialog_data);
T5_MSG_PROTO(extern, chart_msg_chart_gallery, _InoutRef_ P_T5_MSG_CHART_GALLERY_DATA p_t5_msg_chart_gallery_data);

/*
end of internal exports from gr_uigal.c
*/

/*
internal exports from gr_uisty.c
*/

_Check_return_
extern STATUS
gr_chart_legend_process(
    P_CHART_HEADER p_chart_header);

_Check_return_
extern STATUS
gr_chart_margins_process(
    P_CHART_HEADER p_chart_header);

_Check_return_
extern STATUS
gr_chart_style_fill_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id);

_Check_return_
extern STATUS
gr_chart_style_line_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id);

_Check_return_
extern STATUS
gr_chart_style_text_process(
    P_CHART_HEADER p_chart_header,
    _InVal_     GR_CHART_OBJID id);

/*
end of internal exports from gr_uisty.c
*/

/* end of ob_chart.h */
