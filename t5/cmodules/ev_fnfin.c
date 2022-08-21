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
    F64 interest = ss_data_get_real(args[0]) + 1.0;
    F64 fv = ss_data_get_real(args[1]);
    F64 pv = ss_data_get_real(args[2]);
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
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MIXED_SIGNS);

    if(interest <= F64_MIN)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    cterm_result = (log(fv) - log(pv)) / log(interest);

    ss_data_set_real(p_ss_data_res, cterm_result);
}

/******************************************************************************
*
* REAL db(cost, salvage, life, period {, month})
*
******************************************************************************/

PROC_EXEC_PROTO(c_db)
{
    const F64 cost = ss_data_get_real(args[0]);
    const F64 salvage = ss_data_get_real(args[1]);
    const S32 life = (S32) arg_get_real_INT(args[2]);
    const S32 period = (S32) arg_get_real_INT(args[3]); /* ODF has as Number, but how??? */
    const F64 month = (n_args > 4) ? ss_data_get_real(args[4]) : 12.0;
    F64 rate;
    F64 db_result;

    exec_func_ignore_parms();

    if( cost < 0.0      ||
        salvage < 0.0   ||
        salvage > cost  ||
        life < 1        ||
        month < 1.0     ||
        month > 12.0    ||
        period < 1      )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    if(period > life + (12.0 != month))
    {   /* subsequent periods have a depreciation allowance of zero */
        *p_ss_data_res = ss_data_real_zero;
        return;
    }

    /* rate = 1 - ((salvage / cost) ^ (1 / life)), rounded to three decimal places */
    rate = 1.0 - pow((salvage / cost), (1.0 / life));
    rate *= 1000;
    rate = (S32) (rate + 0.5);
    rate /= 1000;

    /* depreciation during a period = (cost - total depreciation from prior periods) * rate */
    if(1 == period)
    {   /* depreciation during first period (adjusted for month) */
        db_result = ((cost * rate) * month) / 12.0;
    }
    else
    {
        F64 total_depreciation = ((cost * rate) * month) / 12.0; /* depreciation during first period, adjust for month */
        S32 prev_period;

        /* depreciation during subsequent periods */
        for(prev_period = 2; prev_period < period; ++prev_period)
        {
            F64 this_db = (cost - total_depreciation) * rate;

            total_depreciation += this_db;
        }

        /* depreciation during this period */
        db_result = (cost - total_depreciation) * rate;

        if((period > life) && (12.0 != month))
            db_result *= (12.0 - month) / 12.0; /* adjust for month */
    }

    ss_data_set_real(p_ss_data_res, db_result);
}

/******************************************************************************
*
* REAL ddb(cost, salvage, life, period {, factor})
*
******************************************************************************/

PROC_EXEC_PROTO(c_ddb)
{
    F64 ddb_result;
    F64 cost = ss_data_get_real(args[0]);
    F64 value = cost;
    F64 salvage = ss_data_get_real(args[1]);
    F64 life = ss_data_get_real(args[2]);
    F64 period = ss_data_get_real(args[3]);
    F64 factor = (n_args > 4) ? ss_data_get_real(args[4]) : 2.0;
    F64 rate;
    S32 i;

    exec_func_ignore_parms();

    if( cost < 0.0      ||
        salvage < 0.0   ||
        salvage > cost  ||
        life < 1.0      ||
        period < 1.0    ||
        period > life   )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    rate = factor / life;

    if(floor(period) == period)
    {   /* integer period */
        F64 depreciation_of_period = 0.0;
        S32 i_period = (S32) period;

        for(i = 0; i < i_period; ++i)
        {
            depreciation_of_period = value * rate;
            if(depreciation_of_period > value - salvage)
                depreciation_of_period = value - salvage;
            value -= depreciation_of_period;
        }

        ddb_result = depreciation_of_period;
    }
    else
    {   /* non-integer period */
        F64 old_value, new_value;

        if(rate >= 1.0)
        {
            rate = 1.0;

            old_value = (period == 1.0) ? cost : 0.0;
        }
        else
        {
            old_value = cost * pow(1.0 - rate, period - 1.0);
        }

        new_value = cost * pow(1.0 - rate, period);

        ddb_result = old_value - ((new_value > salvage) ? new_value : salvage);

        if(ddb_result < 0.0)
            ddb_result = 0.0;
    }

    ss_data_set_real(p_ss_data_res, ddb_result);
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
    const F64 payment = ss_data_get_real(args[0]);
    const F64 interest = ss_data_get_real(args[1]);
    const F64 term = ss_data_get_real(args[2]);
    F64 fv_result;

    exec_func_ignore_parms();

    /* fv(payment, interest, term) = payment * ((1 + interest) ^ term - 1) / interest */
    fv_result = calc_fv(payment, interest, term);

    ss_data_set_real(p_ss_data_res, fv_result);
}

