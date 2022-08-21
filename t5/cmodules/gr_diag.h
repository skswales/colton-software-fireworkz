/* gr_diag.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for gr_diag/gr_rdiag */

/* SKS April/June 1993 split out of gr_chart.h */

#ifndef __gr_diag_h
#define __gr_diag_h

/*
exports from gr_diag.c -- diagram building
*/

/*
exported structure from gr_diag.c
*/

typedef struct GR_DIAG
{
    ARRAY_HANDLE handle;

    struct GR_RISCDIAG * p_gr_riscdiag;
}
GR_DIAG, * P_GR_DIAG, ** P_P_GR_DIAG; /* no const version as most calls will alter */

#if 0
typedef S32 GR_PIXIT; typedef GR_PIXIT * P_GR_PIXIT;
#else
/* Fireworkz */
#define   GR_PIXIT   PIXIT
#define P_GR_PIXIT P_PIXIT

#define GR_PIXITS_PER_RISCOS PIXITS_PER_RISCOS
#define GR_PIXITS_PER_POINT  PIXITS_PER_POINT
#define GR_PIXITS_PER_INCH   PIXITS_PER_INCH
#endif

/*
colour definition: RGB bytes range from 0x00 (full black) to 0xFF (full colour)
*/

typedef struct GR_COLOUR /* sys indep size U32 */
{
    UBF manual   : 1; /* colour has been set manually, now non-automatic */
    UBF visible  : 1;

    UBF reserved : sizeof(U8)*8 - 2*1; /* pack up to char boundary */

    UBF red      : 8; /* RGB seems most widely applicable, maybe allow HSV or CYMK selectors? with a bit and a union */
    UBF green    : 8;
    UBF blue     : 8;
}
GR_COLOUR;

#define gr_colour_set_NONE(col) \
{ \
    (col).visible  = 0; \
    (col).reserved = 0; \
    (col).red      = 0; \
    (col).green    = 0; \
    (col).blue     = 0; \
}

#define gr_colour_set_RGB(col, r, g, b) \
{ \
    (col).visible  =  1;  \
    (col).reserved =  0;  \
    (col).red      = (r); \
    (col).green    = (g); \
    (col).blue     = (b); \
}

#define gr_colour_set_WHITE(col)   gr_colour_set_RGB(col, 0xFF, 0xFF, 0xFF)
#define gr_colour_set_VLTGRAY(col) gr_colour_set_RGB(col, 0xDD, 0xDD, 0xDD)
#define gr_colour_set_LTGRAY(col)  gr_colour_set_RGB(col, 0xBB, 0xBB, 0xBB)
#define gr_colour_set_MIDGRAY(col) gr_colour_set_RGB(col, 0x7F, 0x7F, 0x7F) /* or 0x80 ??? */
#define gr_colour_set_DKGRAY(col)  gr_colour_set_RGB(col, 0x44, 0x44, 0x44)
#define gr_colour_set_BLACK(col)   gr_colour_set_RGB(col, 0x00, 0x00, 0x00)
#define gr_colour_set_BLUE(col)    gr_colour_set_RGB(col, 0x00, 0x00, 0xBB)

/*
style for lines in objects
*/

#define GR_LINE_PATTERN U32

#define GR_LINE_PATTERN_SOLID        ((GR_LINE_PATTERN) 0)
#define GR_LINE_PATTERN_NONE         ((GR_LINE_PATTERN) 1)
#define GR_LINE_PATTERN_THIN         ((GR_LINE_PATTERN) 2)
#define GR_LINE_PATTERN_DASH         ((GR_LINE_PATTERN) 3)
#define GR_LINE_PATTERN_DOT          ((GR_LINE_PATTERN) 4)
#define GR_LINE_PATTERN_DASH_DOT     ((GR_LINE_PATTERN) 5)
#define GR_LINE_PATTERN_DASH_DOT_DOT ((GR_LINE_PATTERN) 6)

typedef struct GR_LINESTYLE
{
    GR_COLOUR fg; /* principal line colour */

    GR_LINE_PATTERN pattern;

    GR_PIXIT width;

#ifdef GR_CHART_FILL_LINE_INTERSTICES
    GR_COLOUR bg; /* fill interstices along line in this colour */
#endif
}
GR_LINESTYLE, * P_GR_LINESTYLE; typedef const GR_LINESTYLE * PC_GR_LINESTYLE;

/* NB. on WINDOWS thick or coloured lines must be done by rectangle fill */

/*
style for fills in objects
*/

