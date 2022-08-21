/* gr_scale.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Scaling procedures for graphics modules and attribute storage */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#include "cmodules/collect.h"

/*
internal routines
*/

_Check_return_
_Ret_maybenull_
static P_BYTE
_gr_point_list_search(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InVal_     GR_LIST_ID list_id);

#define gr_point_list_search(__base_type, cp, series_idx, point, list_id) (\
    (__base_type *) _gr_point_list_search(cp, series_idx, point, list_id) )

_Check_return_
static STATUS
gr_point_list_set(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    /*_In_(~list_id)*/ PC_ANY style,
    _InVal_     GR_LIST_ID list_id);

/* -------------------------------------------------------------------------------- */

extern void
gr_chart_objid_from_axes_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name = GR_CHART_OBJNAME_AXIS;
    id->has_no = 1;
    id->no = (UBF) gr_axes_external_from_idx(cp, axes_idx, axis_idx);
}

/*
converts from GR_CHART_OBJNAME_AXIS(n) to appropriate major/minor tick/grid object
*/

extern void
gr_chart_objid_from_axis_grid(
    _InoutRef_  P_GR_CHART_OBJID id,
    _InVal_     BOOL major)
{
    id->name = GR_CHART_OBJNAME_AXISGRID;
    id->has_subno = 1;
    id->subno = major ? 1 : 2;
}

extern void
gr_chart_objid_from_axis_tick(
    _InoutRef_  P_GR_CHART_OBJID id,
    _InVal_     BOOL major)
{
    id->name = GR_CHART_OBJNAME_AXISTICK;
    id->has_subno = 1;
    id->subno = major ? 1 : 2;
}

extern void
gr_chart_objid_from_series_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name = GR_CHART_OBJNAME_SERIES;
    id->has_no = 1;
    id->no = (UBF) gr_series_external_from_idx(cp, series_idx);
}

extern void
gr_chart_objid_from_series_no(
    _InVal_     GR_ESERIES_NO eseries_no,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name = GR_CHART_OBJNAME_SERIES;
    id->has_no = 1;
    id->no = (UBF) eseries_no;
}

extern void
gr_chart_objid_from_text(
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_GR_CHART_OBJID id)
{
    gr_chart_objid_clear(id);
    id->name = GR_CHART_OBJNAME_TEXT;
    id->has_no = 1;
    id->no = (U8) key;
}

/* -------------------------------------------------------------------------------- */

typedef struct GR_STYLE_COMMON_BLK
{
    U32 offset_of_lbr;
    U32 style_size;
    void (* rerefproc) (
        _ChartRef_  P_GR_CHART cp,
        GR_SERIES_IDX series_idx,
        BOOL add);
#if TRACE_ALLOWED
    PCTSTR list_name;
#endif
}
GR_STYLE_COMMON_BLK; typedef const GR_STYLE_COMMON_BLK * PC_GR_STYLE_COMMON_BLK; /* NB. these are never modified */

#if TRACE_ALLOWED
#define TRACE_STYLE_CLASS(name) , TEXT(name)
#else
#define TRACE_STYLE_CLASS(name) /* nothing */
#endif

static const GR_STYLE_COMMON_BLK
gr_chart_text_common_blk =
{
    offsetof32(GR_CHART, text) + offsetof32(GR_CHART_TEXT, lbr), sizeof32(GR_TEXT), NULL TRACE_STYLE_CLASS("text object")
};

static const GR_STYLE_COMMON_BLK
gr_chart_text_textstyle_common_blk =
{
    offsetof32(GR_CHART, text) + offsetof32(GR_CHART_TEXT, style) + offsetof32(GR_CHART_TEXT_STYLE, lbr), sizeof32(GR_TEXTSTYLE), NULL TRACE_STYLE_CLASS("text object textstyle")
};

static const GR_STYLE_COMMON_BLK
gr_pdrop_fillstyleb_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, pdrop_fillb), sizeof32(GR_FILLSTYLEB), gr_pdrop_list_fillstyleb_reref TRACE_STYLE_CLASS("drop fillstyleb")
};

static const GR_STYLE_COMMON_BLK
gr_pdrop_fillstylec_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, pdrop_fillc), sizeof32(GR_FILLSTYLEC), NULL TRACE_STYLE_CLASS("drop fillstylec")
};

static const GR_STYLE_COMMON_BLK
gr_pdrop_linestyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, pdrop_line), sizeof32(GR_LINESTYLE), NULL TRACE_STYLE_CLASS("drop linestyle")
};

static const GR_STYLE_COMMON_BLK
gr_point_fillstyleb_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_fillb), sizeof32(GR_FILLSTYLEB), gr_point_list_fillstyleb_reref TRACE_STYLE_CLASS("fillstyleb")
};

static const GR_STYLE_COMMON_BLK
gr_point_fillstylec_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_fillc), sizeof32(GR_FILLSTYLEC), NULL TRACE_STYLE_CLASS("fillstylec")
};

static const GR_STYLE_COMMON_BLK
gr_point_linestyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_line), sizeof32(GR_LINESTYLE), NULL TRACE_STYLE_CLASS("linestyle")
};

static const GR_STYLE_COMMON_BLK
gr_point_textstyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_text), sizeof32(GR_TEXTSTYLE), NULL TRACE_STYLE_CLASS("textstyle")
};

static const GR_STYLE_COMMON_BLK
gr_point_barchstyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_barch), sizeof32(GR_BARCHSTYLE), NULL TRACE_STYLE_CLASS("bar chart")
};

static const GR_STYLE_COMMON_BLK
gr_point_barlinechstyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_barlinech), sizeof32(GR_BARLINECHSTYLE), NULL TRACE_STYLE_CLASS("bar & line chart")
};

static const GR_STYLE_COMMON_BLK
gr_point_linechstyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_linech), sizeof32(GR_LINECHSTYLE), NULL TRACE_STYLE_CLASS("line chart")
};

static const GR_STYLE_COMMON_BLK
gr_point_piechdisplstyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_piechdispl), sizeof32(GR_PIECHDISPLSTYLE), NULL TRACE_STYLE_CLASS("pie chart displ")
};

static const GR_STYLE_COMMON_BLK
gr_point_piechlabelstyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_piechlabel), sizeof32(GR_PIECHLABELSTYLE), NULL TRACE_STYLE_CLASS("pie chart label")
};

static const GR_STYLE_COMMON_BLK
gr_point_scatchstyle_common_blk =
{
    offsetof32(GR_SERIES, lbr) + offsetof32(GR_SERIES_LBR, point_scatch), sizeof32(GR_SCATCHSTYLE), NULL TRACE_STYLE_CLASS("scatter chart")
};

/*
array tying GR_LIST_IDs to GR_STYLE_COMMON_BLKs
*/