PROC_EXEC_PROTO(c_odf_fv)
{
    const F64 interest = ss_data_get_real(args[0]);
    const F64 term = ss_data_get_real(args[1]);
    const F64 payment = ss_data_get_real(args[2]);
    F64 odf_fv_result;

    exec_func_ignore_parms();

    /* odf.fv(interest, term, payment) = - payment * ((1 + interest) ^ term - 1) / interest */
    odf_fv_result = - calc_fv(payment,  interest,  term);

    if(n_args > 3)
    {
        const F64 pv = ss_data_get_real(args[3]);
        const F64 fv = pv * pow(1.0 + interest, term);

        odf_fv_result -= fv;
    }

    ss_data_set_real(p_ss_data_res, odf_fv_result);
}

/******************************************************************************
*
* REAL fvschedule(principal, schedule)
*
******************************************************************************/

PROC_EXEC_PROTO(c_fvschedule)
{
    const F64 principal = ss_data_get_real(args[0]);
    const PC_SS_DATA array_schedule = args[1];
    F64 fvschedule_result = principal;
    S32 x_size, y_size;
    S32 ix, iy;

    array_range_sizes(array_schedule, &x_size, &y_size);

    exec_func_ignore_parms();

    for(ix = 0; ix < x_size; ++ix)
    {
        for(iy = 0; iy < y_size; ++iy)
        {
            SS_DATA ss_data;
            F64 interest;

            if(DATA_ID_ERROR == array_range_index(&ss_data, array_schedule, ix, iy, EM_REA)) /* blanks == 0 */
            {
                *p_ss_data_res = ss_data;
                return;
            }

            interest = 1.0 + ss_data_get_real(&ss_data);

            fvschedule_result *= interest; /* product */
        }
    }

    ss_data_set_real(p_ss_data_res, fvschedule_result);
}

/******************************************************************************
*
* REAL odf.irr(principal, values, {guess:number=0.1})
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_irr)
{
    const PC_SS_DATA array_values = args[0];
    F64 r = (n_args > 1) ? ss_data_get_real(args[1]) : 0.1; /* usually in 0..1 */
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
            exec_func_status_return(p_ss_data_res, EVAL_ERR_IRR);
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
                SS_DATA ss_data;

                if(DATA_ID_ERROR == array_range_index(&ss_data, array_values, ix, iy, EM_REA)) /* blanks == 0 */
                {
                    *p_ss_data_res = ss_data;
                    return;
                }

                multiplier *= one_over_one_plus_r; /* progressively smaller */

                sum += ss_data_get_real(&ss_data) * multiplier;
            }
        }

        this_npv = sum;
        this_r = r;
        } /*block*/

        /*reportf(TEXT("ODF.IRR loop %d: this_npv=%g, this_r=%g"), iteration_count, this_npv, this_r);*/
        /* finish condition is target of npv ~= 0 */
        if(fabs(this_npv) < 0.0000001)
        {
            ss_data_set_real(p_ss_data_res, this_r);
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

    exec_func_status_return(p_ss_data_res, EVAL_ERR_IRR);
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
    F64 principal = ss_data_get_real(args[0]);
    F64 interest = ss_data_get_real(args[1]);
    F64 term = ss_data_get_real(args[2]);

    exec_func_ignore_parms();

    /* pmt(principal, interest, term) = principal * interest / (1-(interest+1)^(-term)) */
    ss_data_set_real(p_ss_data_res, calc_pmt(principal, interest, term));
}