/* NB. on RISC OS no way to pattern fill a Draw object */
/* On WINDOWS, it's very hard, you've got window relative brushOrigin poo to deal with unless make a new context per object! */

typedef enum GR_FILL_PATTERN_HANDLE
{
    GR_FILL_PATTERN_NONE = 0
}
GR_FILL_PATTERN_HANDLE;

typedef struct GR_FILLSTYLEB_BITS /* sys indep size U32 (as of 07jul96) */
{
    UBF manual     : 1;
    UBF isotropic  : 1;
    UBF norecolour : 1;
    UBF notsolid   : 1;
    UBF pattern    : 1;

    UBF varied_by_point : 1; /* internal bit returned from styleb query */
    UBF is_marker  : 1; /* internal bit set after styleb query in line/scatter */

    UBF reserved : sizeof(U16)*8 - 7*1;

    UBF reserved2  : 16;
}
GR_FILLSTYLEB_BITS;

typedef struct GR_FILLSTYLEB
{
    GR_FILL_PATTERN_HANDLE pattern; /* zero -> automatic pattern */

    GR_FILLSTYLEB_BITS bits;

#ifdef GR_CHART_FILL_AREA_INTERSTICES
    GR_COLOUR bg; /* alternate colour for fill */
#endif
}
GR_FILLSTYLEB, * P_GR_FILLSTYLEB; typedef const GR_FILLSTYLEB * PC_GR_FILLSTYLEB;

typedef struct GR_FILLSTYLEC
{
    GR_COLOUR fg; /* principal colour for fill */
}
GR_FILLSTYLEC, * P_GR_FILLSTYLEC; typedef const GR_FILLSTYLEC * PC_GR_FILLSTYLEC;

/*
style for strings
*/

typedef struct GR_TEXTSTYLE
{
    GR_COLOUR fg;

    PIXIT size_x;
    PIXIT size_y;

    UBF bold : 1;
    UBF italic : 1;

    UBF reserved1 : sizeof(U16)*8 -1-1;

    UBF reserved2 : 16;

    TCHARZ tstrFontName[64];
}
GR_TEXTSTYLE, * P_GR_TEXTSTYLE; typedef const GR_TEXTSTYLE * PC_GR_TEXTSTYLE;

/* NB. bg and pattern unused on RISC OS as Draw files can't be explicitly pattern filled */

typedef struct GR_DIAG_PROCESS_T
{
    UBF recurse : 1;          /* recurse into group objects */
    UBF recompute : 1;        /* ensure object bboxes recomputed */
    UBF severe_recompute : 1; /* go to system level and recompute too */
    UBF find_children : 1;    /* return children of the id being searched for */

    UBF reserved : sizeof(int)*8 -1-1-1-1;
}
GR_DIAG_PROCESS_T, * P_GR_DIAG_PROCESS_T;

/*
an id stored in the diagram
*/

#if defined(GR_CHART_OBJID_DEFINED)
#define     GR_DIAG_OBJID_T GR_CHART_OBJID
#else
typedef U32 GR_DIAG_OBJID_T;
#endif

/*
diagram
*/

typedef struct GR_DIAG_DIAGHEADER
{
    GR_BOX bbox;

    TCHARZ szCreatorName[12 + 4]; /* must fit FireworkzPro and CH_NULL */

    /* followed immediately by objects */
}
GR_DIAG_DIAGHEADER, * P_GR_DIAG_DIAGHEADER;

/*
objects
*/

/*
object 'names'
*/

#define GR_DIAG_OBJECT_NONE  ((GR_DIAG_OFFSET) 0U)
#define GR_DIAG_OBJECT_FIRST ((GR_DIAG_OFFSET) 1U)
#define GR_DIAG_OBJECT_LAST  ((GR_DIAG_OFFSET) 0xFFFFFFFFU)

#define GR_DIAG_OBJTYPE_NONE          0U
#define GR_DIAG_OBJTYPE_GROUP         1U
#define GR_DIAG_OBJTYPE_TEXT          2U
#define GR_DIAG_OBJTYPE_LINE          3U
#define GR_DIAG_OBJTYPE_RECTANGLE     4U
#define GR_DIAG_OBJTYPE_PIESECTOR     5U
#define GR_DIAG_OBJTYPE_QUADRILATERAL 6U
#define GR_DIAG_OBJTYPE_PICTURE       7U

typedef U32 GR_DIAG_OBJTYPE;

