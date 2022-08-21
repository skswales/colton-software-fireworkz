/* gr_chari.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Internal header file for the gr_chart module */

/* SKS May 1991 */

/*
required includes
*/

#if RISCOS
#ifndef         __xp_skelr_h
#include "ob_skel/xp_skelr.h"
#endif
#endif

#ifndef          __gr_coord_h
#include "cmodules/gr_coord.h"
#endif

#ifndef          __mathxtr2_h
#include "cmodules/mathxtr2.h"
#endif

#if RISCOS || 1
#include "cmodules/gr_rdia3.h"
#endif

#if RISCOS
#define SYSCHARWIDTH_OS     16
#define SYSCHARHEIGHT_OS    32
#define SYSCHARWIDTH_PIXIT  gr_pixit_from_riscos(SYSCHARWIDTH_OS)
#define SYSCHARHEIGHT_PIXIT gr_pixit_from_riscos(SYSCHARHEIGHT_OS)
#else
#define SYSCHARWIDTH_PIXIT  8*16 /* help! */
#endif /* OS */

#if RISCOS
#define SYSCHARWIDTH_MP (SYSCHARWIDTH_OS * GR_MILLIPOINTS_PER_RISCOS)  /* 6400 */
#else
#define SYSCHARWIDTH_MP 4800 /* bodge */
#endif /* OS */

typedef /*unsigned*/ S32 GR_DATASOURCE_NO;

typedef /*unsigned*/ S32 GR_LINE_PATTERN_NO; /* index into table of line dash patterns (defs in gr_diag.ext) */
typedef /*unsigned*/ S32 GR_FILL_PATTERN_NO; /* index into table of fill patterns */
typedef /*unsigned*/ S32 GR_LINESTYLE_WIDTH_NO; /* index into table of line widths */

typedef struct GR_MINMAX_NUMBER
{
    F64 min, max;
}
GR_MINMAX_NUMBER;

/*
an index into the series table of a chart
*/

typedef /*unsigned*/ S32 GR_SERIES_IDX;

/*
an array of data sources is held on a per chart basis
*/

typedef struct GR_DATASOURCE_VALID
{
    UBF n_items : 1;
}
GR_DATASOURCE_VALID;

typedef struct GR_DATASOURCE_CACHE
{
    GR_CHART_ITEMNO n_items;
}
GR_DATASOURCE_CACHE;

typedef struct GR_DATASOURCE
{
    GR_DATASOURCE_HANDLE   dsh;        /* unique non-repeating number */

    P_PROC_GR_CHART_TRAVEL ext_proc;   /* callback to owner */
    P_ANY                  ext_handle; /* using this handle */

    GR_CHART_OBJID id;         /* object using this data source */

    GR_DATASOURCE_VALID valid; /* validity bits for cached items */
    GR_DATASOURCE_CACHE cache; /* cached items */
}
GR_DATASOURCE, * P_GR_DATASOURCE, ** P_P_GR_DATASOURCE;

/*
a clutch of datasources for plotting porpoises
*/

typedef struct GR_DATASOURCE_FOURSOME
{
    GR_DATASOURCE_HANDLE value_x, value_y;
    GR_DATASOURCE_HANDLE error_x, error_y;
}
GR_DATASOURCE_FOURSOME, * P_GR_DATASOURCE_FOURSOME;

/*
style options for bar charts
*/

typedef struct GR_BARCHSTYLE_BITS
{
    UBF manual : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

    UBF pictures_stacked         : 1;
    UBF pictures_stacked_clipper : 1;

    UBF reserved : sizeof(int)*8 - 2*1 - 1;
}
GR_BARCHSTYLE_BITS; /* sys dep size U16/U32 - no expansion */

typedef struct GR_BARCHSTYLE
{
    GR_BARCHSTYLE_BITS bits; /* MUST BE FIRST WORD OF STRUCTURE */

    F64 slot_width_percentage;
}
GR_BARCHSTYLE, * P_GR_BARCHSTYLE; typedef const GR_BARCHSTYLE * PC_GR_BARCHSTYLE;

/*
style options for line charts
*/

typedef struct GR_LINECHSTYLE_BITS
{
    UBF manual : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

    UBF reserved : sizeof(int)*8 - 1;
}
GR_LINECHSTYLE_BITS; /* sys dep size U16/U32 - no expansion */

typedef struct GR_LINECHSTYLE
{
    GR_LINECHSTYLE_BITS bits; /* MUST BE FIRST WORD OF STRUCTURE */

    F64 slot_width_percentage;
}
GR_LINECHSTYLE, * P_GR_LINECHSTYLE; typedef const GR_LINECHSTYLE * PC_GR_LINECHSTYLE;

/*
style options common to both bar and line charts
*/

typedef struct GR_BARLINECHSTYLE_BITS
{
    UBF manual : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

    UBF label_cat : 1;
    UBF label_val : 1;
    UBF label_pct : 1;

    UBF reserved : sizeof(int)*8 - 3*1 - 1;
}
GR_BARLINECHSTYLE_BITS; /* sys dep size U16/U32 - no expansion */

typedef struct GR_BARLINECHSTYLE
{
    GR_BARLINECHSTYLE_BITS bits; /* MUST BE FIRST WORD OF STRUCTURE */

    F64 slot_depth_percentage; /* 3-D only */
}
GR_BARLINECHSTYLE, * P_GR_BARLINECHSTYLE; typedef const GR_BARLINECHSTYLE * PC_GR_BARLINECHSTYLE;

/*
style options for pie charts
*/

typedef struct GR_PIECHDISPLSTYLE_BITS
{
    UBF manual : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

    UBF reserved : sizeof(int)*8 - 1;
}
GR_PIECHDISPLSTYLE_BITS; /* sys dep size U16/U32 - no expansion */

typedef struct GR_PIECHDISPLSTYLE
{
    GR_PIECHDISPLSTYLE_BITS bits; /* MUST BE FIRST WORD OF STRUCTURE */

    F64 radial_displacement;
}
GR_PIECHDISPLSTYLE, * P_GR_PIECHDISPLSTYLE; typedef const GR_PIECHDISPLSTYLE * PC_GR_PIECHDISPLSTYLE;

typedef struct GR_PIECHLABELSTYLE_BITS
{
    UBF manual : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

    UBF label_leg : 1;
    UBF label_val : 1;
    UBF label_pct : 1;

    UBF reserved : sizeof(int)*8 - 3*1 - 1;
}
GR_PIECHLABELSTYLE_BITS; /* sys dep size U16/U32 - no expansion */

