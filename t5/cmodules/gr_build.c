/* gr_build.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart building */

/* SKS June 1993 for Fireworkz */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

/*
internal functions
*/

static void
gr_chart_build_init(
    _ChartRef_  P_GR_CHART cp);

static void
gr_chart_cache_n_contrib(
    _ChartRef_  P_GR_CHART cp);

_Check_return_
static STATUS
gr_chart_chart_addin(
    _ChartRef_  P_GR_CHART cp);

_Check_return_
static STATUS
gr_chart_plotarea_addin(
    _ChartRef_  P_GR_CHART cp);

#if RISCOS && 0

static void
gr_chart_riscos_broadcast_changed(
    _In_z_      PCTSTR filename);

#endif

#if !RISCOS
#define DRAWFILE_EXTENSION_TSTR FILE_EXT_SEP_TSTR TEXT("aff")
#endif

#ifndef __cplusplus

#include <signal.h>

typedef void (__cdecl * P_PROC_SIGNAL) (
    _In_        int sig);

#include <setjmp.h>

static jmp_buf
gr_chart_build_jmp_buf;

static void __cdecl
gr_chart_build_signal_handler(
    _In_        int sig)
{
#if WINDOWS
    if(sig == SIGFPE)
        _fpreset();
#endif

    longjmp(gr_chart_build_jmp_buf, sig);
}

#endif /* __cplusplus */

_Check_return_
static STATUS
gr_chart_build(
    _InVal_     GR_CHART_HANDLE ch)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);
    STATUS status;

#ifndef __cplusplus

    P_PROC_SIGNAL oldfpe = signal(SIGFPE, (P_PROC_SIGNAL) gr_chart_build_signal_handler);

    if(setjmp(gr_chart_build_jmp_buf))
    {
        reportf(TEXT("*** gr_chart_build setjmp returned from signal handler ***"));
        status = create_error(CHART_ERR_EXCEPTION);
    }
    else
    {

#endif /* __cplusplus */

        /* blow some cached info */
        * (int *) &cp->d3.valid = 0;

        for(;;) /* loop for structure */
        {
            P_GR_DIAG p_gr_diag;

            if(cp->bits.realloc_series)
                status_break(status = gr_chart_realloc_series(cp));

            /* reset warning status */
            gr_chart_warning(cp, 0);

            gr_chart_build_init(cp);

            if(NULL == (cp->core.p_gr_diag = p_gr_diag = gr_diag_diagram_new(product_id(), &status)))
                break;

            { /* create some hierarchy to help correlator */
            GR_DIAG_OFFSET bgGroupStart;

            status_break(status = gr_chart_group_new(cp, &bgGroupStart, gr_chart_objid_anon));

            /* add in actual chart area */
            status_break(status = gr_chart_chart_addin(cp));

            /* add in plot area */
            status_break(status = gr_chart_plotarea_addin(cp));

            gr_chart_group_end(cp, &bgGroupStart);
            } /*block*/

            switch(cp->axes[0].chart_type)
            {
            case GR_CHART_TYPE_PIE:
                /* loop over categories plotting sectors */
                status = gr_pie_addin(cp);
                break;

            default: default_unhandled();
#if CHECKING
            case GR_CHART_TYPE_BAR:
            case GR_CHART_TYPE_LINE:
#endif
                status = gr_barlinechart_addin(cp);
                break;

            case GR_CHART_TYPE_SCAT:
                status = gr_scatterchart_addin(cp);
                break;
            }

            status_break(status);

            { /* create some more hierarchy to help correlator */
            GR_DIAG_OFFSET fgGroupStart;

            status_break(status = gr_chart_group_new(cp, &fgGroupStart, gr_chart_objid_anon));

            /* add in legend */
            status_break(status = gr_chart_legend_addin(cp));

            status_break(status = gr_text_addin(cp));

            gr_chart_group_end(cp, &fgGroupStart);
            } /*block*/

            gr_diag_diagram_end(p_gr_diag);

            break; /* end of loop for structure */
            /*NOTREACHED*/
        }

#ifndef __cplusplus

    }

    signal(SIGFPE, oldfpe);

#endif /* __cplusplus */

    return(status);
}

