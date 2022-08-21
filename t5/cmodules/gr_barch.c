/* gr_barch.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Bar and line charts */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#include "ob_ss/xp_ss.h"

#include "ob_ss/ss_linest.h"

/******************************************************************************
*
* '3-D' mappings
*
******************************************************************************/

/* depends on cp->d3, plotarea sizes */

extern void
gr_map_point(
    _InoutRef_  P_GR_POINT xpoint,
    _InRef_     PC_GR_CHART cp,
    _In_        S32 plotindex)
{
    /* note that plotarea is at the back and right when d3.bits.use */
    assert(cp->d3.valid.vector);

    /* so come forwards from the back: plotindex in [0..(cp->cache.n_contrib_series - 1)] */
    /* 0                                --> cp->cache.n_contrib_series */
    /* (cp->cache.n_contrib_series - 1) --> 1 */
    plotindex = cp->cache.n_contrib_series - plotindex;

    xpoint->x = xpoint->x + (GR_COORD) (cp->d3.cache.vector.x /*-ve*/ * plotindex);
    xpoint->y = xpoint->y + (GR_COORD) (cp->d3.cache.vector.y /*-ve*/ * plotindex);
}

extern void
gr_map_point_front_or_back(
    _InoutRef_  P_GR_POINT xpoint,
    _InRef_     PC_GR_CHART cp,
    _InVal_     BOOL front)
{
    /* note that plotarea is at the back and right when d3.bits.use */
    assert(cp->d3.valid.vector);

    if(!front)
    {   /* already at the back */
        return;
    }

    xpoint->x = xpoint->x + cp->d3.cache.vector_full.x /*-ve*/;
    xpoint->y = xpoint->y + cp->d3.cache.vector_full.y /*-ve*/;
}

extern void
gr_map_box_front_and_back(
    _InoutRef_  P_GR_BOX xbox,
    _ChartRef_  P_GR_CHART cp)
{
    /* SKS after PD 4.12 30mar92 - for consistency and code size do this here not in callers */
    xbox->x1 = xbox->x0;
    xbox->y1 = xbox->y0;

    xbox->x0 = xbox->x0 + cp->d3.cache.vector_full.x;
    xbox->y0 = xbox->y0 + cp->d3.cache.vector_full.y;

#if 0
    /* no need to map this one as its already there under current mapping scheme */
    gr_map_point((P_GR_POINT) &xbox->x1, cp, cp->cache.n_contrib_series);
#endif
}

extern void
gr_point_partial_z_shift(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_opt_ PC_GR_POINT apoint,
    _ChartRef_  P_GR_CHART cp,
    _InRef_     PC_F64 z_frac_p)
{
    F64 z_frac = *z_frac_p; /* normally 0.0 <= z <= 1.0 */

    assert(cp->d3.valid.vector);

    xpoint->x = (apoint ? apoint->x : 0) - (GR_PIXIT) (cp->d3.cache.vector.x /*-ve*/ * z_frac);
    xpoint->y = (apoint ? apoint->y : 0) - (GR_PIXIT) (cp->d3.cache.vector.y /*-ve*/ * z_frac);
}

/******************************************************************************
*
* work out actual limits for given series in their current type
*
******************************************************************************/

static S32
gr_actualise_series_plain(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    GR_CHART_ITEMNO total_n_items = 0;
    const P_GR_SERIES serp = getserp(cp, series_idx);

#if 0
    if(1 /*!serp->valid.limits*/)
#endif
    {
        GR_CHART_ITEMNO n_items, item;
        GR_DATASOURCE_FOURSOME dsh;
        F64 value, valsum;
        PC_GR_AXES p_gr_axes = gr_axesp_from_series_idx(cp, series_idx);
        PC_GR_AXIS p_gr_axis = &p_gr_axes->axis[Y_AXIS_IDX];
        BOOL cumulative = serp->bits.cumulative_manual ? serp->bits.cumulative : p_gr_axes->bits.cumulative;

        serp->cache.limits_y.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_y.max = -GR_CHART_NUMBER_MAX;

        gr_get_datasources(cp, series_idx, &dsh, FALSE);

        total_n_items = 0;

        n_items = gr_travel_dsh_n_items(cp, dsh.value_x); /* categ. */

        total_n_items = MAX(total_n_items, n_items);

        n_items = gr_travel_dsh_n_items(cp, dsh.value_y);

        total_n_items = MAX(total_n_items, n_items);

        valsum = 0.0;

        for(item = 0; item < n_items; ++item)
        {
            if(!status_done(gr_travel_dsh_valof(cp, dsh.value_y, item, &value)))
                continue;

            if(value <= 0.0)
            {
                if(p_gr_axis->bits.log_scale)
                    continue;

                if(value < 0.0)
                    if(/*cumulative || SKS after 1.05 18oct93 */ p_gr_axes->bits.stacked)
                        continue;
            }

            if(cumulative)
            {
                valsum += value;
                value = valsum;
            }

            serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, value);
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, value);
        }

        serp->valid.limits = 1;
    }

    return(total_n_items);
}

static S32
gr_actualise_series_error1(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    GR_CHART_ITEMNO total_n_items = 0;
    const P_GR_SERIES serp = getserp(cp, series_idx);

#if 0
    if(1 /*!serp->valid.limits*/)
#endif
    {
        GR_CHART_ITEMNO n_items, item;
        GR_DATASOURCE_FOURSOME dsh;
        F64 value, valsum;
        F64 error, errsum, valincerr;
        PC_GR_AXES p_gr_axes = gr_axesp_from_series_idx(cp, series_idx);
        PC_GR_AXIS p_gr_axis = &p_gr_axes->axis[Y_AXIS_IDX];
        BOOL cumulative = serp->bits.cumulative_manual ? serp->bits.cumulative : p_gr_axes->bits.cumulative;

        serp->cache.limits_y.min = +GR_CHART_NUMBER_MAX;
        serp->cache.limits_y.max = -GR_CHART_NUMBER_MAX;

        gr_get_datasources(cp, series_idx, &dsh, FALSE);

        total_n_items = 0;

        n_items = gr_travel_dsh_n_items(cp, dsh.value_x); /* categ. */

        total_n_items = MAX(total_n_items, n_items);

        n_items = gr_travel_dsh_n_items(cp, dsh.value_y);

        total_n_items = MAX(total_n_items, n_items);

        valsum = errsum = 0.0;

        for(item = 0; item < n_items; ++item)
        {
            if(!status_done(gr_travel_dsh_valof(cp, dsh.value_y, item, &value)))
                continue;

            if(value <= 0.0)
            {
                if(p_gr_axis->bits.log_scale)
                    continue;

                if(value < 0.0)
                    if(/*cumulative || SKS after 1.05 18oct93 */ p_gr_axes->bits.stacked)
                        continue;
            }

            if(cumulative)
            {
                valsum += value;
                value = valsum;
            }

            serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, value);
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, value);

            if(status_done(gr_travel_dsh_valof(cp, dsh.error_y, item, &error)))
            {
                error = fabs(error);

                /* SKS after PD 4.12 30mar92 - correct scaling with errors */
                if(cumulative)
                    error += errsum;

                errsum = error;
            }
            else
                error = errsum;

            if(value < 0.0)
                /* error bars will hang off the bottom;
                 * cumulative forces value +ve unless -ve start
                 * and log scaling has rejected -ve numbers
                */
                valincerr = value - error;
            else
                valincerr = value + error;

            serp->cache.limits_y.min = MIN(serp->cache.limits_y.min, valincerr);
            serp->cache.limits_y.max = MAX(serp->cache.limits_y.max, valincerr);
        }

        serp->valid.limits = 1;
    }

    return(total_n_items);
}

/* determine relative width of bars from number of series as bars being plotted on ALL axes
 * this allows charts like Cars to be plotted sensibly, i.e. 2 bars on one axes set and
 * another differently scaled bar on another axes set. obviously it is only sensible
 * to perform percentage bars on each axes set individually!
*/

_Check_return_
extern GR_PIXIT
gr_categ_pos(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_POINT_NO point)
{
    GR_PIXIT pos;

    pos = cp->barlinech.cache.cat_group_width * point;

    return(pos);
}

_Check_return_
extern GR_PIXIT
gr_value_pos(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx,
    _InRef_     PC_F64 value)
{
    PC_GR_AXIS p_gr_axis = &cp->axes[axes_idx].axis[axis_idx];
    F64 plotval = *value;
    GR_PIXIT pos = (axis_idx == Y_AXIS_IDX) ? cp->plotarea.size.y : cp->plotarea.size.x;
    F64 plotmin;
    F64 plotmax;

    plotmin = p_gr_axis->current.min;
    if( plotval < plotmin)
        plotval = plotmin;

    /* SKS 15jan97 clip to top of plot as well */
    plotmax = p_gr_axis->current.max;
    if( plotval > plotmax)
        plotval = plotmax;

    if(p_gr_axis->bits.log_scale)
    {
        plotval = log10(plotval);
        plotmin = log10(plotmin);
    }

    plotval = plotval - plotmin;

    plotval = plotval / p_gr_axis->current_span;
    pos = (GR_PIXIT) (plotval * pos);

    return(pos);
}