typedef struct GR_PIECHLABELSTYLE
{
    GR_PIECHLABELSTYLE_BITS bits; /* MUST BE FIRST WORD OF STRUCTURE */
}
GR_PIECHLABELSTYLE, * P_GR_PIECHLABELSTYLE; typedef const GR_PIECHLABELSTYLE * PC_GR_PIECHLABELSTYLE;

/*
style options for scatter charts
*/

typedef struct GR_SCATCHSTYLE_BITS
{
    UBF manual : 1; /* MUST BE FIRST BIT IN FIRST WORD OF STRUCTURE */

    UBF label_xval : 1;
    UBF label_yval : 1;
    UBF label__pct : 1; /* meaningless for scatter */

    UBF lines_off : 1;

    UBF reserved : sizeof(int)*8 - 1;
}
GR_SCATCHSTYLE_BITS; /* sys dep size U16/U32 - no expansion */

typedef struct GR_SCATCHSTYLE
{
    GR_SCATCHSTYLE_BITS bits; /* MUST BE FIRST WORD OF STRUCTURE */

    F64 width_percentage;
}
GR_SCATCHSTYLE, * P_GR_SCATCHSTYLE; typedef const GR_SCATCHSTYLE * PC_GR_SCATCHSTYLE;

/*
base plot style for axes/series
*/

#define GR_CHART_TYPE_NONE 0
#define GR_CHART_TYPE_PIE 1
#define GR_CHART_TYPE_BAR 2
#define GR_CHART_TYPE_LINE 3
#define GR_CHART_TYPE_SCAT 4

typedef S32 GR_CHART_TYPE;

#define GR_CHART_TYPE_OVER_BL -1 /* an otherwise invalid chart type (for UI) */

/*
series (one or more per axes set)
*/

/*
type of a series
*/

#define GR_CHART_SERIES_PLAIN 0        /* requires 1 value source */
#define GR_CHART_SERIES_PLAIN_ERROR1 1 /* requires 1 value source and 1 error source */
#define GR_CHART_SERIES_PLAIN_ERROR2 2 /* requires 1 value source and 2 error sources */

#define GR_CHART_SERIES_POINT 3        /* requires 2 value sources */
#define GR_CHART_SERIES_POINT_ERROR1 4 /* requires 2 value sources and 1 error source */
#define GR_CHART_SERIES_POINT_ERROR2 5 /* requires 2 value sources and 2 error sources */

typedef S32 GR_SERIES_TYPE;

typedef struct GR_SERIES_BITS
{
    UBF cumulative           : 1; /* s(n) = sum(s(i)...s(1)) for s(i) >= 0 */
    UBF cumulative_manual    : 1; /* whether derived from axes default or set by punter */

    UBF vary_by_point        : 1; /* automatic variation between points in the same series */
    UBF vary_by_point_manual : 1; /* whether derived from axes default or set by punter */

    UBF best_fit             : 1;
    UBF best_fit_manual      : 1;

    UBF fill_to_axis         : 1; /* fill between line and axis */
    UBF fill_to_axis_manual  : 1; /* whether derived from axes default or set by punter */

    UBF pie_anticlockwise    : 1;

    UBF reserved1            : sizeof(U16)*8 - 1 - 4*2;

    UBF reserved2            : 16;
}
GR_SERIES_BITS; /* U32 for sys indep expansion */

typedef struct GR_SERIES_STYLE
{
    F64                pie_start_heading_degrees;

    GR_FILLSTYLEB      pdrop_fillb;
    GR_FILLSTYLEC      pdrop_fillc;
    GR_LINESTYLE       pdrop_line;

    GR_FILLSTYLEB      point_fillb;
    GR_FILLSTYLEC      point_fillc;
    GR_LINESTYLE       point_line;
    GR_TEXTSTYLE       point_text;

    GR_BARCHSTYLE      point_barch;
    GR_BARLINECHSTYLE  point_barlinech;
    GR_LINECHSTYLE     point_linech;
    GR_PIECHDISPLSTYLE point_piechdispl;
    GR_PIECHLABELSTYLE point_piechlabel;
    GR_SCATCHSTYLE     point_scatch;

    GR_LINESTYLE       bestfit_line;
}
GR_SERIES_STYLE;

typedef struct GR_SERIES_INTERNAL_BITS
{
    UBF descriptor_ok : 1;

    /* not exported, size irrelevant */
}
GR_SERIES_INTERNAL_BITS;

#define GR_SERIES_MAX_DATASOURCES 4
typedef struct GR_SERIES_DATASOURCES
{
    GR_DATASOURCE_NO     n;     /* how many data sources are contributing to this series */
    GR_DATASOURCE_NO     n_req; /* how many data sources are required to contribute to this series */
    GR_DATASOURCE_HANDLE dsh[GR_SERIES_MAX_DATASOURCES]; /* no more than this data sources per series! */
}
GR_SERIES_DATASOURCES;

typedef struct GR_SERIES_LBR
{
    LIST_BLOCK pdrop_fillb;
    LIST_BLOCK pdrop_fillc;
    LIST_BLOCK pdrop_line;

    LIST_BLOCK point_fillb;
    LIST_BLOCK point_fillc;
    LIST_BLOCK point_line;
    LIST_BLOCK point_text;

    LIST_BLOCK point_barch;
    LIST_BLOCK point_barlinech;
    LIST_BLOCK point_linech;
    LIST_BLOCK point_piechdispl;
    LIST_BLOCK point_piechlabel;
    LIST_BLOCK point_scatch;
}
GR_SERIES_LBR;

typedef struct GR_SERIES_VALID
{
    UBF n_items_total : 1;

    UBF limits  : 1;

    /* not exported, size irrelevant */
}
GR_SERIES_VALID;

typedef struct GR_SERIES_CACHE
{
    /* number of items contributing to the series */
    GR_CHART_ITEMNO  n_items_total;

    /* actual min/max limits from this series in its given
     * plotstyle e.g. taking error bars, log axes etc. into account
    */
    GR_MINMAX_NUMBER limits_x;
    GR_MINMAX_NUMBER limits_y;

    F64 best_fit_c;
    F64 best_fit_m;
}
GR_SERIES_CACHE;