static void
gr_chart_build_init(
    _ChartRef_  P_GR_CHART cp)
{
    GR_BOX plotarea;
    S32 /*GR_SERIES_NO*/ n_series_deep;
    GR_POINT new_posn;

    /* reflect 3-D usability */
    switch(cp->axes[0].chart_type)
    {
    case GR_CHART_TYPE_PIE:
    case GR_CHART_TYPE_SCAT:
        cp->d3.bits.use = 0;
        break;

    default: default_unhandled();
#if CHECKING
    case GR_CHART_TYPE_BAR:
    case GR_CHART_TYPE_LINE:
#endif
        cp->d3.bits.use = cp->d3.bits.on;
        break;
    }

    /* given the chart size, work out where the plot area ought to be */

    /* first guess is within the chart margins */
    plotarea.x0 = cp->core.layout.margins.left;
    plotarea.y0 = cp->core.layout.margins.bottom;
    plotarea.x1 = cp->core.layout.width  - cp->core.layout.margins.right;
    plotarea.y1 = cp->core.layout.height - cp->core.layout.margins.top;

    gr_chart_cache_n_contrib(cp);

    /* affected by stacking */
    n_series_deep = cp->cache.n_contrib_series ? cp->cache.n_contrib_series : 1;

    if(cp->d3.bits.use)
    {
        F64 sin_t = sin(cp->d3.turn  * _radians_per_degree);
        F64 cos_t = cos(cp->d3.turn  * _radians_per_degree);
        F64 sin_d = sin(cp->d3.droop * _radians_per_degree);
        F64 cos_d = cos(cp->d3.droop * _radians_per_degree);
        /* NB. the following is now an int 'cos unsigneds promote ints
         * to unsigned and stuff the division result for -ve numbers
        */
        GR_POINT size;

        /* given current allowable size, work out how to share it out */
        size.x = plotarea.x1 - plotarea.x0;
        size.x = (GR_PIXIT) ((size.x * cos_t) / (cos_t + sin_t));
        size.y = plotarea.y1 - plotarea.y0;
        size.y = (GR_PIXIT) ((size.y * cos_d) / (cos_d + sin_d));

        new_posn.x = plotarea.x1 - size.x;
        new_posn.y = plotarea.y1 - size.y;
    }
    else
    {
        /* make null 3-D vectors */
        new_posn.x = plotarea.x0;
        new_posn.y = plotarea.y0;
    }

    cp->d3.cache.vector_full.x = plotarea.x0 - new_posn.x; /* 0 or -ve */
    cp->d3.cache.vector_full.y = plotarea.y0 - new_posn.y; /* 0 or -ve */

    cp->d3.cache.vector.x = cp->d3.cache.vector_full.x / n_series_deep;
    cp->d3.cache.vector.y = cp->d3.cache.vector_full.y / n_series_deep;

    cp->d3.valid.vector = 1;

    plotarea.x0 = new_posn.x;
    plotarea.y0 = new_posn.y;

    cp->plotarea.posn.x = plotarea.x0;
    cp->plotarea.posn.y = plotarea.y0;
    cp->plotarea.size.x = plotarea.x1 - plotarea.x0;
    cp->plotarea.size.y = plotarea.y1 - plotarea.y0;
}

/******************************************************************************
*
* add in any bar or line chart components to the chart
*
******************************************************************************/

static void
gr_chart_cache_n_contrib(
    _ChartRef_  P_GR_CHART cp)
{
    GR_AXES_IDX axes_idx;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES serp;

    cp->cache.n_contrib_series = 0;
    cp->cache.n_contrib_bars = 0;
    cp->cache.n_contrib_lines = 0;

    for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
    {
        const P_GR_AXES axesp = &cp->axes[axes_idx];

        /* only add one effective series holder, one bar place or line
         * place per set of stacked bars on each axes set
        */
        BOOL series_adding = TRUE;
        BOOL bar_adding = TRUE;
        BOOL line_adding = TRUE;

        axesp->cache.n_contrib_bars = 0;
        axesp->cache.n_contrib_lines = 0;

        switch(axesp->chart_type)
        {
        case GR_CHART_TYPE_PIE:
            cp->cache.n_contrib_series = 1;
            break;

        case GR_CHART_TYPE_SCAT:
            /* normally using all series */
            cp->cache.n_contrib_series += axesp->series.end_idx;
            cp->cache.n_contrib_series -= axesp->series.stt_idx;
            break;

        default: default_unhandled();
#if CHECKING
        case GR_CHART_TYPE_BAR:
        case GR_CHART_TYPE_LINE:
#endif
            for(series_idx = axesp->series.stt_idx; series_idx < axesp->series.end_idx; ++series_idx)
            {
                serp = getserp(cp, series_idx);

                switch(serp->chart_type)
                {
                default:
                    /* not bar or line, skip */
                    continue;

                case GR_CHART_TYPE_NONE:
                    if(axesp->chart_type == GR_CHART_TYPE_LINE)
                        goto add_line;

                    /*FALLTHRU*/

                case GR_CHART_TYPE_BAR:
                    if(series_adding)
                        ++cp->cache.n_contrib_series;

                    if(bar_adding)
                        ++axesp->cache.n_contrib_bars;

                    if(axesp->bits.stacked)
                        series_adding = bar_adding = FALSE;
                    break;

                case GR_CHART_TYPE_LINE:
                add_line:;
                    if(series_adding)
                        ++cp->cache.n_contrib_series;

                    if(line_adding)
                        ++axesp->cache.n_contrib_lines;

                    if(axesp->bits.stacked)
                        series_adding = line_adding = FALSE;
                    break;
                }
            }

            break;
        }

        cp->cache.n_contrib_bars  += axesp->cache.n_contrib_bars;
        cp->cache.n_contrib_lines += axesp->cache.n_contrib_lines;
    }
}

_Check_return_
static STATUS
gr_chart_chart_addin(
    _ChartRef_  P_GR_CHART cp)
{
    const GR_CHART_OBJID id = gr_chart_objid_chart;
    GR_DIAG_OFFSET chartGroupStart;
    GR_BOX chartbox;
    GR_FILLSTYLEB fillstyleb;
    GR_FILLSTYLEC fillstylec;
    GR_LINESTYLE linestyle;
    STATUS status = STATUS_OK;

    status_return(gr_chart_group_new(cp, &chartGroupStart, id));

    chartbox.x0 = 0;
    chartbox.y0 = 0;
    chartbox.x1 = chartbox.x0 + cp->core.layout.width;
    chartbox.y1 = chartbox.y0 + cp->core.layout.height;

    gr_chart_objid_fillstyleb_query(cp, id, &fillstyleb);
    gr_chart_objid_fillstylec_query(cp, id, &fillstylec);
    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    /* SKS after 1.05 25oct93 - chart area is crucial to operation and cannot be omitted, so make sure at least a transparent background is used */
    if(fillstyleb.bits.notsolid)
        if(fillstylec.fg.visible)
            fillstylec.fg.visible = 0;

    /*if(!fillstyleb.bits.notsolid)*/
        /*if(linestyle.fg.visible || fillstylec.fg.visible)*/
        {
            GR_BOX box = chartbox;

            if(linestyle.fg.visible)
            {
                /* run line around the interior of the chart object */
                GR_PIXIT half_lw = linestyle.width / 2;

                box.x0 += half_lw;
                box.y0 += half_lw;
                box.x1 -= half_lw;
                box.y1 -= half_lw;
            }

            status_return(gr_diag_rectangle_new(cp->core.p_gr_diag, NULL, id, &box, &linestyle, &fillstylec));
        }

    if(fillstyleb.bits.pattern)
        status_return(gr_chart_scaled_picture_add(cp, id, &chartbox, &fillstyleb, &fillstylec));

    gr_chart_group_end(cp, &chartGroupStart);

    return(status);
}

