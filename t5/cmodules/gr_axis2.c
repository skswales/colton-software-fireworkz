/* gr_axis2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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
    _InVal_     F64 logval,
    _OutRef_    P_F64 exponent)
{
    F64 mantissa = modf(logval, exponent);

    /* NB. consider numbers such as log10(0.2) ~ -0.698 = (-1.0 exp) + (0.30103 man) */
    if(mantissa < 0.0)
    {
        mantissa += 1.0;
        *exponent -= 1.0;
    }

    /* watch for logs going awry */
    if(mantissa < LOG_SIG_EPS)
    {   /* keep rounded down */
        return(0.0);
    }

    if((1.0 - mantissa) < LOG_SIG_EPS)
    {   /* round up */
        *exponent += 1.0;
        return(0.0);
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
    P_GR_AXIS_TICKS p_axis_ticks;

    /* how far up the interval is the zero line? */
    /* answer: categories thinks zero is at left */
    p_axis->zero_frac = 0.0;

    /* category axis major ticks */
    p_axis_ticks = &p_axis->major;

    p_axis_ticks->current = gr_lin_major(total_n_points - 0.0);
    /* forbid sub-category ticks */
    if( p_axis_ticks->current < 1.0)
        p_axis_ticks->current = 1.0;

    if(p_axis_ticks->bits.manual)
    {
        if(p_axis_ticks->punter <= 0.0)
            p_axis_ticks->current = 0.0; /* off */
        else
        {
            F64 test = p_axis_ticks->punter / p_axis_ticks->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                p_axis_ticks->current = p_axis_ticks->punter;
        }
    }

    /* category axis minor ticks */
    p_axis_ticks = &p_axis->minor;

    p_axis_ticks->current = gr_lin_major(/*2.0 **/ p_axis->major.current);
    /* forbid sub-category ticks */
    if( p_axis_ticks->current < 1.0)
        p_axis_ticks->current = 1.0;

    if(p_axis_ticks->bits.manual)
    {
        if(p_axis_ticks->punter <= 0.0)
            p_axis_ticks->current = 0.0; /* off */
        else
        {
            F64 test = p_axis_ticks->punter / p_axis_ticks->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                p_axis_ticks->current = p_axis_ticks->punter;
        }
    }
}

/******************************************************************************
*
* given punter limits, flags and actual values for this
* value axis, form a current minmax and set up cached vars
*
******************************************************************************/

