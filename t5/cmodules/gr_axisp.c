/* gr_axisp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart axis handling */

/* SKS November 1991 / Fireworkz world conversion April 1993 */

#include "common/gflags.h"

#define EXPOSE_RISCOS_FONT 1

#include "ob_chart/ob_chart.h"

#define AXIS_RATIO_SILLY_MIN 0.02 /* SKS 25jul97 be a bit more lenient about silly graph labelling */
#define AXIS_RATIO_SILLY_MAX 50.0

/******************************************************************************
*
* convert an axes,axis index pair into an external axes number
*
******************************************************************************/

_Check_return_
extern GR_EAXES_NO
gr_axes_external_from_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx)
{
    PTR_ASSERT(cp);

    if(cp->d3.bits.use)
        return((axes_idx * 3) + axis_idx + 1);

    return((axes_idx * 2) + axis_idx + 1);
}

/******************************************************************************
*
* convert an external axes number into an axes,axis index pair
*
******************************************************************************/

/*ncr*/
extern GR_AXIS_IDX
gr_axes_idx_from_external(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_EAXES_NO eaxes_no,
    _OutRef_    P_GR_AXES_IDX p_axes_idx)
{
    const GR_AXES_IDX axes_idx = (eaxes_no - 1);

    PTR_ASSERT(cp);

    myassert0x(eaxes_no > 0, TEXT("external axes <= 0"));
   
    if(cp->d3.bits.use)
    {
        myassert0x(eaxes_no <= 6, TEXT("external axes id >= 6"));
        *p_axes_idx = (axes_idx >= 3);
        return(axes_idx - (*p_axes_idx * 3));
    }

    myassert0x(eaxes_no <= 4, TEXT("external axes id >= 4"));
    *p_axes_idx = (axes_idx >= 2);
    return(axes_idx - (*p_axes_idx * 2));
}

/******************************************************************************
*
* return the axes index of a series index
*
******************************************************************************/

_Check_return_
extern GR_AXES_IDX
gr_axes_idx_from_series_idx(
    _InRef_     PC_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    return(series_idx >= cp->axes[0].series.end_idx);
}

/******************************************************************************
*
* return the axes ptr of a series index
*
******************************************************************************/

_Check_return_
extern P_GR_AXES
gr_axesp_from_series_idx(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    GR_AXES_IDX axes_idx = (series_idx >= cp->axes[0].series.end_idx);

    return(&cp->axes[axes_idx]);
}

_Check_return_
extern P_GR_AXIS
gr_axisp_from_external(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_EAXES_NO eaxes_no)
{
    GR_AXES_IDX axes_idx;
    GR_AXIS_IDX axis_idx = gr_axes_idx_from_external(cp, eaxes_no, &axes_idx);

    return(&cp->axes[axes_idx].axis[axis_idx]);
}

_Check_return_
static STATUS
gr_numtopowstr(
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_F64 evalue,
    _InRef_     PC_F64 ebase,
    _InVal_     BOOL log_scale,
    _In_        BOOL log_label)
{
    F64 value = *evalue;

    if(value < 0.0)
    {
        F64 abs_value = fabs(value);
        PtrPutByte(buffer, CH_MINUS_SIGN__BASIC);
        return(gr_numtopowstr(ustr_AddBytes_wr(buffer, 1), elemof_buffer - 1, &abs_value, ebase, log_scale, log_label));
    }

    if((value >= U32_MAX) || (value < 1E-4))
        log_label = TRUE;

    if(value == 0.0)
        log_label = FALSE;

    if(log_label)
    {
        F64 base = *ebase;
        F64 lnz = log(value) / log(base);
        F64 exponent;
        F64 mantissa = splitlognum(&lnz, &exponent);

        if((mantissa == 0.0 /*log(1.0)*/) && log_scale)
            consume_int(ustr_xsnprintf(buffer, elemof_buffer, USTR_TEXT(S32_FMT "^" S32_FMT), (S32) base, (S32) exponent));
        else
        {
            mantissa = pow(base, mantissa);

#if USTR_IS_SBSTR
            /* use Latin-1 'times' character */
            consume_int(ustr_xsnprintf(buffer, elemof_buffer, USTR_TEXT("%g" "\xD7"     S32_FMT "^" S32_FMT), mantissa, (S32) base, (S32) exponent));
#else
            /* use Unicode 'times' character */
            consume_int(ustr_xsnprintf(buffer, elemof_buffer, USTR_TEXT("%g" "\xC3\x97" S32_FMT "^" S32_FMT), mantissa, (S32) base, (S32) exponent));
#endif
        }
    }
    else
        consume_int(ustr_xsnprintf(buffer, elemof_buffer, USTR_TEXT("%g"), value));

    /* convert output iff necessary */
    if(CH_FULL_STOP != g_ss_recog_context.decimal_point_char)
    {
        P_USTR ustr_dp = ustrchr(buffer, CH_FULL_STOP);

        if(NULL != ustr_dp)
        {
#if !USTR_IS_SBSTR
            U32 bytes_of_char = uchars_bytes_of_char_encoding(g_ss_recog_context.decimal_point_char);
            if(bytes_of_char > 1)
            {   /* make space for a longer replacement character */
                PC_USTR ustr_tail = ustr_AddBytes(ustr_dp, 1);
                const U32 tail_bytes = ustrlen32p1(ustr_tail);
                memmove32(ustr_AddBytes_wr(ustr_dp, bytes_of_char), ustr_tail, tail_bytes);
            }
#endif
            (void) uchars_char_encode(ustr_dp, elemof_buffer - PtrDiffBytesU32(ustr_dp, buffer), g_ss_recog_context.decimal_point_char);
        }
    }

    return(STATUS_OK);
}

#define N_NUMFORM_DECIMALS 12

static UCHARZ numform_numeric_ustr_buf[32]; /* must use persistent format along axis! */

