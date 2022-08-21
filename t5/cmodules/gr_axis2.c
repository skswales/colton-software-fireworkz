/* gr_axis2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Chart axis handling */

/* SKS November 1991 / Fireworkz world conversion April 1993 */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#define AXIS_RATIO_SILLY_MIN 0.02 /* SKS 25jul97 be a bit more lenient about silly graph labelling */
#define AXIS_RATIO_SILLY_MAX 50.0

/* allow for logs of numbers (especially those not in same base as FP representation) becoming imprecise */
#define LOG_SIG_EPS 1E-8

_Check_return_
extern F64
splitlognum(
    _InRef_     PC_F64 logval,
    _OutRef_    P_F64 exponent)
{
    F64 mantissa = modf(*logval, exponent);

    /* NB. consider numbers such as log10(0.2) ~ -0.698 = (-1.0 exp) + (0.30103 man) */
    if(mantissa < 0.0)
    {
        mantissa += 1.0;
        *exponent -= 1.0;
    }

    /* watch for logs going awry */
    if(mantissa < LOG_SIG_EPS)
        /* keep rounded down */
        mantissa = 0.0;

    else if((1.0 - mantissa) < LOG_SIG_EPS)
        /* round up */
    {
        mantissa = 0.0;
        *exponent += 1.0;
    }

    return(mantissa);
}

/******************************************************************************
*
* given punter limits, flags and actual values for this
* category axis, form a current minmax and set up cached vars
*
******************************************************************************/

extern void
gr_axis_form_category(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_POINT_NO total_n_points)
{
    const GR_AXES_IDX axes_idx = 0;
    const GR_AXIS_IDX axis_idx = X_AXIS_IDX;
    const P_GR_AXIS p_axis = &cp->axes[axes_idx].axis[axis_idx];
    P_GR_AXIS_TICKS ticksp;

    /* how far up the interval is the zero line? */
    /* answer: categories thinks zero is at left */
    p_axis->zero_frac = 0.0;

    /* category axis major ticks */
    ticksp = &p_axis->major;

    ticksp->current = gr_lin_major(total_n_points - 0.0);
    /* forbid sub-category ticks */
    if( ticksp->current < 1.0)
        ticksp->current = 1.0;

    if(ticksp->bits.manual)
    {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
        {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
        }
    }

    /* category axis minor ticks */
    ticksp = &p_axis->minor;

    ticksp->current = gr_lin_major(/*2.0 **/ p_axis->major.current);
    /* forbid sub-category ticks */
    if( ticksp->current < 1.0)
        ticksp->current = 1.0;

    if(ticksp->bits.manual)
    {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
        {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
        }
    }
}

/******************************************************************************
*
* given punter limits, flags and actual values for this
* value axis, form a current minmax and set up cached vars
*
******************************************************************************/

#if __STDC_VERSION__ < 199901L

#if WINDOWS

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h" /* for _log2_e */
#endif

_Check_return_
static inline double
log2(_InVal_ double d)
{
    return(log(d) * _log2_e);
}

#endif /* OS */

#endif /* __STDC_VERSION__ */