#define GR_DIAG_OBJHDR_DEF    \
    GR_DIAG_OBJTYPE    type;  \
    U32                n_bytes; \
    GR_BOX             bbox;  \
    GR_DIAG_OBJID_T    objid; \
    DRAW_DIAG_OFFSET   sys_off /* offset in system-dependent (actually RISC OS and Windows both use Drawfile) representation of corresponding object */

#define DIAG_OBJHDR(__base_type, pObject, field) ( \
    (__base_type *) ((P_U8) (pObject) + offsetof32(GR_DIAG_OBJHDR, field)))

#define DIAG_OBJBOX(pObject, field) ( \
    (P_GR_COORD) ((P_U8) (pObject) + offsetof32(GR_DIAG_OBJHDR, bbox) + offsetof32(GR_BOX, field)))

typedef struct GR_DIAG_OBJHDR
{
    GR_DIAG_OBJHDR_DEF;
}
GR_DIAG_OBJHDR;

/*
groups are simply encapulators
*/

typedef struct GR_DIAG_OBJGROUP
{
    GR_DIAG_OBJHDR_DEF;
    /*TCHARZ name[12];*/
}
GR_DIAG_OBJGROUP;

/*
objects with position
*/

#define GR_DIAG_POSOBJHDR_DEF \
    GR_DIAG_OBJHDR_DEF;       \
    GR_POINT pos

typedef struct GR_DIAG_POSOBJHDR
{
    GR_DIAG_POSOBJHDR_DEF;
}
GR_DIAG_POSOBJHDR;

typedef struct GR_DIAG_OBJLINE
{
    GR_DIAG_POSOBJHDR_DEF;
    GR_POINT offset;
    GR_LINESTYLE linestyle;
}
GR_DIAG_OBJLINE;

typedef struct GR_DIAG_OBJPIESECTOR
{
    GR_DIAG_POSOBJHDR_DEF;
    GR_PIXIT radius;
    F64 alpha, beta;
    GR_LINESTYLE linestyle;
    GR_FILLSTYLEC fillstylec;
    GR_POINT p0, p1;
}
GR_DIAG_OBJPIESECTOR;

typedef struct GR_DIAG_OBJPICTURE
{
    GR_DIAG_POSOBJHDR_DEF;
    GR_SIZE size;
    IMAGE_CACHE_HANDLE picture;
    GR_FILLSTYLEB fillstyleb;
    GR_FILLSTYLEC fillstylec;
}
GR_DIAG_OBJPICTURE;

typedef struct GR_DIAG_OBJQUADRILATERAL
{
    GR_DIAG_POSOBJHDR_DEF; /* pos == BL */
    GR_POINT offset1;
    GR_POINT offset2;
    GR_POINT offset3;
    GR_LINESTYLE linestyle;
    GR_FILLSTYLEC fillstylec;
}
GR_DIAG_OBJQUADRILATERAL;

typedef struct GR_DIAG_OBJRECTANGLE
{
    GR_DIAG_POSOBJHDR_DEF;
    GR_SIZE size;
    GR_LINESTYLE linestyle;
    GR_FILLSTYLEC fillstylec;
}
GR_DIAG_OBJRECTANGLE;

typedef struct GR_DIAG_OBJTEXT
{
    GR_DIAG_POSOBJHDR_DEF;
    GR_SIZE size;
    GR_TEXTSTYLE textstyle;

    /* data stored in here */
}
GR_DIAG_OBJTEXT;

/*
internally exported functions from gr_diag.c
*/

extern void
gr_diag_diagram_dispose(
    _InoutRef_ /*never NULL*/ P_P_GR_DIAG p_p_gr_diag);

extern GR_DIAG_OFFSET
gr_diag_diagram_end(
    _InoutRef_  P_GR_DIAG p_gr_diag);

_Check_return_
_Ret_maybenull_
extern P_GR_DIAG
gr_diag_diagram_new(
    _In_z_      PCTSTR szCreatorName,
    _OutRef_    P_STATUS p_status);

extern void
gr_diag_diagram_reset_bbox(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_PROCESS_T process);

_Check_return_
extern STATUS
gr_diag_diagram_save(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _In_z_      PCTSTR filename);

extern U32
gr_diag_group_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_DIAG_OFFSET pGroupStart);

_Check_return_
extern STATUS
gr_diag_group_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pGroupStart,
    _InVal_     GR_DIAG_OBJID_T objid);

_Check_return_
extern STATUS
gr_diag_line_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset,
    _InRef_     PC_GR_LINESTYLE linestyle);

_Check_return_
extern U32
gr_diag_object_base_size(
    _InVal_     GR_DIAG_OBJTYPE objectType);