_Check_return_
extern STATUS
gr_chart_diagram(
    _InVal_     GR_CHART_HANDLE ch,
    /*out*/ P_P_GR_DIAG p_p_gr_diag)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);

    *p_p_gr_diag = cp->core.p_gr_diag;

    return(NULL != *p_p_gr_diag);
}

/******************************************************************************
*
* propagate damage from datasources and
* user modifications into chart, both in
* editing window (if present) and to disc
* (if present) for client reload
*
******************************************************************************/

extern void
gr_chart_diagram_ensure(
    _InVal_     GR_CHART_HANDLE ch)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(ch);
    STATUS requires_building;
    STATUS status;

    if(NULL == cp)
        return;

    requires_building = cp->core.modified || (NULL == cp->core.p_gr_diag) || (NULL == cp->core.p_gr_diag->p_gr_riscdiag) || (0 == cp->core.p_gr_diag->p_gr_riscdiag->draw_diag.length);

    if(!requires_building)
        return; /* amazingly, this wasn't in PD4! */

    callback_from_gr_chart(cp->core.ext_handle, GR_CHART_CALLBACK_RC_SELECTION_KILL_REPR);

    gr_diag_diagram_dispose(&cp->core.p_gr_diag);

    if(status_fail(status = gr_chart_build(cp->core.ch)))
        gr_chart_warning(cp, status);

    callback_from_gr_chart(cp->core.ext_handle, GR_CHART_CALLBACK_RC_SELECTION_MAKE_REPR);
}

/******************************************************************************
*
* add in legend
*
******************************************************************************/

static void
gr_chart_legend_boxes_prepare(
    _InRef_     PC_GR_POINT curpt,
    _OutRef_    P_GR_BOX rect_box,
    _OutRef_opt_ P_GR_BOX line_box,
    _OutRef_opt_ P_GR_BOX pict_box,
    _OutRef_    P_GR_BOX text_box,
    _InRef_     PC_GR_TEXTSTYLE textstyle,
    _InVal_     GR_PIXIT swidth_px)
{
    rect_box->x0 = curpt->x;
    rect_box->y0 = curpt->y;
    rect_box->x1 = rect_box->x0 + (textstyle->size_y * 7) / 8;
    rect_box->y1 = rect_box->y0 + (textstyle->size_y * 6) / 8;

    if(line_box)
    {
        line_box->x0 = rect_box->x0 - (rect_box->x1 - rect_box->x0) / 4;
        line_box->y0 =                (rect_box->y0 + rect_box->y1) / 2;
        line_box->x1 = rect_box->x1 + (rect_box->x1 - rect_box->x0) / 4;
        line_box->y1 = line_box->y0;
    }

    /* embed point marker in a smaller box */
    if(pict_box)
    {
        pict_box->x0 = rect_box->x0 + (rect_box->x1 - rect_box->x0) / 8;
        pict_box->y0 = rect_box->y0 + (rect_box->y1 - rect_box->y0) / 8;
        pict_box->x1 = rect_box->x1 - (rect_box->x1 - rect_box->x0) / 8;
        pict_box->y1 = rect_box->y1 - (rect_box->y1 - rect_box->y0) / 8;
    }

    text_box->x0 = rect_box->x1 + (3 * textstyle->size_y) / 4;
    text_box->y0 = curpt->y;
    text_box->x1 = text_box->x0 + swidth_px;
    text_box->y1 = text_box->y0 + textstyle->size_y;
}

_Check_return_
static STATUS
gr_chart_legend_boxes_fitq(
    _InVal_     S32 in_rows,
    _InRef_     PC_GR_BOX legendbox,
    P_GR_POINT curpt,
    /*inout*/ P_GR_POINT maxpt,
    _InVal_     GR_PIXIT linespacing,
    P_GR_BOX text_box,
    /*inout*/ GR_PIXIT * maxlegx)
{
    S32 res = 1; /* normal return */

    if(in_rows)
    {
        /* see if legend will fit in remainder of row */
        if(text_box->x1 > legendbox->x1)
        {
            /* drop to start of next row */
            curpt->x = legendbox->x0;

            text_box->y0 -= curpt->y;
            curpt->y     -= linespacing;
            text_box->y0 += curpt->y;

            if(text_box->y0 < legendbox->y0)
                return(0);

            res = 2; /* recompute boxes on return */
        }

        /* record our maximum travel in the -ve y direction */
        maxpt->y = MIN(maxpt->y, text_box->y0);
    }
    else
    {
        /* see if legend will fit in remainder of column */
        if(text_box->y0 < legendbox->y0)
        {
            /* move to top of next column */
            curpt->y = legendbox->y1;

            text_box->x1 -= curpt->x;
            curpt->x = *maxlegx + linespacing;
            text_box->x1 += curpt->x;

            /* reset maxlegx for this new column */
            *maxlegx = legendbox->x0;

            if(text_box->x1 > legendbox->x1)
                return(0);

            res = 2; /* recompute boxes on return */
        }

        /* record our maximum travel in the +ve x direction */
        maxpt->x = MAX(maxpt->x, text_box->x1);
    }

    return(res);
}

