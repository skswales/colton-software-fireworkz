/* gr_chart.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Charting module interface */

/* SKS May 1991 */

#ifndef __gr_chart_h
#define __gr_chart_h

/*
chart object 'names'
*/

#define GR_CHART_OBJNAME_ANON       0

#define GR_CHART_OBJNAME_CHART      1 /* only one of these per chart */
#define GR_CHART_OBJNAME_PLOTAREA   2 /* now 0..2 */
#define GR_CHART_OBJNAME_LEGEND     3 /* only one of these per chart */

#define GR_CHART_OBJNAME_TEXT       4 /* as many as you want */

#define GR_CHART_OBJNAME_SERIES     5
#define GR_CHART_OBJNAME_POINT      6

#define GR_CHART_OBJNAME_DROPSERIES 7
#define GR_CHART_OBJNAME_DROPPOINT  8

#define GR_CHART_OBJNAME_AXIS       9 /* 0..5 */
#define GR_CHART_OBJNAME_AXISGRID   10
#define GR_CHART_OBJNAME_AXISTICK   11

#define GR_CHART_OBJNAME_BESTFITSER 12

#define GR_CHART_OBJNAME_LEGDSERIES 13
#define GR_CHART_OBJNAME_LEGDPOINT  14

/* if it gets more than 15 here, increase below bitfield size */

#define GR_CHART_OBJID_NAME int

/*
this is a 32-bit id
*/

typedef struct GR_CHART_OBJID
{
    UBF name      : 4; /*GR_CHART_OBJID_NAME :  C can't use enum as synonym for int in bitfields*/

    UBF reserved  : (sizeof(U8)*8-4-1-1); /* pack up to char boundary after has_xxx bits */

    UBF has_no    : 1;
    UBF has_subno : 1;

    UBF no        : 8;

    UBF subno     : 16;
}
GR_CHART_OBJID, * P_GR_CHART_OBJID; typedef const GR_CHART_OBJID * PC_GR_CHART_OBJID;

#define gr_chart_objid_clear(p_id) \
    * (P_U32) (p_id) = 0

#define gr_chart_objid_equal(pc_id_1, pc_id_2) ( \
    (* (PC_U32) pc_id_1) == (* (PC_U32) pc_id_2) )

#define BUF_MAX_GR_CHART_OBJID_REPR 48

#define GR_CHART_OBJID_SUBNO_MAX U16_MAX

#define GR_CHART_OBJID_DEFINED

#ifndef          __gr_diag_h
#include "cmodules/gr_diag.h"
#endif

/*
exported types
*/

typedef S32 GR_MILLIPOINT;

/*
convert from RISC OS OS units to pixits
*/
#define gr_pixit_from_riscos(os) ( \
    (os) * (GR_PIXIT) GR_PIXITS_PER_RISCOS )

/*
convert from pixits to millipoints
*/
#define gr_mp_from_pixit(p) ( \
    (p) * GR_MILLIPOINTS_PER_PIXIT )

/*
convert from pixits to RISC OS Draw units
*/
#define gr_riscDraw_from_pixit(p) ( \
    (p) * GR_RISCDRAW_PER_PIXIT )

/*
convert from points to RISC OS Draw units
*/
#define gr_riscDraw_from_point(p) ( \
    (p) * GR_RISCDRAW_PER_POINT )

/*
exports from gr_chart.c
*/

/*
abstract 32-bit chart handle for export
*/

typedef enum GR_CHART_HANDLE
{
    GR_CHART_HANDLE_NONE = 0
}
GR_CHART_HANDLE, * P_GR_CHART_HANDLE;

/*
abstract 32-bit chart comms handle for export
*/

typedef enum GR_INT_HANDLE
{
    GR_INT_HANDLE_NONE = 0
}
GR_INT_HANDLE, * P_GR_INT_HANDLE;

typedef enum GR_DATASOURCE_HANDLE /* abstract 32-bit, unique non-repeating number */
{
    GR_DATASOURCE_HANDLE_NONE  = 0,
    GR_DATASOURCE_HANDLE_START = 1, /* used for internal label adding */
    GR_DATASOURCE_HANDLE_TEXTS = 2  /* used for external texts adding */
}
GR_DATASOURCE_HANDLE, * P_GR_DATASOURCE_HANDLE;

#define   GR_INT_HANDLE   GR_DATASOURCE_HANDLE
#define P_GR_INT_HANDLE P_GR_DATASOURCE_HANDLE

typedef S32 GR_CHART_ITEMNO;
typedef /*unsigned*/ S32 GR_ESERIES_NO;
typedef /*unsigned*/ S32 GR_EAXES_NO;

#define GR_CHART_NUMBER_MAX DBL_MAX

typedef struct GR_CHART_NUMPAIR
{
    F64 x, y;
}
GR_CHART_NUMPAIR, * P_GR_CHART_NUMPAIR;

/*
values that are passed from data source to chart via callbacks
*/

#define GR_CHART_VALUE_NONE    0 /* value unavailable or not convertible to usable format */
#define GR_CHART_VALUE_TEXT    1
#define GR_CHART_VALUE_NUMBER  2
#define GR_CHART_VALUE_N_ITEMS 3

#define GR_CHART_VALUE_TYPE int

#define GR_CHART_VALUE_REQ_NONE   0
#define GR_CHART_VALUE_REQ_TEXT   1
#define GR_CHART_VALUE_REQ_NUMBER 2

#define GR_CHART_VALUE_REQ_TYPE int