_Check_return_
extern STATUS
gr_diag_object_correlate_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_POINT point,
    _InRef_     PC_GR_SIZE size,
    _Inout_updates_(recursion_limit) P_GR_DIAG_OFFSET pHitObject /*[]out*/,
    _InVal_     U32 recursion_limit,
    _InVal_     GR_DIAG_OFFSET sttObject,
    _InVal_     GR_DIAG_OFFSET endObject);

extern U32
gr_diag_object_end(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InRef_     PC_GR_DIAG_OFFSET pObjectStart);

_Check_return_
extern BOOL
gr_diag_object_first(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InoutRef_  P_GR_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_BYTE ppObjHdr);

_Check_return_
extern STATUS
gr_diag_object_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _OutRef_opt_ P_GR_DIAG_OFFSET pObjectStart,
    _Out_opt_   P_P_BYTE ppObject,
    P_ANY p_gr_diag_objhdr /*size poked*/,
    _InVal_     U32 extraBytes);

_Check_return_
extern BOOL
gr_diag_object_next(
    _InRef_     P_GR_DIAG p_gr_diag,
    _InoutRef_  P_GR_DIAG_OFFSET pSttObject,
    _InVal_     GR_DIAG_OFFSET endObject,
    _OutRef_    P_P_BYTE ppObjHdr);

extern void
gr_diag_object_reset_bbox_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    /*out*/ P_GR_BOX pBox,
    _InVal_     GR_DIAG_OFFSET sttObject,
    _InVal_     GR_DIAG_OFFSET endObject,
    _InVal_     GR_DIAG_PROCESS_T process);

extern GR_DIAG_OFFSET
gr_diag_object_search_between(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InVal_     GR_DIAG_OFFSET sttObject,
    _InVal_     GR_DIAG_OFFSET endObject,
    _InVal_     GR_DIAG_PROCESS_T process);

_Check_return_
extern STATUS
gr_diag_parallelogram_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_diag_piesector_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InVal_     PIXIT radius,
    _InRef_     PC_F64 alpha,
    _InRef_     PC_F64 beta,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_diag_quadrilateral_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_POINT pPos,
    _InRef_     PC_GR_POINT pOffset1,
    _InRef_     PC_GR_POINT pOffset2,
    _InRef_     PC_GR_POINT pOffset3,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_diag_rectangle_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_diag_scaled_picture_add(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _InVal_     IMAGE_CACHE_HANDLE picture,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_diag_text_new(
    _InoutRef_  P_GR_DIAG p_gr_diag,
    _Out_opt_   P_GR_DIAG_OFFSET pObjectStart,
    _InVal_     GR_DIAG_OBJID_T objid,
    _InRef_     PC_GR_BOX pBox,
    _In_z_      PC_USTR ustr,
    _InRef_     PC_GR_TEXTSTYLE textstyle);

/*
end of exports from gr_diag.c
*/

#if RISCOS || 1

/*
exports from gr_rdiag.c -- RISC OS Draw file creation
*/

/*
exported structure from gr_rdiag.c
*/

typedef struct GR_RISCDIAG_PROCESS_T
{
    UBF recurse:1;    /* recurse into group objects */
    UBF recompute:1;  /* ensure object bboxes recomputed if possible (some object sizes defined by their bbox) */

    UBF reserved : sizeof(int)*8-1-1;
}
GR_RISCDIAG_PROCESS_T;

typedef struct GR_RISCDIAG_RISCOS_FONTLIST_ENTRY
{
    SBCHARZ szHostFontName[64];
}
GR_RISCDIAG_RISCOS_FONTLIST_ENTRY, * P_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY; typedef const GR_RISCDIAG_RISCOS_FONTLIST_ENTRY * PC_GR_RISCDIAG_RISCOS_FONTLIST_ENTRY;

/* no GR_INCH_PER_xxx - an inch is too big */

#define GR_POINTS_PER_INCH          72      /* carved in lithographic stone har har */
#define GR_MILLIPOINTS_PER_INCH     72000   /* ditto */
#define GR_RISCDRAW_PER_INCH        ((S32) GR_RISCDRAW_PER_RISCOS   * GR_RISCOS_PER_INCH)  /* 46080      */
#define GR_RISCOS_PER_INCH          180     /* Acorn derived from 90dpi (logical inch) decent monitor size x 2OS/pixel */

/* no GR_xxx_PER_MILLIPOINT - too small */

/* no GR_POINTS_PER_PIXIT - 1/20 */
#define GR_MILLIPOINTS_PER_PIXIT    (GR_MILLIPOINTS_PER_INCH  / GR_PIXITS_PER_INCH)  /* exact: 50  */
#define GR_RISCDRAW_PER_PIXIT       (GR_RISCDRAW_PER_INCH     / GR_PIXITS_PER_INCH)  /* exact: 32  */
/* no GR_RISCOS_PER_PIXIT - 1/9 */

#define GR_MILLIPOINTS_PER_POINT    1000
#define GR_COORDS_PER_POINT         20     /* logical twips */
#define GR_RISCDRAW_PER_POINT       (GR_RISCDRAW_PER_INCH     / GR_POINTS_PER_INCH)  /* exact: 640 */
/* no GR_RISCOS_PER_POINT - 5/2 */

/* no GR_POINTS_PER_RISCOS - 2/5 */
#define GR_MILLIPOINTS_PER_RISCOS   (GR_MILLIPOINTS_PER_INCH  / GR_RISCOS_PER_INCH)  /* exact: 400 */
#define GR_RISCDRAW_PER_RISCOS      256    /* Acorn subpixel units ('football field' space) */

/* no GR_xxx_PER_RISCDRAW - too small */

/*
gr_rdiag.c
*/

/*
RISC OS Draw allocation
*/

_Check_return_
_Ret_writes_maybenull_(size)
extern P_BYTE
_gr_riscdiag_ensure(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     U32 size,
    _OutRef_    P_STATUS p_status);

#define gr_riscdiag_ensure(__base_type, p_gr_riscdiag, size, p_status) ( \
    (__base_type *) _gr_riscdiag_ensure(p_gr_riscdiag, size, p_status) )

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_normalise_stt(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET sttObject_in);