_Check_return_
extern STATUS
gr_chart_legend_addin(
    _ChartRef_  P_GR_CHART cp)
{
    const P_GR_CHART_LEGEND legend = &cp->legend;
    const P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_BOX legendbox;
    GR_BOX legend_margins;
    GR_DIAG_OFFSET legendGroupStart;
    GR_CHART_OBJID legend_id = gr_chart_objid_legend;
    GR_CHART_OBJID intern_id;
    GR_CHART_TYPE chart_type;
    GR_AXES_IDX axes_idx;
    P_GR_AXES axesp;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES serp;
    GR_POINT curpt, maxpt;
    GR_PIXIT maxlegx;
    GR_TEXTSTYLE textstyle;
    GR_PIXIT linespacing;
    GR_LINESTYLE linestyle;
    GR_FILLSTYLEB fillstyleb;
    GR_FILLSTYLEC fillstylec;
    S32 n_for_legend;
    GR_BOX rect_box, line_box, pict_box, text_box;
    GR_CHART_VALUE cv;
    HOST_FONT host_font;
    GR_MILLIPOINT swidth_mp;
    GR_PIXIT swidth_px;
    S32 res;
    S32 pass_out;
    STATUS status = STATUS_OK;

    if(!legend->bits.on)
        return(1);

    gr_chart_objid_fillstyleb_query(cp, legend_id, &fillstyleb);
    gr_chart_objid_fillstylec_query(cp, legend_id, &fillstylec);
    gr_chart_objid_linestyle_query(cp, legend_id, &linestyle);
    gr_chart_objid_textstyle_query(cp, legend_id, &textstyle);

    linespacing = (textstyle.size_y * 12) / 10;

    chart_type = cp->axes[0].chart_type;

    switch(chart_type)
    {
    case GR_CHART_TYPE_PIE:
        axes_idx = 0;
        series_idx = 0;
        n_for_legend = gr_travel_series_n_items_total(cp, series_idx);
        break;

    default:
        n_for_legend = 0;

        for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            n_for_legend += cp->axes[axes_idx].series.end_idx;
            n_for_legend -= cp->axes[axes_idx].series.stt_idx;
        }
        break;
    }

    /* if nothing to legend, return */
    if(!n_for_legend)
        return(status);

    legend_margins.x0 = MAX(GR_CHART_LEGEND_LM,      linespacing  / 2);
    legend_margins.y0 = MAX(GR_CHART_LEGEND_BM, (2 * linespacing) / 4);
    legend_margins.x1 = MAX(GR_CHART_LEGEND_RM,      linespacing  / 2);
    legend_margins.y1 = MAX(GR_CHART_LEGEND_TM, (5 * linespacing) / 4); /* account for baseline offset here */

    if(legend->bits.manual)
    {
        legendbox.x0 = legend->posn.x;
        legendbox.y0 = legend->posn.y;
        legendbox.x1 = legend->posn.x + legend->size.x;
        legendbox.y1 = legend->posn.y + legend->size.y;

        /* take margins off box */
        legendbox.x0 += legend_margins.x0;
        legendbox.y0 += legend_margins.y0;
        legendbox.x1 -= legend_margins.x1;
        legendbox.y1 -= legend_margins.y1;

        /* ensure always 'reasonable' size even if punter has set wally */
        legendbox.x1 = MAX(legendbox.x1, legendbox.x0 + 3 * linespacing);
        legendbox.y0 = MIN(legendbox.y0, legendbox.y1 - 2 * linespacing);
    }
    else
    {
        if(legend->bits.in_rows)
        {
            /* legend at bottom, goes right across rows */
            GR_PIXIT x_size;

            /* ensure always 'reasonable' size even if punter has set wally */
            x_size = cp->core.layout.size.x;
            x_size = MAX(x_size, 4 * linespacing);
            x_size = MIN(x_size, cp->core.layout.width);

            legendbox.x0 = cp->core.layout.margins.left;
            legendbox.x1 = legendbox.x0 + x_size;

            legendbox.y1 = 0;

            /* ensure box down by this amount */
            legendbox.y1 -= GR_CHART_LEGEND_TM;

            /* apply margin (and baseline offset) to top edge */
            legendbox.y1 -= legend_margins.y1;

            legendbox.y0 = S32_MIN; /* can grow to be as deep as it needs to be */
        }
        else
        {
            /* legend at right, goes down in columns */
            GR_PIXIT estdepth, y_size;

            legendbox.x0 = cp->core.layout.width;
            legendbox.x1 = S32_MAX; /* can grow to be as wide as it needs to be */

            /* ensure always 'reasonable' size even if punter has set wally */
            y_size = cp->core.layout.size.y;
            y_size = MAX(y_size, linespacing);
            y_size = MIN(y_size, cp->core.layout.height);

            /* have a guess at how big the legend ought to be, and centre if possible */
            estdepth = n_for_legend * linespacing;
            estdepth = MIN(estdepth, y_size);

            legendbox.y0 = (cp->core.layout.margins.bottom + y_size - estdepth) / 2;
            legendbox.y1 = legendbox.y0 + estdepth;

            /* ensure box out by this amount */
            legendbox.x0 += GR_CHART_LEGEND_LM;

            /* apply margin to left hand edge */
            legendbox.x0 += legend_margins.x0;

            /* NB. no output movement on legendbox.y1 */
        }
    }

    /* record furthest positions we get to */
    maxpt.x = legendbox.x0;
    maxpt.y = legendbox.y1;

    host_font = gr_riscdiag_host_font_from_textstyle(&textstyle);

    res = 1;

    for(pass_out = 0; pass_out <= 1; ++pass_out)
    {
        if(pass_out && !legend->bits.manual)
        {
            S32 size;

            /* now we can use the info gathered on pass 1 to make a reasonable legend box */
            if(legend->bits.in_rows)
            {
                size = legendbox.y1 - maxpt.y;

#if 1
                legendbox.y0 = 0 + (GR_CHART_LEGEND_TM + legend_margins.y0);
                legendbox.y1 = legendbox.y0 + size;
#else
                legendbox.y0 = maxpt.y;
#endif
            }
            else
            {
                size = maxpt.x - legendbox.x0;

#if 1
                legendbox.x1 = cp->core.layout.width - (GR_CHART_LEGEND_LM + legend_margins.x1);
                legendbox.x0 = legendbox.x1 - size;
#else
                legendbox.x1 = maxpt.x;
#endif
            }
        }

        legend->posn.x = legendbox.x0;
        legend->posn.y = legendbox.y0;
        legend->size.x = legendbox.x1 - legendbox.x0;
        legend->size.y = legendbox.y1 - legendbox.y0;

        /* recompute outer box from inner box and margins (including baseline offset) */
        legend->posn.x -= legend_margins.x0;
        legend->posn.y -= legend_margins.y0;
        legend->size.x += legend_margins.x0 + legend_margins.x1;
        legend->size.y += legend_margins.y0 + legend_margins.y1;

        maxlegx = legendbox.x0;

        curpt.x = legendbox.x0;
        curpt.y = legendbox.y1;

        if(pass_out)
        {
            GR_BOX legend_outer_rect;

            status_break(status = gr_chart_group_new(cp, &legendGroupStart, legend_id));

            legend_outer_rect.x0 = legend->posn.x;
            legend_outer_rect.y0 = legend->posn.y;
            legend_outer_rect.x1 = legend->posn.x + legend->size.x;
            legend_outer_rect.y1 = legend->posn.y + legend->size.y;

            if(!fillstyleb.bits.notsolid)
                status_break(status = gr_diag_rectangle_new(p_gr_diag, NULL, legend_id, &legend_outer_rect, &linestyle, &fillstylec));

            if(fillstyleb.bits.pattern)
                status_break(status = gr_chart_scaled_picture_add(cp, legend_id, &legend_outer_rect, &fillstyleb, &fillstylec));
        }

        switch(chart_type)
        {
        case GR_CHART_TYPE_PIE:
            {
            /* loop plotting a rectangle of colour and category label for each category */
            GR_CHART_ITEMNO point;

            gr_chart_objid_clear(&intern_id);
            intern_id.name = GR_CHART_OBJNAME_LEGDPOINT;
            intern_id.no = UBF_PACK(gr_series_external_from_idx(cp, 0));
            intern_id.has_no = 1;
            intern_id.has_subno = 1;

            for(point = 0; point < n_for_legend; ++point)
            {
                gr_travel_categ_label(cp, point, &cv);

                swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(cv.data.text));
                swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

                gr_chart_legend_boxes_prepare(&curpt, &rect_box, NULL, NULL, &text_box, &textstyle, swidth_px);

                res = gr_chart_legend_boxes_fitq(legend->bits.in_rows, &legendbox, &curpt, &maxpt, linespacing, &text_box, &maxlegx);

                if(res == 0)
                    break;

                maxlegx = MAX(maxlegx, text_box.x1);

                if(pass_out)
                {
                    if(res == 2)
                        gr_chart_legend_boxes_prepare(&curpt, &rect_box, NULL, NULL, &text_box, &textstyle, swidth_px);

                    intern_id.subno = UBF_PACK(gr_point_external_from_key(point));

                    gr_chart_objid_linestyle_query(cp, intern_id, &linestyle);
                    gr_chart_objid_fillstylec_query(cp, intern_id, &fillstylec);

                    status_break(status = gr_diag_rectangle_new(p_gr_diag, NULL, intern_id, &rect_box, &linestyle, &fillstylec));

                    if(swidth_px)
                        status_break(status = gr_chart_text_new(cp, intern_id, &text_box, ustr_bptr(cv.data.text), &textstyle));
                }

                if(legend->bits.in_rows)
                    curpt.x = text_box.x1 + (linespacing / 2);
                else
                    curpt.y -= linespacing;
            }

            break;
            }

        default:
            /* loop plotting the point marker and series label for each series */

            for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
            {
                axesp = &cp->axes[axes_idx];

                for(series_idx = axesp->series.stt_idx; series_idx < axesp->series.end_idx; ++series_idx)
                {
                    /* loop plotting a rectangle of colour and category label for each category */

                    gr_travel_series_label(cp, series_idx, &cv);

                    swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(cv.data.text));
                    swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

                    gr_chart_legend_boxes_prepare(&curpt, &rect_box, &line_box, &pict_box, &text_box, &textstyle, swidth_px);

                    res = gr_chart_legend_boxes_fitq(legend->bits.in_rows, &legendbox, &curpt, &maxpt, linespacing, &text_box, &maxlegx);

                    if(res == 0)
                        break;

                    maxlegx = MAX(maxlegx, text_box.x1);

                    if(pass_out)
                    {
                        if(res == 2)
                            gr_chart_legend_boxes_prepare(&curpt, &rect_box, &line_box, &pict_box, &text_box, &textstyle, swidth_px);

                        gr_chart_objid_from_series_no(gr_series_external_from_idx(cp, series_idx), &intern_id);
                        intern_id.name = GR_CHART_OBJNAME_LEGDSERIES;

                        gr_chart_objid_fillstyleb_query(cp, intern_id, &fillstyleb);
                        gr_chart_objid_fillstylec_query(cp, intern_id, &fillstylec);

                        serp = getserp(cp, series_idx);

                        chart_type = (serp->chart_type != GR_CHART_TYPE_NONE)
                                  ? serp->chart_type
                                  : axesp->chart_type;

                        if(chart_type == GR_CHART_TYPE_BAR)
                        {
                            gr_chart_objid_linestyle_query(cp, intern_id, &linestyle);

                            if(!fillstyleb.bits.notsolid)
                                status_break(status = gr_diag_rectangle_new(p_gr_diag, NULL, intern_id, &rect_box, &linestyle, &fillstylec));

                            if(fillstyleb.bits.pattern)
                                status_break(status = gr_chart_scaled_picture_add(cp, intern_id, &rect_box, &fillstyleb, &fillstylec));
                        }
                        else
                        {
                            fillstyleb.bits.is_marker = 1;

                            if(cp->d3.bits.use && (chart_type == GR_CHART_TYPE_LINE))
                            {
                                /* draw a section of ribbon through the box */
                                line_box.y0 -= (rect_box.x1 - rect_box.x0) / 4;
                                line_box.y1 += (rect_box.x1 - rect_box.x0) / 4;

                                gr_chart_objid_linestyle_query(cp, intern_id, &linestyle);

                                if(!fillstyleb.bits.notsolid)
                                    status_break(status = gr_diag_rectangle_new(p_gr_diag, NULL, intern_id, &line_box, &linestyle, &fillstylec));
                            }
                            else
                            {
                                /* draw a centred line through the box */
                                if(chart_type == GR_CHART_TYPE_LINE)
                                    gr_point_linestyle_query(cp, gr_series_idx_from_external(cp, intern_id.no), 0, &linestyle, 1);
                                else
                                    gr_chart_objid_linestyle_query(cp, intern_id, &linestyle);

                                status_break(status = gr_chart_line_new(cp, intern_id, &line_box, &linestyle));
                            }

                            if(fillstyleb.bits.pattern)
                                status_break(status = gr_chart_scaled_picture_add(cp, intern_id, &pict_box, &fillstyleb, &fillstylec));
                        }

                        if(swidth_px)
                            status_break(status = gr_chart_text_new(cp, intern_id, &text_box, ustr_bptr(cv.data.text), &textstyle));
                    }

                    if(legend->bits.in_rows)
                        curpt.x = text_box.x1 + linespacing / 2;
                    else
                        curpt.y -= linespacing;
                }
            }

            break;
        }

        if(pass_out)
            gr_chart_group_end(cp, &legendGroupStart);

        status_break(status);
    }

    gr_riscdiag_host_font_dispose(&host_font);

    return(status);
}