_Check_return_
static STATUS
gr_numtonumstr(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InRef_     PC_F64 iter_val)
{
    NUMFORM_PARMS numform_parms;
    EV_DATA ev_data;

    zero_struct(numform_parms);
    numform_parms.ustr_numform_numeric = ustr_bptr(numform_numeric_ustr_buf);

    ev_data_set_real(&ev_data, *iter_val);

    return(numform(p_quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms));
}

/* work out the numform format to use for this axis */

static void
gr_numtonumstr_init(
    _InRef_     PC_F64 iter_val_min,
    _InRef_     PC_F64 iter_val_max,
    _InVal_     unsigned int decimals)
{
    F64 fabs_iter_val_min = fabs(*iter_val_min);
    F64 fabs_iter_val_max = fabs(*iter_val_max);
    F64 max_abs = MAX(fabs_iter_val_min, fabs_iter_val_max);

    if((max_abs >= U32_MAX) || (decimals >= N_NUMFORM_DECIMALS))
    {
        /*                       0123456789 */
        ustr_xstrkpy(ustr_bptr(numform_numeric_ustr_buf), sizeof32(numform_numeric_ustr_buf), USTR_TEXT("0.x00e+00"));
        numform_numeric_ustr_buf[2] = g_ss_recog_context.decimal_point_char;
    }
    else if(decimals > 0)
    {
        /*                       01234567890123456789 */
        ustr_xstrkpy(ustr_bptr(numform_numeric_ustr_buf), sizeof32(numform_numeric_ustr_buf), USTR_TEXT("#,x##0.x000000000000"));
        assert(strlen(numform_numeric_ustr_buf) == 8+N_NUMFORM_DECIMALS);
        numform_numeric_ustr_buf[2] = g_ss_recog_context.thousands_char;
        numform_numeric_ustr_buf[7] = g_ss_recog_context.decimal_point_char;
        numform_numeric_ustr_buf[8+decimals] = CH_NULL;
    }
    else
    {
        /*                       0123456789 */
        ustr_xstrkpy(ustr_bptr(numform_numeric_ustr_buf), sizeof32(numform_numeric_ustr_buf), USTR_TEXT("#,x##0"));
        numform_numeric_ustr_buf[2] = g_ss_recog_context.thousands_char;
    }
}

/******************************************************************************
*
* helper routines for looping along axes for grids, labels and ticks
*
******************************************************************************/

typedef struct GR_AXIS_ITERATOR
{
    F64 iter, step;

    F64 mantissa; /* for log scaling */
    F64 exponent;
    F64 base;
}
GR_AXIS_ITERATOR, * P_GR_AXIS_ITERATOR;

static void
gr_axis_iterator_renormalise_log(
    /*inout*/ P_GR_AXIS_ITERATOR p_iter)
{
    F64 lna = log(p_iter->base);
    F64 lnz = log(p_iter->iter);

    lnz = lnz / lna;

    lnz = splitlognum(&lnz, &p_iter->exponent);

    p_iter->mantissa = pow(p_iter->base, lnz);

    p_iter->iter = p_iter->mantissa * pow(p_iter->base, p_iter->exponent);
}

_Check_return_
static BOOL
gr_axis_iterator_first(
    _InRef_     PC_GR_AXIS p_axis,
    _InVal_     BOOL major,
    _InoutRef_  P_GR_AXIS_ITERATOR p_iter)
{
    IGNOREPARM_InVal_(major);

    p_iter->iter = p_axis->current.min;

    if(p_axis->bits.log_scale)
    {
        p_iter->base = p_axis->bits.log_base;

        gr_axis_iterator_renormalise_log(p_iter);
    }
    else
        p_iter->base = 10;

    return(TRUE);
}

static void
gr_axis_iterator_last(
    _InRef_     PC_GR_AXIS p_axis,
    _InVal_     BOOL major,
    _InoutRef_  P_GR_AXIS_ITERATOR p_iter)
{
    IGNOREPARM_InVal_(major);

    p_iter->iter = p_axis->current.max;

    if(p_axis->bits.log_scale)
    {
        p_iter->base = p_axis->bits.log_base;

        gr_axis_iterator_renormalise_log(p_iter);
    }
}

_Check_return_
static BOOL
gr_axis_iterator_next(
    _InRef_     PC_GR_AXIS p_axis,
    _InVal_     BOOL major,
    _InoutRef_  P_GR_AXIS_ITERATOR p_iter)
{
    if(p_axis->bits.log_scale)
    {
        if(major)
            p_iter->mantissa *= p_iter->step; /* use step as multiplier */
        else
            p_iter->mantissa += p_iter->step; /* use step as adder */

        p_iter->iter = p_iter->mantissa * pow(p_iter->base, p_iter->exponent);

        if(p_iter->mantissa >= p_iter->base)
            gr_axis_iterator_renormalise_log(p_iter);
    }
    else
    {
        p_iter->iter += p_iter->step;

        /* SKS after PD 4.12 26mar92 - correct for very small FP rounding errors (wouldn't loop up to 3.0 in 0.1 steps) */
        if(fabs(p_iter->iter - p_axis->current.max) / p_iter->step < 0.000244140625) /* 2^-12 */
        {
            p_iter->iter = p_axis->current.max;
            return(TRUE);
        }
    }

    return(p_iter->iter <= p_axis->current.max);
}

/* an empirically derived aesthetic ratio. argue with SKS */
#define TICKLEN_FRAC 64

/******************************************************************************
*
* category axis
*
******************************************************************************/

/*
category axis - major and minor grids
*/