static void
gr_axis_form_value_log(
    P_GR_AXES p_axes,
    P_GR_AXIS p_axis,
    _InVal_     BOOL is_y_axis)
{
    P_GR_AXIS_TICKS ticksp;
    F64 lna;

    p_axis->current = p_axis->actual;

    if(!p_axes->bits.stacked_pct || !is_y_axis)
    {
        if(p_axis->bits.manual)
        {
            if(p_axis->punter.min > 0.0)
                p_axis->current.min = MIN(p_axis->current.min, p_axis->punter.min);
            if(p_axis->punter.max > 0.0)
                p_axis->current.max = MAX(p_axis->current.max, p_axis->punter.max);
        }
    }

    if(p_axis->bits.incl_zero)
    {
        /* in log scaling mode this 'obviously' means an exponent of zero! */
        p_axis->current.min = MIN(p_axis->current.min, 1.0);
        p_axis->current.max = MAX(p_axis->current.max, 1.0);
    }

    /* account for stupid graphs */
    if( p_axis->current.min < 10.0 * F64_MIN)
        p_axis->current.min = 10.0 * F64_MIN;

    /* account for stupid graphs */
    if( p_axis->current.max < 10.0 * F64_MIN)
        p_axis->current.max = 10.0 * F64_MIN;

    /* value axis major ticks (multiplier between ticks) */
    ticksp = &p_axis->major;

    /* attempt to find a useful and 'pretty' major interval using both deduced and punter values */
    /* my guesses are always in base 10 */
    ticksp->current = gr_lin_major(log10(p_axis->current.max) - log10(p_axis->current.min));
    if( ticksp->current < 1.0)
        ticksp->current = 1.0;
    p_axis->bits.log_base = 10;
    ticksp->current = pow(p_axis->bits.log_base, ticksp->current);

    if(ticksp->bits.manual)
    {
        if(ticksp->punter <= 1.0) /* must be multiplying by something significant each time */
            ticksp->current = 0.0; /* off */
        else
        {
            /* look at punter value and split into base and exponent step adder */
            /* first try and split punter value up as base 10 animacule */
            F64 test = log10(ticksp->punter);
            F64 exponent;
            F64 mantissa = splitlognum(&test, &exponent);

            /* was number close enough to a power of the base? */
            if(mantissa < LOG_SIG_EPS)
            {
                test = log10(ticksp->current);

                consume(F64, splitlognum(&test, &exponent));

                test = ticksp->punter / exponent;

                if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                    ticksp->current = ticksp->punter;
            }
            else
            {
                /* try base 2 */
                test = log2(ticksp->punter);

                mantissa = splitlognum(&test, &exponent);

                /* was number close enough to a power of the base? */
                if(mantissa < LOG_SIG_EPS)
                {
                    ticksp->current = ticksp->punter;
                    p_axis->bits.log_base = 2;
                }
            }
        }
    }

    lna = log((F64) p_axis->bits.log_base);

    /* Round down the current.min to a multiple of the major value towards an exponent of -infinity */
    if(!p_axis->bits.manual || (p_axis->current.min != p_axis->punter.min))
    {
        /* compute log-to-the-base-of-major-multiplier of the current.min */
        F64 ftest;

        if(2 == p_axis->bits.log_base)
            ftest = log2(p_axis->current.min);
        else if(10 == p_axis->bits.log_base)
            ftest = log10(p_axis->current.min);
        else
        {
            ftest = log(p_axis->current.min);

            ftest = ftest / lna;
        }

        ftest = floor(ftest);

        p_axis->current.min = pow(p_axis->bits.log_base, ftest); /* could underflow */

        if( p_axis->current.min < F64_MIN)
            p_axis->current.min = pow(10.0, DBL_MIN_10_EXP); /* may look bad for non-base 10 but who cares */
    }

    /* account for stupid graphs */
    if( p_axis->current.max <= p_axis->current.min)
        p_axis->current.max  = p_axis->current.min * (ticksp->current ? ticksp->current : 2.0);

    /* Round up the current.max to a multiple of the major value towards an exponent of +infinity */
    if(!p_axis->bits.manual || (p_axis->current.max != p_axis->punter.max))
    {
        /* compute log-to-the-base-of-major-multiplier of the current.max */
        F64 ctest;

        if(2 == p_axis->bits.log_base)
            ctest = log2(p_axis->current.max);
        else if(10 == p_axis->bits.log_base)
            ctest = log10(p_axis->current.max);
        else
        {
            ctest = log(p_axis->current.max);

            ctest = ctest / lna;
        }

        ctest = ceil(ctest);

        p_axis->current.max = pow(p_axis->bits.log_base, ctest);
    }

    /* cache the spanned interval NOW as major scaling might have modified the endpoints! */
    if(2 == p_axis->bits.log_base)
        p_axis->current_span = log2(p_axis->current.max) - log2(p_axis->current.min);
    else if(10 == p_axis->bits.log_base)
        p_axis->current_span = log10(p_axis->current.max) - log10(p_axis->current.min);
    else
        p_axis->current_span = (log(p_axis->current.max) - log(p_axis->current.min)) / lna;

    p_axis->zero_frac = 0.0; /* no real zero so say at bottom */

    /* value axis minor ticks (mantissa increment between ticks) */
    ticksp = &p_axis->minor;

    ticksp->current = 1.0;

    if(ticksp->bits.manual)
    {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
        {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
        }
    }
}