static void
gr_axis_form_value_log(
    P_GR_AXES p_axes,
    P_GR_AXIS p_axis,
    _InVal_     BOOL is_y_axis)
{
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

    { /* value axis major ticks (multiplier between ticks) */
    P_GR_AXIS_TICKS p_axis_ticks = &p_axis->major;

    /* attempt to find a useful and 'pretty' major interval using both deduced and punter values */
    /* my guesses are always in base 10 */
    p_axis_ticks->current = gr_lin_major(log10(p_axis->current.max) - log10(p_axis->current.min));
    if( p_axis_ticks->current < 1.0)
        p_axis_ticks->current = 1.0;
    p_axis->bits.log_base = 10;
    p_axis_ticks->current = pow(p_axis->bits.log_base, p_axis_ticks->current);

    if(p_axis_ticks->bits.manual)
    {
        if(p_axis_ticks->punter <= 1.0) /* must be multiplying by something significant each time */
            p_axis_ticks->current = 0.0; /* off */
        else
        {
            /* look at punter value and split into base and exponent step adder */
            /* first try and split punter value up as base 10 animacule */
            F64 test = log10(p_axis_ticks->punter);
            F64 exponent;
            F64 mantissa = splitlognum(test, &exponent);

            /* was number close enough to a power of the base? */
            if(mantissa < LOG_SIG_EPS)
            {
                test = log10(p_axis_ticks->current);

                consume(F64, splitlognum(test, &exponent));

                test = p_axis_ticks->punter / exponent;

                if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                    p_axis_ticks->current = p_axis_ticks->punter;
            }
            else
            {
                /* try base 2 */
                test = log2(p_axis_ticks->punter);

                mantissa = splitlognum(test, &exponent);

                /* was number close enough to a power of the base? */
                if(mantissa < LOG_SIG_EPS)
                {
                    p_axis_ticks->current = p_axis_ticks->punter;
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
        p_axis->current.max  = p_axis->current.min * (p_axis_ticks->current ? p_axis_ticks->current : 2.0);
    } /*block*/

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

    { /* value axis minor ticks (mantissa increment between ticks) */
    const P_GR_AXIS_TICKS p_axis_ticks = &p_axis->minor;

    p_axis_ticks->current = 1.0;

    if(p_axis_ticks->bits.manual)
    {
        if(p_axis_ticks->punter <= 0.0)
            p_axis_ticks->current = 0.0; /* off */
        else
        {
            F64 test = p_axis_ticks->punter / p_axis_ticks->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                p_axis_ticks->current = p_axis_ticks->punter;
        }
    }
    } /*block*/
}

static void
gr_axis_form_value_lin(
    P_GR_AXIS p_axis)
{
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

    { /* value axis major ticks */
    P_GR_AXIS_TICKS p_axis_ticks = &p_axis->major;

    /* attempt to find a useful and 'pretty' major interval using both deduced and punter values */
    p_axis_ticks->current = gr_lin_major(p_axis->current.max - p_axis->current.min);
    major_interval = p_axis_ticks->current;

    if(p_axis_ticks->bits.manual)
    {
        if(p_axis_ticks->punter <= 0.0)
            p_axis_ticks->current = 0.0; /* off */
        else
        {
            F64 test = p_axis_ticks->punter / p_axis_ticks->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
            {
                p_axis_ticks->current = p_axis_ticks->punter;
                major_interval = p_axis_ticks->current;
            }
        }
    }
    /* SKS after PD 4.12 26mar92 - in auto mode only: RJM reckons this is what punters would like and I agree */
    else
    {
        if(p_axis_ticks->current == 0.5)
        {
            p_axis_ticks->current = 1.0;
            major_interval = p_axis_ticks->current;
        }
    }
    } /*block*/

    { /* remember for decimal places rounding of values shown on axis */
    const F64 log10_major = log10(major_interval);
    F64 exponent;
    const F64 mantissa = splitlognum(log10_major, &exponent);
    const int decimals = (int) floor(exponent);
    UNREFERENCED_LOCAL_VARIABLE(mantissa);
    p_axis->major.bits.decimals = UBF_PACK((decimals < 0) ? (unsigned int) -decimals : 0U);
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

    { /* value axis minor ticks */
    P_GR_AXIS_TICKS p_axis_ticks = &p_axis->minor;

    p_axis_ticks->current = gr_lin_major(/*2.0 **/ major_interval);

    if(p_axis_ticks->bits.manual)
    {
        if(p_axis_ticks->punter <= 0.0)
            p_axis_ticks->current = 0.0; /* off */
        else
        {
            F64 test = p_axis_ticks->punter / p_axis_ticks->current;

            if((test >= AXIS_RATIO_SILLY_MIN) && (test <= AXIS_RATIO_SILLY_MAX)) /* use punter value unless it is silly or we area doing autocalc */
                p_axis_ticks->current = p_axis_ticks->punter;
        }
    }
    } /*block*/
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
static inline BOOL
gr_lin_major_test(
    _InVal_     F64 mantissa,
    _InVal_     F64 cutoff,
    _InoutRef_  P_F64 p_test,
    _InVal_     F64 divisor)
{
    if(mantissa < cutoff)
    {
        *p_test = *p_test / divisor;
        return(TRUE);
    }

    return(FALSE);
}

/* e.g.
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

    mantissa = splitlognum(test, &exponent);

    test = pow(10.0, exponent);

    if(gr_lin_major_test(mantissa, cutoff[0], &test, divisor[0]))
        return(test);

    if(gr_lin_major_test(mantissa, cutoff[1], &test, divisor[1]))
        return(test);

    if(gr_lin_major_test(mantissa, cutoff[2], &test, divisor[2]))
        return(test);

    return(test);
}

/* end of gr_axis2.c */