typedef struct GR_SERIES
{
    /* cloning of series descriptors starts here */

    GR_SERIES_TYPE sertype; /* determines number of sources needed and how used */

    GR_CHART_TYPE chart_type; /* base type */

    GR_SERIES_BITS bits; /* U32 for sys indep expansion */

    GR_SERIES_STYLE style;

    /* cloning stops here. NB. things must refer to offsetof32(gr_series, GR_SERIES_CLONE_END) */

#define GR_SERIES_CLONE_END internal_bits

    GR_SERIES_INTERNAL_BITS internal_bits;

    GR_SERIES_DATASOURCES datasources;

    GR_SERIES_LBR lbr;

    /* validity bits for cached items */
    GR_SERIES_VALID valid;

    /* cached items */
    GR_SERIES_CACHE cache;
}
GR_SERIES, * P_GR_SERIES;

/*
an 'external' point numbering type - starts at 1
*/
typedef /*unsigned*/ S32 GR_POINT_NO;

/*
descriptor for an individual axis
*/

/*
where an axis is drawn
*/

#define GR_AXIS_POSITION_LZR_ZERO   0 /* zero or closest edge of plotted area */
#define GR_AXIS_POSITION_LZR_LEFT   1 /* left(bottom) edge of plotted area */
#define GR_AXIS_POSITION_LZR_RIGHT  2 /* right(top) edge of plotted area */

#define GR_AXIS_POSITION_BZT_ZERO   GR_AXIS_POSITION_LZR_ZERO
#define GR_AXIS_POSITION_BZT_BOTTOM GR_AXIS_POSITION_LZR_LEFT
#define GR_AXIS_POSITION_BZT_TOP    GR_AXIS_POSITION_LZR_RIGHT

#define GR_AXIS_POSITION_ARF_AUTO   0 /* deduce position from lzr in 3-D */
#define GR_AXIS_POSITION_ARF_REAR   1
#define GR_AXIS_POSITION_ARF_FRONT  2

/*
where ticks are drawn on the axis
*/

#define GR_AXIS_TICK_POSITION_FULL 0       /* fully across axis */
#define GR_AXIS_TICK_POSITION_NONE 1       /* unticked */
#define GR_AXIS_TICK_POSITION_HALF_LEFT 2  /* to left(bottom) of axis */
#define GR_AXIS_TICK_POSITION_HALF_RIGHT 3 /* to right(top) of axis */

#define GR_AXIS_TICK_POSITION_HALF_BOTTOM GR_AXIS_TICK_POSITION_HALF_LEFT
#define GR_AXIS_TICK_POSITION_HALF_TOP    GR_AXIS_TICK_POSITION_HALF_RIGHT

typedef struct GR_AXIS_TICKS_BITS
{
    UBF decimals  : 10; /* for use in decimal places rounding of value labels */

    UBF manual    : 1;  /* if punter has specified values */
    UBF grid      : 1;

    UBF tick      : 3;  /* must be big enough to fit a gr_axis_tick_shape */

    UBF reserved1 : sizeof(U16)*8 - 10 - 2*1 - 3;

    UBF reserved2 : 16;
}
GR_AXIS_TICKS_BITS; /* U32 for sys indep expansion */

typedef struct GR_AXIS_TICKS_STYLE
{
    GR_LINESTYLE grid;
    GR_LINESTYLE tick;
}
GR_AXIS_TICKS_STYLE;

typedef struct GR_AXIS_TICKS
{
    GR_AXIS_TICKS_BITS bits;

    F64 punter, current;

    GR_AXIS_TICKS_STYLE style;
}
GR_AXIS_TICKS, * P_GR_AXIS_TICKS; typedef const GR_AXIS_TICKS * PC_GR_AXIS_TICKS;

#define GR_CHART_AXISTICK_MAJOR 1
#define GR_CHART_AXISTICK_MINOR 2

typedef struct GR_AXIS_BITS
{
    UBF manual       : 1; /* if punter has specified values */
    UBF incl_zero    : 1; /* axis scaling forced to include zero */
    UBF log_scale    : 1; /* log scaled? */
    UBF log_label    : 1; /* log labelled? */

    UBF log_base     : 4;

    UBF lzr          : 2; /* must be big enough to fit a GR_AXIS_POSITION_LZR */
    UBF arf          : 2; /* must be big enough to fit a GR_AXIS_POSITION_ARF */

    UBF reverse      : 1; /* min/max reversal */

    UBF reserved_for_trunc_stagger_state : 3; /* big enough for a stagger of 8; 0 -> truncated; 1 -> cycle of 2 etc.*/

    UBF reserved0    : 1; /* please use me */

    UBF reserved1    : sizeof(U32)*8 - 1 - 1 - 3 - 2*2 - 1*4 - 4*1;
}
GR_AXIS_BITS; /* U32 for sys indep expansion */

typedef struct GR_AXIS_STYLE
{
    GR_LINESTYLE axis;
    GR_TEXTSTYLE label;
}
GR_AXIS_STYLE;

typedef struct GR_AXIS
{
    GR_MINMAX_NUMBER punter;  /* punter specified min/max for this axis */
    GR_MINMAX_NUMBER actual;  /* actual min/max for the assoc data */
    GR_MINMAX_NUMBER current;

    F64 current_span; /* current.max - current.min */
    F64 zero_frac;    /* how far up the axis is the zero line? */
    F64 current_frac; /* major span as a fraction of the current span */

    GR_AXIS_BITS bits;

    GR_AXIS_STYLE style;

    GR_AXIS_TICKS major, minor;
}
GR_AXIS, * P_GR_AXIS; typedef const GR_AXIS * PC_GR_AXIS;

/*
descriptor for an axes set and how series belonging to it are plotted
*/

typedef struct GR_AXES_BITS
{
    UBF cumulative    : 1; /* default series cumulative state */
    UBF vary_by_point : 1; /* default series vary style by point state */
    UBF best_fit      : 1; /* default series line of best fit state */
    UBF fill_to_axis  : 1; /* default series fill to axis state */

    UBF stacked       : 1; /* series plotted on these axes (bars & lines only) are stacked */
    UBF stacked_pct   : 1; /* series plotted on these axes (mostly as bars) are 100% stacked */

    UBF reserved1     : sizeof(U16)*8 - 6;
    UBF reserved2     : 16;
}
GR_AXES_BITS; /* U32 for sys indep expansion */

typedef struct GR_AXES_SERIES
{
    /* SKS after PD 4.12 26mar92 - what series no this axes set should start at (-ve -> user forced, +ve auto) */
    S32 start_series;

    GR_SERIES_IDX stt_idx; /* what series_idx this axes set starts at */
    GR_SERIES_IDX end_idx;
}
GR_AXES_SERIES;

typedef struct GR_AXES_STYLE
{
    GR_BARCHSTYLE      barch;
    GR_BARLINECHSTYLE  barlinech;
    GR_LINECHSTYLE     linech;
    GR_PIECHDISPLSTYLE piechdispl;
    GR_PIECHLABELSTYLE piechlabel;
    GR_SCATCHSTYLE     scatch;
}
GR_AXES_STYLE;