_Check_return_
extern GR_PIXIT
gr_value_pos_rel(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx,
    _InRef_     PC_F64 value)
{
    PC_GR_AXIS p_gr_axis = &cp->axes[axes_idx].axis[axis_idx];
    F64 plotval = *value;
    GR_PIXIT pos = (axis_idx == Y_AXIS_IDX) ? cp->plotarea.size.y : cp->plotarea.size.x;

    plotval = plotval / p_gr_axis->current_span;
    pos = (GR_PIXIT) (plotval * pos);

    return(pos);
}

/******************************************************************************
*
* bars and lines
*
******************************************************************************/

_Check_return_
static STATUS
gr_barline_label_point(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_POINT_NO point,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_F64 value,
    _InRef_     PC_GR_BOX txt_boxp)
{
    STATUS status = STATUS_OK;
    GR_BOX box = *txt_boxp;
    GR_TEXTSTYLE textstyle;
    GR_CHART_VALUE cv;
    GR_MILLIPOINT swidth_mp;
    GR_PIXIT swidth_px, halfwidth, halfheight;
    NUMFORM_PARMS numform_parms;
    EV_DATA ev_data;
    QUICK_UBLOCK quick_ublock;
    quick_ublock_setup(&quick_ublock, cv.data.text);

    cv.data.text[0] = CH_NULL;

    ev_data_set_real(&ev_data, *value);

    zero_struct(numform_parms);
    if(fabs(ev_data.arg.fp) >= U32_MAX)
        numform_parms.ustr_numform_numeric = USTR_TEXT("0.00e+00");
    else if(fabs(ev_data.arg.fp) < 1.0)
        numform_parms.ustr_numform_numeric = USTR_TEXT("0.##");
    else
        numform_parms.ustr_numform_numeric =
            (floor(ev_data.arg.fp) == ev_data.arg.fp)
                ? USTR_TEXT("#,##0")
                : USTR_TEXT("#,##0.00");

    status = numform(&quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms);

    quick_ublock_dispose_leaving_buffer_valid(&quick_ublock);

    status_return(status);

    gr_point_textstyle_query(cp, series_idx, point, &textstyle);

    {
    HOST_FONT host_font = gr_riscdiag_host_font_from_textstyle(&textstyle);
    swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(cv.data.text));
    gr_riscdiag_host_font_dispose(&host_font);
    } /*block*/

    swidth_px = swidth_mp  / GR_MILLIPOINTS_PER_PIXIT;

    if(strlen(cv.data.text))
    {
        halfwidth = swidth_px / 2;
        halfheight = textstyle.size_y / 2;

        box.x1 = box.x0 + halfwidth;
        box.y1 = box.y0 + halfheight;
        box.x0 = box.x0 - halfwidth;
        box.y0 = box.y0 - halfheight;

        status = gr_chart_text_new(cp, id, &box, ustr_bptr(cv.data.text), &textstyle);
    }

    return(status);
}

/******************************************************************************
*
* stacked series handling for bar and line charts
*
* here lies a cache of data that can be
* accessed by points in subsequent series
*
******************************************************************************/

static
struct GR_BARLINECH_STACKING
{
    BOOL on;
    F64 value;
    F64 error;
    GR_PIXIT valpoint;
    F64 total;
    BOOL drop_filled;
}
stacking;

_Check_return_
static STATUS
gr_barlinech_stacking_init(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_POINT_NO point)
{
    GR_SERIES_IDX series_idx;
    /*const*/ P_GR_SERIES serp;
    F64 value;
    STATUS res;

    stacking.on = 1;
    stacking.value = 0.0;
    stacking.error = 0.0;
    stacking.valpoint = cp->barlinech.cache.zeropoint_y;
    stacking.total = 0.0;
    stacking.drop_filled = 0; /* 1 would mean no need to consider drawing bottom vanes, but leave 0 for possible -ve pitch */

    if(!cp->axes[axes_idx].bits.stacked_pct)
        return(1);

    res = 0;

    for(series_idx = cp->axes[axes_idx].series.stt_idx; series_idx < cp->axes[axes_idx].series.end_idx; ++series_idx)
    {
        serp = getserp(cp, series_idx);

        if(!status_done(gr_travel_dsh_valof(cp, serp->datasources.dsh[0], point, &value)))
            continue;

        if(value < 0.0)
            continue;

        stacking.total += value;

        /* something to plot in this category */
        res = 1;
    }

    if(stacking.total)
        stacking.total /= 100.0; /* so value / total is in 0 .. 100 */
    else
    {
        /* nothing plottable as %ge in this category */
        gr_chart_warning(cp, CHART_ERR_NEGATIVE_OR_ZERO_IGNORED_STACK);
        return(0);
    }

    return(res);
}

enum GR_BARLINECH_PASS_TYPE
{
    GR_BARLINECH_RIBBON_PASS,
    GR_BARLINECH_POINT_PASS
};

/******************************************************************************
*
* initialise parts of barlinech cache on a per pass basis
*
******************************************************************************/

static void
gr_barlinech_pass_init(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp)
{
    /* cumulative series cache */
    lcp->old_value.y = 0.0;
    lcp->old_error.y = 0.0;

    /* ribbon-can-be-drawn-back_to cache */
    lcp->had_first = 0;
}

/******************************************************************************
*
* initialise barlinech cache on a per series basis
*
******************************************************************************/

static void
gr_barlinech_series_init(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp)
{
    const P_GR_SERIES serp = getserp(lcp->cp, lcp->series_idx);
    const P_GR_AXES p_gr_axes = gr_axesp_from_series_idx(lcp->cp, lcp->series_idx);

    lcp->cumulative = serp->bits.cumulative_manual ? serp->bits.cumulative : p_gr_axes->bits.cumulative;
    lcp->fill_to_axis = serp->bits.fill_to_axis_manual ? serp->bits.fill_to_axis : p_gr_axes->bits.fill_to_axis;
    lcp->bl_best_fit = serp->bits.best_fit_manual ? serp->bits.best_fit : p_gr_axes->bits.best_fit;

    gr_chart_objid_from_series_idx(lcp->cp, lcp->series_idx, &lcp->serid);

    lcp->drop_serid = lcp->serid;
    lcp->drop_serid.name = GR_CHART_OBJNAME_DROPSERIES;

    lcp->point_id = lcp->serid;
    lcp->point_id.name = GR_CHART_OBJNAME_POINT;
    lcp->point_id.has_subno = 1;

    lcp->pdrop_id = lcp->point_id;
    lcp->pdrop_id.name = GR_CHART_OBJNAME_DROPPOINT;

    lcp->slot_depth_percentage = 0;

    gr_get_datasources(lcp->cp, lcp->series_idx, &lcp->dsh, FALSE);
}

/******************************************************************************
*
* bars and lines start points similarly enough
*
******************************************************************************/

_Check_return_
static STATUS
gr_barlinech_point_start(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp,
    _InVal_     GR_POINT_NO point,
    /*out*/ P_GR_CHART_NUMPAIR value,
    /*out*/ P_GR_CHART_NUMPAIR error)
{
    const P_GR_CHART cp = lcp->cp;
    const GR_AXES_IDX axes_idx = lcp->axes_idx;
    const P_GR_AXES p_gr_axes = &cp->axes[axes_idx];

    lcp->point_id.subno = UBF_PACK(gr_point_external_from_key(point));

    lcp->pdrop_id.subno = lcp->point_id.subno;

    if(!status_done(gr_travel_dsh_valof(cp, lcp->dsh.value_y, point, &value->y)))
    {
        /* skip this point and start afresh with the line */
        lcp->had_first = FALSE;
        return(0);
    }

    if(lcp->cumulative)
    {
#if 0 /* SKS after 1.05 18oct93 */
        if(value->y <= 0.0)
            value->y = lcp->old_value.y;
        else
#endif
        {
            value->y += lcp->old_value.y;
            lcp->old_value.y = value->y;
        }
    }

    if(lcp->dsh.error_y != GR_DATASOURCE_HANDLE_NONE)
    {
        if(status_done(gr_travel_dsh_valof(cp, lcp->dsh.error_y, point, &error->y)))
        {
            error->y = fabs(error->y);

            if(lcp->cumulative)
                error->y += lcp->old_error.y;

            lcp->old_error.y = error->y;
        }
        else
            /* SKS 22jan92 says blanks in error record maintain previous error values from that series */
            error->y = lcp->old_error.y;
    }
    else
        error->y = 0.0;

    /* NB. do value & error cumulation before deciding to wimp out */

    /* can't plot -ve or zero values on log scaled chart */
    if(p_gr_axes->axis[Y_AXIS_IDX].bits.log_scale)
        if(value->y <= 0.0)
        {
            /* SKS after PD 4.12 30mar92 - skip this point and start afresh with the line */
            lcp->had_first = FALSE;
            return(0);
        }

    if(stacking.on)
    {
        /* don't plot -ve values on stacked plot */
        if(value->y < 0.0)
        {
            /* SKS after PD 4.12 30mar92 - skip this point and start afresh with the line */
            lcp->had_first = FALSE;
            return(0);
        }

        /* adjust for possible 100% plot */
        if(stacking.total)
            value->y /= stacking.total;

        /* add in previous series' value and error */
        value->y += stacking.value;
        error->y += stacking.error;

        stacking.value = value->y;
        stacking.error = error->y;
    }

    return(1);
}