static const PC_GR_STYLE_COMMON_BLK
gr_style_common_blks[GR_LIST_N_IDS] =
{
    &gr_chart_text_common_blk,
    &gr_chart_text_textstyle_common_blk,

    &gr_pdrop_fillstyleb_common_blk,
    &gr_pdrop_fillstylec_common_blk,
    &gr_pdrop_linestyle_common_blk,

    &gr_point_fillstyleb_common_blk,
    &gr_point_fillstylec_common_blk,
    &gr_point_linestyle_common_blk,
    &gr_point_textstyle_common_blk,

    &gr_point_barchstyle_common_blk,
    &gr_point_barlinechstyle_common_blk,
    &gr_point_linechstyle_common_blk,
    &gr_point_piechdisplstyle_common_blk,
    &gr_point_piechlabelstyle_common_blk,
    &gr_point_scatchstyle_common_blk
};

/******************************************************************************
*
* create an entry on a list to store style data
* associated with a data point and store it
*
******************************************************************************/

_Check_return_
static STATUS
common_list_set(
    P_LIST_BLOCK p_list_block,
    _InVal_     LIST_ITEMNO item,
    _In_reads_bytes_opt_(style_size) PC_ANY style,
    _InVal_     U32 style_size)
{
    P_BYTE pt;

    PTR_ASSERT(p_list_block);

    if(NULL == (pt = collect_goto_item(BYTE, p_list_block, item)))
    {
        P_LIST_ITEM it;

        if(NULL == (it = list_createitem(p_list_block, item, style_size, FALSE)))
            return(status_nomem());
        else
            pt = list_itemcontents(BYTE, it);
    }

    if(style)
        memcpy32(pt, style, style_size);

    return(STATUS_OK);
}

static void
common_list_fillstyleb_reref(
    P_LIST_BLOCK p_list_block,
    _InVal_     BOOL add)
{
    LIST_ITEMNO key;
    P_GR_FILLSTYLEB pt;

    for(pt = collect_first(GR_FILLSTYLEB, p_list_block, &key);
        pt;
        pt = collect_next(GR_FILLSTYLEB, p_list_block, &key))
    {
        /* add/lose ref to particular picture. DOES NOT destroy stored pattern handle */
        gr_cache_ref((GR_CACHE_HANDLE) pt->pattern, add);
    }
}

/******************************************************************************
*
* lists rooted in the chart
*
******************************************************************************/

#define gr_chart_list_get_p_list_block(cp, cbp) \
    PtrAddBytes(P_LIST_BLOCK, (cp), (cbp)->offset_of_lbr)

extern void
gr_chart_list_delete(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_LIST_BLOCK p_list_block = gr_chart_list_get_p_list_block(cp, cbp);

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_chart_list_delete %s list ") PTR_XTFMT, cbp->list_name, p_list_block);

    assert(!cbp->rerefproc);

    collect_delete(p_list_block);
}

_Check_return_
extern STATUS
gr_chart_list_duplic(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_LIST_BLOCK p_list_block = gr_chart_list_get_p_list_block(cp, cbp);
    LIST_BLOCK lb;
    STATUS status;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_chart_list_duplic %s list ") PTR_XTFMT, cbp->list_name, p_list_block);

    zero_struct(lb);

    status = collect_copy(&lb, p_list_block);

    *p_list_block = lb;

    assert(!cbp->rerefproc);

    return(status);
}

#if defined(UNUSED_KEEP_ALIVE)

extern P_ANY
gr_chart_list_first(
    _ChartRef_  P_GR_CHART cp,
    _OutRef_    P_LIST_ITEMNO p_key,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    return(_collect_first(gr_chart_list_get_p_list_block(cp, cbp), p_key CODE_ANALYSIS_ONLY_ARG(cbp->style_size)));
}

