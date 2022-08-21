/* ev_fnfin.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Financial function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/******************************************************************************
*
* Financial functions
*
******************************************************************************/

/*
Financial functions calculated using ARRAY_RANGE_XXX processing:
IRR()
MIRR()
NPV()
*/

/******************************************************************************
*
* REAL cterm(interest, fv, pv)
*
******************************************************************************/

PROC_EXEC_PROTO(c_cterm)
{
    F64 interest = args[0]->arg.fp + 1.0;
    F64 fv = args[1]->arg.fp;
    F64 pv = args[2]->arg.fp;
    F64 cterm_result;
    BOOL fv_negative = FALSE; /* SKS after 1.07 14jul94 allow it to work out debts! */
    BOOL pv_negative = FALSE;

    exec_func_ignore_parms();

    if(fv < 0.0)
    {
        fv = fabs(fv);
        fv_negative = TRUE;
    }

    if(pv < 0.0)
    {
        pv = fabs(pv);
        pv_negative = TRUE;
    }

    if(fv_negative != pv_negative)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MIXED_SIGNS);
        return;
    }

    if(interest <= F64_MIN)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    cterm_result = (log(fv) - log(pv)) / log(interest);

    ev_data_set_real(p_ev_data_res, cterm_result);
}

/******************************************************************************
*
* REAL db(cost, salvage, life, period {, month})
*
******************************************************************************/

PROC_EXEC_PROTO(c_db)
{
    const F64 cost = args[0]->arg.fp;
    const F64 salvage = args[1]->arg.fp;
    const S32 life = (S32) args[2]->arg.fp;
    const S32 period = (S32) args[3]->arg.fp;
    const F64 month = (n_args > 4) ? args[4]->arg.fp : 12.0;
    F64 rate;
    F64 db_result;

    exec_func_ignore_parms();

    if(cost < 0.0     ||
       salvage > cost ||
       life < 1       ||
       month < 1.0    ||
       month > 12.0   ||
       period < 1     ||
       (period > life + (12.0 != month)) )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    /* rate = 1 - ((salvage / cost) ^ (1 / life)), rounded to three decimal places */
    rate = 1.0 - pow((salvage / cost), (1.0 / life));
    rate *= 1000;
    rate = (S32) (rate + 0.5);
    rate /= 1000;

    /* depreciation during a period = (cost - total depreciation from prior periods) * rate */
    if(1 == period)
    {   /* depreciation during first period */
        db_result = cost * rate;

        if(12.0 != month)
            db_result *= month / 12.0; /* adjust for month */
    }
    else
    {
        F64 total_depreciation = (cost * rate) * month / 12.0; /* depreciation during first period, adjust for month */
        S32 prev_period;

        /* depreciation during subsequent periods */
        for(prev_period = 2; prev_period < period; ++prev_period)
        {
            F64 this_db = (cost - total_depreciation) * rate;

            total_depreciation += this_db;
        }

        /* depreciation during this period */
        db_result = (cost - total_depreciation) * rate;

        if((12.0 != month) && (period > life))
            db_result *= (12.0 - month) / 12.0; /* adjust for month */
    }

    ev_data_set_real(p_ev_data_res, db_result);
}

/******************************************************************************
*
* REAL ddb(cost, salvage, life, period {, factor})
*
******************************************************************************/

PROC_EXEC_PROTO(c_ddb)
{
    F64 cost = args[0]->arg.fp;
    F64 value = cost;
    F64 salvage = args[1]->arg.fp;
    S32 life = (S32) args[2]->arg.fp;
    S32 period = (S32) args[3]->arg.fp;
    F64 factor = (n_args > 4) ? args[4]->arg.fp : 2.0;
    F64 cur_period = 0.0; /* ddb_result */
    S32 i;

    exec_func_ignore_parms();

    if(cost < 0.0     ||
       salvage > cost ||
       life < 1       ||
       period < 1     ||
       period > life)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    for(i = 0; i < period; ++i)
    {
        cur_period = (value * factor) / life;
        if(value - cur_period < salvage)
            cur_period = value - salvage;
        value -= cur_period;
    }

    ev_data_set_real(p_ev_data_res, cur_period);
}

/******************************************************************************
*
* REAL fv(payment, interest, term)
*
* REAL odf.fv(interest, term, payment)
*
******************************************************************************/

_Check_return_
static F64
calc_fv(
    _InVal_     F64 payment,
    _InVal_     F64 interest,
    _InVal_     F64 term)
{
    /* payment * ((1 + interest) ^ term - 1) / interest */
    return(payment * (pow(1.0 + interest, term) - 1.0) / interest);
}