static void
gr_axis_form_value_lin(
    P_GR_AXIS p_axis)
{
    P_GR_AXIS_TICKS ticksp;
    F64 major_interval;

    p_axis->current = p_axis->actual;

    if(p_axis->bits.manual)
    {
#if 1
        /* SKS 15jan97 generally allow for wankers wanting to set silly limits by hand - means we'll have to clip in chart creation */
        p_axis->current.min = p_axis->punter.min;
        p_axis->current.max = p_axis->punter.max;
#else
        if(GR_CHART_TYPE_SCAT == cp->axes[0].chart_type) /* SKS 03mar95 allow for wankers wanting to set silly limits by hand */
        {
            p_axis->current.min = p_axis->punter.min;
            p_axis->current.max = p_axis->punter.max;
        }
        else
        {
            p_axis->current.min = MIN(p_axis->current.min, p_axis->punter.min);
            p_axis->current.max = MAX(p_axis->current.max, p_axis->punter.max);
        }
#endif
    }

    if(p_axis->bits.incl_zero)
    {
        p_axis->current.min = MIN(p_axis->current.min, 0.0);
        p_axis->current.max = MAX(p_axis->current.max, 0.0);
    }

    /* value axis major ticks */
    ticksp = &p_axis->major;

    /* attempt to find a useful and 'pretty' major interval using both deduced and punter values */
    ticksp->current = gr_lin_major(p_axis->current.max - p_axis->current.min);
    major_interval = ticksp->current;

    if(ticksp->bits.manual)
    {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
        {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
            {
                ticksp->current = ticksp->punter;
                major_interval = ticksp->current;
            }
        }
    }
    /* SKS after PD 4.12 26mar92 - in auto mode only: RJM reckons this is what punters would like and I agree */
    else
    {
        if(ticksp->current == 0.5)
        {
            ticksp->current = 1.0;
            major_interval = ticksp->current;
        }
    }

    { /* remember for decimal places rounding of values shown on axis */
    F64 log10_major = log10(major_interval);
    F64 exponent;
    F64 mantissa = splitlognum(&log10_major, &exponent);
    int decimals = (int) floor(exponent);
    IGNOREPARM(mantissa);
    p_axis->major.bits.decimals = (decimals < 0) ? -decimals : 0;
    } /*block*/

    /* Round down the current.min to a multiple of the major value towards -infinity */
    if(!p_axis->bits.manual || (p_axis->current.min != p_axis->punter.min))
    {
        F64 ftest = floor(p_axis->current.min / major_interval);

        p_axis->current.min = ftest * major_interval;
    }

    /* account for stupid graphs */
    if( p_axis->current.max <= p_axis->current.min)
        p_axis->current.max  = p_axis->current.min + major_interval;

    /* Round up the current.max to a multiple of the major value towards +infinity */
    if(!p_axis->bits.manual || (p_axis->current.max != p_axis->punter.max))
    {
        F64 ctest = ceil( p_axis->current.max / major_interval);

        p_axis->current.max = ctest * major_interval;
    }

    /* cache the spanned interval NOW as major scaling might have modified the endpoints! */
    p_axis->current_span = p_axis->current.max - p_axis->current.min;

    /* how far up the interval is the zero line? */
    p_axis->zero_frac = (0.0 - p_axis->current.min) / p_axis->current_span;

    /* this is limited to either end of the interval */
    if( p_axis->zero_frac < 0.0)
        p_axis->zero_frac = 0.0;
    if( p_axis->zero_frac > 1.0)
        p_axis->zero_frac = 1.0;

    /* major tick interval as a fraction of the current span */
    p_axis->current_frac = major_interval / p_axis->current_span;

    /* value axis minor ticks */
    ticksp = &p_axis->minor;

    ticksp->current = gr_lin_major(/*2.0 **/ major_interval);

    if(ticksp->bits.manual)
    {
        if(ticksp->punter <= 0.0)
            ticksp->current = 0.0; /* off */
        else
        {
            F64 test = ticksp->punter / ticksp->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                ticksp->current = ticksp->punter;
        }
    }
}

extern void
gr_axis_form_value(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx)
{
    const P_GR_AXES p_axes = &cp->axes[axes_idx];
    const P_GR_AXIS p_axis = &p_axes->axis[axis_idx];

    if(p_axis->bits.log_scale)
        gr_axis_form_value_log(p_axes, p_axis, Y_AXIS_IDX == axis_idx);
    else
        gr_axis_form_value_lin(p_axis);
}

/******************************************************************************
*
* guess what would be a good major mark interval
*
******************************************************************************/

/* helper routine needed to stop MSC8.00 compiler barfing on release */

_Check_return_
static BOOL
gr_lin_major_test(
    _InRef_     PC_F64 p_mantissa,
    _InRef_     PC_F64 p_cutoff,
    _InoutRef_  P_F64 p_test,
    _InRef_     PC_F64 p_divisor)
{
    if(*p_mantissa < *p_cutoff)
    {
        *p_test = *p_test / *p_divisor;
        return(TRUE);
    }

    return(FALSE);
}

/* eg.
 *   99 -> 1, .9956 -> 10 so leave 10
 *   49 -> 1, .6902 -> 10 but make 5
 *   19 -> 1, .2788 -> 10 but make 2
*/

extern F64
gr_lin_major(
    _InVal_     F64 span)
{
    static const F64 cutoff[3] =
    {
        0.11394 /* approx. log10(1.2 + 0.1)*/,
        0.39794 /* approx. log10(2.4 + 0.1)*/,
        0.78533 /* approx. log10(6.0 + 0.1)*/
    };
    static const F64 divisor[3] =
    {
        10.0/1,
        10.0/2,
        10.0/5
    };
    F64 test, mantissa, exponent;

    if(span == 0.0)
        return(1.0);

    test = log10(fabs(span));

    mantissa = splitlognum(&test, &exponent);

    test = pow(10.0, exponent);

    if(gr_lin_major_test(&mantissa, &cutoff[0], &test, &divisor[0]))
        return(test);

    if(gr_lin_major_test(&mantissa, &cutoff[1], &test, &divisor[1]))
        return(test);

    if(gr_lin_major_test(&mantissa, &cutoff[2], &test, &divisor[2]))
        return(test);

    return(test);
}

/* end of gr_axis2.c */