extern P_ANY
gr_chart_list_next(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    return(_collect_next(gr_chart_list_get_p_list_block(cp, cbp), p_key CODE_ANALYSIS_ONLY_ARG(cbp->style_size)));
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* query/set the general chart style associated with an object
*
******************************************************************************/

typedef struct GR_CHART_OBJID_CHARTSTYLE_DESC
{
    U32 style_size;
    U32 axes_offset;
    U32 series_offset;
    GR_LIST_ID /*U32*/ list_id; /* GR_LIST_ID */
}
GR_CHART_OBJID_CHARTSTYLE_DESC; typedef const GR_CHART_OBJID_CHARTSTYLE_DESC * PC_GR_CHART_OBJID_CHARTSTYLE_DESC;

/*
bar chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_barchstyle_desc =
{
                          sizeof32(GR_BARCHSTYLE),
    offsetof32(GR_AXES_STYLE,         barch) + offsetof32(GR_AXES,   style),
    offsetof32(GR_SERIES_STYLE, point_barch) + offsetof32(GR_SERIES, style),
                        GR_LIST_POINT_BARCHSTYLE
};

/*
bar & line chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_barlinechstyle_desc =
{
                          sizeof32(GR_BARLINECHSTYLE),
    offsetof32(GR_AXES_STYLE,         barlinech) + offsetof32(GR_AXES,   style),
    offsetof32(GR_SERIES_STYLE, point_barlinech) + offsetof32(GR_SERIES, style),
                        GR_LIST_POINT_BARLINECHSTYLE
};

/*
line chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_linechstyle_desc =
{
                          sizeof32(GR_LINECHSTYLE),
    offsetof32(GR_AXES_STYLE,         linech) + offsetof32(GR_AXES,   style),
    offsetof32(GR_SERIES_STYLE, point_linech) + offsetof32(GR_SERIES, style),
                        GR_LIST_POINT_LINECHSTYLE
};

/*
pie chart styles
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_piechdisplstyle_desc =
{
                          sizeof32(GR_PIECHDISPLSTYLE),
    offsetof32(GR_AXES_STYLE,         piechdispl) + offsetof32(GR_AXES,   style),
    offsetof32(GR_SERIES_STYLE, point_piechdispl) + offsetof32(GR_SERIES, style),
                        GR_LIST_POINT_PIECHDISPLSTYLE
};

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_piechlabelstyle_desc =
{
                          sizeof32(GR_PIECHLABELSTYLE),
    offsetof32(GR_AXES_STYLE,         piechlabel) + offsetof32(GR_AXES,   style),
    offsetof32(GR_SERIES_STYLE, point_piechlabel) + offsetof32(GR_SERIES, style),
                        GR_LIST_POINT_PIECHLABELSTYLE
};

/*
scatter chart style
*/

static const GR_CHART_OBJID_CHARTSTYLE_DESC
gr_chart_objid_scatchstyle_desc =
{
                          sizeof32(GR_SCATCHSTYLE),
    offsetof32(GR_AXES_STYLE,         scatch) + offsetof32(GR_AXES,   style),
    offsetof32(GR_SERIES_STYLE, point_scatch) + offsetof32(GR_SERIES, style),
                        GR_LIST_POINT_SCATCHSTYLE
};

/******************************************************************************
*
* query objid
*
******************************************************************************/

static BOOL
gr_chart_objid_chartstyle_query(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    /*out*/ P_ANY style,
    _InRef_     PC_GR_CHART_OBJID_CHARTSTYLE_DESC desc)
{
    PC_BYTE bpt;
    GR_AXES_IDX axes_idx;
    GR_SERIES_IDX series_idx;
    BOOL using_default = 0;

    PTR_ASSERT(cp);
    PTR_ASSERT(style);
    PTR_ASSERT(desc);

    switch(id.name)
    {
    default:
        /* use style of first axes */
        axes_idx = 0;
        goto lookup_axes_style;

    case GR_CHART_OBJNAME_AXIS:
    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        consume(GR_AXIS_IDX, gr_axes_idx_from_external(cp, id.no, &axes_idx));

    lookup_axes_style:;
        bpt = PtrAddBytes(PC_BYTE, &cp->axes[axes_idx], desc->axes_offset);
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_BESTFITSER:
    case GR_CHART_OBJNAME_LEGDSERIES:
        series_idx = gr_series_idx_from_external(cp, id.no);

    lookup_series_style:;
        bpt = PtrAddBytes(PC_BYTE, getserp(cp, series_idx), desc->series_offset);

        if(((* (PC_BYTE) bpt) & 1) == 0 /*!pt->bits.manual*/)
        {
            axes_idx = gr_axes_idx_from_series_idx(cp, series_idx);
            using_default = 1;
            goto lookup_axes_style;
        }

        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
    case GR_CHART_OBJNAME_LEGDPOINT:
        series_idx = gr_series_idx_from_external(cp, id.no);

        bpt = _gr_point_list_search(cp, series_idx, gr_point_key_from_external(id.subno), desc->list_id);

        if(!bpt)
        {
            using_default = 1;
            goto lookup_series_style; /* with series_idx valid */
        }

        break;
    }

    memcpy32(style, bpt, desc->style_size);

    return(using_default);
}

/******************************************************************************
*
* set objid
*
******************************************************************************/

_Check_return_
static STATUS
gr_chart_objid_chartstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    /*_In_(~desc->style_size)*/ PC_ANY style,
    _InRef_     PC_GR_CHART_OBJID_CHARTSTYLE_DESC desc)
{
    P_BYTE bpt;
    GR_AXES_IDX axes_idx;
    GR_SERIES_IDX series_idx;

    PTR_ASSERT(cp);
    PTR_ASSERT(style);
    PTR_ASSERT(desc);

    switch(id.name)
    {
    default:
        /* set style of first axes */
        axes_idx = 0;
        goto set_axes_style;

    case GR_CHART_OBJNAME_AXIS:
    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        consume(GR_AXIS_IDX, gr_axes_idx_from_external(cp, id.no, &axes_idx));

    set_axes_style:;

        /* set all series on axes to auto and remove deviant points */
        for(series_idx = cp->axes[axes_idx].series.stt_idx; series_idx < cp->axes[axes_idx].series.end_idx; ++series_idx)
        {
            /* remove deviant point data from this series */
            gr_point_list_delete(cp, series_idx, desc->list_id);

            /* set series to auto */
            bpt = PtrAddBytes(P_BYTE, getserp(cp, series_idx), desc->series_offset);

            *bpt &= ~1; /*serp->style.point_xxxch.bits.manual = 0;*/
        }

        bpt = PtrAddBytes(P_BYTE, &cp->axes[axes_idx], desc->axes_offset);
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_BESTFITSER:
        series_idx = gr_series_idx_from_external(cp, id.no);

        /* remove deviant point data from this series */
        gr_point_list_delete(cp, series_idx, desc->list_id);

        bpt = PtrAddBytes(P_BYTE, getserp(cp, series_idx), desc->series_offset);
        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_point_list_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style, desc->list_id));
    }

    PTR_ASSERT(bpt);
    if(bpt)
        memcpy32(bpt, style, desc->style_size);

    return(STATUS_OK);
}

/******************************************************************************
*
* query/set the bar, bar & line, line, scatter chart style associated with an object
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_chart_objid_barchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_BARCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_barchstyle_desc));
}

_Check_return_
extern STATUS
gr_chart_objid_barchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BARCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_barchstyle_desc));
}

/*ncr*/
extern BOOL
gr_chart_objid_barlinechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_BARLINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_barlinechstyle_desc));
}

_Check_return_
extern STATUS
gr_chart_objid_barlinechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BARLINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_barlinechstyle_desc));
}

/*ncr*/
extern BOOL
gr_chart_objid_linechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_LINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_linechstyle_desc));
}

_Check_return_
extern STATUS
gr_chart_objid_linechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_LINECHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_linechstyle_desc));
}

/*ncr*/
extern BOOL
gr_chart_objid_piechdisplstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHDISPLSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_piechdisplstyle_desc));
}

_Check_return_
extern STATUS
gr_chart_objid_piechdisplstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_PIECHDISPLSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_piechdisplstyle_desc));
}

/*ncr*/
extern BOOL
gr_chart_objid_piechlabelstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_PIECHLABELSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_piechlabelstyle_desc));
}

_Check_return_
extern STATUS
gr_chart_objid_piechlabelstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_PIECHLABELSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_piechlabelstyle_desc));
}

/*ncr*/
extern BOOL
gr_chart_objid_scatchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_SCATCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_query(cp, id, style, &gr_chart_objid_scatchstyle_desc));
}

_Check_return_
extern STATUS
gr_chart_objid_scatchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_SCATCHSTYLE style)
{
    return(gr_chart_objid_chartstyle_set(cp, id, style, &gr_chart_objid_scatchstyle_desc));
}

/******************************************************************************
*
* query/set the fill style associated with an object
*
******************************************************************************/

#define GR_FILLSTYLEC_N_DEFAULTS 9

static GR_FILLSTYLEC gr_fillstylec_defaults[GR_FILLSTYLEC_N_DEFAULTS]; /* starts up all zeros */

_Check_return_
_Ret_valid_
static PC_GR_FILLSTYLEC
gr_fillstylec_default(
    _InVal_     GR_CHART_ITEMNO item)
{
    static BOOL fill_init = FALSE;

    if(!fill_init)
    {
        static const int fill_with_wimp[GR_FILLSTYLEC_N_DEFAULTS] =
        {
            3  /* mid grey */,

            /* this vvv will be the first one used */
            11 /* simply red    */,
            9  /* mellow yellow */,
            14 /* orange        */,
            15 /* light blue    */,
            10 /* light green   */,
            12 /* fresh cream   */,
            13 /* dark green    */,
            8  /* dark blue     */
        };

        UINT i;
        P_GR_FILLSTYLEC pt = gr_fillstylec_defaults;

        fill_init = TRUE;

        for(i = 0; i < elemof32(fill_with_wimp); ++i, ++pt)
        {
            PC_RGB p_rgb = &rgb_stash[fill_with_wimp[i]];
            pt->fg.red = p_rgb->r;
            pt->fg.green = p_rgb->g;
            pt->fg.blue = p_rgb->b;
            pt->fg.reserved = 0;
            pt->fg.manual = 0;
            pt->fg.visible = 1;
        }
    }

    assert(item >= 0);

    return(&gr_fillstylec_defaults[item % GR_FILLSTYLEC_N_DEFAULTS]);
}