/* add in plot area */

_Check_return_
static STATUS
gr_chart_plotarea_addin(
    _ChartRef_  P_GR_CHART cp)
{
    const P_GR_CHART_PLOTAREA plotarea = &cp->plotarea;
    const P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    GR_CHART_OBJID id;
    GR_BOX plotareabox;
    GR_DIAG_OFFSET plotGroupStart;
    GR_FILLSTYLEB fillstyleb;
    GR_FILLSTYLEC fillstylec;
    GR_LINESTYLE linestyle;
    STATUS status = STATUS_OK;

    /* SKS after PD 4.12 30mar92 - now individual plotareas don't inherit from each other, divorce selection grouping */
    status_return(gr_chart_group_new(cp, &plotGroupStart, gr_chart_objid_anon));

    gr_chart_objid_clear(&id);
    id.name = GR_CHART_OBJNAME_PLOTAREA;
    id.has_no = 1;
    id.no = GR_CHART_PLOTAREA_REAR;

    plotareabox.x0 = plotarea->posn.x;
    plotareabox.y0 = plotarea->posn.y;
    plotareabox.x1 = plotarea->posn.x + plotarea->size.x;
    plotareabox.y1 = plotarea->posn.y + plotarea->size.y;

    /* NB. never display wall and floor of box for pies or scatters! */

    gr_chart_objid_fillstyleb_query(cp, id, &fillstyleb);
    gr_chart_objid_fillstylec_query(cp, id, &fillstylec);
    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    if(!fillstyleb.bits.notsolid)
        status_return(gr_diag_rectangle_new(p_gr_diag, NULL, id, &plotareabox, &linestyle, &fillstylec));

    if(fillstyleb.bits.pattern)
        status_return(gr_chart_scaled_picture_add(cp, id, &plotareabox, &fillstyleb, &fillstylec));

    if(cp->d3.bits.use)
    {
        GR_POINT origin, ps1, ps2, ps3;

        /* wall */
        id.no = GR_CHART_PLOTAREA_WALL;

        gr_chart_objid_fillstylec_query(cp, id, &fillstylec);
        gr_chart_objid_linestyle_query(cp, id, &linestyle);

        origin.x = cp->plotarea.posn.x;
        origin.y = cp->plotarea.posn.y;

        ps1.x = cp->d3.cache.vector_full.x; /* -ve */
        ps1.y = cp->d3.cache.vector_full.y;

        ps2.x = ps1.x;
        ps2.y = ps1.y + plotarea->size.y;

        ps3.x = 0;
        ps3.y = plotarea->size.y;

        if(ps1.x)
            status_return(gr_diag_quadrilateral_new(p_gr_diag, NULL, id, &origin, &ps1, &ps2, &ps3, &linestyle, &fillstylec));

        /* floor */
        id.no = GR_CHART_PLOTAREA_FLOOR;

        gr_chart_objid_fillstylec_query(cp, id, &fillstylec);
        gr_chart_objid_linestyle_query(cp, id, &linestyle);

        ps2.x = ps1.x + plotarea->size.x;
        ps2.y = ps1.y;

        ps3.x = plotarea->size.x;
        ps3.y = 0;

        if(ps2.y)
            status_return(gr_diag_quadrilateral_new(p_gr_diag, NULL, id, &origin, &ps1, &ps2, &ps3, &linestyle, &fillstylec));
    }

    gr_chart_group_end(cp, &plotGroupStart);

    return(status);
}

