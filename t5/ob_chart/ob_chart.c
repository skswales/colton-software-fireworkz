/* ob_chart.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart interface for Fireworkz */

/* SKS March 1993 */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#include "cmodules/collect.h"

/*
callback routines
*/

PROC_UREF_EVENT_PROTO(static, proc_uref_event_ob_chart);

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_CHART)
extern PC_U8 rb_chart_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_CHART &rb_chart_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_CHART LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_CHART DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_CHART LOAD_RESOURCES

/*
internal routines
*/

PROC_EVENT_PROTO(static, scheduled_event_chart);

_Check_return_
_Ret_maybenull_
static P_CHART_ELEMENT
chart_element_add(
    P_CHART_HEADER p_chart_header,
    CHART_RANGE_TYPE type);

static void
chart_element_subtract(
    P_CHART_ELEMENT p_chart_element);

static
PROC_GR_CHART_TRAVEL_PROTO(proc_travel_chart);

/* -------------------------------------------------------------------------------------------- */

static const ARG_TYPE
chart_args_s32_nm[] =
{
    ARG_TYPE_S32 | ARG_OPTIONAL, 
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_chart[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY /*extref*/,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_category_data[] =
{
    ARG_TYPE_BOOL | ARG_MANDATORY, /*label first item*/

    ARG_TYPE_COL | ARG_MANDATORY,  /*tl*/
    ARG_TYPE_ROW | ARG_MANDATORY,
    ARG_TYPE_COL | ARG_MANDATORY,  /*br*/
    ARG_TYPE_ROW | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_series_data[] =
{
    ARG_TYPE_BOOL | ARG_MANDATORY, /*label first item*/
    ARG_TYPE_S32 | ARG_MANDATORY,  /*series type*/

    ARG_TYPE_COL | ARG_OPTIONAL, /*tl*/
    ARG_TYPE_ROW | ARG_OPTIONAL,
    ARG_TYPE_COL | ARG_OPTIONAL, /*br*/
    ARG_TYPE_ROW | ARG_OPTIONAL,

    ARG_TYPE_COL | ARG_OPTIONAL, /*x tl*/
    ARG_TYPE_ROW | ARG_OPTIONAL,
    ARG_TYPE_COL | ARG_OPTIONAL, /*x br*/
    ARG_TYPE_ROW | ARG_OPTIONAL,

    ARG_TYPE_COL | ARG_OPTIONAL, /*y err tl*/
    ARG_TYPE_ROW | ARG_OPTIONAL,
    ARG_TYPE_COL | ARG_OPTIONAL, /*y err br*/
    ARG_TYPE_ROW | ARG_OPTIONAL,

    ARG_TYPE_COL | ARG_OPTIONAL, /*x err tl*/
    ARG_TYPE_ROW | ARG_OPTIONAL,
    ARG_TYPE_COL | ARG_OPTIONAL, /*x err br*/
    ARG_TYPE_ROW | ARG_OPTIONAL,

    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_s32[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_s32_s32[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_s32_s32_s32_s32[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_NONE 
};

static const ARG_TYPE
chart_args_f64[] =
{
    ARG_TYPE_F64 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_objid[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32 | ARG_OPTIONAL,
    ARG_TYPE_S32 | ARG_OPTIONAL,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_drawfile_embedded[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_RAW_DS | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_drawfile_reference[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_fillstyleb[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY, /*bits*/
    ARG_TYPE_S32 | ARG_MANDATORY, /*extref*/
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_fillstylec[] =
{
    ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_BOOL, /*rgbt*/
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_linestyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY, /*pattern*/
    ARG_TYPE_S32 | ARG_MANDATORY, /*width*/
    ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_BOOL, /*rgbt*/
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_textstyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,   /*height*/
    ARG_TYPE_S32 | ARG_MANDATORY,   /*width*/
    ARG_TYPE_TSTR | ARG_MANDATORY,  /*typeface*/
    ARG_TYPE_BOOL | ARG_MANDATORY,  /*bold*/
    ARG_TYPE_BOOL | ARG_MANDATORY,  /*italic*/
    ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_U8N | ARG_MANDATORY, ARG_TYPE_BOOL, /*rgbt*/
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_barchstyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_F64 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_barlinechstyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_F64 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_linechstyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_F64 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_piechdisplstyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_F64 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_piechlabelstyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_scatchstyle[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_F64 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
chart_args_text[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_NONE
};

/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

                                                                                                    /*   fi                         reject if file insertion */
                                                                                                    /*      ti                      reject if template insertion */
                                                                                                    /*         mi                   maybe interactive */
                                                                                                    /*            ur                unrecordable */
                                                                                                    /*               up             unrepeatable */
                                                                                                    /*                  xi          exceptional inline */
                                                                                                    /*                     md       modify document */
                                                                                                    /*                        mf    memory froth */
                                                                                                    /*                           nn supress newline on save */

    { "ActivateMenuChart",      NULL,                       T5_CMD_ACTIVATE_MENU_CHART,                 { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },

    { "Gallery",                chart_args_s32_nm,          T5_CMD_CHART_GALLERY,                       { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "ChartEdit",              chart_args_s32_nm,          T5_CMD_CHART_EDIT,                          { 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ChartStyle",             chart_args_s32_nm,          T5_CMD_CHART_STYLE,                         { 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ChartEditX",             chart_args_s32_nm,          T5_CMD_CHART_EDITX,                         { 0, 0, 0, 0, 0, 0, 0, 0, 0 } },

    { "Chart",                  chart_args_chart,           T5_CMD_CHART_IO_1,                          { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "CategoryData",           chart_args_category_data,   T5_CMD_CHART_IO_2,                          { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "SeriesData",             chart_args_series_data,     T5_CMD_CHART_IO_3,                          { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },

    /* done manually */

    { "CObject",                chart_args_objid,           T5_CMD_CHART_IO_OBJID,                      { 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ChartText",              chart_args_text,            T5_CMD_CHART_IO_TEXTCONTENTS },

    { "ChartDrawFileReference", chart_args_drawfile_reference, T5_CMD_CHART_IO_PICT_TRANS_REF },
    { "ChartDrawFileEmbedded",  chart_args_drawfile_embedded,  T5_CMD_CHART_IO_PICT_TRANS_EMB },

                                                                                                    /*   fi ti mi ur up xi md mf nn */
    /* gr_chart */

    { "ChartAxes",              chart_args_s32,             T5_CMD_CHART_IO_AXES_MAX },
    { "ChartSize",              chart_args_s32_s32,         T5_CMD_CHART_IO_CORE_LAYOUT },
    { "ChartMargins",           chart_args_s32_s32_s32_s32, T5_CMD_CHART_IO_CORE_MARGINS },

    { "LegendBits",             chart_args_s32,             T5_CMD_CHART_IO_LEGEND_BITS,                { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "Legend",                 chart_args_s32_s32_s32_s32, T5_CMD_CHART_IO_LEGEND_POSN,                { 0, 0, 0, 0, 0, 0, 0, 0, 0/*1*/ } }, /* and size too */

    { "Chart3D",                chart_args_s32,             T5_CMD_CHART_IO_D3_BITS,                    { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "Chart3DPitch",           chart_args_f64,             T5_CMD_CHART_IO_D3_DROOP,                   { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "Chart3DRoll",            chart_args_f64,             T5_CMD_CHART_IO_D3_TURN,                    { 0, 0, 0, 0, 0, 0, 0, 0, 0/*1*/ } },

    { "Bar2DOverlap",           chart_args_s32,             T5_CMD_CHART_IO_BARCH_SLOT_2D_OVERLAP },
    { "Line2DShift",            chart_args_s32,             T5_CMD_CHART_IO_LINECH_SLOT_2D_SHIFT },

                                                                                                    /*   fi ti mi ur up xi md mf nn */
    /* gr_axes */

    { "AxesBits",               chart_args_s32,             T5_CMD_CHART_IO_AXES_BITS },
    { "AxesSeriesType",         chart_args_s32,             T5_CMD_CHART_IO_AXES_SERIES_TYPE }, /* default series type */
    { "AxesChartType",          chart_args_s32,             T5_CMD_CHART_IO_AXES_CHART_TYPE },  /* default series chart type */
    { "AxesSeriesStart",        chart_args_s32,             T5_CMD_CHART_IO_AXES_START_SERIES },

                                                                                                    /*   fi ti mi ur up xi md mf nn */
    /* gr_axis */

    { "AxisBits",               chart_args_s32,             T5_CMD_CHART_IO_AXIS_BITS,                  { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "AxisMin",                chart_args_f64,             T5_CMD_CHART_IO_AXIS_PUNTER_MIN,            { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "AxisMax",                chart_args_f64,             T5_CMD_CHART_IO_AXIS_PUNTER_MAX,            { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },

    { "AxisMajor",              chart_args_s32,             T5_CMD_CHART_IO_AXIS_MAJOR_BITS,            { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "AxisMajorTicks",         chart_args_f64,             T5_CMD_CHART_IO_AXIS_MAJOR_PUNTER,          { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },

    { "AxisMinor",              chart_args_s32,             T5_CMD_CHART_IO_AXIS_MINOR_BITS,            { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "AxisMinorTicks",         chart_args_f64,             T5_CMD_CHART_IO_AXIS_MINOR_PUNTER,          { 0, 0, 0, 0, 0, 0, 0, 0, 0/*1*/ } },

                                                                                                    /*   fi ti mi ur up xi md mf nn */
    /* gr_series */

    { "SeriesBits",             chart_args_s32,             T5_CMD_CHART_IO_SERIES_BITS,                { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SeriesPieHeading",       chart_args_f64,             T5_CMD_CHART_IO_SERIES_PIE_HEADING,         { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SeriesType",             chart_args_s32,             T5_CMD_CHART_IO_SERIES_SERIES_TYPE,         { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SeriesTypeChart",        chart_args_s32,             T5_CMD_CHART_IO_SERIES_CHART_TYPE,          { 0, 0, 0, 0, 0, 0, 0, 0, 0/*1*/ } },

                                                                                                    /*   fi ti mi ur up xi md mf nn */
    /* harder to poke, more generic types */

    { "CSFillB",                chart_args_fillstyleb,      T5_CMD_CHART_IO_FILLSTYLEB },
    { "CSFillC",                chart_args_fillstylec,      T5_CMD_CHART_IO_FILLSTYLEC },
    { "CSLine",                 chart_args_linestyle,       T5_CMD_CHART_IO_LINESTYLE },
    { "CSText",                 chart_args_textstyle,       T5_CMD_CHART_IO_TEXTSTYLE },

    { "ChartStyleBar",          chart_args_barchstyle,      T5_CMD_CHART_IO_BARCHSTYLE },
    { "ChartStyleBL",           chart_args_barlinechstyle,  T5_CMD_CHART_IO_BARLINECHSTYLE },
    { "ChartStyleLine",         chart_args_linechstyle,     T5_CMD_CHART_IO_LINECHSTYLE },
    { "ChartStylePie",          chart_args_piechdisplstyle, T5_CMD_CHART_IO_PIECHDISPLSTYLE },
    { "ChartStylePieLabel",     chart_args_piechlabelstyle, T5_CMD_CHART_IO_PIECHLABELSTYLE },
    { "ChartStyleScatter",      chart_args_scatchstyle,     T5_CMD_CHART_IO_SCATCHSTYLE },

    { "ChartTextPos",           chart_args_s32_s32_s32_s32, T5_CMD_CHART_IO_TEXTPOS },

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

extern void
gr_chart_warning(
    _ChartRef_  P_GR_CHART p_gr_chart,
    _InVal_     STATUS status)
{
    UNREFERENCED_PARAMETER_ChartRef_(p_gr_chart);
    UNREFERENCED_PARAMETER_InVal_(status);
}

/* try to allocate charts in multiples of this */
#define CHART_ELEMENT_GRANULAR 8

/*
the list of charts
*/

static LIST_BLOCK
charts_list_block;

/******************************************************************************
*
* add the given shape of data to the chart
*
******************************************************************************/

#if 0
    STATUS has_unused_labels = FALSE;

    /* only consider adding labels if we haven't already done so to this chart */
    if(chart_shapedesc.bits.label_first_range)
    {
        if((chart_shapedesc.bits.chart_type == GR_CHART_TYPE_SCAT) || !gr_chart_query_labels(p_chart_header->ch))
            maybe_add_labels = TRUE;
        else
        {
            /* this is one range we won't have to bother adding then.
             * if it's the only one then do nothing more
            */
            if(!--n_ranges)
                return(1);

            has_unused_labels = TRUE;
        }
    }

    if(has_unused_labels)
    {
        /* turn them off and skip them */
        chart_shapedesc.bits.label_first_range = 0;
        ++end_key;
    }

#endif

_Check_return_
extern STATUS
chart_add(
    P_CHART_HEADER p_chart_header,
    P_CHART_SHAPEDESC e_p_chart_shapedesc)
{
    /* derive new blocks from the passed block & nz lists */
    CHART_SHAPEDESC chart_shapedesc = *e_p_chart_shapedesc;
    U32 n_ranges = chart_shapedesc.n_ranges;
    P_LIST_BLOCK p_list_block;
    LIST_ITEMNO stt_key, end_key;
    STATUS status = STATUS_OK;

    /* cater for ob_chart's side of the descriptor growing */
    status_return(chart_element_ensure(p_chart_header, n_ranges));

    p_list_block = chart_shapedesc.bits.range_over_columns ? &chart_shapedesc.nz_cols : &chart_shapedesc.nz_rows;

    /* NB this apparently loopy code is due to the stt_key = end_key; below! */
    end_key = (LIST_ITEMNO) (chart_shapedesc.bits.range_over_columns ? chart_shapedesc.region.tl.col : chart_shapedesc.region.tl.row);

    for(;;)
    {
        CHART_SHAPEDESC local_chart_shapedesc = chart_shapedesc;
        PC_U8 p_u8;

        /* find a contiguous range of columns/rows to add as a block */
        stt_key = end_key;

        if((p_u8 = collect_first_from(U8, p_list_block, &stt_key)) == NULL)
            /* no more ranges */
            break;

        if(stt_key /*found*/ < end_key /*desired start*/)
        {
            /* stt_key was in a hole and found the previous item, so do a next */
            if((p_u8 = collect_next(U8, p_list_block, &stt_key)) == NULL)
                break;
        }

        UNREFERENCED_PARAMETER(p_u8);

        end_key = stt_key + 1;

        chart_shapedesc.bits.label_first_range = 0; /* can have no more than one */

        if(local_chart_shapedesc.bits.range_over_columns)
        {
            local_chart_shapedesc.region.tl.col = (COL) stt_key;
            local_chart_shapedesc.region.br.col = local_chart_shapedesc.region.tl.col + 1;
        }
        else
        {
            local_chart_shapedesc.region.tl.row = (ROW) stt_key;
            local_chart_shapedesc.region.br.row = local_chart_shapedesc.region.tl.row + 1;
        }

        status_break(status = chart_element(p_chart_header, &local_chart_shapedesc));
    }

    return(status);
}

extern P_ARRAY_HANDLE
chart_automapper_help_handle(
    P_ANY client_handle)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) client_handle;
    const P_DOCU p_docu = p_docu_from_docno(p_chart_header->docno);
    return(&p_docu->chart_automapper);
}

extern void
chart_automapper_help_name(
    P_ANY client_handle,
    P_CHART_AUTOMAP_HELP p_chart_automap_help)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) client_handle;
    const P_DOCU p_docu = p_docu_from_docno(p_chart_header->docno);
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);
    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, FALSE));
    tstr_xstrkpy(p_chart_automap_help->wholename, BUF_MAX_PATHSTRING, quick_tblock_tstr(&quick_tblock));
    quick_tblock_dispose(&quick_tblock);
}

_Check_return_
extern STATUS
chart_element(
    P_CHART_HEADER p_chart_header,
    P_CHART_SHAPEDESC p_chart_shapedesc)
{
    STATUS status = STATUS_OK;

    if( (p_chart_shapedesc->region.tl.col < p_chart_shapedesc->region.br.col) &&
        (p_chart_shapedesc->region.tl.row < p_chart_shapedesc->region.br.row) ) /*inc,exc*/
    {
        const P_CHART_ELEMENT p_chart_element = chart_element_add(p_chart_header, p_chart_shapedesc->bits.range_over_columns ? CHART_RANGE_COL : CHART_RANGE_ROW);

        PTR_ASSERT(p_chart_element);

        p_chart_element->docno = p_chart_shapedesc->docno;
        p_chart_element->region = p_chart_shapedesc->region;

        p_chart_element->bits.label_first_range = p_chart_shapedesc->bits.label_first_range;
        p_chart_element->bits.label_first_item  = p_chart_shapedesc->bits.label_first_item;

        if(status_ok(status = uref_add_dependency(p_docu_from_docno(p_chart_element->docno), &p_chart_element->region, proc_uref_event_ob_chart, (CLIENT_HANDLE) p_chart_element, &p_chart_element->uref_handle, FALSE)))
        {
            if(p_chart_element->bits.label_first_range)
                status = gr_chart_add_labels(&p_chart_header->ch, proc_travel_chart, p_chart_element, &p_chart_element->gr_int_handle);
            else
                status = gr_chart_add(       &p_chart_header->ch, proc_travel_chart, p_chart_element, &p_chart_element->gr_int_handle);

        }

        if(status_fail(status))
            /* failed to add, so remove allocation */
            chart_element_subtract(p_chart_element);
    }

    return(status);
}

/******************************************************************************
*
* allocate offset for a new element
*
* NB. element must have had space allocated already!
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_CHART_ELEMENT
chart_element_add(
    P_CHART_HEADER p_chart_header,
    CHART_RANGE_TYPE type)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_chart_header->h_elem); ++i)
    {
        const P_CHART_ELEMENT p_chart_element = array_ptr(&p_chart_header->h_elem, CHART_ELEMENT, i);

        if(p_chart_element->bits.type == CHART_RANGE_NONE)
        {
            p_chart_element->bits.type = type;

            p_chart_element->p_chart_header = p_chart_header;

            return(p_chart_element);
        }
    }

    assert0();
    return(NULL);
}

/******************************************************************************
*
* dispose of a chart
*
******************************************************************************/

extern void
chart_dispose(
    /*inout*/ P_P_CHART_HEADER p_p_chart_header)
{
    P_CHART_HEADER p_chart_header = *p_p_chart_header;
    ARRAY_INDEX i;

    if(NULL == p_chart_header)
        return;

    *p_p_chart_header = NULL;

    /* kill the chart list entry */
    collect_subtract_entry(&charts_list_block, p_chart_header->chartdatakey);
    collect_compress(&charts_list_block);

    chart_selection_clear(p_chart_header, NULL);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_chart_header->selection.p_gr_riscdiag));

    /* loop over data sources, removing deps */
    for(i = 0; i < array_elements(&p_chart_header->h_elem); ++i)
    {
        const P_CHART_ELEMENT p_chart_element = array_ptr(&p_chart_header->h_elem, CHART_ELEMENT, i);

        if(p_chart_element->bits.type != CHART_RANGE_NONE)
            chart_element_subtract(p_chart_element);
    }

    al_array_dispose(&p_chart_header->h_elem);

    /* can kill the chart and its baggage in one (quick) fell swoop */
    gr_chart_dispose(&p_chart_header->ch);

    /* clear out the null event we have (probably) scheduled for this chart (maybe even during its removal!) */
    if(p_chart_header->recalc.state != CHART_UNMODIFIED)
    {
        p_chart_header->recalc.state = CHART_UNMODIFIED;
        trace_1(TRACE__SCHEDULED, TEXT("chart_dispose - *** scheduled_event_remove(docno=%d)"), p_chart_header->docno);
        scheduled_event_remove(p_chart_header->docno, T5_EVENT_SCHEDULED, scheduled_event_chart, (CLIENT_HANDLE) p_chart_header);
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_chart_header));
}

/******************************************************************************
*
* grow chart descriptor as necessary to take more elements
*
******************************************************************************/

_Check_return_
extern STATUS
chart_element_ensure(
    P_CHART_HEADER p_chart_header,
    _In_        U32 n_ranges)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i, old_elements;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(CHART_ELEMENT_GRANULAR, sizeof32(CHART_ELEMENT), TRUE);

    /* nuffink to allocate */
    if(n_ranges == 0)
        return(STATUS_OK);

    /* count number of holes present. don't reuse dead entries - they're there for a purpose! */
    old_elements = array_elements(&p_chart_header->h_elem);

    for(i = 0; i < old_elements; ++i)
    {
        const P_CHART_ELEMENT p_chart_element = array_ptr(&p_chart_header->h_elem, CHART_ELEMENT, i);

        if(p_chart_element->bits.type == CHART_RANGE_NONE)
            if(--n_ranges == 0)
                /* holes can take the load */
                return(STATUS_OK);
    }

    /* cater for ob_chart's side of the chart descriptor growing */

    if(NULL == al_array_extend_by(&p_chart_header->h_elem, CHART_ELEMENT, n_ranges, &array_init_block, &status))
        return(status);

    /* update references to the currently allocated chart elements (stored for us by gr_chart.c, sk_uref.c) */
    for(i = 0; i < old_elements; ++i)
    {
        const P_CHART_ELEMENT p_chart_element = array_ptr(&p_chart_header->h_elem, CHART_ELEMENT, i);

        if(p_chart_element->bits.type != CHART_RANGE_NONE)
        {
            status_consume(uref_change_handle(p_chart_element->docno, p_chart_element->uref_handle, (CLIENT_HANDLE) p_chart_element));

            status_consume(gr_chart_change_handle(p_chart_header->ch, p_chart_element->gr_int_handle, p_chart_element));
        }
    }

    return(status);
}

/******************************************************************************
*
* remove element
*
* NB. loads of protection as it's noted for recursing
*
******************************************************************************/

static void
chart_element_subtract(
    P_CHART_ELEMENT p_chart_element)
{
    p_chart_element->bits.type = CHART_RANGE_NONE;

    assert(array_elements(&p_chart_element->p_chart_header->h_elem));

    if(p_chart_element->uref_handle != UREF_HANDLE_INVALID)
    {
        UREF_HANDLE uref_handle = p_chart_element->uref_handle;
        p_chart_element->uref_handle = UREF_HANDLE_INVALID;
        uref_del_dependency(p_chart_element->docno, uref_handle);
    }

    if(p_chart_element->gr_int_handle)
    {
        GR_INT_HANDLE gr_int_handle = p_chart_element->gr_int_handle;
        p_chart_element->gr_int_handle = GR_DATASOURCE_HANDLE_NONE;
        status_consume(gr_chart_subtract(p_chart_element->p_chart_header->ch, &gr_int_handle));
    }
}

extern void
chart_modify(
    _InoutRef_  P_CHART_HEADER p_chart_header)
{
    switch(p_chart_header->recalc.state)
    {
    case CHART_UNMODIFIED:
        p_chart_header->recalc.state = CHART_MODIFIED_AWAITING_REBUILD;
        trace_1(TRACE__SCHEDULED, TEXT("chart_modify - *** scheduled_event_after(docno=%d, 0)"), p_chart_header->docno);
        status_assert(scheduled_event_after(p_chart_header->docno, T5_EVENT_SCHEDULED, scheduled_event_chart, (CLIENT_HANDLE) p_chart_header, 0));
        break;

    case CHART_MODIFIED_AWAITING_REBUILD:
        p_chart_header->recalc.state = CHART_MODIFIED_AGAIN;
        break;

    default: default_unhandled();
    case CHART_MODIFIED_AGAIN:
        break;
    }
}

static void
chart_modify_in_a_bit(
    _InoutRef_  P_CHART_HEADER p_chart_header)
{
    switch(p_chart_header->recalc.state)
    {
    case CHART_UNMODIFIED:
        p_chart_header->recalc.state = CHART_MODIFIED_AWAITING_REBUILD;
        trace_1(TRACE__SCHEDULED, TEXT("chart_modify_in_a_bit - *** scheduled_event_after(docno=%d, n)"), p_chart_header->docno);
        status_assert(scheduled_event_after(p_chart_header->docno, T5_EVENT_SCHEDULED, scheduled_event_chart, (CLIENT_HANDLE) p_chart_header, MONOTIMEDIFF_VALUE_FROM_MS(500)));
        break;

    case CHART_MODIFIED_AWAITING_REBUILD:
        p_chart_header->recalc.state = CHART_MODIFIED_AGAIN;
        break;

    default: default_unhandled();
    case CHART_MODIFIED_AGAIN:
        break;
    }
}

extern void
chart_modify_docu(
    _InoutRef_  P_CHART_HEADER p_chart_header)
{
    docu_modify(p_docu_from_docno(p_chart_header->docno));
    chart_modify(p_chart_header);
}

extern void
chart_rebuild_after_modify(
    _InoutRef_  P_CHART_HEADER p_chart_header)
{
    if(p_chart_header->recalc.state != CHART_UNMODIFIED)
    {   /* clear any scheduled events for this chart */
        p_chart_header->recalc.state = CHART_UNMODIFIED;
        trace_1(TRACE__SCHEDULED, TEXT("chart_rebuild_after_modify - *** scheduled_event_remove(docno=%d), clear pending rebuild"), p_chart_header->docno);
        scheduled_event_remove(p_chart_header->docno, T5_EVENT_SCHEDULED, scheduled_event_chart, (CLIENT_HANDLE) p_chart_header);
    }

    /* bonk the chart */
    gr_chart_modify_and_rebuild(p_chart_header->ch);

    {
    NOTE_UPDATE_OBJECT note_update_object;
    note_update_object.object_id = OBJECT_ID_CHART;
    note_update_object.object_data_ref = p_chart_header;
    status_assert(object_call_id(OBJECT_ID_NOTE, p_docu_from_docno(p_chart_header->docno), T5_MSG_NOTE_UPDATE_OBJECT, &note_update_object));
    } /*block*/
}

_Check_return_
static STATUS
chart_event_scheduled(
    _InRef_     P_SCHEDULED_EVENT_BLOCK p_scheduled_event_block)
{
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_scheduled_event_block->client_handle;

    if((p_chart_header->recalc.state == CHART_MODIFIED_AGAIN)
#if !defined(ensure_memory_froth) /* when defined, it's STATUS_OK */
    || status_fail(ensure_memory_froth())
#endif
    || global_preferences.chart_update_manual)
    {   /* reschedule until enough core or document has stabilised */
        p_chart_header->recalc.state = CHART_MODIFIED_AWAITING_REBUILD;
        trace_1(TRACE__SCHEDULED, TEXT("chart_event_scheduled: needs another update - *** scheduled_event_after(docno=%d, n)"), p_chart_header->docno);
        return(status_wrap(scheduled_event_after(p_chart_header->docno, T5_EVENT_SCHEDULED, scheduled_event_chart, (CLIENT_HANDLE) p_chart_header, MONOTIMEDIFF_VALUE_FROM_MS(500))));
    }

    chart_rebuild_after_modify(p_chart_header);
    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, scheduled_event_chart)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(t5_message)
    {
    case T5_EVENT_SCHEDULED:
        return(chart_event_scheduled((P_SCHEDULED_EVENT_BLOCK) p_data));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* prepare a new chart for adding to
*
******************************************************************************/

_Check_return_
extern STATUS
chart_new(
    /*out*/ P_P_CHART_HEADER p_p_chart_header,
    _InVal_     S32 use_preferred,
    _InVal_     S32 new_untitled)
{
    P_CHART_HEADER p_chart_header;
    STATUS status;

    /* allocate header */
    if(NULL == (p_chart_header = *p_p_chart_header = al_ptr_calloc_elem(CHART_HEADER, 1, &status)))
        return(status);

    p_chart_header->recalc.state = CHART_UNMODIFIED;

    for(;;) /* loop for structure */
    {
        static LIST_ITEMNO chartdatakey_gen = 0x52834000; /* NB. starting at a non-zero position, not tbs! */

        /* subsequent failure irrelevant to monotonic handle generator */
        LIST_ITEMNO chartdatakey = chartdatakey_gen++;
        CHART_LISTED_DATA chart_listed_data;

        /* allocate core for selection fake gr_riscdiag */
        if(NULL == (p_chart_header->selection.p_gr_riscdiag = al_ptr_calloc_elem(GR_RISCDIAG, 1, &status)))
            break;

        /* store key in chart header */
        p_chart_header->chartdatakey = chartdatakey;

        /* merely store pointer to chart header on list */
        chart_listed_data.p_chart_header = p_chart_header;

        if(NULL == collect_add_entry_elem(CHART_LISTED_DATA, &charts_list_block, &chart_listed_data, chartdatakey, &status))
            break;

        UNREFERENCED_PARAMETER_InVal_(use_preferred);
        status = (/*use_preferred
                  ? gr_chart_preferred_new(&p_chart_header->ch, p_chart_header)
                  :*/ gr_chart_new(&p_chart_header->ch, p_chart_header, new_untitled));

        if(status_fail(status))
        {
            collect_subtract_entry(&charts_list_block, chartdatakey);
            collect_compress(&charts_list_block);
        }

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

    if(status_fail(status))
    {
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_chart_header->selection.p_gr_riscdiag));
        al_ptr_dispose(P_P_ANY_PEDANTIC(p_p_chart_header));
    }

    return(status);
}

/******************************************************************************
*
* set up chart shape from marked block in document
*
******************************************************************************/

extern void
chart_shape_continue(
    _InoutRef_  P_CHART_SHAPEDESC p_chart_shapedesc)
{
    const P_LIST_BLOCK p_list_block = p_chart_shapedesc->bits.range_over_columns ? &p_chart_shapedesc->nz_cols : &p_chart_shapedesc->nz_rows;
    LIST_ITEMNO key = (LIST_ITEMNO) (p_chart_shapedesc->bits.range_over_columns ? p_chart_shapedesc->region.tl.col : p_chart_shapedesc->region.tl.row);
    PC_U8 p_u8;

    /* remove superfluous list now - NB. opposite order to all other occurrences! */
    collect_delete(p_chart_shapedesc->bits.range_over_columns ? &p_chart_shapedesc->nz_rows : &p_chart_shapedesc->nz_cols);

    p_chart_shapedesc->n_ranges = 0;

    for(p_u8 = collect_first_from(U8, p_list_block, &key);
        p_u8;
        p_u8 = collect_next(U8, p_list_block, &key))
    {
        BOOL subtract;

        if(*p_u8 & CELL_TYPE_NUMBER)
            subtract = (--(p_chart_shapedesc->allowed_numbers) < 0);
        else if(*p_u8 & CELL_TYPE_STRING)
            subtract = (--(p_chart_shapedesc->allowed_strings) < 0);
        else
            subtract = TRUE;

        if(subtract)
            collect_subtract_entry(p_list_block, key);
        else
            p_chart_shapedesc->n_ranges += 1;
    }

    collect_compress(p_list_block);
}

extern void
chart_shape_end(
    P_CHART_SHAPEDESC p_chart_shapedesc)
{
    collect_delete(&p_chart_shapedesc->nz_cols);
    collect_delete(&p_chart_shapedesc->nz_rows);
}

extern void
chart_shape_labels(
    P_CHART_SHAPEDESC p_chart_shapedesc)
{
    /* SKS after PD 4.12 24mar92 -  more care needed with top left corner for predictability */
    if(p_chart_shapedesc->bits.label_top_left)
    {
        if(p_chart_shapedesc->bits.range_over_manual && !p_chart_shapedesc->bits.range_over_columns)
        {
            /* if we are definitely arranging across rows then do opposite to the below case */
            if(!p_chart_shapedesc->bits.label_left_col)
            {
                if((p_chart_shapedesc->nz_n.row > 1) && p_chart_shapedesc->bits.number_left_col)
                    p_chart_shapedesc->bits.label_top_row  = 1;
                else
                    p_chart_shapedesc->bits.label_left_col = 1;
            }
        }
        else
        {
            /* give the label at top left to the top row only if it already has labels,
             * otherwise give to the left column if more than one column and otherwise empty top row
             * (else adding a column with a series label is impossible)
             * this makes adding to chart with series headings work as before (sort of)
            */
            if(!p_chart_shapedesc->bits.label_top_row)
            {
                if((p_chart_shapedesc->nz_n.col > 1) && p_chart_shapedesc->bits.number_top_row)
                    p_chart_shapedesc->bits.label_left_col = 1;
                else
                    p_chart_shapedesc->bits.label_top_row  = 1;
            }
        }
    }

    if(!p_chart_shapedesc->bits.range_over_manual)
    {
        /* SKS after PD 4.12 24mar92 - change test to be more careful and predictable */
        SLR slr = p_chart_shapedesc->nz_n;

        if(p_chart_shapedesc->bits.label_left_col)
            --slr.col;
        if(p_chart_shapedesc->bits.label_top_row)
            --slr.row;

        if(slr.col != slr.row)
            p_chart_shapedesc->bits.range_over_columns = (U8) (slr.col < slr.row);
        else if(p_chart_shapedesc->bits.label_top_row)
            p_chart_shapedesc->bits.range_over_columns = TRUE;
        else if(p_chart_shapedesc->bits.label_left_col)
            p_chart_shapedesc->bits.range_over_columns = FALSE;
        else
            p_chart_shapedesc->bits.range_over_columns = TRUE;
    }

    p_chart_shapedesc->bits.label_first_range = (U8) (p_chart_shapedesc->bits.range_over_columns ? p_chart_shapedesc->bits.label_left_col : p_chart_shapedesc->bits.label_top_row);
    p_chart_shapedesc->bits.label_first_item  = (U8) (p_chart_shapedesc->bits.range_over_columns ? p_chart_shapedesc->bits.label_top_row  : p_chart_shapedesc->bits.label_left_col);
}

_Check_return_
static U8
determine_cell_type(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_OBJECT_DATA p_object_data)
{
    OBJECT_DATA_READ object_data_read;
    EV_IDNO ev_idno;

    object_data_read.object_data = *p_object_data;
    ev_data_set_blank(&object_data_read.ev_data);
    status_consume(object_call_id(object_data_read.object_data.object_id, p_docu, T5_MSG_OBJECT_DATA_READ, &object_data_read));
    if(RPN_DAT_ARRAY == (ev_idno = object_data_read.ev_data.did_num))
        ev_idno = ss_array_element_index_wr(&object_data_read.ev_data, 0, 0)->did_num;
    ss_data_free_resources(&object_data_read.ev_data);

    switch(ev_idno)
    {
    default:
    case RPN_DAT_BLANK:
        return(CELL_TYPE_BLANK);

    case RPN_DAT_REAL:
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        return(CELL_TYPE_NUMBER);

    case RPN_DAT_DATE: /* SKS after 1.05 10oct93 */
    case RPN_DAT_STRING:
        return(CELL_TYPE_STRING);
    }
}

_Check_return_
extern STATUS
chart_shape_start(
    _DocuRef_   P_DOCU p_docu,
    P_CHART_SHAPEDESC p_chart_shapedesc,
    P_DOCU_AREA p_docu_area)
{
    STATUS status = STATUS_OK;

    p_chart_shapedesc->bits.number_top_left = 0;
    p_chart_shapedesc->bits.label_top_left  = 0;

    p_chart_shapedesc->bits.number_left_col = 0;
    p_chart_shapedesc->bits.label_left_col  = 0;

    p_chart_shapedesc->bits.number_top_row  = 0;
    p_chart_shapedesc->bits.label_top_row   = 0;

    p_chart_shapedesc->bits.some_number_cells = 0;

    p_chart_shapedesc->allowed_numbers = S32_MAX;
    p_chart_shapedesc->allowed_strings = 1;

    switch(p_chart_shapedesc->bits.chart_type)
    {
    default:
        break;

    case GR_CHART_TYPE_SCAT:
        p_chart_shapedesc->allowed_strings = 0;
        break;

    case GR_CHART_TYPE_PIE:
        p_chart_shapedesc->allowed_numbers = 1;
        break;
    }

    p_chart_shapedesc->min.col = MAX_COL;
    p_chart_shapedesc->min.row = MAX_ROW;
    p_chart_shapedesc->max.col = 0;
    p_chart_shapedesc->max.row = 0;

    list_init(&p_chart_shapedesc->nz_cols);
    list_init(&p_chart_shapedesc->nz_rows);

    {
    SCAN_BLOCK scan_block;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, SCAN_AREA, p_docu_area, OBJECT_ID_NONE)))
    {
        OBJECT_DATA object_data;

        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            /* if there's some usable cell contents, mark the col & row the cell is in */
            const U8 cell_type = determine_cell_type(p_docu, &object_data);

            switch(cell_type)
            {
            default: default_unhandled();
            case CELL_TYPE_BLANK:
                continue;

            case CELL_TYPE_NUMBER:
                p_chart_shapedesc->bits.some_number_cells = 1;
                break;

            case CELL_TYPE_STRING:
                break;
            }

            p_chart_shapedesc->min.col = MIN(p_chart_shapedesc->min.col, object_data.data_ref.arg.slr.col);
            p_chart_shapedesc->min.row = MIN(p_chart_shapedesc->min.row, object_data.data_ref.arg.slr.row);
            p_chart_shapedesc->max.col = MAX(p_chart_shapedesc->max.col, object_data.data_ref.arg.slr.col);
            p_chart_shapedesc->max.row = MAX(p_chart_shapedesc->max.row, object_data.data_ref.arg.slr.row);

            { /* this column is not blank */
            const P_LIST_BLOCK p_list_block = &p_chart_shapedesc->nz_cols;
            const LIST_ITEMNO key = object_data.data_ref.arg.slr.col;
            P_U8 p_u8;

            if((p_u8 = collect_goto_item(U8, p_list_block, key)) == NULL)
            {
                if(NULL == collect_add_entry_elem(U8, p_list_block, &cell_type, key, &status))
                    break;
            }
            else
                *p_u8 |= cell_type;
            } /*block*/

            { /* this row is not blank */
            const P_LIST_BLOCK p_list_block = &p_chart_shapedesc->nz_rows;
            const LIST_ITEMNO key = (LIST_ITEMNO) object_data.data_ref.arg.slr.row;
            P_U8 p_u8;

            if((p_u8 = collect_goto_item(U8, p_list_block, key)) == NULL)
            {
                if(NULL == collect_add_entry_elem(U8, p_list_block, &cell_type, key, &status))
                    break;
            }
            else
                *p_u8 |= cell_type;
            } /*block*/
        }
    }
    } /*block*/

    if(status_fail(status))
    {
        chart_shape_end(p_chart_shapedesc);

        return(status);
    }

    /* adjust the end points */
    p_chart_shapedesc->max.col += 1; /*excl*/
    p_chart_shapedesc->max.row += 1; /*excl*/

    p_chart_shapedesc->region.tl = p_chart_shapedesc->min;
    p_chart_shapedesc->region.br = p_chart_shapedesc->max;
    p_chart_shapedesc->region.whole_col = 0;
    p_chart_shapedesc->region.whole_row = 0;

    p_chart_shapedesc->n.col = p_chart_shapedesc->region.br.col - p_chart_shapedesc->region.tl.col;
    p_chart_shapedesc->n.row = p_chart_shapedesc->region.br.row - p_chart_shapedesc->region.tl.row;

    p_chart_shapedesc->nz_n.col = 0;
    p_chart_shapedesc->nz_n.row = 0;

    if(p_chart_shapedesc->min.col < p_chart_shapedesc->max.col)
    {
        /* count the number of cols actually used */
        const P_LIST_BLOCK p_list_block = &p_chart_shapedesc->nz_cols;
        LIST_ITEMNO key;
        PC_U8 p_u8;

        for(p_u8 = collect_first(U8, p_list_block, &key);
            p_u8;
            p_u8 = collect_next(U8, p_list_block, &key))
        {
            p_chart_shapedesc->nz_n.col += 1;
        }
    }

    if(p_chart_shapedesc->min.row < p_chart_shapedesc->max.row + 1)
    {
        /* count the number of rows actually used */
        const P_LIST_BLOCK p_list_block = &p_chart_shapedesc->nz_rows;
        LIST_ITEMNO key;
        PC_U8 p_u8;

        for(p_u8 = collect_first(U8, p_list_block, &key);
            p_u8;
            p_u8 = collect_next(U8, p_list_block, &key))
        {
            p_chart_shapedesc->nz_n.row += 1;
        }
    }

    if((0 == p_chart_shapedesc->nz_n.col) || (0 == p_chart_shapedesc->nz_n.row))
    {
        chart_shape_end(p_chart_shapedesc);

        return(create_error(CHART_ERR_NO_DATA));
    }

    { /* what is at the top left? */
    const SLR slr = p_chart_shapedesc->region.tl;
    OBJECT_DATA object_data;
    U8 cell_type;

    status_consume(object_data_from_slr(p_docu, &object_data, &slr));
    cell_type = determine_cell_type(p_docu, &object_data);

    switch(cell_type)
    {
    default:
        break;

    case CELL_TYPE_NUMBER:
        p_chart_shapedesc->bits.number_top_left = 1;
        break;

    case CELL_TYPE_STRING:
        p_chart_shapedesc->bits.label_top_left = 1;
        break;
    }
    } /*block*/

    { /* see whether left column is a label set. can skip top left as that's been covered */
    SLR slr = p_chart_shapedesc->region.tl;

    while(++slr.row < p_chart_shapedesc->region.br.row)
    {
        OBJECT_DATA object_data;
        U8 cell_type;

        status_consume(object_data_from_slr(p_docu, &object_data, &slr));
        cell_type = determine_cell_type(p_docu, &object_data);

        switch(cell_type)
        {
        default:
            break;

        case CELL_TYPE_NUMBER:
            p_chart_shapedesc->bits.number_left_col = 1;
            break;

        case CELL_TYPE_STRING:
            p_chart_shapedesc->bits.label_left_col = 1;
            break;
        }
    }
    } /*block*/

    { /* see whether top row is a label set. can skip top left as that's been covered */
    SLR slr = p_chart_shapedesc->region.tl;

    while(++slr.col < p_chart_shapedesc->region.br.col)
    {
        OBJECT_DATA object_data;
        U8 cell_type;

        status_consume(object_data_from_slr(p_docu, &object_data, &slr));
        cell_type = determine_cell_type(p_docu, &object_data);

        switch(cell_type)
        {
        default:
            break;

        case CELL_TYPE_NUMBER:
            p_chart_shapedesc->bits.number_top_row = 1;
            break;

        case CELL_TYPE_STRING:
            p_chart_shapedesc->bits.label_top_row = 1;
            break;
        }
    }
    } /*block*/

    return(STATUS_OK);
}

/*
main events
*/

MAEVE_EVENT_PROTO(static, maeve_event_ob_chart)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_FOCUS_CHANGED:
        /* pass some sideways */
        return(object_chart_edit_sideways(p_docu, t5_message, p_data));

    default:
        return(STATUS_OK);
    }
}

/*
object services hook
*/

_Check_return_
static STATUS
chart_msg_recalced(void)
{
    /* loop over all charts, find those that are in the MODIFIED_AGAIN state, and rebuild those and the AWAITING_REBUILD ones */
    LIST_ITEMNO chartdatakey;
    P_CHART_LISTED_DATA p_chart_listed_data;

    if(!collect_has_data(&charts_list_block))
        return(STATUS_OK);

    if(global_preferences.chart_update_manual)
        return(STATUS_OK);

    for(p_chart_listed_data = collect_first(CHART_LISTED_DATA, &charts_list_block, &chartdatakey);
        p_chart_listed_data;
        p_chart_listed_data = collect_next(CHART_LISTED_DATA, &charts_list_block, &chartdatakey))
    {
        const P_CHART_HEADER p_chart_header = p_chart_listed_data->p_chart_header;

        if(p_chart_header->recalc.state != CHART_UNMODIFIED)
            chart_rebuild_after_modify(p_chart_header);
    }

    return(STATUS_OK);
}

MAEVE_SERVICES_EVENT_PROTO(static, maeve_services_event_ob_chart)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER(p_data);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_RECALCED:
        return(chart_msg_recalced());

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* callback from uref to update references
*
******************************************************************************/

PROC_UREF_EVENT_PROTO(static, proc_uref_event_ob_chart)
{
    const P_CHART_ELEMENT p_chart_element = (P_CHART_ELEMENT) p_uref_event_block->uref_id.client_handle;
    const P_CHART_HEADER p_chart_header = p_chart_element->p_chart_header;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

#if TRACE_ALLOWED && 1
    if_constant(tracing(TRACE_APP_UREF))
    {
        TCHARZ buffer[64];
        PCTSTR p_msg;
        switch(t5_message)
        {
        default: assert0();         p_msg = TEXT("Unknown"); break;
        case T5_MSG_UREF_CLOSE1:    p_msg = TEXT("CLOSE1"); break;
        case T5_MSG_UREF_CLOSE2:    p_msg = TEXT("CLOSE2"); break;
        case T5_MSG_UREF_UREF:      p_msg = TEXT("UREF"); break;
        case T5_MSG_UREF_DELETE:    p_msg = TEXT("DELETE"); break;
        case T5_MSG_UREF_SWAP_ROWS: p_msg = TEXT("SWAP_ROWS"); break;
        case T5_MSG_UREF_CHANGE:    p_msg = TEXT("CHANGE"); break;
        case T5_MSG_UREF_OVERWRITE: p_msg = TEXT("OVERWRITE"); break;
        }
        consume_int(tstr_xsnprintf(buffer, elemof32(buffer),
                                   TEXT("chart_uref(%s) tl ") COL_TFMT TEXT(",") ROW_TFMT TEXT("; br ") COL_TFMT TEXT(",") ROW_TFMT TEXT("; wh_col ") S32_TFMT TEXT("; wh_row ") S32_TFMT,
                                   p_msg,
                                   p_uref_event_block->uref_id.region.tl.col,
                                   p_uref_event_block->uref_id.region.tl.row,
                                   p_uref_event_block->uref_id.region.br.col,
                                   p_uref_event_block->uref_id.region.br.row,
                                   (S32) p_uref_event_block->uref_id.region.whole_col,
                                   (S32) p_uref_event_block->uref_id.region.whole_row));
        uref_trace_reason(p_uref_event_block->reason.code, buffer);
    }
#endif

    switch(p_uref_event_block->reason.code)
    {
    default: default_unhandled();
#if CHECKING
    case DEP_NONE:
#endif
        break;

    case DEP_DELETE:
        switch(p_chart_element->bits.type)
        {
        default:
#if CHECKING
            myassert1(TEXT("chart element type ") S32_TFMT TEXT(" not COL or ROW or NONE"), p_chart_element->bits.type);

            /*FALLTHRU*/

        case CHART_RANGE_NONE:
#endif
            break;

        case CHART_RANGE_COL:
        case CHART_RANGE_ROW:
            {
            switch(uref_match_region(&p_chart_element->region, t5_message, p_uref_event_block))
            {
            default:
#if CHECKING
            case DEP_UPDATE:
                assert0();
                break;

            case DEP_INFORM:
                /* cell contents in our range are being deleted; we will get a uref */
            case DEP_NONE:
#endif
                break;

            case DEP_DELETE:
                chart_element_subtract(p_chart_element);
                chart_modify_in_a_bit(p_chart_element->p_chart_header);
                break;
            }

            break;
            }
        }

        break;

    case DEP_UPDATE:
        /* simple motion of elements harmless; just update structures
         * if rows(columns) have been inserted/added into a col(row)-based element then recalc chart
        */
        switch(p_chart_element->bits.type)
        {
        default:
#if CHECKING
            myassert1(TEXT("chart element type ") S32_TFMT TEXT(" not COL or ROW or NONE"), p_chart_element->bits.type);

            /*FALLTHRU*/

        case CHART_RANGE_NONE:
#endif
            break;

        case CHART_RANGE_COL:
            {
            REGION element_region = p_chart_element->region;

            switch(uref_match_region(&p_chart_element->region, t5_message, p_uref_event_block))
            {
            default:
#if CHECKING
            case DEP_DELETE:
            case DEP_INFORM:
                assert0();

                /*FALLTHRU*/

            case DEP_NONE:
#endif
                break;

            case DEP_UPDATE:
                if((p_chart_element->region.br.row - p_chart_element->region.tl.row) != (element_region.br.row - element_region.tl.row))
                {
                    /* col-based live range has had row insertion/deletion: recalc */
                    gr_chart_damage(p_chart_header->ch, p_chart_element->gr_int_handle);
                    chart_modify_in_a_bit(p_chart_header);
                }
                break;
            }

            break;
            }

        case CHART_RANGE_ROW:
            {
            REGION element_region = p_chart_element->region;

            switch(uref_match_region(&p_chart_element->region, t5_message, p_uref_event_block))
            {
            default:
#if CHECKING
            case DEP_DELETE:
            case DEP_INFORM:
                assert0();

                /*FALLTHRU*/

            case DEP_NONE:
#endif
                break;

            case DEP_UPDATE:
                if((p_chart_element->region.br.col - p_chart_element->region.tl.col) != (element_region.br.col - element_region.tl.col))
                {
                    /* row-based live range has had col insertion/deletion: recalc */
                    gr_chart_damage(p_chart_header->ch, p_chart_element->gr_int_handle);
                    chart_modify_in_a_bit(p_chart_header);
                }
                break;
            }

            break;
            }
        }

        break;

    case DEP_INFORM:
        switch(p_chart_element->bits.type)
        {
        default:
            break;

        case CHART_RANGE_COL:
        case CHART_RANGE_ROW:
            {
            switch(uref_match_region(&p_chart_element->region, t5_message, p_uref_event_block))
            {
            default:
#if CHECKING
            case DEP_UPDATE:
            case DEP_DELETE:
                assert0();

                /*FALLTHRU*/

            case DEP_NONE:
#endif
                break;

            case DEP_INFORM:
                switch(t5_message)
                {
                default:
                    break;

                case T5_MSG_UREF_OVERWRITE:
                case T5_MSG_UREF_CHANGE:
                    gr_chart_damage(p_chart_header->ch, p_chart_element->gr_int_handle);
                    chart_modify_in_a_bit(p_chart_header);
                    break;
                }
                break;
            }

            break;
            }
        }
        break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* procedure exported to the chart module to get data from the document
*
******************************************************************************/

static
PROC_GR_CHART_TRAVEL_PROTO(proc_travel_chart)
{
    const P_CHART_ELEMENT p_chart_element = (P_CHART_ELEMENT) client_handle;
    DOCNO docno;
    SLR slr;

    if(NULL == val)
    {
        /* client calling us to remove ourseleves */
        chart_element_subtract(p_chart_element);
        return(1);
    }

    /* determine actual cell to go to */
    switch(p_chart_element->bits.type)
    {
    case CHART_RANGE_COL:
        { /* col as input source: travel to the nth row therein */

        /* return size of range; gets complicated with array handling:
         * need to construct an enumeration list mapping external chart
         * addresses to real sub-travel() addresses
         * or else limit such to only supply one array per range
        */
        if(item < 0)
        {
            switch(item)
            {
            case GR_CHART_ITEMNO_N_ITEMS:
                /* return size of range */
                val->type = GR_CHART_VALUE_N_ITEMS;
                val->data.n_items = p_chart_element->region.br.row - p_chart_element->region.tl.row;

                /* if range has a label then number of items is one less */
                if(p_chart_element->bits.label_first_item)
                    val->data.n_items -= 1;

                return(1);

            case GR_CHART_ITEMNO_LABEL:
                /* return label for range */

                /* if range has label then it's the first item */
                if(!p_chart_element->bits.label_first_item)
                    return(0);

                assert(val->req_type == GR_CHART_VALUE_REQ_TEXT);
                item = 0;
                break;

            default:
                return(0);
            }
        }
        else
        {   /* if range has label then skip the first item */
            if(p_chart_element->bits.label_first_item)
                item += 1;
        }

        slr = p_chart_element->region.tl;

        slr.row += (ROW) item;

        if(slr.row >= p_chart_element->region.br.row)
            return(0);

        break;
        }

    default:
        myassert1x(p_chart_element->bits.type == CHART_RANGE_NONE, TEXT("chart element type ") S32_TFMT TEXT(" not COL or ROW or NONE"), p_chart_element->bits.type);

        /*FALLTHRU*/

    case CHART_RANGE_NONE:
        if(item == GR_CHART_ITEMNO_N_ITEMS)
        {
            val->type = GR_CHART_VALUE_N_ITEMS;
            val->data.n_items = 0;
        }

        return(1);

    case CHART_RANGE_ROW:
        { /* row as input source: travel to the nth column therein */

        if(item < 0)
        {
            switch(item)
            {
            case GR_CHART_ITEMNO_N_ITEMS:
                /* return size of range */
                val->type = GR_CHART_VALUE_N_ITEMS;
                val->data.n_items = p_chart_element->region.br.col - p_chart_element->region.tl.col;

                /* if range has a label then number of items is one less */
                if(p_chart_element->bits.label_first_item)
                    val->data.n_items -= 1;

                return(1);

            case GR_CHART_ITEMNO_LABEL:
                /* return label for range */

                /* if range has label then it's the first item */
                if(!p_chart_element->bits.label_first_item)
                    return(0);

                assert(val->req_type == GR_CHART_VALUE_REQ_TEXT);
                item = 0;
                break;

            default:
                return(0);
            }
        }
        else
        {   /* if range has label then skip the first item */
            if(p_chart_element->bits.label_first_item)
                item += 1;
        }

        slr = p_chart_element->region.tl;

        slr.col += (COL) item;

        if(slr.col >= p_chart_element->region.br.col)
            return(0);

        break;
        }
    }

    if((docno = p_chart_element->docno) == DOCNO_NONE)
        return(0);

    {
    const P_DOCU p_docu = p_docu_from_docno(docno);
    OBJECT_DATA_READ object_data_read;
    P_EV_DATA p_ev_data;
    STYLE_BIT_NUMBER style_bit_number;

    status_consume(object_data_from_slr(p_docu, &object_data_read.object_data, &slr));
    ev_data_set_blank(&object_data_read.ev_data);
    status_consume(object_call_id(object_data_read.object_data.object_id, p_docu, T5_MSG_OBJECT_DATA_READ, &object_data_read));
    p_ev_data = (RPN_DAT_ARRAY == object_data_read.ev_data.did_num)
              ? ss_array_element_index_wr(&object_data_read.ev_data, 0, 0)
              : &object_data_read.ev_data;

    /* SKS after 1.05 10oct93 speeds up numform lookup */
    switch(p_ev_data->did_num)
    {
    default: default_unhandled();
#if CHECKING
    case RPN_DAT_REAL:
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
#endif
        style_bit_number = STYLE_SW_PS_NUMFORM_NU;
        break;

    case RPN_DAT_DATE:
        style_bit_number = STYLE_SW_PS_NUMFORM_DT;
        break;

    case RPN_DAT_BLANK:
    case RPN_DAT_STRING:
    case RPN_DAT_ERROR:
        style_bit_number = STYLE_SW_PS_NUMFORM_SE;
        break;
    }

    switch(val->req_type)
    {
    default: default_unhandled();
#if CHECKING
    case GR_CHART_VALUE_REQ_NUMBER:
#endif
        if(RPN_DAT_REAL == p_ev_data->did_num)
        {
            val->type = GR_CHART_VALUE_NUMBER;
            val->data.number = object_data_read.ev_data.arg.fp;
            break;
        }

        if( (RPN_DAT_BOOL8  == p_ev_data->did_num) ||
            (RPN_DAT_WORD8  == p_ev_data->did_num) ||
            (RPN_DAT_WORD16 == p_ev_data->did_num) ||
            (RPN_DAT_WORD32 == p_ev_data->did_num) )
        {
            val->type = GR_CHART_VALUE_NUMBER;
            val->data.number = (F64) object_data_read.ev_data.arg.integer;
            break;
        }

        /*FALLTHRU*/

    case GR_CHART_VALUE_REQ_TEXT:
        {
        STYLE_SELECTOR selector;
        STYLE style;
        NUMFORM_PARMS numform_parms;
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup(&quick_ublock, val->data.text);

        style_selector_clear(&selector);
        style_selector_bit_set(&selector, style_bit_number);

        style_init(&style);
        style.para_style.h_numform_nu = 0;
        style.para_style.h_numform_dt = 0;
        style.para_style.h_numform_se = 0;
        style_from_slr(p_docu, &style, &selector, &slr);

        /*zero_struct(numform_parms);*/
        numform_parms.ustr_numform_numeric   = array_ustr(&style.para_style.h_numform_nu);
        numform_parms.ustr_numform_datetime  = array_ustr(&style.para_style.h_numform_dt);
        numform_parms.ustr_numform_texterror = array_ustr(&style.para_style.h_numform_se);
        numform_parms.p_numform_context = get_p_numform_context(p_docu);

        (void) numform(&quick_ublock, P_QUICK_TBLOCK_NONE, p_ev_data, &numform_parms);

        quick_ublock_dispose_leaving_buffer_valid(&quick_ublock);

        val->type = GR_CHART_VALUE_TEXT;

        break;
        }
    }

    ss_data_free_resources(&object_data_read.ev_data);
    } /*block*/

    return(1);
}

#if 0

static
PROC_GR_CHART_TRAVEL_PROTO(proc_travel_chart_text)
{
    const P_CHART_ELEMENT p_chart_element = (P_CHART_ELEMENT) client_handle;

    UNREFERENCED_PARAMETER(item);

    if(!val)
    {
        /* client calling us to remove ourselves */
        assert(p_chart_element->bits.type == CHART_RANGE_TXT);

        /* source cells going away: must kill use in chart */
        chart_element_subtract(p_chart_element);
        chart_modify_in_a_bit(p_chart_element->p_chart_header);

        return(1);
    }

    /* initial guess is nothing */
    val->type = GR_CHART_VALUE_NONE;

    { /* determine actual cell to go to */
    const P_DOCU p_docu = p_docu_from_docno(p_chart_element->docno);
    SLR slr = p_chart_element->region.tl;
    OBJECT_DATA_READ object_data_read;
    P_EV_DATA p_ev_data;

    if(STATUS_DONE != object_data_from_slr(p_docu, &object_data_read.object_data, &slr))
        return(0);

    ev_data_set_blank(&object_data_read.ev_data);
    status_consume(object_call_id(object_data_read.object_data.object_id, p_docu, T5_MSG_OBJECT_DATA_READ, &object_data_read));
    p_ev_data = (RPN_DAT_ARRAY == object_data_read.ev_data.did_num)
              ? ss_array_element_index_wr(&object_data_read.ev_data, 0, 0)
              : &object_data_read.ev_data;
    switch(p_ev_data->did_num)
    {
    default:
    case RPN_DAT_BLANK:
        break;

    case RPN_DAT_REAL:
        val->type = GR_CHART_VALUE_NUMBER;
        val->data.number = object_data_read.ev_data.arg.fp;
        break;

    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        val->type = GR_CHART_VALUE_NUMBER;
        val->data.number = (F64) object_data_read.ev_data.arg.integer;
        break;

    case RPN_DAT_STRING:
        val->type = GR_CHART_VALUE_TEXT;
        ustr_xstrkpy(val->data.text, elemof32(val->data.text), object_data_read.ev_data.arg.p_string);
        break;
    }
    ss_data_free_resources(&object_data_read.ev_data);
    } /*block*/

    return(1);
}

#endif

extern P_CHART_HEADER
p_chart_header_from_docu_last(
    _DocuRef_   P_DOCU p_docu)
{
    P_CHART_LISTED_DATA p_chart_listed_data = collect_goto_item(CHART_LISTED_DATA, &charts_list_block, p_docu->last_chart_edited);

    return(p_chart_listed_data ? p_chart_listed_data->p_chart_header : NULL);
}

/******************************************************************************
*
* Chart object event handler
*
******************************************************************************/

_Check_return_
static STATUS
chart_msg_startup(void)
{
    status_return(resource_init(OBJECT_ID_CHART, P_BOUND_MESSAGES_OBJECT_ID_CHART, P_BOUND_RESOURCES_OBJECT_ID_CHART));

#if WINDOWS
    {
    static const RESOURCE_BITMAP_ID id_common_btn_16x16 = { OBJECT_ID_CHART, CHART_ID_BM_COM_BTN_ID };
    static const RESOURCE_BITMAP_ID id_common_bar = { OBJECT_ID_CHART, CHART_ID_BM_COMBAR_ID };
    static const RESOURCE_BITMAP_ID id_common_lin = { OBJECT_ID_CHART, CHART_ID_BM_COMLIN_ID };
    static const RESOURCE_BITMAP_ID id_common_ovr = { OBJECT_ID_CHART, CHART_ID_BM_COMOVR_ID };
    static const RESOURCE_BITMAP_ID id_common_pie = { OBJECT_ID_CHART, CHART_ID_BM_COMPIE_ID };
    static const RESOURCE_BITMAP_ID id_common_sct = { OBJECT_ID_CHART, CHART_ID_BM_COMSCT_ID };
    status_assert(resource_bitmap_tool_size_register(&id_common_btn_16x16, 16, 16));
    status_assert(resource_bitmap_tool_size_register(&id_common_bar, 64, 60));
    status_assert(resource_bitmap_tool_size_register(&id_common_lin, 64, 60));
    status_assert(resource_bitmap_tool_size_register(&id_common_ovr, 64, 60));
    status_assert(resource_bitmap_tool_size_register(&id_common_pie, 64, 60));
    status_assert(resource_bitmap_tool_size_register(&id_common_sct, 64, 60));
    } /*block*/
#endif

    status_consume(register_object_construct_table(OBJECT_ID_CHART, object_construct_table, FALSE /* no inlines */));

    return(maeve_services_event_handler_add(maeve_services_event_ob_chart));
}

PROC_EVENT_PROTO(static, chart_msg_initclose)
{
    PC_MSG_INITCLOSE p_msg_initclose = (PC_MSG_INITCLOSE) p_data;

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(chart_msg_startup());
        break;

    case T5_MSG_IC__EXIT1:
        chart_automapper_del(P_ARRAY_HANDLE_NONE);
        status_consume(resource_close(OBJECT_ID_CHART));
        return(STATUS_OK);

    case T5_MSG_IC__INIT1:
        p_docu->chart_automapper = 0;
        status_return(maeve_event_handler_add(p_docu, maeve_event_ob_chart, (CLIENT_HANDLE) 0));
        return(STATUS_OK);

    case T5_MSG_IC__CLOSE1:
        chart_automapper_del(&p_docu->chart_automapper);
        return(STATUS_OK);

    case T5_MSG_IC__CLOSE2:
        maeve_event_handler_del(p_docu, maeve_event_ob_chart, (CLIENT_HANDLE) 0);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }

    /* pass some sideways */
    return(object_chart_io_sideways(p_docu, t5_message, p_data));
}

OBJECT_PROTO(extern, object_chart);
OBJECT_PROTO(extern, object_chart)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(chart_msg_initclose(p_docu, t5_message, p_data));

    case T5_CMD_ACTIVATE_MENU_CHART:
        p_docu->flags.next_chart_unpinned = 0;
        return(t5_cmd_activate_menu(p_docu, t5_message));

    case T5_MSG_CHART_GALLERY:
        return(chart_msg_chart_gallery(p_docu, t5_message, (P_T5_MSG_CHART_GALLERY_DATA) p_data));

    case T5_MSG_CHART_DIALOG:
        return(chart_msg_chart_dialog(p_docu, t5_message, (P_T5_MSG_CHART_DIALOG_DATA) p_data));

    /*********************************************************************************************/

    case T5_CMD_CHART_IO_AXES_MAX:
    case T5_CMD_CHART_IO_CORE_LAYOUT:
    case T5_CMD_CHART_IO_CORE_MARGINS:
    case T5_CMD_CHART_IO_LEGEND_BITS:
    case T5_CMD_CHART_IO_LEGEND_POSN:
    case T5_CMD_CHART_IO_D3_BITS:
    case T5_CMD_CHART_IO_D3_DROOP:
    case T5_CMD_CHART_IO_D3_TURN:
    case T5_CMD_CHART_IO_BARCH_SLOT_2D_OVERLAP:
    case T5_CMD_CHART_IO_LINECH_SLOT_2D_SHIFT:
    case T5_CMD_CHART_IO_AXES_BITS:
    case T5_CMD_CHART_IO_AXES_SERIES_TYPE:
    case T5_CMD_CHART_IO_AXES_CHART_TYPE:
    case T5_CMD_CHART_IO_AXES_START_SERIES:
    case T5_CMD_CHART_IO_AXIS_BITS:
    case T5_CMD_CHART_IO_AXIS_PUNTER_MIN:
    case T5_CMD_CHART_IO_AXIS_PUNTER_MAX:
    case T5_CMD_CHART_IO_AXIS_MAJOR_BITS:
    case T5_CMD_CHART_IO_AXIS_MAJOR_PUNTER:
    case T5_CMD_CHART_IO_AXIS_MINOR_BITS:
    case T5_CMD_CHART_IO_AXIS_MINOR_PUNTER:
    case T5_CMD_CHART_IO_SERIES_BITS:
    case T5_CMD_CHART_IO_SERIES_PIE_HEADING:
    case T5_CMD_CHART_IO_SERIES_SERIES_TYPE:
    case T5_CMD_CHART_IO_SERIES_CHART_TYPE:
    case T5_CMD_CHART_IO_FILLSTYLEB:
    case T5_CMD_CHART_IO_FILLSTYLEC:
    case T5_CMD_CHART_IO_LINESTYLE:
    case T5_CMD_CHART_IO_TEXTSTYLE:
    case T5_CMD_CHART_IO_BARCHSTYLE:
    case T5_CMD_CHART_IO_BARLINECHSTYLE:
    case T5_CMD_CHART_IO_LINECHSTYLE:
    case T5_CMD_CHART_IO_PIECHDISPLSTYLE:
    case T5_CMD_CHART_IO_PIECHLABELSTYLE:
    case T5_CMD_CHART_IO_SCATCHSTYLE:
    case T5_CMD_CHART_IO_TEXTPOS:
        return(chart_io_gr_construct_load(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_MSG_NOTE_ENSURE_SAVED:
    case T5_MSG_SAVE:
    case T5_MSG_NOTE_LOAD_INTREF_FROM_EXTREF:
    case T5_MSG_LOAD_ENDED:
    case T5_CMD_CHART_IO_1:
    case T5_CMD_CHART_IO_2:
    case T5_CMD_CHART_IO_3:
    case T5_CMD_CHART_IO_OBJID:
    case T5_CMD_CHART_IO_PICT_TRANS_REF:
    case T5_CMD_CHART_IO_PICT_TRANS_EMB:
    case T5_CMD_CHART_IO_TEXTCONTENTS:
        return(object_chart_io_sideways(p_docu, t5_message, p_data));

    default:
        return(object_chart_edit_sideways(p_docu, t5_message, p_data));
    }
}

/* end of ob_chart.c */