/*ncr*/
extern BOOL
gr_chart_objid_fillstyleb_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_FILLSTYLEB style)
{
    PC_GR_FILLSTYLEB pt;
    BOOL using_default = 0;

    PTR_ASSERT(cp);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
        pt = &cp->chart.areastyleb;
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        assert(id.no < GR_CHART_N_PLOTAREAS);
        pt = &cp->plotarea.area[(id.no < GR_CHART_N_PLOTAREAS) ? id.no : 0].areastyleb;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.areastyleb;
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_LEGDSERIES:
        pt = &gr_seriesp_from_external(cp, id.no)->style.point_fillb;
        break;

    case GR_CHART_OBJNAME_DROPSERIES:
        pt = &gr_seriesp_from_external(cp, id.no)->style.pdrop_fillb;
        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_LEGDPOINT:
        return(gr_point_fillstyleb_query(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_pdrop_fillstyleb_query(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    default:
        myassert1(TEXT("gr_chart_objid_fillstyleb_query of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        using_default = 1;
        pt = &cp->chart.areastyleb;
        break;
    }

    *style = *pt; /* with varied_by_point clear */

    return(using_default);
}

_Check_return_
extern STATUS
gr_chart_objid_fillstyleb_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_FILLSTYLEB style)
{
    P_GR_FILLSTYLEB pt;
    GR_SERIES_IDX series_idx;

    PTR_ASSERT(cp);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
        pt = &cp->chart.areastyleb;
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        assert(id.no < GR_CHART_N_PLOTAREAS);
        if(id.no >= GR_CHART_N_PLOTAREAS)
            return(1);

        pt = &cp->plotarea.area[id.no].areastyleb;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.areastyleb;
        break;

    case GR_CHART_OBJNAME_SERIES:
        series_idx = gr_series_idx_from_external(cp, id.no);

        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_FILLSTYLEB);

        pt = &getserp(cp, series_idx)->style.point_fillb;
        break;

    case GR_CHART_OBJNAME_DROPSERIES:
        series_idx = gr_series_idx_from_external(cp, id.no);

        gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_FILLSTYLEB);

        pt = &getserp(cp, series_idx)->style.pdrop_fillb;
        break;

    case GR_CHART_OBJNAME_POINT:
        return(gr_point_fillstyleb_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_pdrop_fillstyleb_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    default:
        myassert1(TEXT("gr_chart_objid_fillstyleb_set of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        return(1);
    }

    gr_fillstyleb_pict_change(pt, &style->pattern);

    *pt = *style;

    /* check validity of style bits */
    if(pt->pattern != GR_FILL_PATTERN_NONE)
        pt->bits.pattern = 1;

    return(1);
}

/*ncr*/
extern BOOL
gr_chart_objid_fillstylec_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_FILLSTYLEC style)
{
    PC_GR_FILLSTYLEC pt;
    BOOL using_default = 0;

    PTR_ASSERT(cp);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
        pt = &cp->chart.areastylec;
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        assert(id.no < GR_CHART_N_PLOTAREAS);
        pt = &cp->plotarea.area[(id.no < GR_CHART_N_PLOTAREAS) ? id.no : 0].areastylec;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.areastylec;
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_LEGDSERIES:
        pt = &gr_seriesp_from_external(cp, id.no)->style.point_fillc;

        if(!pt->fg.manual)
        {
            using_default = 1;
            pt = gr_fillstylec_default(id.no);
        }
        break;

    case GR_CHART_OBJNAME_DROPSERIES:
        pt = &gr_seriesp_from_external(cp, id.no)->style.pdrop_fillc;

        if(!pt->fg.manual)
        {
            using_default = 1;
            pt = gr_fillstylec_default(id.no);
        }
        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_LEGDPOINT:
        return(gr_point_fillstylec_query(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_pdrop_fillstylec_query(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    default:
        myassert1(TEXT("gr_chart_objid_fillstylec_query of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        using_default = 1;
        pt = gr_fillstylec_default(0);
        break;
    }

    *style = *pt; /* with varied_by_point clear */

    return(using_default);
}

_Check_return_
extern STATUS
gr_chart_objid_fillstylec_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_FILLSTYLEC style)
{
    P_GR_FILLSTYLEC pt;
    GR_SERIES_IDX series_idx;

    PTR_ASSERT(cp);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
        pt = &cp->chart.areastylec;
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        assert(id.no < GR_CHART_N_PLOTAREAS);
        if(id.no >= GR_CHART_N_PLOTAREAS)
            return(1);

        pt = &cp->plotarea.area[id.no].areastylec;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.areastylec;
        break;

    case GR_CHART_OBJNAME_SERIES:
        series_idx = gr_series_idx_from_external(cp, id.no);

        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_FILLSTYLEC);

        pt = &getserp(cp, series_idx)->style.point_fillc;
        break;

    case GR_CHART_OBJNAME_DROPSERIES:
        series_idx = gr_series_idx_from_external(cp, id.no);

        gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_FILLSTYLEC);

        pt = &getserp(cp, series_idx)->style.pdrop_fillc;
        break;

    case GR_CHART_OBJNAME_POINT:
        return(gr_point_fillstylec_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_pdrop_fillstylec_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    default:
        myassert1(TEXT("gr_chart_objid_fillstylec_set of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        return(1);
    }

    *pt = *style;

    return(1);
}

/******************************************************************************
*
* query/set the line style associated with an object
*
******************************************************************************/

_Check_return_
_Ret_valid_
static PC_GR_LINESTYLE
gr_linestyle_default(
    GR_CHART_LINESTYLE_CACHE * p_cache,
    _InVal_     GR_CHART_ITEMNO item,
    _InVal_     BOOL bodge_2d)
{
    if(!p_cache->init)
    {
        static const int fill_with_wimp[GR_LINESTYLE_2D_N_DEFAULTS] =
        {
            3  /* mid grey */,

            /* this vvv will be the first one used */
            11 /* simply red    */,
            13 /* dark green    */,
            8  /* dark blue     */,
            14 /* orange        */,
            15 /* light blue    */,
            9  /* mellow yellow */,
            10 /* light green   */,
            12 /* fresh cream   */
        };

        UINT i;
        P_GR_LINESTYLE pt = &p_cache->twod_defaults[0];

        p_cache->init = TRUE;

        gr_colour_set_BLACK(p_cache->defaults.fg);
                            p_cache->defaults.fg.manual = 0;

        /* leave rest alone. thin solid lines ok */

        for(i = 0; i < elemof32(fill_with_wimp); ++i, ++pt)
        {
            PC_RGB p_rgb = &rgb_stash[fill_with_wimp[i]];
            pt->fg.red = p_rgb->r;
            pt->fg.green = p_rgb->g;
            pt->fg.blue = p_rgb->b;
            pt->fg.reserved = 0;
            pt->fg.manual = 0;
            pt->fg.visible = 1;

            pt->width = 8; /* pixits - not thin by default */
        }
    }

    assert(item >= 0);

    return(bodge_2d ? &p_cache->twod_defaults[(item + 1) % GR_LINESTYLE_2D_N_DEFAULTS] : &p_cache->defaults);
}

/*ncr*/
extern BOOL
gr_chart_objid_linestyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_LINESTYLE style)
{
    PC_GR_LINESTYLE pt;
    P_GR_AXIS axisp;
    BOOL using_default = 0;
    S32 linewidth_divisor = 1;

    PTR_ASSERT(cp);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
        pt = &cp->chart.borderstyle;
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        assert(id.no < GR_CHART_N_PLOTAREAS);
        pt = &cp->plotarea.area[(id.no < GR_CHART_N_PLOTAREAS) ? id.no : 0].borderstyle;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.borderstyle;
        break;

    case GR_CHART_OBJNAME_AXIS:
        pt = &gr_axisp_from_external(cp, id.no)->style.axis;
        break;

    case GR_CHART_OBJNAME_AXISGRID:
        axisp = gr_axisp_from_external(cp, id.no);

        switch(id.subno)
        {
        case GR_CHART_AXISTICK_MINOR:
            pt = &axisp->minor.style.grid;
            if(pt->fg.manual)
                break;

            using_default = 1;
            linewidth_divisor *= 2;

            /*FALLTHRU*/

        default:
        case GR_CHART_AXISTICK_MAJOR:
            pt = &axisp->major.style.grid;
            if(pt->fg.manual)
                break;

            using_default = 1;
            linewidth_divisor *= 16;

            pt = &axisp->style.axis;
            break;
        }
        break;

    case GR_CHART_OBJNAME_AXISTICK:
        axisp = gr_axisp_from_external(cp, id.no);

        switch(id.subno)
        {
        case GR_CHART_AXISTICK_MINOR:
            pt = &axisp->minor.style.tick;
            if(pt->fg.manual)
                break;

            using_default = 1;
            linewidth_divisor *= 2;

            /*FALLTHRU*/

        default:
        case GR_CHART_AXISTICK_MAJOR:
            pt = &axisp->major.style.tick;
            if(pt->fg.manual)
                break;

            using_default = 1;
            linewidth_divisor *= 8;

            pt = &axisp->style.axis;
            break;
        }
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_LEGDSERIES:
        pt = &gr_seriesp_from_external(cp, id.no)->style.point_line;
        break;

    case GR_CHART_OBJNAME_DROPSERIES:
        pt = &gr_seriesp_from_external(cp, id.no)->style.pdrop_line;
        break;

    case GR_CHART_OBJNAME_BESTFITSER:
        pt = &gr_seriesp_from_external(cp, id.no)->style.bestfit_line;
        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_LEGDPOINT:
        return(gr_point_linestyle_query(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style, 0));

    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_pdrop_linestyle_query(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    default:
        myassert1(TEXT("gr_chart_objid_linestyle_query of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        pt = gr_linestyle_default(&cp->linestyle_cache, 0, 0);
        break;
    }

    if(!pt->fg.manual)
    {
        pt = gr_linestyle_default(&cp->linestyle_cache, 0, 0);

        using_default = 1;
    }

    *style = *pt;

    if(linewidth_divisor != 1)
        style->width /= linewidth_divisor; /* principally for axis major/minor derivations */

    return(using_default);
}

_Check_return_
extern STATUS
gr_chart_objid_linestyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_LINESTYLE style)
{
    P_GR_LINESTYLE pt;
    P_GR_AXIS axisp;
    GR_SERIES_IDX series_idx;

    PTR_ASSERT(cp);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
        pt = &cp->chart.borderstyle;
        break;

    case GR_CHART_OBJNAME_PLOTAREA:
        if(id.no >= GR_CHART_N_PLOTAREAS)
            return(STATUS_OK);

        pt = &cp->plotarea.area[id.no].borderstyle;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.borderstyle;
        break;

    case GR_CHART_OBJNAME_AXIS:
        pt = &gr_axisp_from_external(cp, id.no)->style.axis;
        break;

    case GR_CHART_OBJNAME_AXISGRID:
        axisp = gr_axisp_from_external(cp, id.no);

        switch(id.subno)
        {
        case GR_CHART_AXISTICK_MINOR:
            pt = &axisp->minor.style.grid;
            break;

        case GR_CHART_AXISTICK_MAJOR:
            pt = &axisp->major.style.grid;
            break;

        default:
            return(STATUS_OK);
        }
        break;

    case GR_CHART_OBJNAME_AXISTICK:
        axisp = gr_axisp_from_external(cp, id.no);

        switch(id.subno)
        {
        case GR_CHART_AXISTICK_MINOR:
            pt = &axisp->minor.style.tick;
            break;

        case GR_CHART_AXISTICK_MAJOR:
            pt = &axisp->major.style.tick;
            break;

        default:
            return(STATUS_OK);
        }
        break;

    case GR_CHART_OBJNAME_SERIES:
        series_idx = gr_series_idx_from_external(cp, id.no);

        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_LINESTYLE);

        pt = &getserp(cp, series_idx)->style.point_line;
        break;

    case GR_CHART_OBJNAME_DROPSERIES:
        series_idx = gr_series_idx_from_external(cp, id.no);

        gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_LINESTYLE);

        pt = &getserp(cp, series_idx)->style.pdrop_line;
        break;

    case GR_CHART_OBJNAME_BESTFITSER:
        pt = &gr_seriesp_from_external(cp, id.no)->style.bestfit_line;
        break;

    case GR_CHART_OBJNAME_POINT:
        return(gr_point_linestyle_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_pdrop_linestyle_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    default:
        myassert1(TEXT("gr_chart_objid_linestyle_set of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        return(STATUS_FAIL);
    }

    *pt = *style;

    return(STATUS_OK);
}

/******************************************************************************
*
* query/set the text style associated with an object
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_chart_objid_textstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _OutRef_    P_GR_TEXTSTYLE style)
{
    PC_GR_TEXTSTYLE pt;
    BOOL using_default = 0;

    PTR_ASSERT(cp);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_PLOTAREA:
        /* return the base style for the chart */
        pt = &cp->text.style.base;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.textstyle;
        break;

    case GR_CHART_OBJNAME_AXIS:
    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        pt = &gr_axisp_from_external(cp, id.no)->style.label;
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_BESTFITSER:
    case GR_CHART_OBJNAME_LEGDSERIES:
        pt = &gr_seriesp_from_external(cp, id.no)->style.point_text;
        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
    case GR_CHART_OBJNAME_LEGDPOINT:
        return(gr_point_textstyle_query(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    case GR_CHART_OBJNAME_TEXT:
        pt = collect_goto_item(GR_TEXTSTYLE, gr_chart_list_get_p_list_block(cp, gr_style_common_blks[GR_LIST_CHART_TEXT_TEXTSTYLE]), id.no);
        break;

    default:
        myassert1(TEXT("gr_chart_objid_textstyle_query of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        pt = NULL;
        break;
    }

    if(!pt || !pt->fg.manual)
    {
        using_default = 1;
        pt = &cp->text.style.base;
    }

    *style = *pt;

    return(using_default);
}

_Check_return_
extern STATUS
gr_chart_objid_textstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_TEXTSTYLE style)
{
    P_GR_TEXTSTYLE pt;
    GR_SERIES_IDX series_idx;

    PTR_ASSERT(cp);
    PTR_ASSERT(style);

    switch(id.name)
    {
    case GR_CHART_OBJNAME_ANON:
    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_PLOTAREA:
        /* set the base style for the chart */
        pt = &cp->text.style.base;
        break;

    case GR_CHART_OBJNAME_LEGEND:
        pt = &cp->legend.textstyle;
        break;

    case GR_CHART_OBJNAME_AXIS:
    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        pt = &gr_axisp_from_external(cp, id.no)->style.label;
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_BESTFITSER:
        series_idx = gr_series_idx_from_external(cp, id.no);

        gr_point_list_delete(cp, series_idx, GR_LIST_POINT_TEXTSTYLE);

        pt = &getserp(cp, series_idx)->style.point_text;
        break;

    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPPOINT:
        return(gr_point_textstyle_set(cp, gr_series_idx_from_external(cp, id.no), gr_point_key_from_external(id.subno), style));

    case GR_CHART_OBJNAME_TEXT:
        {
        const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[GR_LIST_CHART_TEXT_TEXTSTYLE];
        return(common_list_set(gr_chart_list_get_p_list_block(cp, cbp), id.no, style, cbp->style_size));
        }

    default:
        myassert1(TEXT("gr_chart_objid_textstyle_set of %s"), report_tstr(gr_chart_object_name_from_id_quick(id)));
        return(STATUS_FAIL);
    }

    *pt = *style;

    return(STATUS_OK);
}

/******************************************************************************
*
* change/delete pictures in fillstylebs
*
******************************************************************************/

extern void
gr_fillstyleb_pict_change(
    /*inout*/ P_GR_FILLSTYLEB style,
    const GR_FILL_PATTERN_HANDLE * newpict)
{
    P_GR_CACHE_HANDLE p_gr_cache_handle = (P_GR_CACHE_HANDLE) &style->pattern;
    GR_CACHE_HANDLE new_cah = newpict ? ((GR_CACHE_HANDLE) *newpict) : GR_CACHE_HANDLE_NONE;
    gr_cache_reref(p_gr_cache_handle, new_cah);
}

extern void
gr_fillstyleb_pict_delete(
    /*inout*/ P_GR_FILLSTYLEB style)
{
    gr_fillstyleb_pict_change(style, NULL);
}

/******************************************************************************
*
* add/lose refs to pictures in fillstylebs
*
******************************************************************************/

extern void
gr_fillstyleb_ref_add(
    _InRef_     PC_GR_FILLSTYLEB style)
{
    gr_cache_ref((GR_CACHE_HANDLE) style->pattern, 1);
}

extern void
gr_fillstyleb_ref_lose(
    _InRef_     PC_GR_FILLSTYLEB style)
{
    gr_cache_ref((GR_CACHE_HANDLE) style->pattern, 0);
}

#define gr_point_list_get_p_list_block(serp, cbp) \
    PtrAddBytes(P_LIST_BLOCK, (serp), (cbp)->offset_of_lbr)

_Check_return_
static STATUS
gr_pdrop_and_point_common_fillstyleb_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     LIST_ITEMNO point,
    _InRef_opt_ PC_GR_FILLSTYLEB style,
    _InVal_     GR_LIST_ID list_id)
{
    /* see if style already exists */
    P_GR_FILLSTYLEB pt = gr_point_list_search(GR_FILLSTYLEB, cp, series_idx, point, list_id);

    if(pt)
    {
        if(NULL == style)
        {
            /* delete this style item */
            const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];

            /* lose the picture ref */
            gr_fillstyleb_ref_lose(pt);

            /* don't mangle following entry numbering */
            collect_subtract_entry(gr_point_list_get_p_list_block(getserp(cp, series_idx), cbp), point);

            pt = NULL;
        }
        else
        {
            /* change the picture ref */
            if(pt->pattern != style->pattern)
            {
                gr_fillstyleb_ref_lose(pt);
                *pt = *style;
                gr_fillstyleb_ref_add(pt);
            }
            else
                *pt = *style;
        }
    }
    else
    {
        if(style)
        {
            status_return(gr_point_list_set(cp, series_idx, point, style, list_id));

            pt = gr_point_list_search(GR_FILLSTYLEB, cp, series_idx, point, list_id);
            PTR_ASSERT(pt);

            /* add a picture ref */
            gr_fillstyleb_ref_add(pt);
        }
        /* else not present, who cares? */
    }

    if(pt)
    {
        /* check validity of style bits */
        if(pt->pattern != GR_FILL_PATTERN_NONE)
            pt->bits.pattern = 1;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* query/set the fill style associated with a data pdrop
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_pdrop_fillstyleb_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEB style)
{
    PC_GR_FILLSTYLEB pt = gr_point_list_search(GR_FILLSTYLEB, cp, series_idx, (LIST_ITEMNO) point, GR_LIST_PDROP_FILLSTYLEB);
    BOOL using_default = 0;

    if(!pt)
    {
        using_default = 1;

        pt = &getserp(cp, series_idx)->style.pdrop_fillb;

        if(!pt->bits.manual)
        {
            /* use point-derived style ... */
            consume_bool(gr_point_fillstyleb_query(cp, series_idx, point, style));

            /* ... but still treat as defaulting from here */
            return(using_default);
        }
    }

    *style = *pt;

    return(using_default);
}

/*ncr*/
extern BOOL
gr_pdrop_fillstylec_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEC style)
{
    PC_GR_FILLSTYLEC pt = gr_point_list_search(GR_FILLSTYLEC, cp, series_idx, (LIST_ITEMNO) point, GR_LIST_PDROP_FILLSTYLEC);
    BOOL using_default = 0;

    if(!pt)
    {
        using_default = 1;

        pt = &getserp(cp, series_idx)->style.pdrop_fillc;

        if(!pt->fg.manual)
        {
            /* use point-derived style ... */
            consume_bool(gr_point_fillstylec_query(cp, series_idx, point, style));

            /* ... but still treat as defaulting from here */
            return(using_default);
        }
    }

    *style = *pt;

    return(using_default);
}

_Check_return_
extern STATUS
gr_pdrop_fillstyleb_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEB style)
{
    return(gr_pdrop_and_point_common_fillstyleb_set(cp, series_idx, point, style, GR_LIST_PDROP_FILLSTYLEB));
}

_Check_return_
extern STATUS
gr_pdrop_fillstylec_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEC style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_PDROP_FILLSTYLEC));
}