typedef struct GR_AXES_CACHE
{
    S32 /*GR_SERIES_NO*/ n_contrib_bars;
    S32 /*GR_SERIES_NO*/ n_contrib_lines;
    S32 /*GR_SERIES_NO*/ n_series; /* for loading and shaving */
}
GR_AXES_CACHE;

typedef struct GR_AXES
{
    GR_SERIES_TYPE sertype; /* default series type for series creation on these axes */

    GR_CHART_TYPE chart_type; /* default chart type ... */

    GR_AXES_BITS bits;

    GR_AXES_SERIES series;

    GR_AXES_STYLE style;

    GR_AXES_CACHE cache;

#define X_AXIS_IDX 0U
#define Y_AXIS_IDX 1U
#define Z_AXIS_IDX 2U
    GR_AXIS axis[3]; /* x, y, z */
}
GR_AXES, * P_GR_AXES; typedef const GR_AXES * PC_GR_AXES;

typedef U32 GR_AXIS_IDX;
typedef U32 GR_AXES_IDX; typedef GR_AXES_IDX * P_GR_AXES_IDX;

#define GR_CHART_PLOTAREA_REAR 0
#define GR_CHART_PLOTAREA_WALL 1
#define GR_CHART_PLOTAREA_FLOOR 2
#define GR_CHART_N_PLOTAREAS 3

/*
a chart
*/

typedef struct GR_CHART_DATASOURCES
{
    ARRAY_HANDLE mh;    /* data source descriptor array */
}
GR_CHART_DATASOURCES;

typedef struct GR_CHART_LAYOUT_MARGINS
{
    GR_PIXIT left;
    GR_PIXIT bottom;
    GR_PIXIT right;
    GR_PIXIT top;
}
GR_CHART_LAYOUT_MARGINS;

typedef struct GR_CHART_LAYOUT
{
    GR_PIXIT width;
    GR_PIXIT height;

    GR_CHART_LAYOUT_MARGINS  margins;

    GR_POINT size; /* x = width - (margin.left + margin.right) etc. never -ve */
}
GR_CHART_LAYOUT, * P_GR_CHART_LAYOUT;

typedef struct GR_CHART_CORE /* the core of the chart - preserve over clone */
{
    GR_CHART_HANDLE     ch;                 /* the handle under which this structure is exported */

    U32 ceh;                                /* the handle of the single chart editor that is allowed on this chart */

    PTSTR               currentfilename;    /* full pathname of where (if anywhere) this file is stored */
    PTSTR               currentdrawname;    /* full pathname of where (if anywhere) this file was stored as a Draw file */
    GR_DATASOURCE       category_datasource;

    P_ANY ext_handle;                       /* a handle which the creator of the chart passed us */

    GR_CHART_DATASOURCES datasources;

    GR_CHART_LAYOUT layout;

    P_GR_DIAG  p_gr_diag;                   /* the diagram constructed by this chart */

    S32 modified;
}
GR_CHART_CORE;

typedef struct GR_CHART_CHART               /* attributes of 'chart' object */
{
    GR_FILLSTYLEB areastyleb;
    GR_FILLSTYLEC areastylec;
    GR_LINESTYLE borderstyle;
}
GR_CHART_CHART;

typedef struct GR_CHART_PLOTAREA_AREA
{
    GR_FILLSTYLEB areastyleb;
    GR_FILLSTYLEC areastylec;
    GR_LINESTYLE borderstyle;
}
GR_CHART_PLOTAREA_AREA;

typedef struct GR_CHART_PLOTAREA            /* attributes of 'plotarea' object */
{
    GR_POINT posn;         /* offset of plotted area in chart */
    GR_POINT size;         /* how big the plotted area is */

    GR_CHART_PLOTAREA_AREA area[GR_CHART_N_PLOTAREAS];
}
GR_CHART_PLOTAREA, * P_GR_CHART_PLOTAREA;

typedef struct GR_CHART_LEGEND_BITS
{
    UBF on        : 1;
    UBF in_rows   : 1;
    UBF manual    : 1; /* manually repositioned */

    UBF reserved1 : sizeof(U16)*8 - 3*1;
    UBF reserved2 : 16;
}
GR_CHART_LEGEND_BITS; /* U32 for sys indep expansion */

typedef struct GR_CHART_LEGEND              /* attributes of 'legend' object */
{
    GR_CHART_LEGEND_BITS bits;

#if RISCOS
#define GR_CHART_LEGEND_HANG_LEFT gr_pixit_from_riscos(8)
#define GR_CHART_LEGEND_HANG_DOWN gr_pixit_from_riscos(8)
#define GR_CHART_LEGEND_LM gr_pixit_from_riscos(12)
#define GR_CHART_LEGEND_RM gr_pixit_from_riscos(12)
#define GR_CHART_LEGEND_BM gr_pixit_from_riscos(8)
#define GR_CHART_LEGEND_TM gr_pixit_from_riscos(8)
#else
#define GR_CHART_LEGEND_HANG_LEFT 8*(8)
#define GR_CHART_LEGEND_HANG_DOWN 8*(8)
#define GR_CHART_LEGEND_LM 8*(12)
#define GR_CHART_LEGEND_RM 8*(12)
#define GR_CHART_LEGEND_BM 8*(8)
#define GR_CHART_LEGEND_TM 8*(8)
#endif

    GR_POINT posn;         /* offset in chart */
    GR_POINT size;         /* computed size */

    GR_FILLSTYLEB areastyleb;
    GR_FILLSTYLEC areastylec;
    GR_LINESTYLE borderstyle;

    GR_TEXTSTYLE textstyle;
}
GR_CHART_LEGEND, * P_GR_CHART_LEGEND;

typedef struct GR_CHART_SERIES
{
    ARRAY_HANDLE mh; /* series descriptor array */
    GR_SERIES_IDX n_in_use;

    S32 /*GR_SERIES_NO*/ _unused_was_overlay_start; /* 0 -> auto */
}
GR_CHART_SERIES;

typedef struct GR_CHART_TEXT_STYLE
{
    LIST_BLOCK lbr; /* list of attributes for the gr_texts */

    GR_TEXTSTYLE base; /* base text style for text objects */
}
GR_CHART_TEXT_STYLE;

typedef struct GR_CHART_TEXT
{
    LIST_BLOCK lbr; /* list of gr_texts */

    GR_CHART_TEXT_STYLE style;
}
GR_CHART_TEXT;