static void
shade_fillstylec(
    _OutRef_    P_GR_FILLSTYLEC p_gr_fillstylec,
    _InRef_     PC_GR_FILLSTYLEC p_gr_fillstylec_in,
    _InVal_     int multiplier,
    _InVal_     int divisor)
{
    *p_gr_fillstylec = *p_gr_fillstylec_in;

    p_gr_fillstylec->fg.red   = p_gr_fillstylec_in->fg.red   * multiplier / divisor;
    p_gr_fillstylec->fg.green = p_gr_fillstylec_in->fg.green * multiplier / divisor;
    p_gr_fillstylec->fg.blue  = p_gr_fillstylec_in->fg.blue  * multiplier / divisor;
}

_Check_return_
static STATUS
gr_barchart_point_addin(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp,
    _InVal_     GR_CHART_ITEMNO point)
{
    STATUS status = STATUS_OK;
    const P_GR_CHART cp = lcp->cp;
    const GR_AXES_IDX axes_idx = lcp->axes_idx;
    const P_GR_AXES p_gr_axes = &cp->axes[axes_idx];
    const GR_SERIES_IDX series_idx = lcp->series_idx;
    const P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_CHART_NUMPAIR value, error;
    GR_POINT valpoint, botpoint, toppoint;
    GR_POINT zsv;
    GR_BARCHSTYLE barchstyle;
    GR_BARLINECHSTYLE barlinechstyle;
    GR_FILLSTYLEB point_fillstyleb;
    GR_FILLSTYLEC point_fillstylec;
    GR_LINESTYLE point_linestyle;
    GR_DIAG_OFFSET point_group_start;
    GR_BOX pre_z_shift_box;
    GR_BOX err_box = { 0, 0, 0, 0 }; /* Keep dataflower happy */
    GR_PIXIT tbar_halfsize;
    STATUS labelling;

    GR_BOX rect_box;
    GR_PIXIT bar_width;

    if(!gr_barlinech_point_start(lcp, point, &value, &error))
        return(status);

    status_return(gr_chart_group_new(cp, &point_group_start, lcp->point_id));

    gr_point_fillstyleb_query(cp, series_idx, point, &point_fillstyleb);
    gr_point_fillstylec_query(cp, series_idx, point, &point_fillstylec);
    point_fillstyleb.bits.is_marker = 0;
    gr_point_linestyle_query(cp, series_idx, point, &point_linestyle, 0);

    gr_point_barchstyle_query(    cp, series_idx, point, &barchstyle);
    gr_point_barlinechstyle_query(cp, series_idx, point, &barlinechstyle);

    bar_width = (GR_PIXIT) ((double) cp->barch.cache.slot_width * barchstyle.slot_width_percentage / 100.0);

    labelling = 0; /* or +1 */

    valpoint.y = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &value.y);

    /* create assuming simple origin - 3-D transformed using plotarea later */

    /* place in right category group */
    valpoint.x = (int) gr_categ_pos(cp, point);

    /* shift slot along group according to overlap (hi overlap -> low shift) */
    valpoint.x += (int) (cp->barch.cache.slot_shift * lcp->barindex);

    /* centre bar in its slot */
    valpoint.x += (int) ((cp->barch.cache.slot_width - bar_width) / 2);

    botpoint.x = valpoint.x;
    toppoint.x = valpoint.x + bar_width; /* diagonally across from botpoint */

    /* normally plot up from zero line (or side, or previous
     * stacked valpoint; won't be stacked if -ve) to point
    */
    botpoint.y = stacking.on ? stacking.valpoint : cp->barlinech.cache.zeropoint_y;
    toppoint.y = valpoint.y;

    if(stacking.on)
        stacking.valpoint = valpoint.y;

    if(value.y < 0.0)
    {
        /* plot down from zero line (or side; can't be stacked if -ve!) to point */
        GR_PIXIT tmppoint_y;

        tmppoint_y = botpoint.y; /* swap */
        botpoint.y = toppoint.y;
        toppoint.y = tmppoint_y;

        labelling = (0 - labelling);
    }

    /* map together into 3-D world */
    if(cp->d3.bits.use)
    {
        gr_map_point(&valpoint, cp, lcp->plotindex);
        gr_map_point(&botpoint, cp, lcp->plotindex);
        gr_map_point(&toppoint, cp, lcp->plotindex);
    }

    /* map into absolute plot area */
    valpoint.x += cp->plotarea.posn.x;
    valpoint.y += cp->plotarea.posn.y;
    botpoint.x += cp->plotarea.posn.x;
    botpoint.y += cp->plotarea.posn.y;
    toppoint.x += cp->plotarea.posn.x;
    toppoint.y += cp->plotarea.posn.y;

    /* note where the box ended up for error bar porpoises */
    pre_z_shift_box.x0 = botpoint.x;
    pre_z_shift_box.y0 = botpoint.y;
    pre_z_shift_box.x1 = toppoint.x;
    pre_z_shift_box.y1 = toppoint.y;

    tbar_halfsize = (pre_z_shift_box.x1 - pre_z_shift_box.x0) / 2; /* no picts for points */
    tbar_halfsize = tbar_halfsize / 2;

    if(error.y || (labelling != 0))
    {
        /* plot little T centred on bottom or top and/or note for labelling */
        err_box.x0 = (pre_z_shift_box.x1 + pre_z_shift_box.x0) / 2;
        err_box.y0 = 0;         /* move later */

        if(cp->d3.bits.use)
        {
            F64 z_frac = 0.5;

            gr_point_partial_z_shift((P_GR_POINT) &err_box.x0, (P_GR_POINT) &err_box.x0, cp, &z_frac);
        }

        err_box.x1 = err_box.x0;
    }

    /* error tit on bottom? */
    if(error.y && (value.y < 0.0))
    {
        GR_PIXIT errsize;

        /* show error -relative to value (no problem with logs as value -ve) */
        errsize = gr_value_pos_rel(cp, axes_idx, Y_AXIS_IDX, &error.y);

        /* move to bottom, flipping end down (but always shift same way) */

        /* horizontal part of T at bottom */
        err_box.y1 = pre_z_shift_box.y0 + err_box.y0;
        err_box.y0 = err_box.y1         - errsize;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));

        /* vertical part of T at bottom */
        err_box.x1 = err_box.x0 + tbar_halfsize;
        err_box.x0 = err_box.x0 - tbar_halfsize;

        err_box.y1 = err_box.y0;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));
    }

    if(cp->d3.bits.use)
    {
        /* introduce (small) z shift for non-100% depth centring */
        F64 z_frac;

        lcp->slot_depth_percentage = MAX(lcp->slot_depth_percentage, barlinechstyle.slot_depth_percentage);

        z_frac = (100.0 - barlinechstyle.slot_depth_percentage) / 100.0 / 2.0; /* only half you twerp! */

        gr_point_partial_z_shift(&zsv, NULL, cp, &z_frac);

        rect_box.x0 = pre_z_shift_box.x0 + zsv.x;
        rect_box.y0 = pre_z_shift_box.y0 + zsv.y;
        rect_box.x1 = pre_z_shift_box.x1 + zsv.x;
        rect_box.y1 = pre_z_shift_box.y1 + zsv.y;
    }
    else
        rect_box = pre_z_shift_box;

    /* the actual box (hiding away in here) */
    if(!point_fillstyleb.bits.notsolid)
    {
        if(rect_box.y0 != rect_box.y1)
            status_return(gr_diag_rectangle_new(p_gr_diag, NULL, lcp->point_id, &rect_box, &point_linestyle, &point_fillstylec));

        /* only do 3-D if some solid selected */
        if(cp->d3.bits.use)
        {
            GR_POINT origin, ps1, ps2;
            F64 z_frac;

            /* build side and top parallelograms (no bottom) */
            z_frac = barlinechstyle.slot_depth_percentage / 100.0;

            /* side, iff has non-zero height */
            origin.x = rect_box.x1;
            origin.y = rect_box.y0;

            gr_point_partial_z_shift(&ps1, NULL, cp, &z_frac);

            ps2.x = ps1.x;
            ps2.y = ps1.y + (toppoint.y - botpoint.y);

            if(ps1.x && (rect_box.y0 != rect_box.y1))
            {
                GR_FILLSTYLEC side_fillstylec;

                shade_fillstylec(&side_fillstylec, &point_fillstylec, 1, 2);

                status_return(gr_diag_parallelogram_new(p_gr_diag, NULL, lcp->point_id, &origin, &ps1, &ps2, &point_linestyle, &side_fillstylec));
            }

            /* top, even if non-zero height */
            origin.x = rect_box.x0;
            origin.y = rect_box.y1;

            ps2 = ps1; /* cheat and reuse this parallel vector */

            ps1.x = 0 + (toppoint.x - botpoint.x);
            ps1.y = 0;

            ps2.x   += ps1.x; /* simply shifted to the right place again */
            ps2.y   += ps1.y;

            if(ps2.y)
            {
                GR_FILLSTYLEC top_fillstylec;

                shade_fillstylec(&top_fillstylec, &point_fillstylec, 2, 3);

                status_return(gr_diag_parallelogram_new(p_gr_diag, NULL, lcp->point_id, &origin, &ps1, &ps2, &point_linestyle, &top_fillstylec));
            }
        }
    }

    if(point_fillstyleb.bits.pattern)
    {
        if(!barchstyle.bits.pictures_stacked)
            status = gr_chart_scaled_picture_add(cp, lcp->point_id, &rect_box, &point_fillstyleb, &point_fillstylec);
        else
        {
            const PC_GR_AXIS p_gr_axis = &p_gr_axes->axis[Y_AXIS_IDX];
            const PC_GR_AXIS_TICKS p_axis_ticks = &p_gr_axis->major;
            GR_PIXIT major_spanner_y;
            F64 frac, use_major;
            F64 iter, new_iter;
            STATUS had_last;
            GR_BOX pict_box = rect_box;

            frac = p_gr_axis->current_frac;
            use_major = p_axis_ticks->current;

            if(frac < 0.020)
            {
                frac = 0.020;
                use_major = frac * p_gr_axis->current_span;
            }

            major_spanner_y = (GR_PIXIT) (frac * cp->plotarea.size.y);

            if(value.y < 0.0)
            {
                /* pictures hanging down from top */
                pict_box.y0 = pict_box.y1 - major_spanner_y;
                iter = MIN(p_gr_axis->current.max, 0.0);
            }
            else
            {
                /* pictures standing up from bottom */
                pict_box.y1 = pict_box.y0 + major_spanner_y;
                iter = MAX(p_gr_axis->current.min, 0.0); /* copes with log scale */
            }

            for(;;)
            {
                BOOL add_pict = TRUE;

                if(value.y < 0.0)
                {
                    new_iter = iter - use_major;

                    had_last = (new_iter < value.y);

                    if(had_last) /* SKS after 1.08beta allow rounding of items between ticks */
                    {
                        F64 f64 = iter - use_major / 2;

                        add_pict = (f64 > value.y);
                    }
                }
                else
                {
                    new_iter = p_gr_axis->bits.log_scale ? (iter * use_major) : (iter + use_major);

                    had_last = (new_iter > value.y);

                    if(had_last) /* SKS after 1.08beta allow rounding of items between ticks */
                    {
                        F64 f64 = p_gr_axis->bits.log_scale ? (iter * use_major / 2) : (iter + use_major / 2);

                        add_pict = (f64 < value.y);
                    }
                }

                if(!add_pict && !barchstyle.bits.pictures_stacked_clipper)
                    break;

                status_break(status = gr_chart_scaled_picture_add(cp, lcp->point_id, &pict_box, &point_fillstyleb, &point_fillstylec));

                if(had_last)
                    break;

                iter = new_iter;

                /* shift the pict_box */
                if(value.y < 0.0)
                {
                    pict_box.y0 -= major_spanner_y;
                    pict_box.y1 -= major_spanner_y;
                }
                else
                {
                    pict_box.y0 += major_spanner_y;
                    pict_box.y1 += major_spanner_y;
                }
            }
        }

        status_return(status);
    }

    /* error tit on top? */
    if(error.y && (value.y >= 0.0))
    {
        GR_PIXIT errsize;

        /* show error +relative to value */

        if(p_gr_axes->axis[Y_AXIS_IDX].bits.log_scale)
        {
            F64 valincerr = value.y + error.y;
            errsize  = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &valincerr);
            errsize -= gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &value.y);
        }
        else
            errsize  = gr_value_pos_rel(cp, axes_idx, Y_AXIS_IDX, &error.y);

        /* move to top, right way up */

        /* vertical part of T */
        err_box.y0 = pre_z_shift_box.y1 + err_box.y0; /* 3-D shift */
        err_box.y1 = err_box.y0         + errsize;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));

        /* horizontal part of T at top */
        err_box.x1 = err_box.x0 + tbar_halfsize;
        err_box.x0 = err_box.x0 - tbar_halfsize;

        err_box.y0 = err_box.y1;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));

        /* restore unshifted x posn for possible labelling */
        err_box.x0 += tbar_halfsize;
    }

    if(labelling != 0)
    {
        GR_BOX txt_box;

        txt_box = err_box;

        status_return(gr_barline_label_point(cp, series_idx, point, lcp->point_id, &value.y, &txt_box));
    }

    gr_chart_group_end(cp, &point_group_start);

    return(status);
}