/******************************************************************************
*
* query/set the line style associated with a data pdrop
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_pdrop_linestyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style)
{
    PC_GR_LINESTYLE pt = gr_point_list_search(GR_LINESTYLE, cp, series_idx, (LIST_ITEMNO) point, GR_LIST_PDROP_LINESTYLE);
    BOOL using_default = 0;

    if(!pt)
    {
        using_default = 1;

        pt = &getserp(cp, series_idx)->style.pdrop_line;

        if(!pt->fg.manual)
        {
            /* use point-derived style ... */
            consume_bool(gr_point_linestyle_query(cp, series_idx, point, style, 0));

            /* ... but still treat as defaulting from here */
            return(using_default);
        }
    }

    *style = *pt;

    return(using_default);
}

_Check_return_
extern STATUS
gr_pdrop_linestyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_LINESTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_PDROP_LINESTYLE));
}

/******************************************************************************
*
* point lists are rooted in series descriptors
*
******************************************************************************/

extern void
gr_point_list_delete(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_GR_SERIES serp = getserp(cp, series_idx);
    const P_LIST_BLOCK p_list_block = gr_point_list_get_p_list_block(serp, cbp);

    trace_3(TRACE_MODULE_GR_CHART, TEXT("gr_point_list_delete series_idx ") U32_TFMT TEXT(" %s list ") PTR_XTFMT, series_idx, cbp->list_name, p_list_block);

    /* remove refs before delete */
    if(cbp->rerefproc)
        (* cbp->rerefproc) (cp, series_idx, 0);

    collect_delete(p_list_block);
}