typedef struct GR_CHART_BITS
{
    UBF realloc_series : 1; /* need to reallocate ds to series */

    /* not exported, size irrelevant */
}
GR_CHART_BITS;

typedef struct GR_CHART_D3_BITS
{
    UBF on        : 1; /* 3-D embellishment? applies to whole chart */
    UBF use       : 1; /* whether 3-D is in use, e.g. pie & scat turn off */

    UBF reserved1 : sizeof(U16)*8 - 1;
    UBF reserved2 : 16;
}
GR_CHART_D3_BITS; /* U32 for sys indep expansion */

typedef struct GR_CHART_D3_VALID
{
    UBF vector : 1;

    /* not exported, size irrelevant */
}
GR_CHART_D3_VALID;

typedef struct GR_CHART_D3_CACHE
{
    GR_POINT vector;            /* how much a point is displaced by per series in depth */

    GR_POINT vector_full;       /* full displacement from front to back */
}
GR_CHART_D3_CACHE;

typedef struct GR_CHART_D3
{
    GR_CHART_D3_BITS bits;

    F64 droop; /* angle about the horizontal x-axis, bringing top into view */
    F64 turn;  /* angle about the vertical y-axis, bringing side into view */

    GR_CHART_D3_VALID valid; /* validity bits for cached items */
    GR_CHART_D3_CACHE cache; /* cached items */
}
GR_CHART_D3;

typedef struct GR_CHART_CACHE
{
    S32 /*GR_SERIES_NO*/ n_contrib_series; /* how many series are contributing to this chart from all axes */
    S32 /*GR_SERIES_NO*/ n_contrib_bars;
    S32 /*GR_SERIES_NO*/ n_contrib_lines;
}
GR_CHART_CACHE;

typedef struct GR_CHART_BARCH_CACHE
{
    GR_PIXIT slot_width;
    GR_PIXIT slot_shift;
}
GR_CHART_BARCH_CACHE;

typedef struct GR_CHART_BARCH
{
    F64 slot_overlap_percentage;

    GR_CHART_BARCH_CACHE cache;
}
GR_CHART_BARCH;

typedef struct GR_CHART_BARLINECH_CACHE
{
    GR_PIXIT cat_group_width;
    GR_PIXIT zeropoint_y;
}
GR_CHART_BARLINECH_CACHE;

typedef struct GR_CHART_BARLINECH
{
    GR_CHART_BARLINECH_CACHE cache;
}
GR_CHART_BARLINECH;

typedef struct GR_CHART_LINE_CACHE
{
    GR_PIXIT slot_start;
    GR_PIXIT slot_shift;
}
GR_CHART_LINE_CACHE;

typedef struct GR_CHART_LINECH
{
    F64 slot_shift_percentage;

    GR_CHART_LINE_CACHE cache;
}
GR_CHART_LINECH;

typedef struct GR_CHART_LINESTYLE_CACHE
{
    GR_LINESTYLE defaults;
#define GR_LINESTYLE_2D_N_DEFAULTS 9
    GR_LINESTYLE twod_defaults[GR_LINESTYLE_2D_N_DEFAULTS];
    BOOL init;
}
GR_CHART_LINESTYLE_CACHE;

typedef struct GR_CHART
{
    GR_CHART_CORE core;

    GR_CHART_CHART chart;
    GR_CHART_PLOTAREA plotarea;
    GR_CHART_LEGEND legend;
    GR_CHART_SERIES series;

#define GR_AXES_IDX_MAX 1
    GR_AXES_IDX axes_idx_max; /* zero or one */
    GR_AXES axes[GR_AXES_IDX_MAX + 1]; /* two independent sets of axes you can plot on (these hold series descriptors) */

    GR_CHART_TEXT text;
    GR_CHART_BITS bits;
    GR_CHART_D3 d3;
    GR_CHART_CACHE cache;

    GR_CHART_BARCH barch;
    GR_CHART_BARLINECH barlinech;
    GR_CHART_LINECH linech;

    GR_CHART_LINESTYLE_CACHE linestyle_cache;
}
GR_CHART, * P_GR_CHART; typedef const GR_CHART * PC_GR_CHART;

#define _ChartRef_   const
#define UNREFERENCED_PARAMETER_ChartRef_(p_gr_chart) UNREFERENCED_PARAMETER_CONST(p_gr_chart)

typedef struct GR_TEXT_BITS
{
    UBF being_edited : 1;  /* use during discard to destroy any editors of text objects */
    UBF unused       : 1;
    UBF live_text    : 1;

    UBF reserved     : sizeof(int)*8 - 3*1;
}
GR_TEXT_BITS; /* sys dep size U16/U32 - no expansion */

typedef struct GR_TEXT
{
    GR_BOX box;

    GR_TEXT_BITS bits;

    /* actual text or datasource follows */
}
GR_TEXT, * P_GR_TEXT;

#define gr_text_search_key(p_gr_chart, key) \
    collect_goto_item(GR_TEXT, &(p_gr_chart)->text.lbr, (key))

typedef union P_GR_TEXT_GUTS
{
    P_GR_TEXT p_gr_text; /* used for assignment i.e. p_gr_text_guts.p_gr_text = p_gr_text + 1; */

    P_GR_DATASOURCE p_gr_datasource;
    P_USTR ustr;
}
P_GR_TEXT_GUTS;

/* structure used for caching series data as we go across a chart; also used for line of best fit data */

typedef struct GR_BARLINESCATCH_SERIES_CACHE
{
    P_GR_CHART cp;
    GR_AXES_IDX axes_idx;
    GR_SERIES_IDX series_idx;
    GR_POINT_NO n_points;

    S32 barindex;
    S32 lineindex;
    S32 plotindex;

    BOOL cumulative;
    BOOL bl_best_fit;
    BOOL fill_to_axis;

    GR_CHART_OBJID serid;
    GR_CHART_OBJID drop_serid;
    GR_CHART_OBJID point_id;
    GR_CHART_OBJID pdrop_id;

    GR_DATASOURCE_FOURSOME dsh;

    /* ribbon-can-be-drawn-back_to cache */
    S32 had_first; /* reset each pass */

    /* cumulative series cache */
    GR_CHART_NUMPAIR old_value; /* reset each pass */
    GR_CHART_NUMPAIR old_error; /* reset each pass */

    GR_POINT old_valpoint;
    GR_POINT old_botpoint;
    GR_POINT old_toppoint;

    F64 slot_depth_percentage; /* to place best fit line at front of all points */
}
GR_BARLINESCATCH_SERIES_CACHE, * P_GR_BARLINESCATCH_SERIES_CACHE;