_Check_return_
static STATUS
gr_axis_addin_category_grids(
    _ChartRef_  P_GR_CHART cp,
    _In_        GR_CHART_OBJID id /*may mutate*/,
    _In_        GR_POINT_NO total_n_points /*may mutate*/,
    _InVal_     GR_PIXIT axis_ypos,
    _InVal_     BOOL front,
    _InVal_     BOOL major)
{
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET gridStart;
    GR_POINT_NO point;
    int step;
    GR_LINESTYLE linestyle;
    BOOL draw_main;
    BOOL doing_line_chart = (cp->axes[0].chart_type == GR_CHART_TYPE_LINE);

    if(!ticksp->bits.grid)
        return(status);

    step = (int) ticksp->current;
    if(!step)
        return(status);

    if(front)
    {
        switch(p_axis->bits.arf)
        {
        default:
        case GR_AXIS_POSITION_AUTO:
            return(status);

        case GR_AXIS_POSITION_FRONT:
            draw_main = 1;
            break;

        case GR_AXIS_POSITION_REAR:
            return(status);
        }
    }
    else
    {
#if 1
        /* I now argue that bringing the axis to the front
         * shouldn't affect rear grid drawing state
        */
        draw_main = 1;
#else
        switch(p_axis->bits.arf)
        {
        default:
        case GR_AXIS_POSITION_AUTO:
            draw_main = 1;
            break;

        case GR_AXIS_POSITION_FRONT:
            return(status);

        case GR_AXIS_POSITION_REAR:
            draw_main = 1;
            break;
        }
#endif
    }

    gr_chart_objid_from_axis_grid(&id, major);

    status_return(gr_chart_group_new(cp, &gridStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    if(doing_line_chart)
        if(total_n_points > 0)
            --total_n_points;

    for(point = 0; point <= total_n_points; point += step)
    {
        GR_PIXIT x_pos = gr_categ_pos(cp, point);
        GR_PIXIT y_pos = 0;
        GR_BOX line_box;

        x_pos += cp->plotarea.posn.x;
        y_pos += cp->plotarea.posn.y;

        /* line charts require category ticks centring in slot or so they say */
        if(doing_line_chart)
            x_pos += cp->barlinech.cache.cat_group_width / 2;

        if(draw_main)
        {
            /* vertical grid line up the entire y span */
            line_box.x0 = x_pos;
            line_box.y0 = y_pos;

            line_box.x1 = line_box.x0;
            line_box.y1 = line_box.y0 + cp->plotarea.size.y;

            /* map together to front or back of 3-D world */
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);
            gr_map_point((P_GR_POINT) &line_box.x1, cp, fob);

            status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));

            if(cp->d3.bits.use && front)
            {
                /* diagonal grid lines across the top too ONLY DURING THE FRONT PHASE */
                line_box.x0 = x_pos;
                line_box.y0 = y_pos + cp->plotarea.size.y;

                /* put one to the front and one to the back */
                gr_map_box_front_and_back(&line_box, cp);

                status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
            }
        }

        if(cp->d3.bits.use && !front)
        {
            /* diagonal grid lines across the midplane and floor too ONLY DURING THE REAR PHASE */
            line_box.x0 = x_pos;
            line_box.y0 = y_pos;

            /* one to the front and one to the back */
            gr_map_box_front_and_back(&line_box, cp);

            status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));

            if((axis_ypos != 0) && (axis_ypos != cp->plotarea.size.y))
            {
                /* grid line across middle (simply shift) */
                line_box.y0 += axis_ypos;
                line_box.y1 += axis_ypos;

                status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
            }
        }
    }

    gr_diag_group_end(cp->core.p_gr_diag, &gridStart);

    return(status);
}

/*ncr*/
static U32
gr_axis_ticksizes(
    _ChartRef_  P_GR_CHART cp,
    _InRef_     PC_GR_AXIS_TICKS ticksp,
    _OutRef_    P_GR_PIXIT p_top /* or p_right */,
    _OutRef_    P_GR_PIXIT p_bottom /* or p_left  */)
{
    GR_PIXIT ticksize = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / TICKLEN_FRAC;

    switch(ticksp->bits.tick)
    {
    case GR_AXIS_TICK_POSITION_NONE:
        *p_top = 0;
        *p_bottom = 0;
        return(0);

    case GR_AXIS_TICK_POSITION_HALF_TOP:
        *p_top = ticksize;
        *p_bottom = 0;
        break;

    case GR_AXIS_TICK_POSITION_HALF_BOTTOM:
        *p_top = 0;
        *p_bottom = ticksize;
        break;

    default: default_unhandled();
#if CHECKING
    case GR_AXIS_TICK_POSITION_FULL:
#endif
        *p_top = ticksize;
        *p_bottom = ticksize;
        break;
    }

    return(1);
}

/*
category axis - labels
*/

_Check_return_
static STATUS
gr_axis_addin_category_labels(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InVal_     GR_POINT_NO total_n_points,
    _InVal_     GR_PIXIT axis_ypos,
    _InVal_     BOOL front)
{
/*  const BOOL major = 1; */
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = &p_axis->major;
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET labelStart;
    GR_POINT_NO point;
    int step;
    GR_PIXIT ticksize_top, ticksize_bottom;
    GR_TEXTSTYLE textstyle;
    HOST_FONT host_font;
    GR_MILLIPOINT gwidth_mp;
    GR_MILLIPOINT available_width_mp;
    GR_PIXIT available_width_px;

    step = (int) ticksp->current;
    if(!step)
        return(status);

    status_return(gr_chart_group_new(cp, &labelStart, id));

    (void) gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom);

    gr_chart_objid_textstyle_query(cp, id, &textstyle);

    host_font = gr_riscdiag_host_font_from_textstyle(&textstyle);

    gwidth_mp = cp->barlinech.cache.cat_group_width * GR_MILLIPOINTS_PER_PIXIT;

    available_width_px = (cp->barlinech.cache.cat_group_width * step); /* text covers 1..step categories */
    available_width_mp = available_width_px * GR_MILLIPOINTS_PER_PIXIT;

    for(point = 0; point <  total_n_points /* not <= as never have category label for end tick! */; point += step)
    {
        GR_PIXIT x_pos = gr_categ_pos(cp, point);
        GR_BOX text_box;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;

        text_box.x0  = x_pos;
        text_box.y0  = axis_ypos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        gr_travel_categ_label(cp, point, &cv);

        if(!strlen(cv.data.text))
            continue;

        swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(cv.data.text));

        /* always centre within one cat group */
        if(swidth_mp < available_width_mp)
        {
            text_box.x0 += (available_width_mp - swidth_mp) / (GR_MILLIPOINTS_PER_PIXIT * 2);
        }
        else
        {   /* truncate the little bugger to AVAILABLE WIDTH then centre */
            swidth_mp = gr_host_font_string_truncate(host_font, ustr_bptr(cv.data.text), available_width_mp);

            text_box.x0 += (available_width_mp - swidth_mp) / (GR_MILLIPOINTS_PER_PIXIT * 2);
        }

        switch(p_axis->bits.lzr)
        {
        case GR_AXIS_POSITION_TOP:
            text_box.y0 += (3 * ticksize_top) / 2;
            text_box.y0 += (1 * UBF_UNPACK(GR_PIXIT, textstyle.height)) / 4; /* st. descenders don't crash into ticks */
            break;

        default:
            text_box.y0 -= (3 * ticksize_bottom) / 2;
            text_box.y0 -= (4 * UBF_UNPACK(GR_PIXIT, textstyle.height)) / 4; /* st. ascenders don't crash into ticks (SKS after 1.05 25oct93 changed first 3 to 4) */
            break;
        }

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &text_box.x0, cp, fob);

        text_box.x1 = text_box.x0 + available_width_mp; /* text covers 1..step categories */
        text_box.y1 = text_box.y0 + textstyle.height;

        status_break(status = gr_diag_text_new(cp->core.p_gr_diag, NULL, id, &text_box, cv.data.text, &textstyle));
    }

    gr_riscdiag_host_font_dispose(&host_font);

    gr_diag_group_end(cp->core.p_gr_diag, &labelStart);

    return(status);
}