_Check_return_
extern DRAW_DIAG_OFFSET
gr_riscdiag_normalise_end(
    _InRef_     P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET endObject_in);

/*
RISC OS Draw diagram
*/

extern void
gr_riscdiag_diagram_dispose(
    _InoutRef_  P_P_GR_RISCDIAG p_p_gr_riscdiag);

extern U32
gr_riscdiag_diagram_end(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag);

extern void
gr_riscdiag_diagram_init(
    _OutRef_    P_DRAW_FILE_HEADER pDrawFileHdr,
    _In_z_      PC_SBSTR szCreatorName);

_Check_return_
_Ret_maybenull_
extern P_DRAW_FILE_HEADER
gr_riscdiag_diagram_lock(
    _InoutRef_opt_ P_GR_RISCDIAG p_gr_riscdiag);

_Check_return_
extern STATUS
gr_riscdiag_diagram_new(
    _OutRef_    P_P_GR_RISCDIAG p_p_gr_riscdiag,
    _In_z_      PC_SBSTR szCreatorName,
    _InVal_     ARRAY_HANDLE array_handleR,
    _InVal_     ARRAY_HANDLE array_handleW,
    _InVal_     BOOL create_options_object);

extern void
gr_riscdiag_diagram_reset_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     GR_RISCDIAG_PROCESS_T process);

_Check_return_
extern STATUS
gr_riscdiag_diagram_save(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
gr_riscdiag_diagram_save_into(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  FILE_HANDLE f);

extern void
gr_riscdiag_diagram_setup_from_data(
    _OutRef_    P_GR_RISCDIAG p_gr_riscdiag,
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len);

extern void
gr_riscdiag_diagram_unlock(
    _InoutRef_opt_ P_GR_RISCDIAG p_gr_riscdiag);

/*
RISC OS Draw objects
*/

extern U32
gr_riscdiag_object_base_size(
    _InVal_     U32 type);

extern U32
gr_riscdiag_object_end(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET objectStart);

_Check_return_
extern BOOL
gr_riscdiag_object_first(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG_OFFSET pSttObject,
    _InoutRef_  P_DRAW_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_BYTE ppObject,
    _InVal_     BOOL recurse);

_Check_return_
extern BOOL
gr_riscdiag_object_next(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InoutRef_  P_DRAW_DIAG_OFFSET pSttObject,
    _InRef_     PC_DRAW_DIAG_OFFSET pEndObject,
    _OutRef_    P_P_BYTE ppObject,
    _InVal_     BOOL recurse);

extern void
gr_riscdiag_object_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET objectStart);

extern void
gr_riscdiag_object_reset_bbox_between(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_BOX pBox,
    _InVal_     DRAW_DIAG_OFFSET sttObject,
    _InVal_     DRAW_DIAG_OFFSET endObject,
    _InVal_     GR_RISCDIAG_PROCESS_T process);