/*
special item numbers for callback info
*/

#define GR_CHART_ITEMNO_N_ITEMS (-1)
#define GR_CHART_ITEMNO_LABEL   (-2)

#define GR_CHART_SPECIAL_ITEMNO int

typedef union GR_CHART_VALUE_DATA
{
    F64 number;
    UCHARZ text[256]; /* fixed so callbacks can check to not overwrite */
    GR_CHART_ITEMNO n_items;
}
GR_CHART_VALUE_DATA;

typedef struct GR_CHART_VALUE
{
    GR_CHART_VALUE_REQ_TYPE req_type;
    GR_CHART_VALUE_TYPE type;
    GR_CHART_VALUE_DATA data;
}
GR_CHART_VALUE, * P_GR_CHART_VALUE;

/*
callback to data source from chart to obtain data
*/

typedef STATUS (* P_PROC_GR_CHART_TRAVEL) (
    P_ANY client_handle,
    _InVal_     GR_CHART_ITEMNO item,
    _InoutRef_opt_ P_GR_CHART_VALUE val);

#define PROC_GR_CHART_TRAVEL_PROTO(_proc_name) \
STATUS \
_proc_name( \
    P_ANY client_handle, \
    GR_CHART_ITEMNO item, \
    _InoutRef_opt_ P_GR_CHART_VALUE val)

_Check_return_
extern STATUS
gr_chart_preferred_get_name(
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

_Check_return_
extern STATUS
gr_chart_save_external(
    _InVal_     GR_CHART_HANDLE ch,
    _InoutRef_  FILE_HANDLE file_handle,
    PCTSTR save_filename,
    P_ANY ext_handle);

_Check_return_
extern STATUS
gr_ext_construct_load_this(
    _InVal_     GR_CHART_HANDLE ch,
    P_U8 args,
    _InVal_     U16 contab_idx);

/*
exported functions
*/

_Check_return_
extern STATUS
gr_chart_add(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out);

_Check_return_
extern STATUS
gr_chart_add_labels(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out);

_Check_return_
extern STATUS
gr_chart_add_text(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out);

_Check_return_
extern STATUS
gr_chart_change_handle(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     GR_DATASOURCE_HANDLE int_handle,
    P_ANY new_ext_handle);

extern void
gr_chart_damage(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     GR_INT_HANDLE int_handle);

_Check_return_
extern STATUS
gr_chart_diagram(
    _InVal_     GR_CHART_HANDLE ch,
    /*out*/ P_P_GR_DIAG p_p_gr_diag);

extern void
gr_chart_diagram_ensure(
    _InVal_     GR_CHART_HANDLE ch);

extern void
gr_chart_dispose(
    _InoutRef_  P_GR_CHART_HANDLE chp);

_Check_return_
extern STATUS
gr_chart_insert(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    _OutRef_    P_GR_INT_HANDLE p_int_handle_out,
    _InVal_     GR_INT_HANDLE int_handle_after);

extern void
gr_chart_modify_and_rebuild(
    _InVal_     GR_CHART_HANDLE ch);

_Check_return_
extern STATUS
gr_chart_name_query(
    _InVal_     GR_CHART_HANDLE ch,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

_Check_return_
extern STATUS
gr_chart_name_set(
    _InVal_     GR_CHART_HANDLE ch,
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
gr_chart_new(
    /*out*/ P_GR_CHART_HANDLE chp,
    P_ANY ext_handle,
    _InVal_     S32 new_untitled);

extern U32
gr_chart_order_query(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     GR_INT_HANDLE int_handle);

_Check_return_
extern STATUS
gr_chart_preferred_new(
    /*out*/ P_GR_CHART_HANDLE chp,
    P_ANY ext_handle);

_Check_return_
extern STATUS
gr_chart_preferred_query(void);

_Check_return_
extern STATUS
gr_chart_preferred_save(
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
gr_chart_preferred_set(
    _InVal_     GR_CHART_HANDLE ch);

_Check_return_
extern STATUS
gr_chart_preferred_use(
    _InVal_     GR_CHART_HANDLE ch);

_Check_return_
extern STATUS
gr_chart_query_name(
    _InVal_     GR_CHART_HANDLE ch,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

_Check_return_
extern STATUS
gr_chart_query_labelling(
    _InVal_     GR_CHART_HANDLE ch);

_Check_return_
extern STATUS
gr_chart_query_labels(
    _InVal_     GR_CHART_HANDLE ch);

_Check_return_
extern STATUS
gr_chart_query_modified(
    _InVal_     GR_CHART_HANDLE ch);

_Check_return_
extern STATUS
gr_chart_subtract(
    _InVal_     GR_CHART_HANDLE ch,
    /*inout*/ P_GR_INT_HANDLE p_int_handle);

/*
end of exports from gr_chart.c
*/

/*
exports from gr_texts.c
*/

/*
exported functions
*/

extern void
gr_chart_text_order_set(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     S32 key);

/*
end of exports from gr_texts.c
*/

/*
callback, must be supplied by client
*/

#define GR_CHART_CALLBACK_RC_SELECTION_KILL_REPR 0
#define GR_CHART_CALLBACK_RC_SELECTION_MAKE_REPR 1

#define GR_CHART_CALLBACK_RC int

extern void
callback_from_gr_chart(
    P_ANY client_handle,
    _In_        GR_CHART_CALLBACK_RC rc);

#endif /* __gr_chart_h */

/* end of gr_chart.h */