/*
category axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_category_ticks(
    _ChartRef_  P_GR_CHART cp,
    _In_        GR_CHART_OBJID id /*may mutate*/,
    _In_        GR_POINT_NO total_n_points /*may mutate*/,
    _In_        GR_PIXIT axis_ypos /*may mutate*/,
    _InVal_     BOOL front,
    _InVal_     BOOL major)
{
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET tickStart;
    GR_POINT_NO point;
    int step;
    GR_PIXIT ticksize, ticksize_top, ticksize_bottom;
    GR_LINESTYLE linestyle;
    BOOL doing_line_chart = (cp->axes[0].chart_type == GR_CHART_TYPE_LINE);

    step = (int) ticksp->current;
    if(!step)
        return(status);

    if(0 == gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom))
        return(status);

    if(!major)
    {
        ticksize_top    /= 2;
        ticksize_bottom /= 2;
    }

    axis_ypos -= ticksize_bottom;
    ticksize = ticksize_top + ticksize_bottom;

    gr_chart_objid_from_axis_tick(&id, major);

    status_return(gr_chart_group_new(cp, &tickStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    if(doing_line_chart)
        if(total_n_points > 0)
            --total_n_points;

    for(point = 0; point <= total_n_points; point += step)
    {
        GR_PIXIT x_pos = gr_categ_pos(cp, point);
        GR_BOX line_box;

        /* line charts require category ticks centring in slot or so they say */
        if(doing_line_chart)
            x_pos += cp->barlinech.cache.cat_group_width / 2;

        line_box.x0  = x_pos;
        line_box.y0  = axis_ypos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0 + ticksize;

        status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
    }

    gr_diag_group_end(cp->core.p_gr_diag, &tickStart);

    return(status);
}

/******************************************************************************
*
* category axis
*
******************************************************************************/

_Check_return_
extern STATUS
gr_axis_addin_category(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points,
    _InVal_     BOOL front)
{
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET axisStart;
    GR_PIXIT axis_ypos;
    BOOL draw_main;
    GR_CHART_OBJID id;

    switch(p_axis->bits.lzr)
    {
    case GR_AXIS_POSITION_TOP:
        axis_ypos = cp->plotarea.size.y;
        break;

    case GR_AXIS_POSITION_ZERO:
        axis_ypos = (GR_PIXIT) ((F64) cp->plotarea.size.y *
                                cp->axes[axes_idx].axis[Y_AXIS_IDX].zero_frac); /*NB*/
        break;

    default: default_unhandled();
#if CHECKING
    case GR_AXIS_POSITION_BOTTOM:
#endif
        axis_ypos = 0;
        break;
    }

    switch(p_axis->bits.arf)
    {
    case GR_AXIS_POSITION_FRONT:
    atfront:;
        draw_main = (front == 1);
        break;

    default: default_unhandled();
#if CHECKING
    case GR_AXIS_POSITION_AUTO:
#endif
        if(cp->d3.bits.use)
            if(axis_ypos != cp->plotarea.size.y)
                goto atfront;

        /*FALLTHRU*/

    case GR_AXIS_POSITION_REAR:
        draw_main = (front == 0);
        break;
    }

    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &id);

    status_return(gr_chart_group_new(cp, &axisStart, id));

    /* minor grids */
    status_return(gr_axis_addin_category_grids(cp, id, total_n_points, axis_ypos, front, 0));

    /* major grids */
    status_return(gr_axis_addin_category_grids(cp, id, total_n_points, axis_ypos, front, 1));

    if(draw_main)
    {

        {
        GR_BOX line_box;
        GR_LINESTYLE linestyle;

        /* axis line */
        line_box.x0  = 0;
        line_box.y0  = axis_ypos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1  = line_box.x0 + cp->plotarea.size.x;
        line_box.y1  = line_box.y0;

        gr_chart_objid_linestyle_query(cp, id, &linestyle);

        status_return(gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
        } /*block*/

        /* minor ticks */
        status_return(gr_axis_addin_category_ticks(cp, id, total_n_points, axis_ypos, front, 0));

        /* major ticks */
        status_return(gr_axis_addin_category_ticks(cp, id, total_n_points, axis_ypos, front, 1));

        /* labels */
        status_return(gr_axis_addin_category_labels(cp, id, total_n_points, axis_ypos, front));
    }

    gr_diag_group_end(cp->core.p_gr_diag, &axisStart);

    return(status);
}

/******************************************************************************
*
* value X & Y axes
*
******************************************************************************/

/*
value X axis - major and minor grids
*/

_Check_return_
static STATUS
gr_axis_addin_value_grids_x(
    _ChartRef_  P_GR_CHART cp,
    _In_        GR_CHART_OBJID id /*may mutate*/,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_PIXIT axis_ypos,
    _InVal_     BOOL front,
    _InVal_     BOOL major)
{
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    STATUS status = STATUS_OK;
    GR_AXIS_ITERATOR gr_axis_iterator;
    STATUS loop;
    GR_DIAG_OFFSET gridStart;
    GR_LINESTYLE linestyle;

    IGNOREPARM_InVal_(axis_ypos);

    if(!ticksp->bits.grid)
        return(status);

    gr_axis_iterator.step = ticksp->current;
    if(!gr_axis_iterator.step)
        return(status);

    if(front)
    {
        switch(p_axis->bits.arf)
        {
        case GR_AXIS_POSITION_FRONT:
            break;

        default: default_unhandled();
#if CHECKING
        case GR_AXIS_POSITION_AUTO:
        case GR_AXIS_POSITION_REAR:
#endif
            return(status);
        }
    }
    else
    {
        switch(p_axis->bits.arf)
        {
        case GR_AXIS_POSITION_FRONT:
            return(status);

        default: default_unhandled();
#if CHECKING
        case GR_AXIS_POSITION_AUTO:
        case GR_AXIS_POSITION_REAR:
#endif
            break;
        }
    }

    gr_chart_objid_from_axis_grid(&id, major);

    status_return(gr_chart_group_new(cp, &gridStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, major, &gr_axis_iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &gr_axis_iterator))
    {
        GR_PIXIT x_pos = gr_value_pos(cp, axes_idx, axis_idx, &gr_axis_iterator.iter);
        GR_BOX line_box;

        /* vertical grid line across entire y span */
        line_box.x0  = x_pos;
        line_box.y0  = 0;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0 + cp->plotarea.size.y;

        status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
    }

    gr_diag_group_end(cp->core.p_gr_diag, &gridStart);

    return(status);
}

/*
value Y axis - major and minor grids
*/

_Check_return_
static STATUS
gr_axis_addin_value_grids_y(
    _ChartRef_  P_GR_CHART cp,
    _In_        GR_CHART_OBJID id /*may mutate*/,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_PIXIT axis_xpos,
    _InVal_     BOOL front,
    _InVal_     BOOL major)
{
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET gridStart;
    GR_AXIS_ITERATOR gr_axis_iterator;
    STATUS loop;
    GR_LINESTYLE linestyle;
    BOOL draw_main;

    if(!ticksp->bits.grid)
        return(status);

    gr_axis_iterator.step = ticksp->current;
    if(!gr_axis_iterator.step)
        return(status);

    if(front)
    {
        switch(p_axis->bits.arf)
        {
        default: default_unhandled();
#if CHECKING
        case GR_AXIS_POSITION_AUTO:
#endif
            return(status);

        case GR_AXIS_POSITION_FRONT:
            draw_main = 1;
            break;

        case GR_AXIS_POSITION_REAR:
            return(status);
        }
    }
    else
    {
#if 1
        /* I now argue that bringing the axis to the front
         * shouldn't affect rear grid drawing state
        */
        draw_main = 1;
#else
        switch(p_axis->bits.arf)
        {
        default: default_unhandled();
#if CHECKING
        case GR_AXIS_POSITION_AUTO:
#endif
            draw_main = 1;
            break;

        case GR_AXIS_POSITION_FRONT:
            return(status);

        case GR_AXIS_POSITION_REAR:
            draw_main = 1;
            break;
        }
#endif
    }

    gr_chart_objid_from_axis_grid(&id, major);

    status_return(gr_chart_group_new(cp, &gridStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, major, &gr_axis_iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &gr_axis_iterator))
    {
        GR_PIXIT y_pos = gr_value_pos(cp, axes_idx, axis_idx, &gr_axis_iterator.iter);
        GR_BOX line_box;

        if(draw_main)
        {
            /* horizontal grid line across entire x span */
            line_box.x0  = 0;
            line_box.y0  = y_pos;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            line_box.x1  = line_box.x0 + cp->plotarea.size.x;
            line_box.y1  = line_box.y0;

            /* map together to front or back of 3-D world */
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);
            gr_map_point((P_GR_POINT) &line_box.x1, cp, fob);

            status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));

            if(cp->d3.bits.use && front)
            {
                /* grid lines across front of right hand side ONLY DURING THE FRONT PHASE */

                line_box.x0  = cp->plotarea.size.x;
                line_box.y0  = y_pos;

                line_box.x0 += cp->plotarea.posn.x;
                line_box.y0 += cp->plotarea.posn.y;

                /* put one to the front and one to the back */
                gr_map_box_front_and_back(&line_box, cp);

                status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
            }
        }

        if(cp->d3.bits.use && !front)
        {
            /* grid lines across midplane and side wall ONLY DURING THE REAR PHASE */
            line_box.x0  = 0;
            line_box.y0  = y_pos;

            line_box.x0 += cp->plotarea.posn.x;
            line_box.y0 += cp->plotarea.posn.y;

            /* put one to the front and one to the back */
            gr_map_box_front_and_back(&line_box, cp);

            status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));

            if((axis_xpos != 0) && (axis_xpos != cp->plotarea.size.x))
            {
                /* grid line across middle (simply shift) */
                line_box.x0 += axis_xpos;
                line_box.x1 += axis_xpos;

                status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
            }
        }
    }

    gr_diag_group_end(cp->core.p_gr_diag, &gridStart);

    return(status);
}