_Check_return_
extern STATUS
gr_point_list_duplic(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_GR_SERIES serp = getserp(cp, series_idx);
    const P_LIST_BLOCK p_list_block = gr_point_list_get_p_list_block(serp, cbp);
    LIST_BLOCK lb;
    STATUS status;

    trace_3(TRACE_MODULE_GR_CHART, TEXT("gr_point_list_duplic series_idx ") U32_TFMT TEXT(" %s list ") PTR_XTFMT, series_idx, cbp->list_name, p_list_block);

    zero_struct(lb);

    status = collect_copy(&lb, p_list_block);

    *p_list_block = lb;

    /* add refs after successful duplic */
    if(status_ok(status))
        if(cbp->rerefproc)
            (* cbp->rerefproc) (cp, series_idx, 1);

    return(status);
}

_Check_return_
extern STATUS
gr_point_list_fillstyleb_enum_for_save(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_LIST_ID list_id,
    P_ANY client_handle)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_GR_SERIES serp = getserp(cp, series_idx);
    const P_LIST_BLOCK p_list_block = gr_point_list_get_p_list_block(serp, cbp);
    LIST_ITEMNO key;
    P_GR_FILLSTYLEB pt;

    trace_3(TRACE_MODULE_GR_CHART, TEXT("gr_point_list_fillstyleb_enum_for_save series_idx ") U32_TFMT TEXT(" %s list ") PTR_XTFMT, series_idx, cbp->list_name, p_list_block);

    for(pt = collect_first(GR_FILLSTYLEB, p_list_block, &key);
        pt;
        pt = collect_next(GR_FILLSTYLEB, p_list_block, &key))
    {
        STATUS status;

        if((status = gr_fillstyleb_make_key_for_save(pt, client_handle)) < 0) /* 0 is valid, no pict */
            return(status);
    }

    return(STATUS_DONE);
}