/* structure used for callback from linest() */

typedef struct GR_BARLINESCATCH_LINEST_STATE
{
    GR_DATASOURCE_FOURSOME dsh;
    P_GR_CHART             cp;
    F64                    y_cum; /* cumulative y */
    S32                    x_log;
    S32                    y_log;

    /*OUT*/
    F64                    a[2]; /* just m and c please */
}
GR_BARLINESCATCH_LINEST_STATE, * P_GR_BARLINESCATCH_LINEST_STATE;

_Check_return_
static inline STATUS
gr_chart_group_new(
    _ChartRef_  P_GR_CHART cp,
    P_GR_DIAG_OFFSET groupStart,
    _InVal_     GR_CHART_OBJID id)
{
    return(gr_diag_group_new(cp->core.p_gr_diag, groupStart, id));
}

static inline void
gr_chart_group_end(
    _ChartRef_  P_GR_CHART cp,
    _InRef_     PC_GR_DIAG_OFFSET pGroupStart)
{
    consume(U32, gr_diag_group_end(cp->core.p_gr_diag, pGroupStart));
}

_Check_return_
static inline STATUS
gr_chart_line_new(
    _InoutRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle)
{
    GR_POINT pos, offset;

    pos.x = pBox->x0;
    pos.y = pBox->y0;

    offset.x = pBox->x1 - pBox->x0;
    offset.y = pBox->y1 - pBox->y0;

    return(gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &pos, &offset, linestyle));
}

_Check_return_
static inline STATUS
gr_chart_text_new(
    P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX pBox,
    _In_z_      PC_USTR szText,
    _InRef_     PC_GR_TEXTSTYLE textstyle)
{
    return(gr_diag_text_new(cp->core.p_gr_diag, NULL, id, pBox, szText, textstyle));
}

/*
internally exported functions from gr_chart.c
*/

_Check_return_
extern STATUS
gr_chart_add_series(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     S32 init);

extern void
gr_chart_free_series(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx);

_Check_return_
_Ret_valid_
extern P_GR_CHART
p_gr_chart_from_chart_handle(
    _InVal_     GR_CHART_HANDLE ch);

_Check_return_
_Ret_maybenull_
extern P_GR_CHART
p_gr_chart_from_chart_handle_maybenull(
    _InVal_     GR_CHART_HANDLE ch);

extern GR_DATASOURCE_HANDLE
gr_chart_datasource_insert(
    _ChartRef_  P_GR_CHART cp,
    _InRef_     P_PROC_GR_CHART_TRAVEL ext_proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out,
    _InVal_     GR_INT_HANDLE int_handle_after);

extern P_GR_DATASOURCE
gr_chart_datasource_p_from_h(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh);

_Check_return_
extern STATUS
gr_chart_datasource_subtract_using_dsh(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh);

_Check_return_
extern STATUS
gr_chart_legend_addin(
    _ChartRef_  P_GR_CHART cp);

extern void
gr_chart_object_name_from_id(
    _InRef_opt_ PC_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _Out_writes_z_(elemof_buffer) P_USTR szName,
    _InVal_     U32 elemof_buffer);

_Check_return_
_Ret_z_
extern PCTSTR
gr_chart_object_name_from_id_quick(
    _InVal_     GR_CHART_OBJID id);

_Check_return_
extern BOOL
gr_chart_objid_find_parent(
    _InoutRef_  P_GR_CHART_OBJID id,
    _InVal_     BOOL deepest);

_Check_return_
extern BOOL
gr_chart_objid_find_parent_for_search(
    _InoutRef_  P_GR_CHART_OBJID id);

_Check_return_
extern STATUS
gr_chart_realloc_series(
    _ChartRef_  P_GR_CHART cp);

_Check_return_
extern STATUS
gr_chart_scaled_picture_add(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX box,
    _InRef_     PC_GR_FILLSTYLEB styleb,
    _InRef_     PC_GR_FILLSTYLEC stylec);

extern GR_CHART_ITEMNO
gr_travel_axes_n_items_total(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx);

extern void
gr_travel_categ_label(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_ITEMNO item,
    _OutRef_    P_GR_CHART_VALUE pValue);

_Check_return_
extern STATUS
gr_travel_dsh(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh,
    _InVal_     GR_CHART_ITEMNO item,
    _InoutRef_  P_GR_CHART_VALUE pValue /* req_type must be set */);

extern void
gr_travel_dsh_label(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh,
    _InVal_     GR_CHART_ITEMNO item,
    _OutRef_    P_GR_CHART_VALUE pValue);

extern GR_CHART_ITEMNO
gr_travel_dsh_n_items(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh);

_Check_return_
extern STATUS
gr_travel_dsh_valof(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh,
    _InVal_     GR_CHART_ITEMNO item,
    _OutRef_    P_F64 value);

_Check_return_
extern STATUS
gr_travel_dsp(
    P_GR_DATASOURCE p_gr_datasource,
    _InVal_     GR_CHART_ITEMNO item,
    _InoutRef_  P_GR_CHART_VALUE pValue /* req_type must be set */);

_Check_return_
extern STATUS
gr_travel_dsp_null(
    P_GR_DATASOURCE p_gr_datasource,
    _InVal_     GR_CHART_ITEMNO item);

_Check_return_
extern GR_DATASOURCE_HANDLE
gr_travel_series_dsh_from_ds(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_DATASOURCE_NO ds);

extern void
gr_travel_series_label(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _OutRef_    P_GR_CHART_VALUE pValue);

_Check_return_
extern GR_CHART_ITEMNO
gr_travel_series_n_items(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_DATASOURCE_NO ds);

_Check_return_
extern GR_CHART_ITEMNO
gr_travel_series_n_items_total(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx);

#if RELEASED || !defined(GR_CHART_CHECK_ALLOCS)
#define gr_chart_validate_block(a, from) /* bugger all */
#define gr_chart_validate_heap(from)     /* bugger all */
#else
#define gr_chart_validate_block(a, from) alloc_validate(a, from)
#define gr_chart_validate_heap(from)     alloc_validate(0, from)
#endif

/*
internally exported variables from gr_chart.c
*/

extern /*const-to-you*/ GR_CHART_HANDLE
gr_chart_preferred_ch;

extern const GR_CHART_OBJID
gr_chart_objid_anon;

extern const GR_CHART_OBJID
gr_chart_objid_chart;

extern const GR_CHART_OBJID
gr_chart_objid_legend;

/*
get a series descriptor pointer from series index on a chart
*/

