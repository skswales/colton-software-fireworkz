/* gr_piesg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Pie charts */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

_Check_return_
static inline F64 /*radians*/
reduce_into_range(
    _In_        F64 alpha /*radians*/)
{
    while(alpha > _pi)
        alpha -= _two_pi;

    while(alpha < -_pi)
        alpha += _two_pi;

    return(alpha);
}

_Check_return_
static F64 /*radians*/
conv_heading_to_angle(
    _In_        F64 heading_degrees)
{
    F64 heading_radians, alpha;

    /* sanitise input degrees into [0,360) */
    while(heading_degrees >= 360.0)
        heading_degrees -= 360.0;

    while(heading_degrees < 0.0)
        heading_degrees += 360.0;

    heading_radians = heading_degrees * _radians_per_degree;

    /* if heading <= 90 or  >= 270 it's +ve between 0 and pi */
    /* if heading >  90 and <  270 it's -ve between 0 and pi */
    alpha = _pi_div_two - heading_radians;

    alpha = reduce_into_range(alpha);

    trace_2(TRACE_MODULE_GR_CHART, TEXT("conv_heading_to_angle(%g degrees) yields %g radians"), heading_degrees, alpha);

    return(alpha);
}

/******************************************************************************
*
* construct a pie chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_pie_addin(
    _ChartRef_  P_GR_CHART cp)
{
    const P_GR_DIAG p_gr_diag = cp->core.p_gr_diag;
    const GR_SERIES_IDX series_idx = 0;
    const P_GR_SERIES serp = getserp(cp, series_idx);
    GR_CHART_OBJID serid;
    GR_POINT origin, thisOrigin;
    GR_PIECHDISPLSTYLE piechdisplstyle;
    GR_PIECHLABELSTYLE piechlabelstyle;
    F64 pct_radial_disp_max, point_radial_displacement, base_radial_displacement;
    GR_PIXIT radius;
    GR_POINT_NO point, n_points;
    STATUS neg_or_zero;
    F64 total, value;
    GR_CHART_OBJID id;
    GR_DIAG_OFFSET pieStart;
    GR_DATASOURCE_HANDLE dsh;
    F64 alpha, beta, bisector;
    GR_LINESTYLE linestyle;
    GR_FILLSTYLEC fillstylec;
    GR_TEXTSTYLE textstyle;
    BOOL labelling;
    STATUS status = STATUS_OK;
    int pass;

    /* always centre in plot area */
    origin.x = cp->plotarea.posn.x + cp->plotarea.size.x / 2;
    origin.y = cp->plotarea.posn.y + cp->plotarea.size.y / 2;

    /* query point 1 for base textstyle for pie */
    gr_point_textstyle_query(cp, series_idx, 1, &textstyle);

    /* use about 90% of the available area */
    radius = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / 2;
    if( radius  > textstyle.size_y)
        radius -= textstyle.size_y;
    radius *= 90;
    radius /= 100;

    /* note that there is no point using gr_travel_total_n_items as
     * i)  there is only one series being plotted
     * ii) if there were more category labels than values in S1
     *     then the values would all be zero anyhow
    */
    dsh = gr_travel_series_dsh_from_ds(cp, series_idx, 0);

    n_points = (dsh == GR_DATASOURCE_HANDLE_NONE) ? 0 : gr_travel_dsh_n_items(cp, dsh);

    /* find how we are going to slice up the pie */
    neg_or_zero = 0;
    total = 0.0;
    pct_radial_disp_max = 0.0;

    for(point = 0; point < n_points; ++point)
    {
        if(status_done(gr_travel_dsh_valof(cp, dsh, point, &value)))
        {
            if(value > 0.0)
            {
                total += value;
                gr_point_piechdisplstyle_query(cp, series_idx, point, &piechdisplstyle);
                pct_radial_disp_max = MAX(pct_radial_disp_max, piechdisplstyle.radial_displacement);
            }
            else
                neg_or_zero = 1;
        }
    }

    if(total == 0.0)
    {
        gr_chart_warning(cp, CHART_ERR_NOT_ENOUGH_INPUT_PIE);
        return(status);
    }

    if(neg_or_zero)
        gr_chart_warning(cp, CHART_ERR_NEGATIVE_OR_ZERO_IGNORED_PIE);

    base_radial_displacement = serp->style.point_piechdispl.radial_displacement;

    pct_radial_disp_max += base_radial_displacement;

    gr_chart_objid_from_series_idx(cp, series_idx, &serid);

    status_return(gr_chart_group_new(cp, &pieStart, serid));

    id = serid;
    id.name = GR_CHART_OBJNAME_POINT;
    id.has_subno = 1;

    /* reduce radius correspondingly */
    radius = (GR_PIXIT) (radius / (1.0 + pct_radial_disp_max / 100.0));

    for(pass = 1; pass <= 2; ++pass)
    {
        alpha = conv_heading_to_angle(serp->style.pie_start_heading_degrees);

        for(point = 0; point < n_points; ++point)
        {
            id.subno = UBF_PACK(gr_point_external_from_key(point));

            if(status_done(gr_travel_dsh_valof(cp, dsh, point, &value)) && (value > 0.0))
            {
                BOOL output_segment = TRUE;
                F64 fraction = value / total;
                F64 theta = _two_pi * fraction;

                if(serp->bits.pie_anticlockwise)
                    beta = alpha + theta;
                else
                    beta = alpha - theta;

                bisector = (alpha + beta) / 2;

                gr_point_fillstylec_query(cp, series_idx, point, &fillstylec);
                gr_point_linestyle_query(cp, series_idx, point, &linestyle, 0);

                gr_point_piechdisplstyle_query(cp, series_idx, point, &piechdisplstyle);
                gr_point_piechlabelstyle_query(cp, series_idx, point, &piechlabelstyle);

                point_radial_displacement = piechdisplstyle.radial_displacement + base_radial_displacement;

                labelling = piechlabelstyle.bits.label_leg | piechlabelstyle.bits.label_val | piechlabelstyle.bits.label_pct;

                thisOrigin = origin;

                if(point_radial_displacement != 0.0)
                {
                    thisOrigin.x += (GR_PIXIT) (radius * point_radial_displacement / 100.0 * cos(bisector));
                    thisOrigin.y += (GR_PIXIT) (radius * point_radial_displacement / 100.0 * sin(bisector));
                }

                /* SKS 27jul95 attempts to keep big segments at back so little ones are selectable in crude bbox scheme */
                if(fraction < 0.25)
                {
                    if(pass == 1)
                        /* ignore small segments on pass 1 */
                        output_segment = FALSE;
                }
                else
                {
                    if(pass == 2)
                        /* ignore large segments on pass 2 */
                        output_segment = FALSE;
                }

                if(output_segment)
                {
                    GR_DIAG_OFFSET pointStart = 0;

                    if(labelling)
                        status_break(status = gr_chart_group_new(cp, &pointStart, id));

                    status_break(status = gr_diag_piesector_new(p_gr_diag, NULL, id, &thisOrigin, radius,
                                                                fmin(alpha, beta), fmax(alpha, beta),
                                                                &linestyle, &fillstylec));

                    if(labelling)
                    {
                        GR_CHART_VALUE cv;
                        GR_MILLIPOINT swidth_mp;
                        GR_PIXIT swidth_px;

                        if(piechlabelstyle.bits.label_val)
                            gr_travel_dsh_label(cp, dsh, point, &cv);
                        else if(piechlabelstyle.bits.label_pct)
                        {
                            F64 label_value = value;
                            NUMFORM_PARMS numform_parms;
                            SS_DATA ss_data;
                            QUICK_UBLOCK quick_ublock;
                            quick_ublock_setup(&quick_ublock, cv.data.text);

                            cv.data.text[0] = CH_NULL;

                            /* convert value into %ge value */
                            label_value *= 100.0; /* care with order else 14.0 -> 0.14 -> ~14.0 */
                            label_value /= total; /* ALWAYS in 0.0 - 100.0 */

                            zero_struct(numform_parms);
                            numform_parms.ustr_numform_numeric =
                                (floor(label_value) == label_value)
                                    ? USTR_TEXT("0\\%")
                                    : USTR_TEXT("0.00\\%");

                            ss_data_set_real(&ss_data, label_value);

                            status = numform(&quick_ublock, P_QUICK_TBLOCK_NONE, &ss_data, &numform_parms);

                            quick_ublock_dispose_leaving_buffer_valid(&quick_ublock);
                        }
                        else
                            gr_travel_categ_label(cp, point, &cv);

                        status_break(status);

                        gr_point_textstyle_query(cp, series_idx, point, &textstyle);

                        {
                        HOST_FONT host_font = gr_riscdiag_host_font_from_textstyle(&textstyle);
                        swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(cv.data.text));
                        gr_riscdiag_host_font_dispose(&host_font);
                        } /*block*/

                        swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

                        if(swidth_px)
                        {
                            GR_PIXIT width  = swidth_px;
                            GR_PIXIT height = textstyle.size_y;
                            GR_BOX text_box;

                            /* move further out relative to actual centre of sector rather than centre of pie */
                            text_box.x0 = thisOrigin.x + (GR_PIXIT) (radius * 1.10 * cos(bisector));
                            text_box.y0 = thisOrigin.y + (GR_PIXIT) (radius * 1.10 * sin(bisector));

                            /* shift slightly down to allow for fonts being higher above baseline than deep below it */
                            text_box.y0 -= height / 4;

                            /* place differently according to whether on right or left of centre vertical */
                            if(fabs(reduce_into_range(bisector)) <= _pi_div_two)
                            {
                                text_box.x1 = text_box.x0 + width;
                            }
                            else
                            {
                                /* move start point further out to left */
                                text_box.x1 = text_box.x0;
                                text_box.x0 = text_box.x1 - width;
                            }

                            text_box.y1 = text_box.y0 + (height * 12) / 10;

                            status_break(status = gr_chart_text_new(cp, id, &text_box, ustr_bptr(cv.data.text), &textstyle));
                        }
                    }

                    if(pointStart)
                        gr_chart_group_end(cp, &pointStart);
                }

                /* use this angle as the next segment's start angle */
                alpha = reduce_into_range(beta);
            }
        }
    }

    gr_chart_group_end(cp, &pieStart);

    return(status);
}

/* end of gr_piesg.c */