/* SKS after PD 4.12 30mar92 - added structure for better control */

typedef struct VANE_CONTROL
{
    PC_GR_LINESTYLE pdrop_linestyle;
    PC_GR_FILLSTYLEC pdrop_fillstylec;
    P_GR_CHART_OBJID pdrop_id;

    PC_GR_LINESTYLE point_linestyle;
    PC_GR_FILLSTYLEC point_fillstylec;
    P_GR_CHART_OBJID point_id;

    BOOL at_top;
    BOOL forced;
    BOOL ignore_bottom;
    BOOL is_drop;
}
VANE_CONTROL, * P_VANE_CONTROL;

_Check_return_
static STATUS
gr_linechart_vane_new(
    _ChartRef_  P_GR_CHART cp,
    P_VANE_CONTROL p_vane,
    _InRef_     PC_GR_POINT origin,
    _InRef_     PC_GR_POINT ps1,
    _InRef_     PC_GR_POINT zsv)
{
    GR_POINT ps2;

    ps2.x = ps1->x + zsv->x;
    ps2.y = ps1->y + zsv->y;

    if(!p_vane->forced)
    {
        /* ribbon vanes are always added left to right so just consider forward half without recourse to atn2 */
        F64 alpha = ((F64) ps1->y) / ((F64) ps1->x);
        F64 beta  = ((F64) ps2.y ) / ((F64) ps2.x );

        if(p_vane->at_top)
        {
            if(alpha > beta)
                return(STATUS_OK);
        }
        else
        {
            if(alpha < beta)
                return(STATUS_OK);

            /* SKS after PD 4.12 30mar92 - to avoid 'Escher' line charts we sometimes need to ignore the bottom drop vane */
            if(p_vane->ignore_bottom)
                return(STATUS_OK);
        }
    }

    return(gr_diag_parallelogram_new(cp->core.p_gr_diag, NULL,
                                     p_vane->is_drop ? *p_vane->pdrop_id         : *p_vane->point_id,
                                     origin, ps1, &ps2,
                                     p_vane->is_drop ?  p_vane->pdrop_linestyle  :  p_vane->point_linestyle,
                                     p_vane->is_drop ?  p_vane->pdrop_fillstylec :  p_vane->point_fillstylec));
}