extern void
gr_pdrop_list_fillstyleb_reref(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     BOOL add)
{
    PTR_ASSERT(cp);
    common_list_fillstyleb_reref(&getserp(cp, series_idx)->lbr.pdrop_fillb, add);
}

extern void
gr_point_list_fillstyleb_reref(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     BOOL add)
{
    PTR_ASSERT(cp);
    common_list_fillstyleb_reref(&getserp(cp, series_idx)->lbr.point_fillb, add);
}

_Check_return_
_Ret_maybenull_
extern P_ANY
_gr_point_list_first(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _OutRef_    P_LIST_ITEMNO p_key,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_GR_SERIES serp = getserp(cp, series_idx);
    const P_LIST_BLOCK p_list_block = gr_point_list_get_p_list_block(serp, cbp);
    return(_collect_first(p_list_block, p_key CODE_ANALYSIS_ONLY_ARG(cbp->style_size)));
}

_Check_return_
_Ret_maybenull_
extern P_ANY
_gr_point_list_next(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InoutRef_  P_LIST_ITEMNO p_key,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_GR_SERIES serp = getserp(cp, series_idx);
    const P_LIST_BLOCK p_list_block = gr_point_list_get_p_list_block(serp, cbp);
    return(_collect_next(p_list_block, p_key CODE_ANALYSIS_ONLY_ARG(cbp->style_size)));
}

_Check_return_
_Ret_maybenull_
static P_BYTE
_gr_point_list_search(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_GR_SERIES serp = getserp(cp, series_idx);
    const P_LIST_BLOCK p_list_block = gr_point_list_get_p_list_block(serp, cbp);
    return(collect_goto_item(BYTE, p_list_block, (LIST_ITEMNO) point));
}

_Check_return_
static STATUS
gr_point_list_set(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    /*_In_(~list_id)*/ PC_ANY style,
    _InVal_     GR_LIST_ID list_id)
{
    const PC_GR_STYLE_COMMON_BLK cbp = gr_style_common_blks[list_id];
    const P_GR_SERIES serp = getserp(cp, series_idx);
    const P_LIST_BLOCK p_list_block = gr_point_list_get_p_list_block(serp, cbp);
    return(common_list_set(p_list_block, (LIST_ITEMNO) point, style, cbp->style_size));
}

/******************************************************************************
*
* query/set the fill style associated with a data point
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_point_fillstyleb_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEB style)
{
    const PC_GR_FILLSTYLEB pt = gr_point_list_search(GR_FILLSTYLEB, cp, series_idx, (LIST_ITEMNO) point, GR_LIST_POINT_FILLSTYLEB);
    BOOL using_default = 0;
    BOOL vary_by_point = 0;

    if(!pt)
    {
        const P_GR_SERIES serp = getserp(cp, series_idx);

        vary_by_point = serp->bits.vary_by_point_manual
                      ? serp->bits.vary_by_point
                      : gr_axesp_from_series_idx(cp, series_idx)->bits.vary_by_point;

        using_default = 1;

        /* use series-derived style */
        *style = serp->style.point_fillb;

        style->bits.varied_by_point = vary_by_point;
    }
    else
        *style = *pt;

    return(using_default);
}

/******************************************************************************
*
* query/set the fill style associated with a data point
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_point_fillstylec_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_FILLSTYLEC style)
{
    PC_GR_FILLSTYLEC pt = gr_point_list_search(GR_FILLSTYLEC, cp, series_idx, (LIST_ITEMNO) point, GR_LIST_POINT_FILLSTYLEC);
    BOOL using_default = 0;
    BOOL vary_by_point = 0;

    if(!pt)
    {
        const P_GR_SERIES serp = getserp(cp, series_idx);

        vary_by_point = serp->bits.vary_by_point_manual
                      ? serp->bits.vary_by_point
                      : gr_axesp_from_series_idx(cp, series_idx)->bits.vary_by_point;

        using_default = 1;

        /* use series-derived style unless varying by point */
        if(vary_by_point)
            pt = gr_fillstylec_default(gr_point_external_from_key(point));
        else
        {
            pt = &serp->style.point_fillc;

            if(!pt->fg.manual)
                pt = gr_fillstylec_default(gr_series_external_from_idx(cp, series_idx));
        }
    }

    *style = *pt;

    return(using_default);
}