PROC_EXEC_PROTO(c_odf_pmt)
{
    F64 rate = ss_data_get_real(args[0]);
    F64 nper = ss_data_get_real(args[1]);
    F64 pv = ss_data_get_real(args[2]);

    exec_func_ignore_parms();

    /* odf.pmt(rate, nper, pv) = - pv * rate / (1-(rate+1)^(-nper)) */
    ss_data_set_real(p_ss_data_res, - calc_pmt(pv, rate, nper));
}

/******************************************************************************
*
* REAL pv(payment, interest, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pv)
{
    F64 payment = ss_data_get_real(args[0]);
    F64 interest = ss_data_get_real(args[1]);
    F64 term = ss_data_get_real(args[2]);

    /* payment * (1-(1+interest)^(-term) / interest */
    F64 pv_result = payment * (1.0 - pow(1.0 + interest, -term)) / interest;

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, pv_result);
}

/******************************************************************************
*
* REAL rate(fv, pv, term)
*
******************************************************************************/

PROC_EXEC_PROTO(c_rate)
{
    F64 fv = ss_data_get_real(args[0]);
    F64 pv = ss_data_get_real(args[1]);
    F64 term = ss_data_get_real(args[2]);

    /* rate(fv, pv, term) = (fv / pv) ^ (1/term) -1  */
    F64 rate_result = pow((fv / pv), 1.0 / term) - 1.0;

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, rate_result);
}

/******************************************************************************
*
* REAL sln(cost, salvage, life)
*
******************************************************************************/

PROC_EXEC_PROTO(c_sln)
{
    F64 cost = ss_data_get_real(args[0]);
    F64 salvage = ss_data_get_real(args[1]);
    F64 life = ss_data_get_real(args[2]);
    F64 sln_result = (cost - salvage) / life;

    exec_func_ignore_parms();

    ss_data_set_real(p_ss_data_res, sln_result);
}

/******************************************************************************
*
* REAL syd(cost, salvage, life, period)
*
******************************************************************************/

PROC_EXEC_PROTO(c_syd)
{
    F64 cost = ss_data_get_real(args[0]);
    F64 salvage = ss_data_get_real(args[1]);
    F64 life = ss_data_get_real(args[2]);
    F64 period = ss_data_get_real(args[3]);
    F64 syd_result;

    exec_func_ignore_parms();

    /* syd(cost, salvage, life, period) = (cost-salvage) * (life-period+1) / (life*(life+1)/2) */
    syd_result = ((cost - salvage) * (life - period + 1.0)) / ((life * (life + 1.0) * 0.5));

    ss_data_set_real(p_ss_data_res, syd_result);
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
    F64 rate = ss_data_get_real(args[0]);
    F64 payment = ss_data_get_real(args[1]);
    F64 pv = ss_data_get_real(args[2]);

    exec_func_ignore_parms();

    /* Excel: really up to 5 args but I don't yet support that */

    /* Excel: like TERM() but different parameter order and sign of result */
    /* nper(rate, payment, pv) = - ln(1+(pv * rate/payment)) / ln(1+rate) */
    ss_data_set_real(p_ss_data_res, - term_nper_common(payment, rate, pv));
}

PROC_EXEC_PROTO(c_term)
{
    F64 payment = ss_data_get_real(args[0]);
    F64 interest = ss_data_get_real(args[1]);
    F64 fv = ss_data_get_real(args[2]);

    exec_func_ignore_parms();

    /* term(payment, interest, fv) = ln(1+(fv * interest/payment)) / ln(1+interest) */
    ss_data_set_real(p_ss_data_res, term_nper_common(payment, interest, fv));
}

/* end of ev_fnfin.c */