_Check_return_
static STATUS
gr_linechart_point_addin(
    P_GR_BARLINESCATCH_SERIES_CACHE lcp,
    _InVal_     GR_CHART_ITEMNO point,
    _InVal_     enum GR_BARLINECH_PASS_TYPE pass)
{
    STATUS status = STATUS_OK;
    const P_GR_CHART cp = lcp->cp;
    const GR_AXES_IDX axes_idx = lcp->axes_idx;
    const P_GR_AXES p_gr_axes = &cp->axes[axes_idx];
    const GR_SERIES_IDX series_idx = lcp->series_idx;
    const P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_CHART_NUMPAIR value, error;
    GR_POINT valpoint, botpoint, toppoint;
    GR_POINT cur_valpoint, cur_botpoint, cur_toppoint; /* values after 3-D depth shifting */
    GR_POINT zsv = { 0, 0 }; /* Keep dataflower happy */
    GR_LINECHSTYLE linechstyle;
    GR_BARLINECHSTYLE barlinechstyle;
    GR_FILLSTYLEB point_fillstyleb;
    GR_FILLSTYLEC point_fillstylec;
    GR_LINESTYLE point_linestyle;
    GR_DIAG_OFFSET point_group_start;
    GR_BOX pre_z_shift_box;
    GR_BOX err_box = { 0, 0, 0, 0 }; /* Keep dataflower happy */
    GR_PIXIT tbar_halfsize;
    STATUS labelling;

    GR_FILLSTYLEB pdrop_fillstyleb;
    GR_FILLSTYLEC pdrop_fillstylec;
    GR_LINESTYLE pdrop_linestyle;
    GR_POINT ps1, ps2;
    GR_POINT intermediate = { 0, 0 }; /* Keep dataflower happy */
    GR_PIXIT pict_halfsize;
    VANE_CONTROL vane_ctl;

    if(!gr_barlinech_point_start(lcp, point, &value, &error))
        return(status);

    status_return(gr_chart_group_new(cp, &point_group_start, lcp->point_id));

    gr_point_fillstyleb_query(cp, series_idx, point, &point_fillstyleb);
    gr_point_fillstylec_query(cp, series_idx, point, &point_fillstylec);
    point_fillstyleb.bits.is_marker = 1;
    gr_point_linestyle_query( cp, series_idx, point, &point_linestyle, !cp->d3.bits.use);

    gr_point_linechstyle_query(   cp, series_idx, point, &linechstyle);
    gr_point_barlinechstyle_query(cp, series_idx, point, &barlinechstyle);

    if(lcp->fill_to_axis)
    {
        gr_pdrop_fillstyleb_query( cp, series_idx, point, &pdrop_fillstyleb);
        gr_pdrop_fillstylec_query( cp, series_idx, point, &pdrop_fillstylec);
        gr_pdrop_linestyle_query(  cp, series_idx, point, &pdrop_linestyle);
    }

    labelling = 0; /* or +1 */

    valpoint.y = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &value.y);

    /* create assuming simple origin - 3-D transformed using plotarea later */

    /* place in right category group */
    valpoint.x = gr_categ_pos(cp, point);

    /* shift line along group according to overlap (hi overlap -> low shift) */
    valpoint.x += (int) (cp->linech.cache.slot_shift * lcp->lineindex);

    /* shift line to right offset in group (normally 1/2 way along) */
    valpoint.x += (int) cp->linech.cache.slot_start;

    botpoint.x = valpoint.x;
    toppoint.x = valpoint.x;

    /* normally fill up from zero line (or side, or previous
     * stacked valpoint; won't be stacked if -ve) to line
    */
    botpoint.y = stacking.on ? stacking.valpoint : cp->barlinech.cache.zeropoint_y;
    toppoint.y = valpoint.y;

    if(stacking.on)
        stacking.valpoint = valpoint.y;

    if(value.y < 0.0)
    {
        /* fill down from zero line (or side; can't be stacked if -ve!) to line */
        GR_PIXIT tmppoint_y;

        tmppoint_y = botpoint.y; /* swap */
        botpoint.y = toppoint.y;
        toppoint.y = tmppoint_y;

        labelling = (0 - labelling);
    }

    /* map together into 3-D world */
    if(cp->d3.bits.use)
    {
        gr_map_point(&valpoint, cp, lcp->plotindex);
        gr_map_point(&botpoint, cp, lcp->plotindex);
        gr_map_point(&toppoint, cp, lcp->plotindex);
    }

    /* map into absolute plot area */
    valpoint.x += cp->plotarea.posn.x;
    valpoint.y += cp->plotarea.posn.y;
    botpoint.x += cp->plotarea.posn.x;
    botpoint.y += cp->plotarea.posn.y;
    toppoint.x += cp->plotarea.posn.x;
    toppoint.y += cp->plotarea.posn.y;

    /* note where the box ended up for error bar porpoises */
    pre_z_shift_box.x0 = botpoint.x;
    pre_z_shift_box.y0 = botpoint.y;
    pre_z_shift_box.x1 = toppoint.x;
    pre_z_shift_box.y1 = toppoint.y;

    pict_halfsize = cp->barlinech.cache.cat_group_width / 2;
    pict_halfsize = (GR_PIXIT) ((double) pict_halfsize * linechstyle.slot_width_percentage / 100.0);
    tbar_halfsize = pict_halfsize / 2;

    if(error.y || (labelling != 0))
    {
        /* plot little T centred on bottom or top and/or note for labelling */
        err_box.x0 = (pre_z_shift_box.x1 + pre_z_shift_box.x0) / 2;
        err_box.y0 = 0;         /* move later */

        if(cp->d3.bits.use)
        {
            F64 z_frac = 0.5;

            gr_point_partial_z_shift((P_GR_POINT) &err_box.x0, (P_GR_POINT) &err_box.x0, cp, &z_frac);
        }

        err_box.x1 = err_box.x0;
    }

    /* error tit on bottom? (no problem with logs) */
    if(error.y && (value.y < 0.0) && (pass == GR_BARLINECH_RIBBON_PASS))
    {
        GR_PIXIT errsize = gr_value_pos_rel(cp, axes_idx, Y_AXIS_IDX, &error.y);

        /* move to bottom, flipping end down (but always shift same way) */

        /* vertical part of T at bottom */
        err_box.y1 = pre_z_shift_box.y0 + err_box.y0;
        err_box.y0 = err_box.y1         - errsize;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));

        /* horizontal part of T at bottom */
        err_box.x1 = err_box.x0 + tbar_halfsize;
        err_box.x0 = err_box.x0 - tbar_halfsize;

        err_box.y1 = err_box.y0;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));
    }

    /* preserve valpoint, botpoint and toppoint prior to 3-D depth shifting */
    cur_valpoint = valpoint;
    cur_botpoint = botpoint;
    cur_toppoint = toppoint;

    if(cp->d3.bits.use)
    {
        /* introduce (small) z shift for non-100% depth centring */
        F64 z_frac;

        lcp->slot_depth_percentage = MAX(lcp->slot_depth_percentage, barlinechstyle.slot_depth_percentage);

        z_frac = (100.0 - barlinechstyle.slot_depth_percentage) / 100.0 / 2.0; /* only half you twerp! */

        gr_point_partial_z_shift(&zsv, NULL, cp, &z_frac);

        cur_valpoint.x += zsv.x;
        cur_valpoint.y += zsv.y;
        cur_botpoint.x += zsv.x;
        cur_botpoint.y += zsv.y;
        cur_toppoint.x += zsv.x;
        cur_toppoint.y += zsv.y;

        /* shift the saved points to the same depth! (22jan92) */
        lcp->old_valpoint.x += zsv.x;
        lcp->old_valpoint.y += zsv.y;
        lcp->old_botpoint.x += zsv.x;
        lcp->old_botpoint.y += zsv.y;
        lcp->old_toppoint.x += zsv.x;
        lcp->old_toppoint.y += zsv.y;

        /* find ribbon width 3-D shift and leave in zsv */
        z_frac = barlinechstyle.slot_depth_percentage / 100.0;

        gr_point_partial_z_shift(&zsv, NULL, cp, &z_frac);
    }

    intermediate.x = 0; /* 0 -> not used */

    vane_ctl.pdrop_linestyle = &pdrop_linestyle;
    vane_ctl.pdrop_fillstylec = &pdrop_fillstylec;
    vane_ctl.pdrop_id = &lcp->pdrop_id;

    vane_ctl.point_linestyle = &point_linestyle;
    vane_ctl.point_fillstylec = &point_fillstylec;
    vane_ctl.point_id = &lcp->point_id;

    /* if current drop trapezoid will be transparent then force the lot in */
    vane_ctl.forced = lcp->fill_to_axis && !pdrop_fillstylec.fg.visible;

    /* consider the bottom drop vanes only if the previous stacked series drop trapezoid was transparent */
    vane_ctl.ignore_bottom = stacking.on ? stacking.drop_filled : 1;

    if(lcp->had_first && (pass == GR_BARLINECH_RIBBON_PASS))
    {
        GR_POINT ps3;

        if(lcp->fill_to_axis)
        {
            GR_POINT delta;

            delta.y = 0;

            /* work out where if anywhere the fill needs dividing
             * note that stacking can give problems with an
             * old_bp == cur_tp but neither == zp due to previous series
            */
            if(lcp->old_botpoint.y == cur_toppoint.y)
            {
                if(cp->barlinech.cache.zeropoint_y == cur_toppoint.y)
                {
                    /* not just flat - crossover may exist (dropping from left to right) */
                    intermediate.y = lcp->old_botpoint.y;
                    delta.y = lcp->old_toppoint.y - cur_botpoint.y; /* +ve */
                }
            }
            else if(lcp->old_toppoint.y == cur_botpoint.y)
            {
                if(cp->barlinech.cache.zeropoint_y == cur_botpoint.y)
                {
                    /* not just flat - crossover may exist (ascending from left to right) */
                    intermediate.y = lcp->old_toppoint.y;
                    delta.y = cur_toppoint.y - lcp->old_botpoint.y; /* +ve */
                }
            }

            if(delta.y)
            {
                delta.x = cur_botpoint.x - lcp->old_botpoint.x;

                intermediate.x = delta.x;
                intermediate.x  *= lcp->old_toppoint.y - lcp->old_botpoint.y; /* if zero, or delta.y, then gets cut out below */
                intermediate.x  /= delta.y;

                if(intermediate.x == delta.x)
                    intermediate.x = 0;

                if(intermediate.x != 0)
                    intermediate.x += lcp->old_botpoint.x;
            }

            if(cp->d3.bits.use)
            {
                vane_ctl.at_top = 0;

                /* bottom parallelogram #1 */

                ps1.x = (intermediate.x ? intermediate.x : cur_botpoint.x) - lcp->old_botpoint.x;
                ps1.y = (intermediate.x ? intermediate.y : cur_botpoint.y) - lcp->old_botpoint.y;

                /* belongs to drop if it is flat not part of the actual ribbon */
                vane_ctl.is_drop = (ps1.y == 0) && (lcp->old_botpoint.y != lcp->old_valpoint.y);

                status_return(gr_linechart_vane_new(cp, &vane_ctl, &lcp->old_botpoint, &ps1, &zsv));

                if(intermediate.x)
                {
                    /* bottom parallelogram #2 */

                    ps1.x = cur_botpoint.x - intermediate.x;
                    ps1.y = cur_botpoint.y - intermediate.y;

                    /* if one of the bottom vanes belongs to the drop then the other doesn't */
                    vane_ctl.is_drop = !vane_ctl.is_drop;

                    status_return(gr_linechart_vane_new(cp, &vane_ctl, &intermediate, &ps1, &zsv));
                }

                vane_ctl.at_top = 1;

                /* top drop parallelogram #1 before drop trapezoid */

                ps1.x = (intermediate.x ? intermediate.x : cur_toppoint.x) - lcp->old_toppoint.x;
                ps1.y = (intermediate.x ? intermediate.y : cur_toppoint.y) - lcp->old_toppoint.y;

                /* belongs to drop if it is flat and not part of the actual ribbon */
                vane_ctl.is_drop = (ps1.y == 0) && (lcp->old_toppoint.y != lcp->old_valpoint.y);

                if(vane_ctl.is_drop)
                    status_return(gr_linechart_vane_new(cp, &vane_ctl, &lcp->old_toppoint, &ps1, &zsv));

                if(intermediate.x)
                {
                    /* top drop parallelogram #2 before drop trapezoid */

                    ps1.x = cur_toppoint.x - intermediate.x;
                    ps1.y = cur_toppoint.y - intermediate.y;

                    /* if one of the bottom vanes belongs to the drop then the other doesn't */
                    vane_ctl.is_drop = !vane_ctl.is_drop;

                    if(vane_ctl.is_drop)
                        status_return(gr_linechart_vane_new(cp, &vane_ctl, &intermediate, &ps1, &zsv));
                }
            }

            /* front drop trapezoid - with optional intermediate flip */

            /* offset_BR */
            ps1.x = lcp->old_toppoint.x - lcp->old_botpoint.x;
            ps1.y = lcp->old_toppoint.y - lcp->old_botpoint.y;

            /* offset_TR */
            ps2.x = (intermediate.x ? cur_botpoint.x : cur_toppoint.x) - lcp->old_botpoint.x; /* goto bottom instead of top for flip */
            ps2.y = (intermediate.x ? cur_botpoint.y : cur_toppoint.y) - lcp->old_botpoint.y;

            /* offset_TL */
            ps3.x = (intermediate.x ? cur_toppoint.x : cur_botpoint.x) - lcp->old_botpoint.x;
            ps3.y = (intermediate.x ? cur_toppoint.y : cur_botpoint.y) - lcp->old_botpoint.y;

            status_return(gr_diag_quadrilateral_new(p_gr_diag, NULL, lcp->pdrop_id, &lcp->old_botpoint, &ps1, &ps2, &ps3, &pdrop_linestyle, &pdrop_fillstylec));

            stacking.drop_filled = pdrop_fillstylec.fg.visible;
        }
    }

    if(lcp->had_first && (pass == GR_BARLINECH_RIBBON_PASS))
    {
        if(!cp->d3.bits.use)
        {
            /* the 2-D line joins the actual points */

            GR_BOX line_box;

            line_box.x0 = lcp->old_valpoint.x;
            line_box.y0 = lcp->old_valpoint.y;
            line_box.x1 =      cur_valpoint.x;
            line_box.y1 =      cur_valpoint.y;

            status_return(gr_chart_line_new(cp, lcp->point_id, &line_box, &point_linestyle));
        }
        else if(!lcp->fill_to_axis)
        {
            /* the 3-D non-filled ribbon joins the actual points */

            ps1.x = cur_valpoint.x - lcp->old_valpoint.x;
            ps1.y = cur_valpoint.y - lcp->old_valpoint.y;

            ps2.x = ps1.x + zsv.x;
            ps2.y = ps1.y + zsv.y;

            status_return(gr_diag_parallelogram_new(p_gr_diag, NULL, lcp->point_id, &lcp->old_valpoint, &ps1, &ps2, &point_linestyle, &point_fillstylec));
        }
        else
        {
            vane_ctl.at_top = 1;

            /* top ribbon parallelogram #1 after drop trapezoid */

            ps1.x = (intermediate.x ? intermediate.x : cur_toppoint.x) - lcp->old_toppoint.x;
            ps1.y = (intermediate.x ? intermediate.y : cur_toppoint.y) - lcp->old_toppoint.y;

            /* belongs to drop if it is flat and not part of the actual ribbon */
            vane_ctl.is_drop = (ps1.y == 0) && (lcp->old_toppoint.y != lcp->old_valpoint.y);

            if(!vane_ctl.is_drop)
                status_return(gr_linechart_vane_new(cp, &vane_ctl, &lcp->old_toppoint, &ps1, &zsv));

            if(intermediate.x)
            {
                /* top ribbon parallelogram #2 after drop trapezoid */

                ps1.x = cur_toppoint.x - intermediate.x;
                ps1.y = cur_toppoint.y - intermediate.y;

                /* if one of the top vanes belongs to the drop then the other doesn't */
                vane_ctl.is_drop = !vane_ctl.is_drop;

                if(!vane_ctl.is_drop)
                    status_return(gr_linechart_vane_new(cp, &vane_ctl, &intermediate, &ps1, &zsv));
            }
        }
    }

    if((pass == GR_BARLINECH_RIBBON_PASS) && lcp->fill_to_axis && cp->d3.bits.use)
    {
        /* right side drop parallelogram after ribbon, even for first point */

        ps1.x = zsv.x;
        ps1.y = zsv.y;

        ps2.x = ps1.x;
        ps2.y = ps1.y + (cur_toppoint.y - cur_botpoint.y);

        status_return(gr_diag_parallelogram_new(p_gr_diag, NULL, lcp->pdrop_id, &cur_botpoint, &ps1, &ps2, &pdrop_linestyle, &pdrop_fillstylec));
    }

    lcp->had_first = 1;

    /* leave some non-depth-shifted debris behind to be picked up next time as we go across points */
    lcp->old_valpoint = valpoint;
    lcp->old_botpoint = botpoint;
    lcp->old_toppoint = toppoint;

    /* pretty picture at front? */
    if(point_fillstyleb.bits.pattern && (pass == GR_BARLINECH_POINT_PASS))
    {
        GR_BOX pict_box;

        pict_box.x0 = cur_valpoint.x - pict_halfsize;
        pict_box.y0 = cur_valpoint.y - pict_halfsize;
        pict_box.x1 = cur_valpoint.x + pict_halfsize;
        pict_box.y1 = cur_valpoint.y + pict_halfsize;

        status_return(gr_chart_scaled_picture_add(cp, lcp->point_id, &pict_box, &point_fillstyleb, &point_fillstylec));
    }

    /* error tit on top? */
    if(error.y && (value.y >= 0.0) && (pass == GR_BARLINECH_RIBBON_PASS))
    {
        GR_PIXIT errsize;

        if(p_gr_axes->axis[Y_AXIS_IDX].bits.log_scale)
        {
            F64 valincerr = value.y + error.y;
            errsize  = gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &valincerr);
            errsize -= gr_value_pos(cp, axes_idx, Y_AXIS_IDX, &value.y);
        }
        else
            errsize  = gr_value_pos_rel(cp, axes_idx, Y_AXIS_IDX, &error.y);

        /* move to top, right way up */

        /* vertical part of T */
        err_box.y0 = pre_z_shift_box.y1 + err_box.y0; /* 3-D shift */
        err_box.y1 = err_box.y0         + errsize;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));

        /* horizontal part of T at top */
        err_box.x1 = err_box.x0 + tbar_halfsize;
        err_box.x0 = err_box.x0 - tbar_halfsize;

        err_box.y0 = err_box.y1;

        status_return(gr_chart_line_new(cp, lcp->point_id, &err_box, &point_linestyle));

        /* restore unshifted x posn for possible labelling */
        err_box.x0 += tbar_halfsize;
    }

    if(labelling != 0)
    {
        GR_BOX txt_box;

        txt_box = err_box;

        status_return(gr_barline_label_point(cp, series_idx, point, lcp->point_id, &value.y, &txt_box));
    }

    gr_chart_group_end(cp, &point_group_start);

    return(status);
}