#ifdef UNDEF

_Check_return_
static STATUS
gr_chart_save_draw_file_without_dialog(
    _InVal_     GR_CHART_HANDLE ch,
    _In_z_      PCTSTR filename)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);
    P_GR_RISCDIAG p_gr_riscdiag;
    PCTSTR save_filename;
    S32 res;

    PTR_ASSERT(cp);

    if(!cp->core.p_gr_diag)
        return(0);

    p_gr_riscdiag = cp->core.p_gr_diag->p_gr_riscdiag;
    if(NULL == p_gr_riscdiag)
        return(0);

    assert(filename || cp->core.currentdrawname);

    save_filename = filename ? filename : cp->core.currentdrawname;

    res = gr_riscdiag_diagram_save(p_gr_riscdiag, save_filename, FILETYPE_DRAW);

    if(res > 0)
    {
        if(host_xfer_saved_file_is_safe())
        {
            if(filename /*no point setting same back*/)
                res = gr_chart_name_set(cp->core.ch, save_filename);

#ifdef GR_CHART_SAVES_ONLY_DRAWFILE
            if(filename)
            {
                tstr_clr(&cp->core.currentfilename);
                tstr_set(&cp->core.currentfilename, save_filename);
            }

            cp->core.modified = 0;

            /*setwintitle(cp);*/
#endif

#if RISCOS && 0
            gr_chart_riscos_broadcast_changed(save_filename);
#endif
        }
    }

    return(res);
}