_Check_return_
extern STATUS
gr_point_fillstyleb_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEB style)
{
    return(gr_pdrop_and_point_common_fillstyleb_set(cp, series_idx, point, style, GR_LIST_POINT_FILLSTYLEB));
}

_Check_return_
extern STATUS
gr_point_fillstylec_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_FILLSTYLEC style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_FILLSTYLEC));
}

/******************************************************************************
*
* query/set the line style associated with a data point
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_point_linestyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_LINESTYLE style,
    _InVal_     BOOL bodge_2d)
{
    PC_GR_LINESTYLE pt = gr_point_list_search(GR_LINESTYLE, cp, series_idx, (LIST_ITEMNO) point, GR_LIST_POINT_LINESTYLE);
    BOOL using_default = 0;

    if(!pt)
    {
        using_default = 1;

        /* use series-derived style */
        pt = &getserp(cp, series_idx)->style.point_line;

        if(!pt->fg.manual)
            pt = gr_linestyle_default(&cp->linestyle_cache, bodge_2d ? series_idx : 0, bodge_2d); /* SKS after 1.20/50 make different series different colours when using 2-D line chart */
    }

    *style = *pt;

    return(using_default);
}

_Check_return_
extern STATUS
gr_point_linestyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_LINESTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_LINESTYLE));
}

/******************************************************************************
*
* query/set the text style associated with a data point
*
******************************************************************************/

/*ncr*/
extern BOOL
gr_point_textstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_TEXTSTYLE style)
{
    PC_GR_TEXTSTYLE pt = gr_point_list_search(GR_TEXTSTYLE, cp, series_idx, (LIST_ITEMNO) point, GR_LIST_POINT_TEXTSTYLE);
    BOOL using_default = 0;

    if(!pt)
    {
        using_default = 1;

        /* use series-derived style */
        pt = &getserp(cp, series_idx)->style.point_text;

        if(!pt->fg.manual)
            pt = &cp->text.style.base;
    }

    *style = *pt;

    return(using_default);
}

_Check_return_
extern STATUS
gr_point_textstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_TEXTSTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_TEXTSTYLE));
}

/******************************************************************************
*
* query chart style for point
*
******************************************************************************/

_Check_return_
static BOOL
gr_point_chartstyle_query(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    /*out*/ P_ANY style,
    _InRef_     PC_GR_CHART_OBJID_CHARTSTYLE_DESC desc)
{
    PC_BYTE bpt = gr_point_list_search(BYTE, cp, series_idx, (LIST_ITEMNO) point, desc->list_id);
    BOOL using_default = 0;

    PTR_ASSERT(style);

    if(NULL == bpt)
    {
        using_default = 1;

        /* use series style */
        bpt = PtrAddBytes(P_BYTE, getserp(cp, series_idx), desc->series_offset);

        if((*bpt & 1) == 0 /*!pt->bits.manual*/)
        {
            /* use axes style */
            GR_AXES_IDX axes_idx = gr_axes_idx_from_series_idx(cp, series_idx);

            bpt = PtrAddBytes(P_BYTE, &cp->axes[axes_idx], desc->axes_offset);
        }
    }

    memcpy32(style, bpt, desc->style_size);

    return(using_default);
}

/*ncr*/
extern BOOL
gr_point_barchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_BARCHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, series_idx, point, style, &gr_chart_objid_barchstyle_desc));
}

_Check_return_
extern STATUS
gr_point_barchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_BARCHSTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_BARCHSTYLE));
}

/*ncr*/
extern BOOL
gr_point_barlinechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_BARLINECHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, series_idx, point, style, &gr_chart_objid_barlinechstyle_desc));
}

_Check_return_
extern STATUS
gr_point_barlinechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_BARLINECHSTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_BARLINECHSTYLE));
}

/*ncr*/
extern BOOL
gr_point_linechstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_LINECHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, series_idx, point, style, &gr_chart_objid_linechstyle_desc));
}

_Check_return_
extern STATUS
gr_point_linechstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_LINECHSTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_LINECHSTYLE));
}

/*ncr*/
extern BOOL
gr_point_piechdisplstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_PIECHDISPLSTYLE style)
{
    return(gr_point_chartstyle_query(cp, series_idx, point, style, &gr_chart_objid_piechdisplstyle_desc));
}

_Check_return_
extern STATUS
gr_point_piechdisplstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_PIECHDISPLSTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_PIECHDISPLSTYLE));
}

/*ncr*/
extern BOOL
gr_point_piechlabelstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_PIECHLABELSTYLE style)
{
    return(gr_point_chartstyle_query(cp, series_idx, point, style, &gr_chart_objid_piechlabelstyle_desc));
}

_Check_return_
extern STATUS
gr_point_piechlabelstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_PIECHLABELSTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_PIECHLABELSTYLE));
}

/*ncr*/
extern BOOL
gr_point_scatchstyle_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _OutRef_    P_GR_SCATCHSTYLE style)
{
    return(gr_point_chartstyle_query(cp, series_idx, point, style, &gr_chart_objid_scatchstyle_desc));
}

_Check_return_
extern STATUS
gr_point_scatchstyle_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InRef_     PC_GR_SCATCHSTYLE style)
{
    return(gr_point_list_set(cp, series_idx, (LIST_ITEMNO) point, style, GR_LIST_POINT_SCATCHSTYLE));
}

/******************************************************************************
*
* convert a series index into an external series number
*
******************************************************************************/

_Check_return_
extern GR_ESERIES_NO
gr_series_external_from_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    GR_ESERIES_NO gr_series_no = (GR_ESERIES_NO) (series_idx + 1);

    if(cp->axes_idx_max != 0)
    {
        if(series_idx >= cp->axes[1].series.stt_idx)
        {
            /* paper over the crack */
            gr_series_no -= (cp->axes[1].series.stt_idx - cp->axes[0].series.end_idx);
        }
    }

    return(gr_series_no);
}

/******************************************************************************
*
* convert an external series number into a series index
*
******************************************************************************/

_Check_return_
extern GR_SERIES_IDX
gr_series_idx_from_external(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_ESERIES_NO eseries_no)
{
    GR_SERIES_IDX series_idx = (GR_SERIES_IDX) (eseries_no - 1);

    assert(eseries_no != 0);

    if(series_idx < cp->axes[0].series.end_idx)
        return(series_idx);

    if(cp->axes_idx_max != 0)
    {
        /* paper over the crack */
        series_idx += (cp->axes[1].series.stt_idx - cp->axes[0].series.end_idx);
    }

    return(series_idx);
}

_Check_return_
extern P_GR_SERIES
gr_seriesp_from_external(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_ESERIES_NO eseries_no)
{
    const GR_SERIES_IDX series_idx = gr_series_idx_from_external(cp, eseries_no);

    return(getserp(cp, series_idx));
}

/* end of gr_scale.c */