#define getserp(cp, series_idx) \
    array_ptr32(&(cp)->series.mh, GR_SERIES, (series_idx))

/*
end of internal exports from gr_chart.c
*/

/*
internal exports from gr_texts.c
*/

/*
internally exported functions from gr_texts.c
*/

_Check_return_
extern STATUS
gr_text_addin(
    _ChartRef_  P_GR_CHART cp);

extern void
gr_text_delete(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key);

extern LIST_ITEMNO
gr_text_key_for_new(
    _ChartRef_  P_GR_CHART cp);

_Check_return_
extern STATUS
gr_text_new(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key,
    _In_opt_z_  PC_U8Z szText,
    _InRef_opt_ PC_GR_POINT point);

extern void
gr_text_box_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_GR_BOX p_box);

_Check_return_
extern STATUS
gr_text_box_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key,
    _InRef_     PC_GR_BOX p_box);

/*
end of internal exports from gr_texts.c
*/

/*
internal exports from gr_axisp.c -- axis processing
*/

_Check_return_
extern GR_EAXES_NO
gr_axes_external_from_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx);

/*ncr*/
extern GR_AXIS_IDX
gr_axes_idx_from_external(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_EAXES_NO eaxes_no,
    _OutRef_    P_GR_AXES_IDX axes_idx);

_Check_return_
extern GR_AXES_IDX
gr_axes_idx_from_series_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx);

_Check_return_
extern P_GR_AXES
gr_axesp_from_series_idx(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx);

_Check_return_
extern P_GR_AXIS
gr_axisp_from_external(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_EAXES_NO eaxes_no);

_Check_return_
extern STATUS
gr_axis_addin_category(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points,
    _InVal_     BOOL front_phase);

_Check_return_
extern STATUS
gr_axis_addin_value_x(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     BOOL front_phase);

_Check_return_
extern STATUS
gr_axis_addin_value_y(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     BOOL front_phase);

extern void
gr_axis_form_category(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points);

extern void
gr_axis_form_value(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx);

extern F64
gr_lin_major(
    _InVal_     F64 span);

_Check_return_
extern F64
splitlognum(
    _InRef_     PC_F64 logval,
    _OutRef_    P_F64 exponent);

/*
end of internal exports from gr_axisp.c
*/

/*
internal exports from gr_barch.c
*/

/*
internally exported functions from gr_barch.c
*/

_Check_return_
extern STATUS
gr_barlinechart_addin(
    _ChartRef_  P_GR_CHART cp);

_Check_return_
extern GR_PIXIT
gr_categ_pos(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_POINT_NO point);

extern void
gr_map_point(
    _InoutRef_  P_GR_POINT xpoint,
    _InRef_     PC_GR_CHART cp,
    _In_        S32 plotindex);

extern void
gr_map_point_front_or_back(
    _InoutRef_  P_GR_POINT xpoint,
    _InRef_     PC_GR_CHART cp,
    _InVal_     BOOL front);

extern void
gr_map_box_front_and_back(
    _InoutRef_  P_GR_BOX xbox,
    _ChartRef_  P_GR_CHART cp);

extern void
gr_point_partial_z_shift(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_opt_ PC_GR_POINT apoint,
    _ChartRef_  P_GR_CHART cp,
    _InRef_     PC_F64 z_frac_p);

_Check_return_
extern GR_PIXIT
gr_value_pos(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx,
    _InRef_     PC_F64 value);

_Check_return_
extern GR_PIXIT
gr_value_pos_rel(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx,
    _InRef_     PC_F64 value);

/*
end of internal exports from gr_barch.c
*/

/*
internal exports from gr_piesg.c
*/

/*
internally exported functions from gr_piesg.c
*/

_Check_return_
extern STATUS
gr_pie_addin(
    _ChartRef_  P_GR_CHART cp);

/*
end of internal exports from gr_piesg.c
*/

/*
internal exports from gr_scatc.c
*/

/*
internally exported functions from gr_scatc.c
*/

extern void
gr_get_datasources(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    /*out*/ P_GR_DATASOURCE_FOURSOME dsh,
    _InVal_     BOOL for_io);

_Check_return_
extern STATUS
gr_barlinescatch_best_fit_addin(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp,
    _InVal_     GR_CHART_TYPE chart_type);

PROC_LINEST_DATA_GET_PROTO(extern, gr_barlinescatch_linest_getproc, handle, colID, row);

PROC_LINEST_DATA_GET_PROTO(extern, gr_barlinescatch_linest_getproc_cumulative, handle, colID, row);

PROC_LINEST_DATA_PUT_PROTO(extern, gr_barlinescatch_linest_putproc, handle, colID, row, value);

_Check_return_
extern STATUS
gr_scatterchart_addin(
    _ChartRef_  P_GR_CHART cp);

/*
end of internal exports from gr_scatc.c
*/

/*
DO NOT CHANGE THESE UNLESS FULL RECOMPILATION DONE

KEEP CONSISTENT WITH gr_style_common_blks[] in GR_SCALE.C
*/

#define GR_LIST_CHART_TEXT 0
#define GR_LIST_CHART_TEXT_TEXTSTYLE 1

#define GR_LIST_PDROP_FILLSTYLEC 2
#define GR_LIST_PDROP_FILLSTYLEB 3
#define GR_LIST_PDROP_LINESTYLE 4

#define GR_LIST_POINT_FILLSTYLEB 5
#define GR_LIST_POINT_FILLSTYLEC 6
#define GR_LIST_POINT_LINESTYLE 7
#define GR_LIST_POINT_TEXTSTYLE 8

#define GR_LIST_POINT_BARCHSTYLE 9
#define GR_LIST_POINT_BARLINECHSTYLE 10
#define GR_LIST_POINT_LINECHSTYLE 11
#define GR_LIST_POINT_PIECHDISPLSTYLE 12
#define GR_LIST_POINT_PIECHLABELSTYLE 13
#define GR_LIST_POINT_SCATCHSTYLE 14

#define GR_LIST_N_IDS 15

#define GR_LIST_ID int /* not exported to user files */

/*
internal exports from gr_scale.c
*/

/*
internally exported functions from gr_scale.c
*/

extern void
gr_chart_list_delete(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_LIST_ID list_id);

_Check_return_
extern STATUS
gr_chart_list_duplic(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_LIST_ID list_id);

extern void
gr_chart_objid_from_axes_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_axis_grid(
    _InoutRef_  P_GR_CHART_OBJID id,
    _InVal_     BOOL major_grids);

extern void
gr_chart_objid_from_axis_tick(
    _InoutRef_  P_GR_CHART_OBJID id,
    _InVal_     BOOL major_ticks);