/*
value X axis - labels next to ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_labels_x(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_PIXIT axis_ypos,
    _InVal_     BOOL front)
{
    const BOOL major = 1;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = &p_axis->major;
    STATUS status = STATUS_OK;
    GR_DIAG_OFFSET labelStart;
    GR_AXIS_ITERATOR gr_axis_iterator;
    STATUS loop;
    GR_PIXIT last_x1 = S32_MIN;
    GR_PIXIT ticksize_top, ticksize_bottom;
    GR_TEXTSTYLE textstyle;
    HOST_FONT host_font;
    F64 maxval;

    IGNOREPARM_InVal_(front);

    gr_axis_iterator.step = ticksp->current;
    if(!gr_axis_iterator.step)
        return(status);

    status_return(gr_chart_group_new(cp, &labelStart, id));

    (void) gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom);

    gr_chart_objid_textstyle_query(cp, id, &textstyle);

    host_font = gr_riscdiag_host_font_from_textstyle(&textstyle);

    gr_axis_iterator_last(p_axis, major, &gr_axis_iterator);
    maxval = gr_axis_iterator.iter;

    loop = gr_axis_iterator_first(p_axis, major, &gr_axis_iterator);
    gr_numtonumstr_init(&gr_axis_iterator.iter, &maxval, ticksp->bits.decimals);

    for(; loop && status_ok(status); loop = gr_axis_iterator_next(p_axis, major, &gr_axis_iterator))
    {
        GR_PIXIT x_pos = gr_value_pos(cp, axes_idx, axis_idx, &gr_axis_iterator.iter);
        GR_BOX text_box;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup(&quick_ublock, cv.data.text);

        cv.data.text[0] = CH_NULL;

        text_box.x0  = x_pos;
        text_box.y0  = axis_ypos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        if(p_axis->bits.log_scale || p_axis->bits.log_label)
            status = gr_numtopowstr(ustr_bptr(cv.data.text), elemof32(cv.data.text), &gr_axis_iterator.iter, &gr_axis_iterator.base, p_axis->bits.log_scale, p_axis->bits.log_label);
        else
            status = gr_numtonumstr(&quick_ublock, &gr_axis_iterator.iter);

        if(status_ok(status))
        if(0 != (swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(cv.data.text))))
        {
            GR_PIXIT swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

            /* always centre within one major group at tick point */
            text_box.x0 -= swidth_px / 2;

            if(text_box.x0 >= last_x1)
            {
                switch(p_axis->bits.lzr)
                {
                case GR_AXIS_POSITION_TOP:
                    text_box.y0 += (3 * ticksize_top) / 2;
                    text_box.y0 += (1 * UBF_UNPACK(GR_PIXIT, textstyle.height)) / 4; /* st. descenders don't crash into ticks */
                    break;

                default:
                    text_box.y0 -= (3 * ticksize_bottom) / 2;
                    text_box.y0 -= (4 * UBF_UNPACK(GR_PIXIT, textstyle.height)) / 4; /* st. ascenders don't crash into ticks (SKS see above 25oct93) */
                    break;
                }

                text_box.x1 = text_box.x0 + swidth_px;
                text_box.y1 = text_box.y0 + textstyle.height;

                last_x1 = text_box.x1 + (UBF_UNPACK(GR_PIXIT, textstyle.height) / 8);

                status = gr_diag_text_new(cp->core.p_gr_diag, NULL, id, &text_box, cv.data.text, &textstyle);
            }
        }

        quick_ublock_dispose(&quick_ublock);
    }

    gr_riscdiag_host_font_dispose(&host_font);

    gr_diag_group_end(cp->core.p_gr_diag, &labelStart);

    return(status);
}