_Check_return_
static STATUS
gr_barlinechart_axes_addin(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points,
    _InVal_     BOOL front_phase)
{
    GR_DIAG_OFFSET axesGroupStart;
    GR_AXES_IDX axes_idx;
    STATUS status;

    status_return(gr_chart_group_new(cp, &axesGroupStart, gr_chart_objid_anon));

    if(status_ok(status = gr_axis_addin_category(cp, total_n_points, front_phase)))
    {
        axes_idx = cp->axes_idx_max;

        for(;;)
        {
            status_break(status = gr_axis_addin_value_y(cp, axes_idx, front_phase));

            if(0 == axes_idx)
                break;

            --axes_idx;
        }
    }

    gr_chart_group_end(cp, &axesGroupStart);

    return(status);
}

_Check_return_
extern STATUS
gr_barlinechart_addin(
    _ChartRef_  P_GR_CHART cp)
{
    STATUS status = STATUS_OK;
    GR_CHART_ITEMNO total_n_points, n_points;
    GR_AXES_IDX axes_idx;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES serp;
    GR_SERIES_IDX barindex, lineindex, plotindex;
    S32 /*GR_SERIES_NO*/ n_overlapped;

    total_n_points = 0;

    for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
    {
        const P_GR_AXES p_gr_axes = &cp->axes[axes_idx];
        const P_GR_AXIS p_gr_axis_y = &p_gr_axes->axis[Y_AXIS_IDX];

        if(p_gr_axes->bits.stacked_pct)
        {
            /* negative or zero numbers 'blanked' during stacking */
            p_gr_axis_y->actual.min = 0.0;
            p_gr_axis_y->actual.max = 100.0;
        }
        else if(p_gr_axes->bits.stacked)
        {
            /* negative or zero numbers 'blanked' during stacking */
            p_gr_axis_y->actual.min = 0.0;
            p_gr_axis_y->actual.max = 0.0;
        }
        else
        {
            p_gr_axis_y->actual.min = +GR_CHART_NUMBER_MAX;
            p_gr_axis_y->actual.max = -GR_CHART_NUMBER_MAX;
        }

        for(series_idx = p_gr_axes->series.stt_idx; series_idx < p_gr_axes->series.end_idx; ++series_idx)
        {
            serp = getserp(cp, series_idx);

            switch(serp->sertype)
            {
            case GR_CHART_SERIES_PLAIN_ERROR2:
            case GR_CHART_SERIES_PLAIN_ERROR1:
                n_points = gr_actualise_series_error1(cp, series_idx);
                break;

            default:
                n_points = gr_actualise_series_plain(cp, series_idx);
                break;
            }

            /* accumulate in number of points in this series */

            total_n_points = MAX(total_n_points, n_points);

            if(serp->cache.limits_y.min <= serp->cache.limits_y.max)
            {   /* don't add zombie series to this axis */
                if(p_gr_axes->bits.stacked_pct)
                { /*EMPTY*/ } /* ignore */
                else if(p_gr_axes->bits.stacked)
                {
                    p_gr_axis_y->actual.min += serp->cache.limits_y.min;
                    p_gr_axis_y->actual.max += serp->cache.limits_y.max;
                }
                else
                {
                    p_gr_axis_y->actual.min = MIN(p_gr_axis_y->actual.min, serp->cache.limits_y.min);
                    p_gr_axis_y->actual.max = MAX(p_gr_axis_y->actual.max, serp->cache.limits_y.max);
                }
            }
        }

        if(p_gr_axis_y->actual.min > p_gr_axis_y->actual.max)
        {   /* no valid data seen for this axis - avoid zombie chart creation! */
            p_gr_axis_y->actual.min = 0;
            p_gr_axis_y->actual.max = 1;
        }
    }

    /* loop over axes again after total_n_points accumulated,
     * sussing best fit lines and forming y-axis for each axes set
    */
    for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
    {
        const P_GR_AXES p_gr_axes = &cp->axes[axes_idx];

        if(!p_gr_axes->bits.stacked)
            for(series_idx = p_gr_axes->series.stt_idx; series_idx < p_gr_axes->series.end_idx; ++series_idx)
            {
                serp = getserp(cp, series_idx);

                if(serp->bits.best_fit_manual ? serp->bits.best_fit : p_gr_axes->bits.best_fit)
                {
                    /* derive best fit line parameters */
                    GR_BARLINESCATCH_LINEST_STATE state;
                    BOOL cumulative = serp->bits.cumulative_manual ? serp->bits.cumulative : p_gr_axes->bits.cumulative;

                    gr_get_datasources(cp, series_idx, &state.dsh, FALSE);
                    state.dsh.value_x = GR_DATASOURCE_HANDLE_NONE;
                    state.dsh.error_x = GR_DATASOURCE_HANDLE_NONE;

                    state.cp = cp;
                    state.y_cum = 0.0;
                    state.x_log = 0;
                    state.y_log = (p_gr_axes->axis[Y_AXIS_IDX].bits.log_scale != 0);

                    /* initialise to be on the safe side */
                    state.a[0] = 0.0;
                    state.a[1] = 1.0;

                    {
                    SS_LINEST ss_linest;
                    ss_linest.p_proc_get = cumulative ? gr_barlinescatch_linest_getproc_cumulative : gr_barlinescatch_linest_getproc;
                    ss_linest.p_proc_put = gr_barlinescatch_linest_putproc;
                    ss_linest.client_handle = (CLIENT_HANDLE) &state;
                    ss_linest.m = 1 /* independent x variables */;
                    ss_linest.n = (U32) total_n_points;
                    status_return(object_call_id_load(P_DOCU_NONE, T5_MSG_SS_LINEST, &ss_linest, OBJECT_ID_SS_SPLIT));
                    } /*block*/

                    /* store m and c away for fuschia reference */
                    serp->cache.best_fit_c = state.a[0];
                    serp->cache.best_fit_m = state.a[1];
                }
            }

        gr_axis_form_value(cp, axes_idx, Y_AXIS_IDX);
    }

    gr_axis_form_category(cp, total_n_points);

    /* all category groups are the same width and depth */
    cp->barlinech.cache.cat_group_width = (GR_PIXIT) ((F64) cp->plotarea.size.x / (total_n_points ? total_n_points : 1));

    if(cp->cache.n_contrib_bars)
    {
        n_overlapped = cp->cache.n_contrib_bars - 1;

        /* bars in different series may differ in actual width */
        if(cp->d3.bits.use)
        {
            cp->barch.cache.slot_width = cp->barlinech.cache.cat_group_width;
            cp->barch.cache.slot_shift = 0;
        }
        else
        {
            cp->barch.cache.slot_width = (GR_PIXIT) ((F64) cp->barlinech.cache.cat_group_width / (1.0 + n_overlapped * (1.0 - (cp->barch.slot_overlap_percentage / 100.0))));

            /* shift within category between bar slots of different series (ignored if d3.bits.use) */
            cp->barch.cache.slot_shift = (GR_PIXIT) ((F64) cp->barch.cache.slot_width * (1.0 - (cp->barch.slot_overlap_percentage / 100.0)));
        }
    }

    if(cp->cache.n_contrib_lines)
    {
        cp->linech.cache.slot_start = cp->barlinech.cache.cat_group_width / 2;

        if(cp->d3.bits.use)
            cp->linech.cache.slot_shift = 0;
        else
        {
            cp->linech.cache.slot_shift = (GR_PIXIT) ((F64) cp->barlinech.cache.cat_group_width * (cp->linech.slot_shift_percentage / 100.0));

            cp->linech.cache.slot_shift /= cp->cache.n_contrib_lines;

            cp->linech.cache.slot_start -= (cp->linech.cache.slot_shift * cp->cache.n_contrib_lines) / 2;
        }
    }

    /* add in rear axes and gridlines */
    status_return(gr_barlinechart_axes_addin(cp, total_n_points, FALSE));

    /* add in data on axes */

    /* always plot from the back to the front */
    barindex = cp->cache.n_contrib_bars;
    lineindex = cp->cache.n_contrib_lines;
    plotindex = cp->cache.n_contrib_series;

    if(barindex)
        --barindex;

    if(lineindex)
        --lineindex;

    if(plotindex)
        --plotindex;

    axes_idx = cp->axes_idx_max;

#if 0
    if(cp->cache.n_contrib_bars || cp->cache.n_contrib_lines)
#endif
    for(;;)
    {
        P_GR_BARLINESCATCH_SERIES_CACHE lcp;
        enum GR_BARLINECH_PASS_TYPE pass;
        const P_GR_AXES p_gr_axes = &cp->axes[axes_idx];
        const GR_SERIES_IDX stt_idx = p_gr_axes->series.stt_idx;
        const GR_SERIES_IDX end_idx = p_gr_axes->series.end_idx;
        GR_POINT_NO point;

        /* bits of cache are per axes set plotting */

        /* irrelevant for log scaling */
        cp->barlinech.cache.zeropoint_y = (GR_PIXIT) ((F64) cp->plotarea.size.y *
                                                      p_gr_axes->axis[Y_AXIS_IDX].zero_frac); /*NB*/

        if(p_gr_axes->bits.stacked)
        {
            P_GR_BARLINESCATCH_SERIES_CACHE lc_array;
            /* SKS after PD 4.12 30mar92 - was total_n_points (which is garbage) -> crash if total_n_points < n_series */
            S32 bars_plotted = 0;
            S32 lines_plotted = 0;

            /* allocate core for stacking series memory per axes set */
            if(NULL == (lc_array = al_ptr_alloc_elem(GR_BARLINESCATCH_SERIES_CACHE, (end_idx - stt_idx), &status)))
                break;

            /* initialise each series' cache once */

            for(series_idx = stt_idx; series_idx < end_idx; ++series_idx)
            {
                lcp = &lc_array[series_idx - stt_idx];

                lcp->cp = cp;
                lcp->axes_idx = axes_idx;
                lcp->series_idx = series_idx;
                lcp->n_points = total_n_points;

                lcp->barindex = barindex;
                lcp->lineindex = lineindex;
                lcp->plotindex = plotindex;

                gr_barlinech_series_init(lcp);
            }

            /* line charts need multiple passes to be drawn correctly */

            pass = GR_BARLINECH_RIBBON_PASS;

            for(;;)
            {
                /* reset some bits of each series' cache per pass */

                for(series_idx = stt_idx; series_idx < end_idx; ++series_idx)
                {
                    lcp = &lc_array[series_idx - stt_idx];

                    gr_barlinech_pass_init(lcp);
                }

                /* plot points left to right */

                for(point = 0; point < total_n_points; ++point)
                {
                    /* reset stacking cache for each point */
                    if(!gr_barlinech_stacking_init(cp, axes_idx, point))
                        /* 100% plots may have detected no values to plot */
                        continue;

                    /* plot series from bottom to top of this one point (reverse if -ve pitch) */

                    for(series_idx = stt_idx; series_idx < end_idx; ++series_idx)
                    {
                        lcp = &lc_array[series_idx - stt_idx];

                        serp = getserp(cp, series_idx);

                        switch((serp->chart_type != GR_CHART_TYPE_NONE)
                               ? serp->chart_type
                               : p_gr_axes->chart_type)
                        {
                            default: default_unhandled();
#if CHECKING
                            case GR_CHART_TYPE_BAR:
#endif
                                if(pass != GR_BARLINECH_POINT_PASS)
                                {
                                    status = gr_barchart_point_addin(lcp, point);
                                    bars_plotted = 1;
                                }
                                else
                                    status = STATUS_OK;
                                break;

                            case GR_CHART_TYPE_LINE:
                                status = gr_linechart_point_addin(lcp, point, pass);
                                lines_plotted = 1;
                                break;
                        }

                        status_break(status);
                    }

                    status_break(status);
                }

                status_break(status);

                if(pass == GR_BARLINECH_RIBBON_PASS)
                    pass = GR_BARLINECH_POINT_PASS;
                else /* pass == GR_BARLINECH_POINT_PASS */
                    break;
            }

            al_ptr_dispose(P_P_ANY_PEDANTIC(&lc_array));

            if(bars_plotted)
                if(barindex)
                    --barindex;

            if(lines_plotted)
                if(lineindex)
                    --lineindex;

            if(bars_plotted || lines_plotted)
                if(plotindex)
                    --plotindex;
        }
        else
        {
            GR_BARLINESCATCH_SERIES_CACHE single_series_cache;
            GR_DIAG_OFFSET series_group_start;

            stacking.on = 0;

            /* plot whole series across from back to front */

            series_idx = end_idx;

            while(series_idx > stt_idx)
            {
                --series_idx;

                /* initialise this one series' cache once */

                lcp = &single_series_cache;

                lcp->cp = cp;
                lcp->axes_idx = axes_idx;
                lcp->series_idx = series_idx;
                lcp->n_points = total_n_points;

                lcp->barindex = barindex;
                lcp->lineindex = lineindex;
                lcp->plotindex = plotindex;

                gr_barlinech_series_init(lcp);

                status_break(status = gr_chart_group_new(cp, &series_group_start, lcp->serid));

                serp = getserp(cp, series_idx);

                switch((serp->chart_type != GR_CHART_TYPE_NONE) ? serp->chart_type : p_gr_axes->chart_type)
                {
                default: default_unhandled();
#if CHECKING
                case GR_CHART_TYPE_BAR:
#endif
                    {
                    /* bar charts require but one pass */

                    /* reset some bits of this series' cache for this pass */
                    gr_barlinech_pass_init(lcp);

                    /* loop over points in this one series from left to right */

                    for(point = 0; point < total_n_points; ++point)
                        status_break(status = gr_barchart_point_addin(lcp, point));

                    if(lcp->bl_best_fit)
                        status_break(status = gr_barlinescatch_best_fit_addin(lcp, GR_CHART_TYPE_BAR));

                    if(barindex)
                        --barindex;

                    if(plotindex)
                        --plotindex;

                    break;
                    }

                case GR_CHART_TYPE_LINE:
                    {
                    /* line charts require multiple passes over this one series to be drawn correctly */

                    pass = GR_BARLINECH_RIBBON_PASS;

                    for(;;)
                    {
                        GR_DIAG_OFFSET pass_group_start;

                        /* reset some bits of this series' cache per pass */
                        gr_barlinech_pass_init(lcp);

                        status_break(status = gr_chart_group_new(cp, &pass_group_start, lcp->serid));

                        /* loop over points in this one series from left to right */

                        for(point = 0; point < total_n_points; ++point)
                            status_break(status = gr_linechart_point_addin(lcp, point, pass));

                        gr_chart_group_end(cp, &pass_group_start);

                        status_break(status);

                        /* add in best fit line between the ribbon and point passes iff unstacked */
                        if((pass == GR_BARLINECH_RIBBON_PASS) && lcp->bl_best_fit)
                            status_break(status = gr_barlinescatch_best_fit_addin(lcp, GR_CHART_TYPE_LINE));

                        if(pass == GR_BARLINECH_RIBBON_PASS)
                            pass = GR_BARLINECH_POINT_PASS;
                        else /* pass == GR_BARLINECH_POINT_PASS */
                            break;
                    }

                    if(lineindex)
                        --lineindex;

                    if(plotindex)
                        --plotindex;

                    break;
                    }
                }

                gr_chart_group_end(cp, &series_group_start);

                status_break(status);
            }

            status_break(status);
        }

        status_break(status);

        if(0 == axes_idx)
            break;

        --axes_idx;
    }

    status_return(status);

    /* add in front axes and gridlines */
    return(gr_barlinechart_axes_addin(cp, total_n_points, TRUE));
}

/* end of gr_barch.c */