#endif

#if RISCOS && 0

static void
gr_chart_riscos_broadcast_changed(
    _In_z_      PCTSTR filename)
{
    WimpMessage msg;

    zero_struct(msg);
    msg.hdr.size = offsetof32(ExtendedWimpMessage, data.pd_dde.type.d);
  /*msg.hdr.my_ref = 0;*/ /* fresh msg */
    msg.hdr.action_code = wimp_MPD_DDE;

    msg.data.pd_dde.id = wimp_MPD_DDE_DrawFileChanged;

    tstr_xstrkpy(msg.data.pd_dde.type.d.leafname, elemof32(msg.data.pd_dde.type.d.leafname), filename);

    msg.hdr.size += tstrlen32p1(msg.data.pd_dde.type.d.leafname); /*CH_NULL*/;
    msg.hdr.size  = round_up(msg.hdr.size, 4);

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, 0 /*broadcast*/, BAD_WIMP_I, NULL));
}

#endif /* RISCOS */

typedef struct CHART_AUTOMAP
{
    IMAGE_CACHE_HANDLE image_cache_handle;
    GR_CHART_OBJID id;
    BOOL is_marker; /* as opposed to picture */
}
CHART_AUTOMAP, * P_CHART_AUTOMAP;

_Check_return_
static STATUS
cache_automap_for(
    _In_        GR_CHART_OBJID id,
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle /*extended*/,
    _OutRef_    P_IMAGE_CACHE_HANDLE p_image_cache_handle,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb)
{
    GR_CHART_OBJID parent_id = id;

    *p_image_cache_handle = IMAGE_CACHE_HANDLE_NONE;

    if(!*p_array_handle)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(CHART_AUTOMAP), 0);
        status_return(al_array_alloc_zero(p_array_handle, &array_init_block));
    }

    if(id.has_subno && gr_chart_objid_find_parent(&parent_id, FALSE) && fillstyleb->bits.varied_by_point)
    {
        ARRAY_INDEX i;
        BOOL new_entry = TRUE;

        for(i = 0; i < array_elements(p_array_handle); ++i)
        {
            P_CHART_AUTOMAP p_chart_automap = array_ptr(p_array_handle, CHART_AUTOMAP, i);

            if(p_chart_automap->is_marker == (BOOL) fillstyleb->bits.is_marker)
                if(gr_chart_objid_equal(&id, &p_chart_automap->id))
                {
                    if(p_chart_automap->image_cache_handle != IMAGE_CACHE_HANDLE_NONE)
                    {
                        *p_image_cache_handle = p_chart_automap->image_cache_handle;
                        return(1);
                    }

                    new_entry = FALSE;
                    break; /* scan for parent, this file is not on disk */
                }
        }

        if(new_entry)
        {
            TCHARZ leafname[BUF_MAX_PATHSTRING];
            IMAGE_CACHE_HANDLE image_cache_handle;
            TCHARZ buffer[BUF_MAX_PATHSTRING];
            P_CHART_AUTOMAP p_chart_automap;
            STATUS status = STATUS_OK;

            if(NULL == (p_chart_automap = al_array_extend_by(p_array_handle, CHART_AUTOMAP, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
                return(status);

            p_chart_automap->image_cache_handle = IMAGE_CACHE_HANDLE_NONE;
            p_chart_automap->id = id;
            p_chart_automap->is_marker = fillstyleb->bits.is_marker;

            consume_int(tstr_xsnprintf(leafname, elemof32(leafname),
                                       p_chart_automap->is_marker
                                            ? TEXT("Markers")  FILE_DIR_SEP_TSTR TEXT("PS") U32_TFMT FILE_DIR_SEP_TSTR TEXT("P") U32_TFMT
                                            : TEXT("Pictures") FILE_DIR_SEP_TSTR TEXT("PS") U32_TFMT FILE_DIR_SEP_TSTR TEXT("P") U32_TFMT,
                                       (U32) id.no, (U32) id.subno));

#if !RISCOS
            tstr_xstrkat(leafname, elemof32(leafname), DRAWFILE_EXTENSION_TSTR);
#endif

            buffer[0] = CH_NULL;

            if(cp)
            {
                CHART_AUTOMAP_HELP chart_automap_help;

                chart_automapper_help_name(cp->core.ext_handle, &chart_automap_help);

                if(file_dirname(buffer, chart_automap_help.wholename))
                {
                    tstr_xstrkat(buffer, elemof32(buffer), leafname);
                    status = file_is_file(buffer); /* STATUS_DONE iff found */
                }
            }

            if(STATUS_OK == status)
                status_return(status = file_find_on_path(buffer, elemof32(buffer), file_get_search_path(), leafname));

            if(status_done(status))
            {
                T5_FILETYPE t5_filetype = FILETYPE_DRAW; /* t5_filetype_from_extension(file_extension(buffer)); */

                status_return(image_cache_entry_ensure(&image_cache_handle, buffer, t5_filetype));

                if(!image_cache_loaded_ensure(image_cache_handle))
                    return(image_cache_error_query(image_cache_handle));

                image_cache_ref(image_cache_handle, 1);

                *p_image_cache_handle = p_chart_automap->image_cache_handle = image_cache_handle;
                return(1);
            }
        }

        id = parent_id; /* try point's parent */
    }

    {
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(p_array_handle); ++i)
    {
        P_CHART_AUTOMAP p_chart_automap = array_ptr(p_array_handle, CHART_AUTOMAP, i);

        if(p_chart_automap->is_marker == (BOOL) fillstyleb->bits.is_marker)
            if(gr_chart_objid_equal(&id, &p_chart_automap->id))
            {
                if(p_chart_automap->image_cache_handle != IMAGE_CACHE_HANDLE_NONE)
                {
                    *p_image_cache_handle = p_chart_automap->image_cache_handle;
                    return(1);
                }

                return(0 /*no mapping - already determined that corresponding file does not exist */);
            }
    }

    {
    TCHARZ leafname[BUF_MAX_PATHSTRING];
    IMAGE_CACHE_HANDLE image_cache_handle;
    TCHARZ buffer[BUF_MAX_PATHSTRING];
    P_CHART_AUTOMAP p_chart_automap;
    STATUS status = STATUS_OK;

    if(NULL == (p_chart_automap = al_array_extend_by(p_array_handle, CHART_AUTOMAP, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    p_chart_automap->image_cache_handle = IMAGE_CACHE_HANDLE_NONE;
    p_chart_automap->id = id;
    p_chart_automap->is_marker = fillstyleb->bits.is_marker;

    consume_int(tstr_xsnprintf(leafname, elemof32(leafname),
                               p_chart_automap->is_marker
                                    ? TEXT("Markers")  FILE_DIR_SEP_TSTR TEXT("S") U32_TFMT
                                    : TEXT("Pictures") FILE_DIR_SEP_TSTR TEXT("S") U32_TFMT,
                               (U32) id.no));

#if !RISCOS
    tstr_xstrkat(leafname, elemof32(leafname), DRAWFILE_EXTENSION_TSTR);
#endif

    buffer[0] = CH_NULL;

    if(cp)
    {
        CHART_AUTOMAP_HELP chart_automap_help;

        chart_automapper_help_name(cp->core.ext_handle, &chart_automap_help);

        if(file_dirname(buffer, chart_automap_help.wholename))
        {
            tstr_xstrkat(buffer, elemof32(buffer), leafname);
            status = file_is_file(buffer); /* STATUS_DONE iff found */
        }
    }

    if(STATUS_OK == status)
        status_return(status = file_find_on_path(buffer, elemof32(buffer), file_get_search_path(), leafname));

    if(status_done(status))
    {
        T5_FILETYPE t5_filetype = FILETYPE_DRAW; /* t5_filetype_from_extension(file_extension(buffer)); */

        status_return(image_cache_entry_ensure(&image_cache_handle, buffer, t5_filetype));

        if(!image_cache_loaded_ensure(image_cache_handle))
            return(image_cache_error_query(image_cache_handle));

        image_cache_ref(image_cache_handle, 1);

        *p_image_cache_handle = p_chart_automap->image_cache_handle = image_cache_handle;
        return(1);
    }
    } /*block*/

    return(0 /*no mapping - file does not exist */);
    } /*block*/
}

static ARRAY_HANDLE
h_automapper_global;

extern void
chart_automapper_del(
    P_ARRAY_HANDLE p_array_handle)
{
    if(P_DATA_NONE == p_array_handle)
        p_array_handle = &h_automapper_global;

    {
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(p_array_handle); ++i)
    {
        P_CHART_AUTOMAP p_chart_automap = array_ptr(p_array_handle, CHART_AUTOMAP, i);

        image_cache_ref(p_chart_automap->image_cache_handle, 0);
    }
    } /*block*/

    al_array_dispose(p_array_handle);
}

_Check_return_
extern STATUS
gr_chart_scaled_picture_add(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InRef_     PC_GR_BOX box,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb,
    _InRef_     PC_GR_FILLSTYLEC fillstylec)
{
    IMAGE_CACHE_HANDLE image_cache_handle = (IMAGE_CACHE_HANDLE) fillstyleb->pattern;

    if(fillstyleb->bits.pattern && (image_cache_handle == IMAGE_CACHE_HANDLE_NONE))
    {
        STATUS status = cache_automap_for(id, cp, chart_automapper_help_handle(cp->core.ext_handle), &image_cache_handle, fillstyleb);

        if(status == 0)
            /* invoke global automapper */
            status = cache_automap_for(id, NULL, &h_automapper_global, &image_cache_handle, fillstyleb);

        if(status <= 0)
            return(status);
    }

    return(gr_diag_scaled_picture_add(cp->core.p_gr_diag, NULL, id, box, image_cache_handle, fillstyleb, fillstylec));
}

/* end of gr_build.c */