/*
value Y axis - labels next to ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_labels_y(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_PIXIT axis_xpos,
    _InVal_     BOOL front)
{
    const BOOL major = 1;
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = &p_axis->major;
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_DIAG_OFFSET labelStart;
    GR_AXIS_ITERATOR gr_axis_iterator;
    STATUS loop;
    GR_PIXIT last_y1 = S32_MIN;
    GR_PIXIT ticksize_left, ticksize_right, spacewidth_px;
    GR_TEXTSTYLE textstyle;
    HOST_FONT host_font;
    F64 maxval;

    gr_axis_iterator.step = ticksp->current;
    if(!gr_axis_iterator.step)
        return(status);

    (void) gr_axis_ticksizes(cp, ticksp, &ticksize_right, &ticksize_left); /* NB. order! */

    status_return(gr_chart_group_new(cp, &labelStart, id));

    gr_chart_objid_textstyle_query(cp, id, &textstyle);

    host_font = gr_riscdiag_host_font_from_textstyle(&textstyle);

    { /* allow room for a space at whichever side of the tick we are adding at */
    static /*non-const*/ UCHARZ space_string[] = " ";
    GR_MILLIPOINT swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(space_string));
    spacewidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;
    } /*block*/

    gr_axis_iterator_last(p_axis, major, &gr_axis_iterator);
    maxval = gr_axis_iterator.iter;

    loop = gr_axis_iterator_first(p_axis, major, &gr_axis_iterator);
    gr_numtonumstr_init(&gr_axis_iterator.iter, &maxval, ticksp->bits.decimals);

    for(; loop && status_ok(status); loop = gr_axis_iterator_next(p_axis, major, &gr_axis_iterator))
    {
        GR_PIXIT y_pos = gr_value_pos(cp, axes_idx, axis_idx, &gr_axis_iterator.iter);
        GR_BOX text_box;
        GR_CHART_VALUE cv;
        GR_MILLIPOINT swidth_mp;
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup(&quick_ublock, cv.data.text);

        cv.data.text[0] = CH_NULL;

        text_box.x0  = axis_xpos;
        text_box.y0  = y_pos;

        text_box.x0 += cp->plotarea.posn.x;
        text_box.y0 += cp->plotarea.posn.y;

        text_box.y0 -= (1 * UBF_UNPACK(GR_PIXIT, textstyle.height)) / 4; /* a vague attempt to centre the number on the tick */

        if(text_box.y0 >= last_y1)
        {
            if(p_axis->bits.log_scale || p_axis->bits.log_label)
                status = gr_numtopowstr(ustr_bptr(cv.data.text), sizeof32(cv.data.text), &gr_axis_iterator.iter, &gr_axis_iterator.base, p_axis->bits.log_scale, p_axis->bits.log_label);
            else
                status = gr_numtonumstr(&quick_ublock, &gr_axis_iterator.iter);

            if(status_ok(status))
            if(0 != (swidth_mp = gr_host_font_string_width(host_font, ustr_bptr(cv.data.text))))
            {
                GR_PIXIT swidth_px = swidth_mp / GR_MILLIPOINTS_PER_PIXIT;

                /* if axis at right then left just else right just */
                if(p_axis->bits.lzr == GR_AXIS_POSITION_RIGHT)
                {
                    text_box.x0 += ticksize_right;
                    if(!ticksize_right)
                        text_box.x0 += spacewidth_px; /* SKS after 1.05 25oct93 generously donates a little more space when no ticks */
                    text_box.x0 += spacewidth_px;
                }
                else
                {
                    text_box.x0 -= ticksize_left;
                    if(!ticksize_left)
                        text_box.x0 -= spacewidth_px; /* SKS after 1.05 25oct93 generously donates a little more space when no ticks */
                    text_box.x0 -= spacewidth_px;
                    text_box.x0 -= swidth_px;
                }

                /* map to front or back of 3-D world */
                if(cp->d3.bits.use)
                    gr_map_point((P_GR_POINT) &text_box.x0, cp, fob);

                text_box.x1 = text_box.x0 + swidth_px;
                text_box.y1 = text_box.y0 + textstyle.height;

                /* use standard 120% line spacing as guide */
                last_y1 = text_box.y1 + (2 * UBF_UNPACK(GR_PIXIT, textstyle.height)) / 10;

                status = gr_diag_text_new(cp->core.p_gr_diag, NULL, id, &text_box, cv.data.text, &textstyle);
            }
        }

        quick_ublock_dispose(&quick_ublock);
    }

    gr_riscdiag_host_font_dispose(&host_font);

    gr_diag_group_end(cp->core.p_gr_diag, &labelStart);

    return(status);
}