extern void
gr_chart_objid_from_series_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_series_no(
    _InVal_     GR_ESERIES_NO eseries_no,
    _OutRef_    P_GR_CHART_OBJID id);

extern void
gr_chart_objid_from_text(
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_GR_CHART_OBJID id);

/*ncr*/
extern BOOL
gr_chart_objid_fillstyleb_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_FILLSTYLEB style);

_Check_return_
extern STATUS
gr_chart_objid_fillstyleb_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_FILLSTYLEB style);

/*ncr*/
extern BOOL
gr_chart_objid_fillstylec_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_FILLSTYLEC style);

_Check_return_
extern STATUS
gr_chart_objid_fillstylec_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_FILLSTYLEC style);

/*ncr*/
extern BOOL
gr_chart_objid_linestyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_LINESTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_linestyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_LINESTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_textstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_TEXTSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_textstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_TEXTSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_barchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_BARCHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_barchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BARCHSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_barlinechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_BARLINECHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_barlinechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BARLINECHSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_linechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_LINECHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_linechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_LINECHSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_piechdisplstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHDISPLSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_piechdisplstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_PIECHDISPLSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_piechlabelstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHLABELSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_piechlabelstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_PIECHLABELSTYLE style);

/*ncr*/
extern BOOL
gr_chart_objid_scatchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_SCATCHSTYLE style);

_Check_return_
extern STATUS
gr_chart_objid_scatchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_SCATCHSTYLE style);

extern void
gr_fillstyleb_pict_change(
    /*inout*/ P_GR_FILLSTYLEB style,
    const GR_FILL_PATTERN_HANDLE * newpict);

extern void
gr_fillstyleb_pict_delete(
    /*inout*/ P_GR_FILLSTYLEB style);

extern void
gr_fillstyleb_ref_add(
    _InRef_     PC_GR_FILLSTYLEB style);

extern void
gr_fillstyleb_ref_lose(
    _InRef_     PC_GR_FILLSTYLEB style);

/*ncr*/
extern BOOL
gr_pdrop_fillstyleb_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEB style);

_Check_return_
extern STATUS
gr_pdrop_fillstyleb_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEB style);

/*ncr*/
extern BOOL
gr_pdrop_fillstylec_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEC style);

_Check_return_
extern STATUS
gr_pdrop_fillstylec_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEC style);

/*ncr*/
extern BOOL
gr_pdrop_linestyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style);

_Check_return_
extern STATUS
gr_pdrop_linestyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_LINESTYLE style);

#define gr_point_external_from_key(key) ( \
    (GR_POINT_NO) (key)        + 1)

#define gr_point_key_from_external(extPointID) ( \
    (LIST_ITEMNO) (extPointID) - 1)

extern void
gr_point_list_delete(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_LIST_ID list_id);

_Check_return_
extern STATUS
gr_point_list_duplic(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_LIST_ID list_id);

_Check_return_
extern STATUS
gr_point_list_fillstyleb_enum_for_save(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_LIST_ID list_id,
    P_ANY client_handle);

extern void
gr_pdrop_list_fillstyleb_reref(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     BOOL add);

extern void
gr_point_list_fillstyleb_reref(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     BOOL add);

_Check_return_
_Ret_maybenull_
extern P_ANY
_gr_point_list_first(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _OutRef_    P_LIST_ITEMNO p_key,
    _InVal_     GR_LIST_ID list_id);

_Check_return_
_Ret_maybenull_
extern P_ANY
_gr_point_list_next(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _InVal_     GR_LIST_ID list_id);

/*ncr*/
extern BOOL
gr_point_fillstyleb_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEB style);

_Check_return_
extern STATUS
gr_point_fillstyleb_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEB style);

/*ncr*/
extern BOOL
gr_point_fillstylec_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEC style);

_Check_return_
extern STATUS
gr_point_fillstylec_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEC style);

/*ncr*/
extern BOOL
gr_point_linestyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style,
    _InVal_     BOOL bodge_2d);

_Check_return_
extern STATUS
gr_point_linestyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_LINESTYLE style);

/*ncr*/
extern BOOL
gr_point_textstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_TEXTSTYLE style);

_Check_return_
extern STATUS
gr_point_textstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_TEXTSTYLE style);

/*ncr*/
extern BOOL
gr_point_barchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_BARCHSTYLE style);

_Check_return_
extern STATUS
gr_point_barchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_BARCHSTYLE style);

/*ncr*/
extern BOOL
gr_point_barlinechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_BARLINECHSTYLE style);

_Check_return_
extern STATUS
gr_point_barlinechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_BARLINECHSTYLE style);

/*ncr*/
extern BOOL
gr_point_linechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_LINECHSTYLE style);

_Check_return_
extern STATUS
gr_point_linechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_LINECHSTYLE style);

/*ncr*/
extern BOOL
gr_point_piechdisplstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_PIECHDISPLSTYLE style);

_Check_return_
extern STATUS
gr_point_piechdisplstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_PIECHDISPLSTYLE style);

/*ncr*/
extern BOOL
gr_point_piechlabelstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_PIECHLABELSTYLE style);

_Check_return_
extern STATUS
gr_point_piechlabelstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_PIECHLABELSTYLE style);

/*ncr*/
extern BOOL
gr_point_scatchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_SCATCHSTYLE style);

_Check_return_
extern STATUS
gr_point_scatchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_SCATCHSTYLE style);

_Check_return_
extern GR_ESERIES_NO
gr_series_external_from_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx);

_Check_return_
extern GR_SERIES_IDX
gr_series_idx_from_external(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_ESERIES_NO eseries_no);

_Check_return_
extern P_GR_SERIES
gr_seriesp_from_external(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_ESERIES_NO eseries_no);

_Check_return_
extern PC_GR_LINESTYLE
gr_linestyle_2d_default(
    _InVal_     GR_CHART_ITEMNO item);

/*
end of internal exports from gr_scale.c
*/

/*
number to string conversion
*/

_Check_return_
extern GR_MILLIPOINT
gr_host_font_string_truncate(
    _HfontRef_opt_ HOST_FONT host_font,
    _Inout_z_   PC_USTR ustr /*poked to truncate*/,
    _InVal_     GR_MILLIPOINT swidth_mp_truncate);

_Check_return_
extern GR_MILLIPOINT
gr_host_font_string_width(
    _HfontRef_opt_ HOST_FONT host_font,
    _In_z_      PC_USTR ustr);

extern void
chart_automapper_del(
    P_ARRAY_HANDLE p_array_handle);

/* end of gr_chari.h */