PROC_EXEC_PROTO(c_fv)
{
    const F64 payment = args[0]->arg.fp;
    const F64 interest = args[1]->arg.fp;
    const F64 term = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* fv(payment, interest, term) = payment * ((1 + interest) ^ term - 1) / interest */
    ev_data_set_real(p_ev_data_res, calc_fv(payment, interest, term));
}

PROC_EXEC_PROTO(c_odf_fv)
{
    const F64 interest = args[0]->arg.fp;
    const F64 term = args[1]->arg.fp;
    const F64 payment = args[2]->arg.fp;
    F64 odf_fv_result;

    exec_func_ignore_parms();

    /* odf.fv(interest, term, payment) = - payment * ((1 + interest) ^ term - 1) / interest */
    odf_fv_result = - calc_fv(payment,  interest,  term);

    if(n_args > 3)
    {
        const F64 pv = args[3]->arg.fp;
        const F64 fv = pv * pow(1.0 + interest, term);

        odf_fv_result -= fv;
    }

    ev_data_set_real(p_ev_data_res, odf_fv_result);
}

/******************************************************************************
*
* REAL fvschedule(principal, schedule)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fvschedule)
{
    const F64 principal = args[0]->arg.fp;
    const PC_EV_DATA array_schedule = args[1];
    F64 fvschedule_result = principal;
    S32 x_size, y_size;
    S32 ix, iy;

    array_range_sizes(array_schedule, &x_size, &y_size);

    exec_func_ignore_parms();

    for(ix = 0; ix < x_size; ++ix)
    {
        for(iy = 0; iy < y_size; ++iy)
        {
            EV_DATA ev_data;
            F64 interest;

            if(RPN_DAT_ERROR == array_range_index(&ev_data, array_schedule, ix, iy, EM_REA)) /* blanks == 0 */
            {
                *p_ev_data_res = ev_data;
                return;
            }

            interest = 1.0 + ev_data.arg.fp;

            fvschedule_result *= interest; /* product */
        }
    }

    ev_data_set_real(p_ev_data_res, fvschedule_result);
}

/******************************************************************************
*
* REAL odf.irr(principal, values, {guess:number=0.1})
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_irr)
{
    const PC_EV_DATA array_values = args[0];
    F64 r = (n_args > 1) ? args[1]->arg.fp : 0.1; /* usually in 0..1 */
    F64 last_npv = 0.0; /* keep dataflower happy now that we handle r0->r1 */
    F64 last_r = 0.0;
    U32 iteration_count;
    S32 x_size, y_size;

    array_range_sizes(array_values, &x_size, &y_size);

    exec_func_ignore_parms();

    /* we must converge in 40 guesses */
    for(iteration_count = 0; iteration_count < 40; ++iteration_count)
    {
        F64 this_npv;
        F64 this_r;

        if( (-1.0 == r) /* will divide by zero */||
            (fabs(r) >= 1.0E6) /* heading off to infinity */ )
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_IRR);
            return;
        }

        { /* recalculate NPV for the array of values with trial rate r */
        S32 ix, iy;
        F64 sum = 0.0;
        F64 multiplier = 1.0;
        F64 one_over_one_plus_r = 1.0 / (1.0 + r);

        for(ix = 0; ix < x_size; ++ix)
        {
            for(iy = 0; iy < y_size; ++iy)
            {
                EV_DATA ev_data;

                if(RPN_DAT_ERROR == array_range_index(&ev_data, array_values, ix, iy, EM_REA)) /* blanks == 0 */
                {
                    *p_ev_data_res = ev_data;
                    return;
                }

                multiplier *= one_over_one_plus_r; /* progressively smaller */

                sum += ev_data.arg.fp * multiplier;
            }
        }

        this_npv = sum;
        this_r = r;
        } /*block*/

        /*reportf(TEXT("ODF.IRR loop %d: this_npv=%g, this_r=%g"), iteration_count, this_npv, this_r);*/
        /* finish condition is target of npv ~= 0 */
        if(fabs(this_npv) < 0.0000001)
        {
            ev_data_set_real(p_ev_data_res, this_r);
            return;
        }

        if(0 == iteration_count)
        {   /* generate another point */
            r /= 2.0;
        }
        else
        {   /* new trial rate calculated using secant method */
            F64 step_r = this_npv * (this_r - last_r) / (this_npv - last_npv);
            /*reportf(TEXT("ODF.IRR loop %d: this_npv=%g, last_npv=%g, this_r=%g, last_r=%g -> step_r=%g"), iteration_count, this_npv, last_npv, this_r, last_r, step_r);*/
            r -= step_r;
        }

        /* go for another iteration if allowed */
        last_npv = this_npv;
        last_r = this_r;
    }

    ev_data_set_error(p_ev_data_res, EVAL_ERR_IRR);
}