/*
value X & Y axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_ticks(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXIS_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx,
    _In_        GR_CHART_OBJID id /*may mutate*/,
    _InRef_     PC_GR_POINT axis_pos,
    _InVal_     BOOL front,
    _InVal_     BOOL major,
    _InVal_     GR_PIXIT ticksize,
    _InVal_     BOOL doing_x)
{
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_AXIS_ITERATOR gr_axis_iterator;
    STATUS loop;
    GR_DIAG_OFFSET tickStart;
    GR_LINESTYLE linestyle;

    gr_axis_iterator.step = ticksp->current;
    if(!gr_axis_iterator.step)
        return(status);

    gr_chart_objid_from_axis_tick(&id, major);

    status_return(gr_chart_group_new(cp, &tickStart, id));

    gr_chart_objid_linestyle_query(cp, id, &linestyle);

    for(loop = gr_axis_iterator_first(p_axis, major, &gr_axis_iterator);
        loop;
        loop = gr_axis_iterator_next( p_axis, major, &gr_axis_iterator))
    {
        GR_PIXIT pos = gr_value_pos(cp, axes_idx, axis_idx, &gr_axis_iterator.iter);
        GR_BOX line_box;

        line_box.x0  = doing_x ? pos         : axis_pos->x;
        line_box.y0  = doing_x ? axis_pos->y : pos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1  = line_box.x0;
        line_box.y1  = line_box.y0;

        if(doing_x)
            /* tick is vertical */
            line_box.y1 += ticksize;
        else
            /* tick is horizontal */
            line_box.x1 += ticksize;

        status_break(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle));
    }

    gr_diag_group_end(cp->core.p_gr_diag, &tickStart);

    return(status);
}

/*
value X axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_ticks_x(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_PIXIT axis_ypos,
    _InVal_     BOOL front,
    _InVal_     BOOL major)
{
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    GR_POINT axis_pos;
    GR_PIXIT ticksize, ticksize_top, ticksize_bottom;

    ticksize = MIN(cp->plotarea.size.x, cp->plotarea.size.y) / TICKLEN_FRAC;

    if(0 == gr_axis_ticksizes(cp, ticksp, &ticksize_top, &ticksize_bottom))
        return(STATUS_OK);

    if(!major)
    {
        ticksize_top    /= 2;
        ticksize_bottom /= 2;
    }

    axis_pos.y = axis_ypos - ticksize_bottom;
    ticksize = ticksize_top + ticksize_bottom;

    return(gr_axis_addin_value_ticks(cp, axes_idx, axis_idx, id, &axis_pos, front, major, ticksize, TRUE /*doing_x*/));
}

/*
value Y axis - major and minor ticks
*/

_Check_return_
static STATUS
gr_axis_addin_value_ticks_y(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_OBJID id,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_PIXIT axis_xpos,
    _InVal_     BOOL front,
    _InVal_     BOOL major)
{
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    PC_GR_AXIS_TICKS ticksp = major ? &p_axis->major : &p_axis->minor;
    GR_POINT axis_pos;
    GR_PIXIT ticksize, ticksize_left, ticksize_right;

    if(0 == gr_axis_ticksizes(cp, ticksp, &ticksize_right, &ticksize_left)) /* NB. order! */
        return(STATUS_OK);

    if(!major)
    {
        ticksize_left  /= 2;
        ticksize_right /= 2;
    }

    axis_pos.x = axis_xpos - ticksize_left;
    ticksize = ticksize_left + ticksize_right;

    return(gr_axis_addin_value_ticks(cp, axes_idx, axis_idx, id, &axis_pos, front, major, ticksize, FALSE /*doing_x*/));
}

/******************************************************************************
*
* value X axis
*
******************************************************************************/