/*
RISC OS Draw group objects
*/

extern U32
gr_riscdiag_group_end(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET groupStart);

_Check_return_
extern STATUS
gr_riscdiag_group_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pGroupStart,
    _In_z_      PC_SBSTR szGroupName);

/*
RISC OS Draw font table objects
*/

_Check_return_
extern STATUS
gr_riscdiag_fontlist_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_ARRAY_HANDLE p_array_handleR,
    _InRef_     PC_ARRAY_HANDLE p_array_handleW);

extern void
gr_riscdiag_fontlist_scan(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET startOffset,
    _InVal_     DRAW_DIAG_OFFSET endOffset);

_Check_return_
extern DRAW_FONT_REF16
gr_riscdiag_fontlist_lookup_direct(
    _InRef_     PC_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InVal_     DRAW_DIAG_OFFSET fontListW,
    _InRef_opt_ PC_DRAW_FONTLIST_ELEM pFontListElemR_lookup,
    _InRef_opt_ PC_DRAW_DS_WINFONTLIST_ELEM pFontListElemW_lookup);

extern void
gr_riscdiag_fontlist_lookup_fontref(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InVal_     DRAW_DIAG_OFFSET fontListW,
    _OutRef_    P_DRAW_DIAG_OFFSET pFoundOffsetR,
    _OutRef_    P_DRAW_DIAG_OFFSET pFoundOffsetW,
    _InVal_     DRAW_FONT_REF16 fontRef);

_Check_return_
extern DRAW_FONT_REF16
gr_riscdiag_fontlist_lookup_textstyle(
    _InRef_     PC_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET fontListR,
    _InVal_     DRAW_DIAG_OFFSET fontListW,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle);

_Check_return_
extern int
draw_ds_windows_logfont_compare(
    _InRef_     PC_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont_1,
    _InRef_     PC_DRAW_DS_WINDOWS_LOGFONT p_draw_ds_windows_logfont_2);

extern void
draw_ds_windows_logfont_from_textstyle(
    _OutRef_    P_DRAW_DS_WINDOWS_LOGFONT p_draw_winfontstr,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _In_z_      PC_U8Z szHostFontName);

/*
RISC OS Draw path objects
*/

extern void
gr_riscdiag_path_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart);

/*
RISC OS Draw sprite objects
*/

extern void
gr_riscdiag_sprite_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET spriteStart);

/*
RISC OS Draw string objects
*/

extern void
gr_riscdiag_string_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET textStart);

/*
RISC OS Draw tagged objects
*/

extern void
gr_riscdiag_diagram_tagged_object_strip(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag);

/*
RISC OS Draw option objects
*/

_Check_return_
extern STATUS
gr_riscdiag_options_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pOptionsStart);

/*
RISC OS Draw DIB objects
*/

extern void
gr_riscdiag_dib_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET dibStart);

/*
RISC OS Draw JPEG objects
*/

extern void
gr_riscdiag_jpeg_recompute_bbox(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET jpegStart);

/*
internally exported variables from gr_rdiag.c
*/

/*
transforms
*/

extern const GR_XFORMMATRIX
gr_riscdiag_pixit_from_riscDraw_xformer;

extern const GR_XFORMMATRIX
gr_riscdiag_riscos_from_riscDraw_xformer;

extern const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_pixit_xformer;

extern const GR_XFORMMATRIX
gr_riscdiag_riscDraw_from_mp_xformer;

/*
internally exported functions as macros from gr_rdiag.c
*/

#define gr_riscdiag_getoffptr(__base_type, p_gr_riscdiag, offset) ( \
    PtrAddBytes(__base_type *, (p_gr_riscdiag)->draw_diag.data, offset) )

#define gr_riscdiag_query_offset(p_gr_riscdiag) ( \
    (p_gr_riscdiag)->draw_diag.length )

#define DRAW_OBJHDR(__base_type, pObject, field) \
    PtrAddBytes(__base_type *, pObject, offsetof32(DRAW_OBJECT_HEADER, field))

#define DRAW_OBJBOX(pObject, field) \
    PtrAddBytes(P_S32, pObject, offsetof32(DRAW_OBJECT_HEADER, bbox) + offsetof32(DRAW_BOX, field))

/*
end of exports from gr_rdiag.c
*/

#endif /* RISCOS */

#endif /* __gr_diag_h */

/* end of gr_diag.h */