/******************************************************************************
*
* REAL pmt(principal, interest, term)
*
* REAL odf.pmt(rate, nper, pv)
*
******************************************************************************/

_Check_return_
static F64
calc_pmt(
    _InVal_     F64 principal,
    _InVal_     F64 interest,
    _InVal_     F64 term)
{
    /* principal * interest / (1-(interest+1)^(-term)) */
    return(principal * interest / (1.0 - pow(interest + 1.0, - term)));
}

PROC_EXEC_PROTO(c_pmt)
{
    F64 principal = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp;
    F64 term = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* pmt(principal, interest, term) = principal * interest / (1-(interest+1)^(-term)) */
    ev_data_set_real(p_ev_data_res, calc_pmt(principal, interest, term));
}

PROC_EXEC_PROTO(c_odf_pmt)
{
    F64 rate = args[0]->arg.fp;
    F64 nper = args[1]->arg.fp;
    F64 pv = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* odf.pmt(rate, nper, pv) = - pv * rate / (1-(rate+1)^(-nper)) */
    ev_data_set_real(p_ev_data_res, - calc_pmt(pv, rate, nper));
}

/******************************************************************************
*
* REAL pv(payment, interest, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pv)
{
    F64 payment = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp;
    F64 term = args[2]->arg.fp;

    /* payment * (1-(1+interest)^(-term) / interest */
    F64 pv_result = payment * (1.0 - pow(1.0 + interest, -term)) / interest;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, pv_result);
}

/******************************************************************************
*
* REAL rate(fv, pv, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rate)
{
    F64 fv = args[0]->arg.fp;
    F64 pv = args[1]->arg.fp;
    F64 term = args[2]->arg.fp;

    /* rate(fv, pv, term) = (fv / pv) ^ (1/term) -1  */
    F64 rate_result = pow((fv / pv), 1.0 / term) - 1.0;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, rate_result);
}

/******************************************************************************
*
* REAL sln(cost, salvage, life)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sln)
{
    F64 cost = args[0]->arg.fp;
    F64 salvage = args[1]->arg.fp;
    F64 life = args[2]->arg.fp;
    F64 sln_result = (cost - salvage) / life;

    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, sln_result);
}

/******************************************************************************
*
* REAL syd(cost, salvage, life, period)
*
******************************************************************************/

PROC_EXEC_PROTO(c_syd)
{
    F64 cost = args[0]->arg.fp;
    F64 salvage = args[1]->arg.fp;
    F64 life = args[2]->arg.fp;
    F64 period = args[3]->arg.fp;
    F64 syd_result;

    exec_func_ignore_parms();

    /* syd(cost, salvage, life, period) = (cost-salvage) * (life-period+1) / (life*(life+1)/2) */
    syd_result = ((cost - salvage) * (life - period + 1.0)) / ((life * (life + 1.0) * 0.5));

    ev_data_set_real(p_ev_data_res, syd_result);
}

/******************************************************************************
*
* REAL term(payment, interest, fv)
*
* REAL nper(rate, payment, pv)
*
******************************************************************************/

_Check_return_
static F64
term_nper_common(
    _InVal_     F64 payment,
    _InVal_     F64 interest,
    _InVal_     F64 fv)
{
    /* term(payment, interest, fv) = ln(1+(fv * interest/payment)) / ln(1+interest) */
    F64 numer = log(1.0 + (fv * interest / payment));
    F64 denom = log(1.0 + interest);
    F64 result = numer / denom;
    return(result);
}

PROC_EXEC_PROTO(c_nper)
{
    F64 rate = args[0]->arg.fp;
    F64 payment = args[1]->arg.fp;
    F64 pv = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* Excel: really up to 5 args but I don't yet support that */

    /* Excel: like TERM() but different parameter order and sign of result */
    /* nper(rate, payment, pv) = - ln(1+(pv * rate/payment)) / ln(1+rate) */
    ev_data_set_real(p_ev_data_res, - term_nper_common(payment, rate, pv));
}

PROC_EXEC_PROTO(c_term)
{
    F64 payment = args[0]->arg.fp;
    F64 interest = args[1]->arg.fp ;
    F64 fv = args[2]->arg.fp;

    exec_func_ignore_parms();

    /* term(payment, interest, fv) = ln(1+(fv * interest/payment)) / ln(1+interest) */
    ev_data_set_real(p_ev_data_res, term_nper_common(payment, interest, fv));
}

/* end of ev_fnfin.c */