_Check_return_
extern STATUS
gr_axis_addin_value_x(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     BOOL front)
{
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    STATUS status = STATUS_OK;
    GR_PIXIT axis_ypos;
    GR_DIAG_OFFSET axisStart;
    BOOL draw_main;
    GR_CHART_OBJID id;

    switch(p_axis->bits.lzr)
    {
    case GR_AXIS_POSITION_TOP:
        axis_ypos = cp->plotarea.size.y;
        break;

    case GR_AXIS_POSITION_ZERO:
        axis_ypos = (GR_PIXIT) ((F64) cp->plotarea.size.y *
                                cp->axes[axes_idx].axis[Y_AXIS_IDX].zero_frac); /*NB*/
        break;

    default: default_unhandled();
#if CHECKING
    case GR_AXIS_POSITION_LEFT:
#endif
        axis_ypos = 0;
        break;
    }

    /* NB. in 2-D AUTO and REAR synonymous */

    /* no 3-D to consider here */

    switch(p_axis->bits.arf)
    {
    case GR_AXIS_POSITION_FRONT:
        draw_main = front;
        break;

    default: default_unhandled();
#if CHECKING
    case GR_AXIS_POSITION_AUTO:
    case GR_AXIS_POSITION_REAR:
#endif
        draw_main = !front;
        break;
    }

    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &id);

    status_return(gr_chart_group_new(cp, &axisStart, id));

    /* minor ticks */
    status_return(gr_axis_addin_value_grids_x(cp, id, axes_idx, axis_ypos, front, 0));

    /* major grids */
    status_return(gr_axis_addin_value_grids_x(cp, id, axes_idx, axis_ypos, front, 1));

    if(draw_main)
    {
        GR_BOX line_box;
        GR_LINESTYLE linestyle;
        SS_RECOG_CONTEXT ss_recog_context;

        line_box.x0 = 0;
        line_box.y0 = axis_ypos;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        line_box.x1 = line_box.x0 + cp->plotarea.size.x;
        line_box.y1 = line_box.y0;

        gr_chart_objid_linestyle_query(cp, id, &linestyle);

        /* horizontal main axis line, minor ticks, major ticks, labels */
        ss_recog_context_push(&ss_recog_context);
        if(status_ok(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle)))
        if(status_ok(status = gr_axis_addin_value_ticks_x( cp, id, axes_idx, axis_ypos, front, 0)))
        if(status_ok(status = gr_axis_addin_value_ticks_x( cp, id, axes_idx, axis_ypos, front, 1)))
                     status = gr_axis_addin_value_labels_x(cp, id, axes_idx, axis_ypos, front);
        ss_recog_context_pull(&ss_recog_context);
    }

    gr_diag_group_end(cp->core.p_gr_diag, &axisStart);

    return(status);
}

/******************************************************************************
*
* value Y axis
*
******************************************************************************/

_Check_return_
extern STATUS
gr_axis_addin_value_y(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     BOOL front)
{
    const GR_AXIS_IDX axis_idx = Y_AXIS_IDX;
    PC_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    STATUS status = STATUS_OK;
    S32 /*GR_SERIES_NO*/ fob = (front ? 0 : cp->cache.n_contrib_series);
    GR_PIXIT axis_xpos;
    GR_DIAG_OFFSET axisStart;
    BOOL draw_main;
    GR_CHART_OBJID id;

    switch(p_axis->bits.lzr)
    {
    case GR_AXIS_POSITION_RIGHT:
        axis_xpos = cp->plotarea.size.x;
        break;

    case GR_AXIS_POSITION_ZERO:
        axis_xpos = (GR_PIXIT) ((F64) cp->plotarea.size.x *
                                cp->axes[axes_idx].axis[X_AXIS_IDX].zero_frac); /*NB*/
        break;

    default: default_unhandled();
#if CHECKING
    case GR_AXIS_POSITION_LEFT:
#endif
        axis_xpos = 0;
        break;
    }

    /* NB. in 2-D AUTO and REAR synonymous */

    switch(p_axis->bits.arf)
    {
    case GR_AXIS_POSITION_FRONT:
    atfront:;
        draw_main = (front == 1);
        break;

    default: default_unhandled();
#if CHECKING
    case GR_AXIS_POSITION_AUTO:
#endif
        if(cp->d3.bits.use)
            if(axis_xpos != cp->plotarea.size.x)
                goto atfront;

        /*FALLTHRU*/

    case GR_AXIS_POSITION_REAR:
        draw_main = (front == 0);
        break;
    }

    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &id);

    status_return(gr_chart_group_new(cp, &axisStart, id));

    /* minor ticks */
    status_return(gr_axis_addin_value_grids_y(cp, id, axes_idx, axis_xpos, front, 0));

    /* major grids */
    status_return(gr_axis_addin_value_grids_y(cp, id, axes_idx, axis_xpos, front, 1));

    if(draw_main)
    {
        GR_BOX line_box;
        GR_LINESTYLE linestyle;
        SS_RECOG_CONTEXT ss_recog_context;

        line_box.x0 = axis_xpos;
        line_box.y0 = 0;

        line_box.x0 += cp->plotarea.posn.x;
        line_box.y0 += cp->plotarea.posn.y;

        /* map to front or back of 3-D world */
        if(cp->d3.bits.use)
            gr_map_point((P_GR_POINT) &line_box.x0, cp, fob);

        line_box.x1 = line_box.x0;
        line_box.y1 = line_box.y0 + cp->plotarea.size.y;

        gr_chart_objid_linestyle_query(cp, id, &linestyle);

        /* vertical main axis line, minor ticks, major ticks, labels */
        ss_recog_context_push(&ss_recog_context);
        if(status_ok(status = gr_diag_line_new(cp->core.p_gr_diag, NULL, id, &line_box, &linestyle)))
        if(status_ok(status = gr_axis_addin_value_ticks_y( cp, id, axes_idx, axis_xpos, front, 0)))
        if(status_ok(status = gr_axis_addin_value_ticks_y( cp, id, axes_idx, axis_xpos, front, 1)))
                     status = gr_axis_addin_value_labels_y(cp, id, axes_idx, axis_xpos, front);
        ss_recog_context_pull(&ss_recog_context);
    }

    gr_diag_group_end(cp->core.p_gr_diag, &axisStart);

    return(status);
}

/* end of gr_axisp.c */
