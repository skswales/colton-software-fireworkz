/* ev_eval.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Evaluator outer control/stack */

/* MRJC March 1991 / May 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtri.h"

/*
internal functions
*/

/*ncr*/
static S32
custom_result(
    _Inout_opt_ P_EV_DATA p_ev_data,
    _InVal_     STATUS error);

static S32
process_control_for_cond(
    P_STACK_CONTROL_LOOP p_stack_control_loop,
    _InVal_     S32 step);

static S32
process_control_search(
    _InVal_     S32 block_start,
    _InVal_     S32 block_end,
    _InVal_     S32 block_end_maybe1,
    _InVal_     S32 block_end_maybe2,
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     EV_ROW offset,
    _InVal_     S32 stack_after,
    _InVal_     S32 error);

static void
cell_add_to_ranges_affected(
    _InRef_     PC_EV_SLR p_ev_slr);

/*ncr*/
static STATUS
cell_set_error(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     STATUS error);

_Check_return_
_Ret_maybenull_
static P_STACK_ENTRY
stack_back_search_loop(
    _InVal_     S32 loop_type);

/*
data
*/

P_STACK_ENTRY stack_base = NULL; /* STACK_ENTRY[stack_size] and 0 <= stack_offset < stack_size (stack_base[stack_offset] refers to TOS element) */
U32 stack_offset = 0;
U32 stack_size = 0;

static STACK_FLAGS stack_flags_zero = { 0, 0, 0, 0 };

static ARRAY_HANDLE h_ranges_affected = 0;

/*
global flags about trees
*/

GLOBAL_FLAGS global_flags;

#if TRACE_ALLOWED

/*
this is a local tracer routine
*/

static void
eval_trace(
    _In_z_      PCTSTR tstr)
{
    if_constant(tracing(TRACE_MODULE_EVAL))
    {
#if 0 /* we sometimes call this when stack_offset is 0 and the doc referred to has gone way -> poof */
        U8Z buffer[BUF_EV_LONGNAMLEN];
        EV_SLR ev_slr;

        ev_slr = stack_base[stack_offset].slr;
        ev_slr.ext_ref = 1;

        ev_dec_slr_buf(buffer, elemof32(buffer), ev_slr_docno(&ev_slr), &ev_slr);

        trace_v0(TRACE_MODULE_EVAL, buffer);
#endif
        trace_2(TRACE_MODULE_EVAL, TEXT(" [SP=%d] %s"), stack_offset, tstr);
    }
}

#else

#define eval_trace(tstr)

#endif

/******************************************************************************
*
* initialisation for the array_range functions
*
******************************************************************************/

static void
array_range_stat_block_init_AVEDEV(
    _InoutRef_   P_STAT_BLOCK p_stat_block)
{
    p_stat_block->exec_array_range_id = ARRAY_RANGE_AVEDEV;
    p_stat_block->running_data = ev_data_real_zero; /* allows uniform processing - we just accumulate a sum of values in running_data */
    p_stat_block->mean = 0.0; /* substituted by actual mean for second pass */
    p_stat_block->pass = 1;
}

static void
array_range_stat_block_init_MEDIAN(
    _InoutRef_   P_STAT_BLOCK p_stat_block)
{
    p_stat_block->exec_array_range_id = ARRAY_RANGE_MEDIAN;
    p_stat_block->running_data = ev_data_real_zero; /* ensures integer parameters are promoted to real */
    status_consume(ss_array_make(&p_stat_block->statistics_array, 0, 0));
    p_stat_block->pass = 1;
}

static void
array_range_stat_block_init_MULTINOMIAL(
    _InoutRef_   P_STAT_BLOCK p_stat_block)
{
    p_stat_block->exec_array_range_id = ARRAY_RANGE_MULTINOMIAL;
    p_stat_block->running_data = ev_data_real_zero; /* allows uniform processing - we just accumulate a sum of values in running_data */
    ev_data_set_real(&p_stat_block->multinomial_product, 1.0); /* allows uniform processing - we just accumulate a product of values in multinomial_denominator */
}

static void
array_range_stat_block_init_NPV(
    _InoutRef_   P_STAT_BLOCK p_stat_block,
    _OutRef_opt_ P_S32 p_arg_ix,
    P_EV_DATA p_args[])
{
    p_stat_block->exec_array_range_id = ARRAY_RANGE_NPV;

    p_stat_block->npv_rate = p_args[0]->arg.fp + 1.0;

    if(NULL != p_arg_ix)
        *p_arg_ix = 1;
}

static void
array_range_stat_block_init_IRR(
    _InoutRef_   P_STAT_BLOCK p_stat_block,
    _OutRef_opt_ P_S32 p_arg_ix,
    P_EV_DATA p_args[])
{
    p_stat_block->exec_array_range_id = ARRAY_RANGE_IRR;

    p_stat_block->r = p_args[0]->arg.fp; /* usually in 0..1 */
    p_stat_block->last_r = 0.0;
    p_stat_block->last_npv = 0.0;

    /* finish off with same as NPV */
    p_stat_block->npv_rate = p_args[0]->arg.fp + 1.0;

    if(NULL != p_arg_ix)
        *p_arg_ix = 1;
}

static void
array_range_stat_block_init_MIRR(
    _InoutRef_   P_STAT_BLOCK p_stat_block,
    _OutRef_opt_ P_S32 p_n_args,
    P_EV_DATA p_args[])
{
    p_stat_block->exec_array_range_id = ARRAY_RANGE_MIRR;

    p_stat_block->npv_rate = p_args[1]->arg.fp + 1.0;
    p_stat_block->npv_rate_positive = p_args[2]->arg.fp + 1.0;

    if(NULL != p_n_args)
        *p_n_args = 1;
}

static void
array_range_stat_block_init_SKEW_or_KURT(
    _InoutRef_   P_STAT_BLOCK p_stat_block)
{
    /* all these running values must be initialised */
    p_stat_block->mean = 0.0;
    p_stat_block->pass = 1;
    p_stat_block->M2 = 0.0;
    p_stat_block->M3 = 0.0;
    p_stat_block->M4 = 0.0;
}

extern void
array_range_stat_block_init(
    _OutRef_    P_STAT_BLOCK p_stat_block,
    _OutRef_opt_ P_S32 p_n_args,
    _OutRef_opt_ P_S32 p_arg_ix,
    _InVal_     EV_IDNO function_id,
    P_EV_DATA p_args[],
    _InVal_     S32 arg_count)
{
    if(NULL != p_n_args)
        *p_n_args = arg_count;

    if(NULL != p_arg_ix)
        *p_arg_ix = 0;

    zero_struct_ptr(p_stat_block);

    p_stat_block->running_data.did_num = RPN_DAT_REAL;
    p_stat_block->running_data_positive.did_num = RPN_DAT_REAL;

#if CHECKING /* has been useful for debug */
    p_stat_block->_function_id = function_id;
#endif

    switch(function_id)
    {
    case RPN_FNV_MAX:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MAX;
        ev_data_set_integer(&p_stat_block->running_data, 0); /* allows uniform processing - we just accumulate a sum of values in running_data */
        /* MAX ought not to need init but two_nums_type_match may promote the stored value ensure that it is sensible */
        break;

    case RPN_FNV_MAXA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MAXA;
        ev_data_set_integer(&p_stat_block->running_data, 0); /* allows uniform processing - we just accumulate a sum of values in running_data */
        /* MAX ought not to need init but two_nums_type_match may promote the stored value ensure that it is sensible */
        break;

    case RPN_FNV_MIN:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MIN;
        ev_data_set_integer(&p_stat_block->running_data, 0); /* allows uniform processing - we just accumulate a sum of values in running_data */
        /* MIN ought not to need init but two_nums_type_match may promote the stored value ensure that it is sensible */
        break;

    case RPN_FNV_MINA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_MINA;
        ev_data_set_integer(&p_stat_block->running_data, 0); /* allows uniform processing - we just accumulate a sum of values in running_data */
        /* MIN ought not to need init but two_nums_type_match may promote the stored value ensure that it is sensible */
        break;

    case RPN_FNV_SUM:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_SUM;
        ev_data_set_integer(&p_stat_block->running_data, 0); /* allows uniform processing - we just accumulate a sum of values in running_data */
        /* NB stay in integers for the simplest functions for ARM */
        break;

    case RPN_FNV_AVG:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_AVERAGE;
        ev_data_set_integer(&p_stat_block->running_data, 0); /* allows uniform processing - we just accumulate a sum of values in running_data */
        /* NB stay in integers for the simplest functions for ARM */
        break;

    case RPN_FNV_AVERAGEA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_AVERAGEA;
        ev_data_set_integer(&p_stat_block->running_data, 0); /* allows uniform processing - we just accumulate a sum of values in running_data */
        /* NB stay in integers for the simplest functions for ARM */
        break;

    case RPN_FNV_AND:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_AND;
        ev_data_set_boolean(&p_stat_block->running_data, TRUE); /* allows uniform processing - first FALSE will clear this */
        break;

    case RPN_FNV_OR:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_OR;
        ev_data_set_boolean(&p_stat_block->running_data, FALSE); /* allows uniform processing - first TRUE will set this */
        break;

    case RPN_FNV_XOR:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_XOR;
        ev_data_set_boolean(&p_stat_block->running_data, FALSE); /* allows uniform processing - first TRUE will negate this */
        break;

    case RPN_FNV_AVEDEV:
        array_range_stat_block_init_AVEDEV(p_stat_block);
        break;

    case RPN_FNV_GEOMEAN:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_GEOMEAN;
        p_stat_block->running_data = ev_data_real_zero; /* allows uniform processing - we just accumulate a sum of values in running_data */
        break;

    case RPN_FNV_HARMEAN:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_HARMEAN;
        p_stat_block->running_data = ev_data_real_zero; /* allows uniform processing - we just accumulate a sum of values in running_data */
        break;

    case RPN_FNV_SUMSQ:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_SUMSQ;
        p_stat_block->running_data = ev_data_real_zero; /* allows uniform processing - we just accumulate a sum of values in running_data */
        break;

    case RPN_FNV_MEDIAN:
        array_range_stat_block_init_MEDIAN(p_stat_block);
        break;

    case RPN_FNV_MULTINOMIAL:
        array_range_stat_block_init_MULTINOMIAL(p_stat_block);
        break;

    case RPN_FNF_IRR:
        array_range_stat_block_init_IRR(p_stat_block, p_arg_ix, p_args);
        break;

    case RPN_FNF_MIRR:
        array_range_stat_block_init_MIRR(p_stat_block, p_n_args, p_args); /* NB p_n_args */
        break;

    case RPN_FNF_NPV:
        array_range_stat_block_init_NPV(p_stat_block, p_arg_ix, p_args);
        break;

    case RPN_FNV_PRODUCT:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_PRODUCT;
        ev_data_set_real(&p_stat_block->running_data, 1.0); /* allows uniform processing - we just accumulate a product of values in running_data */
        break;

    case RPN_FNV_KURT:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_KURT;
        array_range_stat_block_init_SKEW_or_KURT(p_stat_block);
        break;

    case RPN_FNV_SKEW:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_SKEW;
        array_range_stat_block_init_SKEW_or_KURT(p_stat_block);
        break;

    case RPN_FNV_SKEW_P:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_SKEW_P;
        array_range_stat_block_init_SKEW_or_KURT(p_stat_block);
        break;

    /* no other priming is required or is sensible for these ones */

    case RPN_FNV_COUNT:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_COUNT;
        break;

    case RPN_FNV_COUNTA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_COUNTA;
        break;

    case RPN_FNF_COUNTBLANK:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_COUNTBLANK;
        break;

    case RPN_FNV_DEVSQ:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_DEVSQ;
        break;

    case RPN_FNV_STD:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_STD;
        break;

    case RPN_FNV_STDEVA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_STDEVA;
        break;

    case RPN_FNV_STDEVPA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_STDEVPA;
        break;

    case RPN_FNV_STDP:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_STDP;
        break;

    case RPN_FNV_VAR:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_VAR;
        break;

    case RPN_FNV_VARA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_VARA;
        break;

    case RPN_FNV_VARP:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_VARP;
        break;

    case RPN_FNV_VARPA:
        p_stat_block->exec_array_range_id = ARRAY_RANGE_VARPA;
        break;

    default: default_unhandled();
        break;
    }
}

/******************************************************************************
*
* process an individual item for NPV / MIRR
*
******************************************************************************/

static void
npv_item(
    _InRef_     PC_F64 p_fp_arg,
    _InRef_     PC_F64 p_npv_rate,
    _InoutRef_  P_F64 p_last_npv_rate,  /*_Out_ on first pass */
    _InoutRef_  P_F64 p_result,         /*_Out_ on first pass */
    _InVal_     S32 count)
{
    if(0 == count)
    {
        *p_last_npv_rate = *p_npv_rate;
        *p_result = *p_fp_arg / *p_npv_rate;
    }
    else
    {
        *p_last_npv_rate *= *p_npv_rate;
        *p_result += *p_fp_arg / *p_last_npv_rate;
    }
}

/******************************************************************************
*
* mini routine to call add for two data items
*
******************************************************************************/

static void
array_range_proc_item_add(
    P_STAT_BLOCK p_stat_block,
    P_EV_DATA p_ev_data)
{
    if(0 == p_stat_block->count)
    {
        p_stat_block->running_data = *p_ev_data;
    }
    else
    {
        P_EV_DATA args[2];
        EV_DATA result_data;
        static const EV_SLR dummy_slr = EV_SLR_INIT;

        args[0] = p_ev_data;
        args[1] = &p_stat_block->running_data;

        ev_data_set_blank(&result_data);

        c_add(args, 2, &result_data, &dummy_slr);

        p_stat_block->running_data = result_data;
    }
}

/******************************************************************************
*
* process a data item for the array_range functions
*
******************************************************************************/

static void
array_range_proc_item(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data);

static void
array_range_proc_blank(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    UNREFERENCED_PARAMETER_InoutRef_(p_ev_data);

    p_stat_block->count_blank += 1;
}

static void
array_range_proc_date(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    assert(0 != p_stat_block->exec_array_range_id);

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
        if((0 == p_stat_block->count) || (ss_data_compare(&p_stat_block->running_data, p_ev_data, FALSE, FALSE) < 0))
            p_stat_block->running_data = *p_ev_data;
        p_stat_block->count += 1;
        break;

    case ARRAY_RANGE_MIN:
        if((0 == p_stat_block->count) || (ss_data_compare(&p_stat_block->running_data, p_ev_data, FALSE, FALSE) > 0))
            p_stat_block->running_data = *p_ev_data;
        p_stat_block->count += 1;
        break;

    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
        /* cope with dates and times within a SUM by calling the add routine */
        array_range_proc_item_add(p_stat_block, p_ev_data);
        p_stat_block->count += 1;
        break;

    default:
        break;
    }

    p_stat_block->count_a += 1;
}

static void
array_range_proc_number_MAX(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x = p_ev_data->arg.fp;
    if((0 == p_stat_block->count) || (x > p_stat_block->running_data.arg.fp))
        ev_data_set_real(&p_stat_block->running_data, x);
}

static void
array_range_proc_number_MIN(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x = p_ev_data->arg.fp;
    if((0 == p_stat_block->count) || (x < p_stat_block->running_data.arg.fp))
        ev_data_set_real(&p_stat_block->running_data, x);
}

static void
array_range_proc_number_running_data_add(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x = p_ev_data->arg.fp;
    if(0 == p_stat_block->count)
        ev_data_set_real(&p_stat_block->running_data, x);
    else
        p_stat_block->running_data.arg.fp += x;
}

static void
array_range_proc_number_AVEDEV(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(1 == p_stat_block->pass)
    {   /* first pass adds up all values to calculate the mean */
        const F64 x = p_ev_data->arg.fp;
        p_stat_block->running_data.arg.fp += x; /* uniform processing for sum of values */
    }
    else/* 2 == p_stat_block->pass */
    {   /* second pass adds up all |(values - mean)| to calculate the mean absolute deviation */
        const F64 dx = p_ev_data->arg.fp - p_stat_block->mean;
        const F64 x = fabs(dx);
        p_stat_block->running_data.arg.fp += x; /* uniform processing for sum of values */
    }
}

static void
array_range_proc_number_GEOMEAN(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x = log(p_ev_data->arg.fp);
    p_stat_block->running_data.arg.fp += x; /* uniform processing for sum of values */
}

static void
array_range_proc_number_HARMEAN(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x = 1.0 / p_ev_data->arg.fp;
    p_stat_block->running_data.arg.fp += x; /* uniform processing for sum of values */
}

static void
array_range_proc_number_MEDIAN(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(1 == p_stat_block->pass)
    {   /* first pass sizes up the stats array */
    }
    else /* 2 == p_stat_block->pass */
    {   /* second pass adds the data to the stats array */
        /* Note that for large data sets it is quicker to do */
        /* a simple aggregation (O(n)), then a quicksort (O(n.log(n)) average) than */
        /* to perform the obvious insertion sort at this stage (O(n^2) average) */
        const F64 x = p_ev_data->arg.fp; /* two_nums_type_match will have promoted (as we have a dummy real in running_data) */
        const S32 iy = p_stat_block->count;
        ev_data_set_real(ss_array_element_index_wr(&p_stat_block->statistics_array, 0, iy), x);
    }
}

static void
array_range_proc_number_MULTINOMIAL(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x = floor(p_ev_data->arg.fp);
    EV_DATA fact_term;

    p_stat_block->running_data.arg.fp += x; /* uniform processing for sum of values */

    factorial_calc(&fact_term, (S32) x); /* may return integer or fp or error */

    if(!two_nums_multiply_try(&p_stat_block->multinomial_product, &p_stat_block->multinomial_product, &fact_term, TRUE /*propogate_errors*/)) /* uniform processing for product of values */
        ev_data_set_error(&p_stat_block->multinomial_product, EVAL_ERR_ARGRANGE);
}

static void
array_range_proc_number_PRODUCT(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x = p_ev_data->arg.fp;
    p_stat_block->running_data.arg.fp *= x; /* uniform processing for product of values */
}

static void
array_range_proc_number_simple_statistics(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if(0 == p_stat_block->count)
    {
        p_stat_block->shift_value = p_ev_data->arg.fp; /* subtract the first of the samples from all samples to avoid crazy overflow */
        p_stat_block->sum_x2 = 0.0;
        p_stat_block->running_data.arg.fp = 0.0;
    }
    else
    {
        F64 x = p_ev_data->arg.fp - p_stat_block->shift_value;
        F64 x2 = x * x;
        p_stat_block->sum_x2 += x2;
        p_stat_block->running_data.arg.fp += x;
    }
}

static void
array_range_proc_number_KURT_or_SKEW(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
#if defined(ONE_PASS_KURT)
    /* uniform processing - update values incrementally */
    /* For details consult Terriberry, Timothy B. (2007), Computing Higher-Order Moments Online */
    /* Substitute a singleton {x} for one of the two subsets in the pairwise update formulae and you will */
    /* obtain this implementation (after http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance) */
    F64 x = p_ev_data->arg.fp; /* no need for an explicit - p_stat_block->shift_value as we subtract the running mean */
    S32 n = p_stat_block->count + 1; /* running total n including this value */
    S32 n_minus_1 = p_stat_block->count; /* n - 1 */
    F64 delta = (x - p_stat_block->mean);
    F64 delta_n = delta / n; /* a common term in the expressions */
    F64 common_term = delta * delta_n * n_minus_1; /* another common term in the expressions */

    /* pairwise updates - note the update order */
    p_stat_block->mean = p_stat_block->mean + delta_n;

    if(ARRAY_RANGE_KURT == p_stat_block->exec_array_range_id)
    {
        F64 delta_n2 = delta_n * delta_n;
        p_stat_block->M4 = p_stat_block->M4
                         + common_term * delta_n2 * ((F64)n*n - 3*n + 3) /* avoid integer overflow with millions of data values */
                         + 6.0 * delta_n2 * p_stat_block->M2
                         - 4.0 * delta_n * p_stat_block->M3;
    }

    p_stat_block->M3 = p_stat_block->M3
                     + common_term * delta_n * (n - 2)
                     - 3.0 * delta_n * p_stat_block->M2;

    p_stat_block->M2 = p_stat_block->M2
                     + common_term; /* you DO need to maintain a running variance! */
#else
    /* Excel-compatible two-pass method */
    if(1 == p_stat_block->pass)
    {   /* first pass adds up all values to calculate the mean and variance */
        if(0 == p_stat_block->count)
        {
            p_stat_block->shift_value = p_ev_data->arg.fp; /* subtract the first of the samples from all samples to avoid crazy overflow */
            p_stat_block->sum_x2 = 0.0;
            p_stat_block->running_data.arg.fp = 0.0;
        }
        else
        {
            F64 x = p_ev_data->arg.fp - p_stat_block->shift_value;
            F64 x2 = x * x;
            p_stat_block->sum_x2 += x2;
            p_stat_block->running_data.arg.fp += x;
        }
    }
    else /* 2 == p_stat_block->pass */
    {   /* second pass adds up all |(values - mean)|^n */
        F64 x = p_ev_data->arg.fp;
        F64 delta = (x - p_stat_block->mean);
        F64 delta2 = delta*delta;

        if(ARRAY_RANGE_KURT == p_stat_block->exec_array_range_id)
        {
            F64 delta4 = delta2*delta2;

            p_stat_block->M4 += delta4;
        }
        else /* ARRAY_RANGE_SKEW == p_stat_block->exec_array_range_id (or ARRAY_RANGE_SKEW_P) */
        {
            F64 delta3 = delta2*delta;

            p_stat_block->M3 += delta3;
        }
    }
#endif
}

static void
array_range_proc_number_SUMSQ(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 x2 = p_ev_data->arg.fp * p_ev_data->arg.fp;
    p_stat_block->running_data.arg.fp += x2; /* uniform processing for sum of values */
}

static void
array_range_proc_number_NPV_or_IRR(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    npv_item(&p_ev_data->arg.fp,
             &p_stat_block->npv_rate,
             &p_stat_block->last_npv_rate /*filled*/,
             &p_stat_block->running_data.arg.fp,
             p_stat_block->count);
}

static void
array_range_proc_number_MIRR(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    const F64 pos_value = MAX(0.0, p_ev_data->arg.fp);
    const F64 neg_value = MIN(0.0, p_ev_data->arg.fp);

    npv_item(&pos_value,
             &p_stat_block->npv_rate_positive,
             &p_stat_block->last_npv_rate_positive /*filled*/,
             &p_stat_block->running_data_positive.arg.fp,
             p_stat_block->count);

    npv_item(&neg_value,
             &p_stat_block->npv_rate,
             &p_stat_block->last_npv_rate /*filled*/,
             &p_stat_block->running_data.arg.fp,
             p_stat_block->count);

}

_Check_return_
static BOOL
array_range_proc_number_try_integer(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    BOOL int_done = FALSE;

    switch(p_stat_block->exec_array_range_id)
    {
    default:
        return(FALSE);

    case ARRAY_RANGE_MAX:
    case ARRAY_RANGE_MAXA:
        if((0 == p_stat_block->count) || (p_ev_data->arg.integer > p_stat_block->running_data.arg.integer))
            ev_data_set_integer(&p_stat_block->running_data, p_ev_data->arg.integer);
        int_done = 1;
        break;

    case ARRAY_RANGE_MIN:
    case ARRAY_RANGE_MINA:
        if((0 == p_stat_block->count) || (p_ev_data->arg.integer < p_stat_block->running_data.arg.integer))
            ev_data_set_integer(&p_stat_block->running_data, p_ev_data->arg.integer);
        int_done = 1;
        break;

    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
    case ARRAY_RANGE_AVERAGEA:
        /* only dealing with individual narrower integer types here but SKS shows this may eventually overflow WORD16 */
        ev_data_set_integer(&p_stat_block->running_data, p_ev_data->arg.integer + p_stat_block->running_data.arg.integer);
        int_done = 1;
        break;

    case ARRAY_RANGE_AND:
        /* any FALSE value forces the result to be FALSE */
        if(0 == p_ev_data->arg.integer)
            ev_data_set_boolean(&p_stat_block->running_data, FALSE);
        int_done = 1;
        break;

    case ARRAY_RANGE_OR:
        /* any TRUE value forces the result to be TRUE */
        if(0 != p_ev_data->arg.integer)
            ev_data_set_boolean(&p_stat_block->running_data, TRUE);
        int_done = 1;
        break;

    case ARRAY_RANGE_XOR:
        /* each successive TRUE value negates the result */
        if(0 != p_ev_data->arg.integer)
            ev_data_set_boolean(&p_stat_block->running_data, !p_stat_block->running_data.arg.boolean);
        int_done = 1;
        break;
    }

    if(!int_done)
        return(FALSE);

    p_stat_block->count += 1;
    p_stat_block->count_a += 1;
    return(TRUE);
}

static void
array_range_proc_number(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    BOOL size_worry = FALSE;

    assert(0 != p_stat_block->exec_array_range_id);

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
    case ARRAY_RANGE_AVERAGEA:
        size_worry = TRUE; /* need to worry about adding >16-bit integers and overflowing */
        break;

    default:
        break;
    }

    if(TWO_INTS == two_nums_type_match(p_ev_data, &p_stat_block->running_data, size_worry))
        if(array_range_proc_number_try_integer(p_stat_block, p_ev_data))
            return;

    /* two_nums_type_match will have promoted */
    assert(RPN_DAT_REAL == p_ev_data->did_num);
    assert(RPN_DAT_REAL == p_stat_block->running_data.did_num);

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
    case ARRAY_RANGE_MAXA:
        array_range_proc_number_MAX(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_MIN:
    case ARRAY_RANGE_MINA:
        array_range_proc_number_MIN(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_AVERAGE:
    case ARRAY_RANGE_AVERAGEA:
        array_range_proc_number_running_data_add(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_AVEDEV:
        array_range_proc_number_AVEDEV(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_GEOMEAN:
        array_range_proc_number_GEOMEAN(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_HARMEAN:
        array_range_proc_number_HARMEAN(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_MEDIAN:
        array_range_proc_number_MEDIAN(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_MULTINOMIAL:
        array_range_proc_number_MULTINOMIAL(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_PRODUCT:
        array_range_proc_number_PRODUCT(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_DEVSQ:
    case ARRAY_RANGE_STD:
    case ARRAY_RANGE_STDEVA:
    case ARRAY_RANGE_STDEVPA:
    case ARRAY_RANGE_STDP:
    case ARRAY_RANGE_VAR:
    case ARRAY_RANGE_VARA:
    case ARRAY_RANGE_VARP:
    case ARRAY_RANGE_VARPA:
        array_range_proc_number_simple_statistics(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_KURT:
    case ARRAY_RANGE_SKEW:
    case ARRAY_RANGE_SKEW_P:
        array_range_proc_number_KURT_or_SKEW(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_SUMSQ:
        array_range_proc_number_SUMSQ(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_NPV:
    case ARRAY_RANGE_IRR:
        array_range_proc_number_NPV_or_IRR(p_stat_block, p_ev_data);
        break;

    case ARRAY_RANGE_MIRR:
        array_range_proc_number_MIRR(p_stat_block, p_ev_data);
        break;

#if CHECKING
    case ARRAY_RANGE_AND: /* should have ensured integer args */
    case ARRAY_RANGE_OR:
    case ARRAY_RANGE_XOR:
        assert0();

        /*FALLTHRU*/
#endif

#if CHECKING
    case ARRAY_RANGE_COUNT:
    case ARRAY_RANGE_COUNTA:
    case ARRAY_RANGE_COUNTBLANK:
#endif
    default:
        break;
    }

    p_stat_block->count += 1;
    p_stat_block->count_a += 1;
}

static void
array_range_proc_others(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    UNREFERENCED_PARAMETER_InoutRef_(p_ev_data);

    assert(0 != p_stat_block->exec_array_range_id);

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_AVERAGEA:
    case ARRAY_RANGE_MAXA:
    case ARRAY_RANGE_MINA:
    case ARRAY_RANGE_STDEVA:
    case ARRAY_RANGE_STDEVPA:
    case ARRAY_RANGE_VARA:
    case ARRAY_RANGE_VARPA:
        {
        EV_DATA ev_data;
        ev_data_set_integer(&ev_data, 0);
        array_range_proc_item(p_stat_block, &ev_data);
        break;
        }

    default:
        p_stat_block->count_a += 1;
        break;
    }
}

static void
array_range_proc_string(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    assert(0 != p_stat_block->exec_array_range_id);

    if(!ss_string_is_blank(p_ev_data))
    {
        array_range_proc_others(p_stat_block, p_ev_data);
        return;
    }

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_AVERAGEA:
    case ARRAY_RANGE_MAXA:
    case ARRAY_RANGE_MINA:
    case ARRAY_RANGE_STDEVA:
    case ARRAY_RANGE_STDEVPA:
    case ARRAY_RANGE_VARA:
    case ARRAY_RANGE_VARPA:
        {
        EV_DATA ev_data;
        ev_data_set_integer(&ev_data, 0);
        array_range_proc_item(p_stat_block, &ev_data);
        break;
        }

    default:
        p_stat_block->count_blank += 1;
        break;
    }
}

static void
array_range_proc_item(
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_REAL:
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        array_range_proc_number(p_stat_block, p_ev_data);
        break;

    case RPN_DAT_DATE:
        array_range_proc_date(p_stat_block, p_ev_data);
        break;

    case RPN_DAT_BLANK:
        array_range_proc_blank(p_stat_block, p_ev_data);
        break;

    case RPN_DAT_STRING:
        array_range_proc_string(p_stat_block, p_ev_data);
        break;

    default:
        array_range_proc_others(p_stat_block, p_ev_data);
        break;
    }
}

/******************************************************************************
*
* calculate the final result for the array_range functions
*
******************************************************************************/

_Check_return_
static BOOL
array_range_proc_finish_running_data(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {
        switch(p_stat_block->running_data.did_num)
        {
        default: default_unhandled();
#if CHECKING
        case RPN_DAT_DATE:
        case RPN_DAT_REAL:
#endif
            *p_ev_data = p_stat_block->running_data;
            break;

        /*case RPN_DAT_BOOL8:*/ /* really shouldn't occur */
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            ev_data_set_integer(p_ev_data, p_stat_block->running_data.arg.integer);
            break;
        }
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_average(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {
        switch(p_stat_block->running_data.did_num)
        {
        default:
            p_stat_block->running_data.arg.fp = (F64) p_stat_block->running_data.arg.integer;

            /*FALLTHRU*/

        case RPN_DAT_REAL:
            {
            const F64 n = (F64) p_stat_block->count;
            const F64 avg_result = p_stat_block->running_data.arg.fp / n;
            ev_data_set_real_ti(p_ev_data, avg_result);
            break;
            }

        case RPN_DAT_DATE:
            *p_ev_data = p_stat_block->running_data;
            if(EV_DATE_NULL != p_ev_data->arg.ev_date.date)
                p_ev_data->arg.ev_date.date /= p_stat_block->count;
            if(EV_TIME_NULL != p_ev_data->arg.ev_date.time)
                p_ev_data->arg.ev_date.time /= p_stat_block->count;
            ss_date_normalise(&p_ev_data->arg.ev_date);
            break;
        }
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_boolean(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {
        const BOOL boolean_result = (0 != p_stat_block->running_data.arg.boolean); /* anything non-zero is forced TRUE here */
        assert(RPN_DAT_BOOL8 == p_stat_block->running_data.did_num);
        ev_data_set_boolean(p_ev_data, boolean_result);
    }
    else
        ev_data_set_boolean(p_ev_data, FALSE);

    return(TRUE);
}

_Check_return_
static BOOL /* have we finished? */
array_range_proc_finish_AVEDEV(
    _OutRef_    P_EV_DATA p_ev_data,
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_S32 p_arg_ix)
{
    if(0 != p_stat_block->count)
    {
        const F64 n = (F64) p_stat_block->count;

        if(1 == p_stat_block->pass)
        {   /* first pass adds up all values to calculate the mean */
            p_stat_block->mean = p_stat_block->running_data.arg.fp / n;

            /* move to second pass */
            p_stat_block->pass += 1;

            /* satisfy interface */
            *p_ev_data = ev_data_real_zero;

            /* reset variables for another pass */
            p_stat_block->running_data.arg.fp = 0.0;
            p_stat_block->count = p_stat_block->count_a = 0;

            assert(NULL != p_arg_ix);
            *p_arg_ix = 0;
            return(FALSE);
        }
        else /* 2 == p_stat_block->pass */
        {   /* second pass adds up all |(values - mean)| to calculate the mean absolute deviation */
            const F64 avedev_result = p_stat_block->running_data.arg.fp / n;
            ev_data_set_real(p_ev_data, avedev_result);
        }
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_COUNT(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    ev_data_set_integer(p_ev_data, p_stat_block->count);

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_COUNTA(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    ev_data_set_integer(p_ev_data, p_stat_block->count_a);

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_COUNTBLANK(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    ev_data_set_integer(p_ev_data, p_stat_block->count_blank);

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_GEOMEAN(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {   /* we have summed the log(value) */
        const F64 n = (F64) p_stat_block->count;
        const F64 geomean_result = exp(p_stat_block->running_data.arg.fp / n);
        /* exp() overflowed? - don't test for underflow case */
        /* shouldn't happen given exp(sum(log(a[i]))/n) */
        if(F64_HUGE_VAL == geomean_result)
            ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE);
        else
            ev_data_set_real(p_ev_data, geomean_result);
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_HARMEAN(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {
        const F64 n = (F64) p_stat_block->count;
        const F64 harmean_result = n / p_stat_block->running_data.arg.fp;
        ev_data_set_real(p_ev_data, harmean_result);
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_MULTINOMIAL(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    EV_DATA fact_term;

    assert(RPN_DAT_REAL == p_stat_block->running_data.did_num);
    assert(RPN_DAT_REAL == p_stat_block->multinomial_product.did_num);

    /* calculate the factorial of the sum of the values */
    factorial_calc(&fact_term, (S32) p_stat_block->running_data.arg.fp); /* may return integer or fp or error */

    /* divide by the product of the factorial of the values */
    if(!two_nums_divide_try(p_ev_data, &fact_term, &p_stat_block->multinomial_product, TRUE /*propogate_errors*/)) /* uniform processing for product of values */
        ev_data_set_error(p_ev_data, EVAL_ERR_ARGRANGE);

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_PRODUCT(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
        ev_data_set_real(p_ev_data, p_stat_block->running_data.arg.fp);
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

/* helper routines which account for small errors in our friendly floating point */

_Check_return_
static F64
calc_devsq(
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    F64 sum_x2 = p_stat_block->sum_x2;
    F64 sum_2 = p_stat_block->running_data.arg.fp * p_stat_block->running_data.arg.fp;
    F64 n = (F64) p_stat_block->count;
    F64 sum_2_d_n = sum_2 / n;
    F64 devsq = sum_x2 - sum_2_d_n;

    if(devsq < 0.0)
        devsq = 0.0;

    return(devsq);
}

_Check_return_
static BOOL
array_range_proc_finish_DEVSQ(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
    {
        const F64 devsq_result = calc_devsq(p_stat_block);
        ev_data_set_real(p_ev_data, devsq_result);
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static F64
calc_variance(
    _InRef_     P_STAT_BLOCK p_stat_block,
    _InVal_     BOOL population_statistics)
{
    F64 n = (F64) p_stat_block->count;
    F64 n_sum_x2 = n * p_stat_block->sum_x2;
    F64 sum_2 = p_stat_block->running_data.arg.fp * p_stat_block->running_data.arg.fp;
    F64 delta = n_sum_x2 - sum_2;
    F64 variance;

    if(delta < 0.0)
        delta = 0.0;

    variance = delta / n;
    variance /= population_statistics ? n : (n - 1.0);

    return(variance);
}

_Check_return_
static BOOL
array_range_proc_finish_standard_deviation(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    BOOL population_statistics = FALSE;

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_STDP:
    case ARRAY_RANGE_STDEVPA:
        population_statistics = TRUE;
        break;

    default:
        break;
    }

    if(p_stat_block->count > (population_statistics ? 0 /*stdp*/: 1 /*std*/))
    {
        const F64 variance = calc_variance(p_stat_block, population_statistics);
        const F64 standard_deviation = sqrt(variance);

        ev_data_set_real(p_ev_data, standard_deviation);
        return(TRUE);
    }

    ev_data_set_error(p_ev_data, EVAL_ERR_DIVIDEBY0);
    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_variance(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    BOOL population_statistics = FALSE;

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_VARP:
    case ARRAY_RANGE_VARPA:
        population_statistics = TRUE;
        break;

    default:
        break;
    }

    if(p_stat_block->count > (population_statistics ? 0 /*varp*/: 1 /*var*/))
    {
        const F64 variance = calc_variance(p_stat_block, population_statistics);

        ev_data_set_real(p_ev_data, variance);
        return(TRUE);
    }

    ev_data_set_error(p_ev_data, EVAL_ERR_DIVIDEBY0);
    return(TRUE);
}

_Check_return_
static F64
calc_skewness(
    _InRef_     P_STAT_BLOCK p_stat_block,
    _InVal_     BOOL population_statistics)
{
#if defined(ONE_PASS_KURT)
    F64 M2_3 = p_stat_block->M2 * p_stat_block->M2 * p_stat_block->M2; /* M2^3 */
    F64 skewness = p_stat_block->M3 * sqrt((F64) p_stat_block->count / M2_3);
#else
    F64 n = (F64) p_stat_block->count;
    F64 n_term = population_statistics ? (1 / n) : (n / ((n - 1) * (n - 2)));
    F64 variance = p_stat_block->M2;
    F64 standard_deviation = sqrt(variance);
    F64 standard_deviation3 = variance * standard_deviation;
    F64 skewness = n_term * p_stat_block->M3 / standard_deviation3;
#endif

    return(skewness);
}

_Check_return_
static F64
calc_kurtosis(
    _InRef_     P_STAT_BLOCK p_stat_block)
{
#if defined(ONE_PASS_KURT)
    F64 n_M4 = (F64) p_stat_block->count * p_stat_block->M4; /* n * M4 */
    F64 M2_2 = p_stat_block->M2 * p_stat_block->M2; /* M2^2 */
    F64 kurtosis = n_M4 / M2_2 - 3;
#else
    F64 n = (F64) p_stat_block->count;
    F64 n_term = (n * (n + 1)) / ((n - 1) * (n - 2) * (n - 3));
    F64 n_term_3 = ((n - 1) * (n - 1)) / ((n - 2) * (n - 3));
    F64 variance = p_stat_block->M2;
    F64 standard_deviation4 = variance * variance;
    F64 kurtosis = (n_term * p_stat_block->M4 / standard_deviation4) - (3 * n_term_3);
#endif

    return(kurtosis);
}

_Check_return_
static BOOL
array_range_proc_finish_SKEW_or_KURT(
    _OutRef_    P_EV_DATA p_ev_data,
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_S32 p_arg_ix)
{
    BOOL population_statistics = FALSE;

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_SKEW_P:
        population_statistics = TRUE;
        break;

    default:
        break;
    }

    if(ARRAY_RANGE_KURT == p_stat_block->exec_array_range_id)
    {
        if(p_stat_block->count < 4)
        {
            ev_data_set_error(p_ev_data, EVAL_ERR_DIVIDEBY0);
            return(TRUE);
        }
    }
    else /* ARRAY_RANGE_SKEW == p_stat_block->exec_array_range_id (or ARRAY_RANGE_SKEW_P) */
    {
        if(p_stat_block->count < 3)
        {
            ev_data_set_error(p_ev_data, EVAL_ERR_DIVIDEBY0);
            return(TRUE);
        }
    }

    if(1 == p_stat_block->pass)
    {   /* first pass adds up all values to calculate the mean and variance */
        p_stat_block->mean = p_stat_block->running_data.arg.fp / (F64) p_stat_block->count;

        p_stat_block->mean += p_stat_block->shift_value; /* NB all the data was shifted for variance error reduction */

        p_stat_block->M2 = calc_variance(p_stat_block, population_statistics);

        /* move to second pass */
        p_stat_block->pass += 1;

        /* satisfy interface */
        *p_ev_data = ev_data_real_zero;

        /* reset variables for another pass */
        p_stat_block->running_data.arg.fp = 0.0;
        p_stat_block->shift_value = 0.0;
        p_stat_block->count = p_stat_block->count_a = 0;

        assert(NULL != p_arg_ix);
        *p_arg_ix = 0;
        return(FALSE);
    }
    else /* 2 == p_stat_block->pass */
    {
        if(ARRAY_RANGE_KURT == p_stat_block->exec_array_range_id)
        {
            const F64 kurtosis_result = calc_kurtosis(p_stat_block);

            ev_data_set_real(p_ev_data, kurtosis_result);
        }
        else /* ARRAY_RANGE_SKEW == p_stat_block->exec_array_range_id (or ARRAY_RANGE_SKEW_P) */
        {
            const F64 skewness_result = calc_skewness(p_stat_block, population_statistics);

            ev_data_set_real(p_ev_data, skewness_result);
        }
    }

    return(TRUE);
}

static void
array_range_proc_median_result(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    S32 x_size, y_size;
    F64 median_result;

    status_consume(array_sort(&p_stat_block->statistics_array, 0));

    array_range_sizes(&p_stat_block->statistics_array, &x_size, &y_size);

    median_result = median_calc_span(&p_stat_block->statistics_array, 0, y_size);

    ev_data_set_real(p_ev_data, median_result);
}

_Check_return_
static BOOL
array_range_proc_finish_MEDIAN(
    _OutRef_    P_EV_DATA p_ev_data,
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_S32 p_arg_ix)
{
    if(1 == p_stat_block->pass)
    {   /* first pass sizes up the stats array */
        if(0 == p_stat_block->count)
        {
            ev_data_set_error(p_ev_data, EVAL_ERR_NO_VALID_DATA);
            return(TRUE);
        }

        /* create stats array to receive the data */
        if(status_fail(ss_array_make(&p_stat_block->statistics_array, 1, p_stat_block->count)))
        {
            *p_ev_data = p_stat_block->running_data; /* copy error */
            return(TRUE);
        }

        /* move to second pass */
        p_stat_block->pass += 1;

        /* satisfy interface */
        *p_ev_data = ev_data_real_zero;

        /* reset variables for another pass */
        p_stat_block->count = p_stat_block->count_a = 0;

        assert(NULL != p_arg_ix);
        *p_arg_ix = 0;
        return(FALSE);
    }
    else /* 2 == p_stat_block->pass */
    {   /* second pass adds the data to the stats array */
        array_range_proc_median_result(p_ev_data, p_stat_block);
        ss_data_free_resources(&p_stat_block->statistics_array);
    }

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_IRR(
    _OutRef_    P_EV_DATA p_ev_data,
    _InoutRef_  P_STAT_BLOCK p_stat_block,
    _InoutRef_  P_S32 p_arg_ix)
{
    if(0 != p_stat_block->count)
    {
        F64 this_npv = p_stat_block->running_data.arg.fp;
        F64 this_r = p_stat_block->r;

        reportf(TEXT("IRR loop %d: this_npv=%g, this_r=%g"), p_stat_block->iteration_count, this_npv, this_r);
        /* finish condition is target of npv ~= 0 */
        if(fabs(this_npv) < 0.0000001)
        {
            ev_data_set_real(p_ev_data, this_r);
            return(TRUE);
        }

        if(0 == p_stat_block->iteration_count)
        {   /* generate another point */
            p_stat_block->r /= 2.0;
        }
        else
        {   /* new trial rate calculated using secant method */
            F64 step_r = this_npv * (this_r - p_stat_block->last_r) / (this_npv - p_stat_block->last_npv);
            reportf(TEXT("IRR loop %d: this_npv=%g, last_npv=%g, this_r=%g, last_r=%g -> step_r=%g"), p_stat_block->iteration_count, this_npv, p_stat_block->last_npv, this_r, p_stat_block->last_r, step_r);
            p_stat_block->r -= step_r;
        }

        p_stat_block->last_npv = this_npv;
        p_stat_block->last_r = this_r;

        p_stat_block->iteration_count += 1;

        if(p_stat_block->iteration_count >= 40)
        {   /* we must converge in 40 guesses */
            ev_data_set_error(p_ev_data, EVAL_ERR_IRR);
            return(TRUE);
        }

        /* satisfy interface */
        *p_ev_data = ev_data_real_zero;

        /* reset variables for another iteration */
        p_stat_block->npv_rate = p_stat_block->r + 1.0;
        p_stat_block->count = p_stat_block->count_a = 0;

        assert(NULL != p_arg_ix);
        *p_arg_ix = 1;
        return(FALSE);
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_MIRR(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(p_stat_block->count_a > 1)
    {
        const F64 count_a = (F64) p_stat_block->count_a;
        const F64 mirr_result =
            pow(-p_stat_block->running_data_positive.arg.fp * pow(p_stat_block->npv_rate_positive, count_a) / (p_stat_block->running_data.arg.fp * p_stat_block->npv_rate),
                1.0 / (count_a - 1.0)
                ) - 1.0;

        ev_data_set_real(p_ev_data, mirr_result);
    }
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

_Check_return_
static BOOL
array_range_proc_finish_NPV(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     P_STAT_BLOCK p_stat_block)
{
    if(0 != p_stat_block->count)
        *p_ev_data = p_stat_block->running_data;
    else
        *p_ev_data = ev_data_real_zero;

    return(TRUE);
}

/*ncr*/
static BOOL /* have we finished? */
array_range_proc_finish(
    _OutRef_    P_EV_DATA p_ev_data,
    P_STAT_BLOCK p_stat_block,
    P_S32 p_arg_ix)
{
    assert(0 != p_stat_block->exec_array_range_id);

    switch(p_stat_block->exec_array_range_id)
    {
    case ARRAY_RANGE_MAX:
    case ARRAY_RANGE_MAXA:
    case ARRAY_RANGE_MIN:
    case ARRAY_RANGE_MINA:
    case ARRAY_RANGE_SUM:
    case ARRAY_RANGE_SUMSQ:
        return(array_range_proc_finish_running_data(p_ev_data, p_stat_block));

    case ARRAY_RANGE_AVERAGE:
    case ARRAY_RANGE_AVERAGEA:
        return(array_range_proc_finish_average(p_ev_data, p_stat_block));

    case ARRAY_RANGE_AND:
    case ARRAY_RANGE_OR:
    case ARRAY_RANGE_XOR:
        return(array_range_proc_finish_boolean(p_ev_data, p_stat_block));

    case ARRAY_RANGE_AVEDEV:
        return(array_range_proc_finish_AVEDEV(p_ev_data, p_stat_block, p_arg_ix));

    case ARRAY_RANGE_COUNT:
        return(array_range_proc_finish_COUNT(p_ev_data, p_stat_block));

    case ARRAY_RANGE_COUNTA:
        return(array_range_proc_finish_COUNTA(p_ev_data, p_stat_block));

    case ARRAY_RANGE_COUNTBLANK:
        return(array_range_proc_finish_COUNTBLANK(p_ev_data, p_stat_block));

    case ARRAY_RANGE_DEVSQ:
        return(array_range_proc_finish_DEVSQ(p_ev_data, p_stat_block));

    case ARRAY_RANGE_GEOMEAN:
        return(array_range_proc_finish_GEOMEAN(p_ev_data, p_stat_block));

    case ARRAY_RANGE_HARMEAN:
        return(array_range_proc_finish_HARMEAN(p_ev_data, p_stat_block));

    case ARRAY_RANGE_MEDIAN:
        return(array_range_proc_finish_MEDIAN(p_ev_data, p_stat_block, p_arg_ix));

    case ARRAY_RANGE_MULTINOMIAL:
        return(array_range_proc_finish_MULTINOMIAL(p_ev_data, p_stat_block));

    case ARRAY_RANGE_PRODUCT:
        return(array_range_proc_finish_PRODUCT(p_ev_data, p_stat_block));

    case ARRAY_RANGE_STD:
    case ARRAY_RANGE_STDP:
    case ARRAY_RANGE_STDEVA:
    case ARRAY_RANGE_STDEVPA:
        return(array_range_proc_finish_standard_deviation(p_ev_data, p_stat_block));

    case ARRAY_RANGE_VAR:
    case ARRAY_RANGE_VARP:
    case ARRAY_RANGE_VARA:
    case ARRAY_RANGE_VARPA:
        return(array_range_proc_finish_variance(p_ev_data, p_stat_block));

#if defined(ONE_PASS_KURT)
    case ARRAY_RANGE_KURT:
        return(array_range_proc_kurt_result(p_ev_data, p_stat_block));

    case ARRAY_RANGE_SKEW:
    case ARRAY_RANGE_SKEW_P:
        return(array_range_proc_skew_result(p_ev_data, p_stat_block, population_statistics));
#else
    case ARRAY_RANGE_KURT:
    case ARRAY_RANGE_SKEW:
    case ARRAY_RANGE_SKEW_P:
        return(array_range_proc_finish_SKEW_or_KURT(p_ev_data, p_stat_block, p_arg_ix));
#endif

    case ARRAY_RANGE_NPV:
        return(array_range_proc_finish_NPV(p_ev_data, p_stat_block));

    case ARRAY_RANGE_IRR:
        return(array_range_proc_finish_IRR(p_ev_data, p_stat_block, p_arg_ix));

    case ARRAY_RANGE_MIRR:
        return(array_range_proc_finish_MIRR(p_ev_data, p_stat_block));

    default: default_unhandled();
        return(TRUE);
    }
}

/******************************************************************************
*
* dispose of an array_range block
*
******************************************************************************/

static void
array_range_block_dispose(
    P_STACK_ARRAY_RANGE p_stack_array_range)
{
    P_ARRAY_RANGE_BLOCK p_array_range_block = p_stack_array_range->p_array_range_block;
    S32 ix;
    for(ix = 0; ix < p_array_range_block->n_args; ++ix)
        ss_data_free_resources(&p_array_range_block->args[ix]);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_stack_array_range->p_array_range_block));
}

/******************************************************************************
*
* jump to a given row in a macro
*
******************************************************************************/

static S32
custom_jmp(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     EV_ROW offset,
    _InVal_     S32 stack_after)
{
    P_STACK_ENTRY p_stack_entry = stack_back_search(stack_offset, EXECUTING_MACRO);

    if(NULL == p_stack_entry)
        stack_base[stack_offset].stack_flags.type = INTERNAL_ERROR;
    else
    {
        P_EV_CUSTOM p_ev_custom;
        S32 res;
        EV_ROW first_row, last_row;
        EV_SLR slr;

        res = 0;

        p_ev_custom = array_ptr(&custom_def.h_table, EV_CUSTOM, p_stack_entry->data.stack_executing_custom.custom_num);

        last_row  = ev_numrow(ev_slr_docno(p_ev_slr));
        first_row = p_ev_custom->owner.row;
        slr       = *p_ev_slr;
        slr.row  += offset;

        if(slr.docno != stack_base[stack_offset].slr.docno || /* equivalent UBF */
             slr.col != stack_base[stack_offset].slr.col   || /* equivalent UBF */
             slr.row >= last_row             ||
             slr.row <= first_row)
            res = create_error(EVAL_ERR_BADGOTO);
        else
            p_stack_entry->data.stack_executing_custom.next_slot = slr;

        if(res < 0)
            custom_result(NULL, res);
        else
            stack_set(stack_after);
    }

    return(NEW_STATE);
}

/******************************************************************************
*
* finish custom function returning result given
*
******************************************************************************/

static void
custom_result_slr_deref(
    _InoutRef_  P_EV_DATA p_ev_data)
{
    if((RPN_DAT_SLR == p_ev_data->did_num) && ev_doc_check_custom(ev_slr_docno(&p_ev_data->arg.slr)))
        ev_slr_deref(p_ev_data, &p_ev_data->arg.slr);
}

/*ncr*/
static S32
custom_result(
    _Inout_opt_ P_EV_DATA p_ev_data,
    _InVal_     STATUS error)
{
    P_EV_DATA p_ev_data_res, p_ev_data_final;
    P_STACK_ENTRY p_stack_entry_macro;

    /* if we were simply passed an error... */
    if(NULL == p_ev_data)
    {
        static EV_DATA err_res_data = { RPN_DAT_BLANK };
        ev_data_set_error(&err_res_data, error);
        p_ev_data = &err_res_data;
    }

    /* find the most recent macro */
    if(NULL == (p_stack_entry_macro = stack_back_search(stack_offset, EXECUTING_MACRO)))
        stack_base[stack_offset].stack_flags.type = INTERNAL_ERROR;
    else
    {
        /* point to macro result area on stack */
        p_ev_data_res = stack_index_ptr_data(p_stack_entry_macro->data.stack_executing_custom.stack_base, -2);

        if(p_stack_entry_macro->data.stack_executing_custom.in_array)
        {
            P_EV_DATA p_array_element;

            p_array_element = ss_array_element_index_wr(p_ev_data_res,
                                                     p_stack_entry_macro->data.stack_executing_custom.x_pos,
                                                     p_stack_entry_macro->data.stack_executing_custom.y_pos);

            if(data_is_array_range(p_ev_data))
                ev_data_set_error(p_array_element, EVAL_ERR_NESTEDARRAY);
            else
            {
                status_assert(ss_data_resource_copy(p_array_element, p_ev_data));
                custom_result_slr_deref(p_array_element);
            }

            p_ev_data_final = p_array_element;
        }
        else
        {
            status_assert(ss_data_resource_copy(p_ev_data_res, p_ev_data));
            custom_result_slr_deref(p_ev_data);
            p_ev_data_final = p_ev_data_res;
        }

        if(ev_data_is_error(p_ev_data_final))
        {
            P_STACK_ENTRY p_stack_entry = &stack_base[stack_offset];
            p_ev_data_final->arg.ev_error.type = ERROR_CUSTOM;
            p_ev_data_final->arg.ev_error.docno = p_stack_entry->slr.docno; /* equivalent UBF */
            p_ev_data_final->arg.ev_error.col = p_stack_entry->slr.col;
            p_ev_data_final->arg.ev_error.row = p_stack_entry->slr.row;
        }

        /* reset stack to executing macro */
        stack_set(stack_offset(p_stack_entry_macro));

        /* switch to custom_complete */
        stack_base[stack_offset].stack_flags.type = MACRO_COMPLETE;
    }

    return(NEW_STATE);
}

/******************************************************************************
*
* do the sequential steps invloved
* in custom function execution
*
******************************************************************************/

static S32
custom_sequence(
    P_STACK_ENTRY p_stack_entry)
{
    EV_SLR next_slot = p_stack_entry->data.stack_executing_custom.next_slot;
    EV_SLR last_slot = p_stack_entry->data.stack_executing_custom.next_slot;
    S32 res = STATUS_OK;

    /* get pointer to result */

    ++p_stack_entry->data.stack_executing_custom.next_slot.row;
    last_slot.row = ev_numrow(ev_slr_docno(&last_slot));

    /* have we gone off end of file ? */
    if(next_slot.row >= last_slot.row)
        res = create_error(EVAL_ERR_NORETURN);
    /* switch to calculate next cell in macro */
    else if(status_ok(res = stack_check_n(1)))
    {
        P_STACK_ENTRY p_stack_entry;

        assert(0 == res);

        stack_offset += 1;

        p_stack_entry = &stack_base[stack_offset];

        p_stack_entry->slr = next_slot;
        p_stack_entry->stack_flags = stack_flags_zero;
        p_stack_entry->stack_flags.type = CALC_SLOT;

        p_stack_entry->stack_flags.inmacro = 1;
        p_stack_entry->data.stack_in_calc.eval_block.p_ev_cell = NULL;
    }

    /* if we get an error, abort and go to complete state */
    if(status_fail(res))
    {
        P_EV_DATA p_ev_data_res;

        p_ev_data_res = stack_index_ptr_data(p_stack_entry->data.stack_executing_custom.stack_base, -2);
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, res);
        stack_base[stack_offset].stack_flags.type = MACRO_COMPLETE;
    }

    return(NEW_STATE);
}

/******************************************************************************
*
* collapse an array into a specific element
* when evaluating a database condition
*
******************************************************************************/

extern void
dbase_array_index(
    P_EV_DATA p_ev_data_out,
    P_EV_DATA p_ev_data_in,
    P_STACK_DBASE p_stack_dbase,
    _InVal_     EV_TYPE ev_type)
{
    EV_DATA ev_data;

    if(RPN_DAT_NAME == p_ev_data_in->did_num)
        name_deref(&ev_data, p_ev_data_in->arg.h_name);
    else
        status_assert(ss_data_resource_copy(&ev_data, p_ev_data_in));

    if(data_is_array_range(&ev_data))
        (void) array_range_index(p_ev_data_out, &ev_data, p_stack_dbase->ix, p_stack_dbase->iy, ev_type);
    else
        status_assert(ss_data_resource_copy(p_ev_data_out, p_ev_data_in));

    ss_data_free_resources(&ev_data);
}

/******************************************************************************
*
* dispose of a dbase block
*
******************************************************************************/

static void
dbase_function_block_dispose(
    P_STACK_DBASE p_stack_dbase)
{
    ss_data_free_resources(&p_stack_dbase->arg0);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_stack_dbase->p_stat_block));
}

/******************************************************************************
*
* dispose of a setvalue block
*
******************************************************************************/

static void
dispose_setvalue_block(
    P_STACK_SETVALUE p_stack_setvalue)
{
    ss_data_free_resources(&p_stack_setvalue->ev_data_arg_0);
    ss_data_free_resources(&p_stack_setvalue->ev_data_arg_1);
}

/******************************************************************************
*
* dispose of a lookup block
*
******************************************************************************/

static void
lookup_block_dispose(
    P_STACK_LOOKUP p_stack_lookup)
{
    ss_data_free_resources(&p_stack_lookup->arg1);
    ss_data_free_resources(&p_stack_lookup->arg2);
    ss_data_free_resources(&p_stack_lookup->p_lookup_block->target_data);
    ss_data_free_resources(&p_stack_lookup->p_lookup_block->result_data);
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_stack_lookup->p_lookup_block));
}

/******************************************************************************
*
* initialise lookup block
*
******************************************************************************/

extern void
lookup_block_init(
    _OutRef_    P_LOOKUP_BLOCK p_lookup_block,
    _InRef_opt_ PC_EV_DATA p_ev_data_target,
    _InVal_     S32 lookup_id,
    _InVal_     S32 match,
    _InVal_     BOOL all_occs)
{
    ev_data_set_blank(&p_lookup_block->target_data);

    if(p_ev_data_target)
        status_assert(ss_data_resource_copy(&p_lookup_block->target_data, p_ev_data_target));

    ev_data_set_blank(&p_lookup_block->result_data);

    p_lookup_block->lookup_id = lookup_id;

    if(match < 0)
        p_lookup_block->match = -1;
    else if(match > 0)
        p_lookup_block->match = 1;
    else
        p_lookup_block->match = 0;

    p_lookup_block->ix = 0;
    p_lookup_block->n_found = 0;
    p_lookup_block->ix_match = 0;
    p_lookup_block->all_occs = all_occs;
    p_lookup_block->lookup_horz = FALSE;
}

/******************************************************************************
*
* see bfind() in xstring.c for the inspiring (but not the same) implementation
*
* <0 stopped at break count
* =0 no success
* >0 got there (n_found)
*
******************************************************************************/

static S32
hvlookup_using_bfind(
    P_STACK_LOOKUP p_stack_lookup,
    _InVal_     S32 n_entries)
{
    P_LOOKUP_BLOCK p_lookup_block = p_stack_lookup->p_lookup_block;
    S32 lookup_res = 0; /* item not found */

    if(n_entries)
    {
        S32 e = n_entries;
        S32 s = 0;

        for(;;)
        {
            S32 compare_res;
            S32 n = (e - s) / 2;
            S32 t = s + n;

            p_lookup_block->ix = t;

            { /* get data item and compare to target */
            EV_DATA ev_data;
            (void) array_range_index(&ev_data,
                                     &p_stack_lookup->arg1,
                                     p_lookup_block->lookup_horz ? p_lookup_block->ix : 0,
                                     p_lookup_block->lookup_horz ? 0 : p_lookup_block->ix,
                                     EM_CONST);
            compare_res = ss_data_compare(&p_lookup_block->target_data, &ev_data, FALSE, FALSE);
            ss_data_free_resources(&ev_data);
            } /*block*/

            if(0 == compare_res)
            {
                p_lookup_block->n_found = 1;
                p_lookup_block->ix_match = p_lookup_block->ix;
                lookup_res = 1;
                return(lookup_res);
            }

            if(compare_res > 0)
                s = t;
            else
                e = t;

            if(0 == n)
            {
                switch(p_lookup_block->match)
                {
                default:
                    break;

                case -1:
                    if(compare_res > 0)
                    {
                        if(t + 1 < n_entries)
                        {
                            p_lookup_block->n_found = 1;
                            p_lookup_block->ix_match = t + 1;
                            lookup_res = 1;
                        }
                    }
                    else /* if(compare_res < 0) */
                    {
                        assert(compare_res < 0);
                        assert(t == 0);
                        p_lookup_block->n_found = 1;
                        p_lookup_block->ix_match = t;
                        lookup_res = 1;
                    }
                    break;

                case +1:
                    if(compare_res > 0)
                    {
                        p_lookup_block->n_found = 1;
                        p_lookup_block->ix_match = t;
                        lookup_res = 1;
                    }
                    break;
                }

                break;
            }

            /* else go round again ... (we should go round at most log2(n_entries) times so sod the timeout) */
        }
    }

    return(lookup_res);
}

static S32
lookup_process(
    P_STACK_LOOKUP p_stack_lookup,
    _InVal_     MONOTIME time_started)
{
    P_LOOKUP_BLOCK p_lookup_block = p_stack_lookup->p_lookup_block;
    S32 lookup_res = 0;
    S32 x_size, y_size;
    S32 limit;
    BOOL allow_wild_match = FALSE;

    array_range_sizes(&p_stack_lookup->arg1, &x_size, &y_size);

    switch(p_lookup_block->lookup_id)
    {
    default: default_unhandled();
#if CHECKING
    case LOOKUP_HLOOKUP:
    case LOOKUP_VLOOKUP:
#endif
        p_lookup_block->lookup_horz = (LOOKUP_HLOOKUP == p_lookup_block->lookup_id);
        return(hvlookup_using_bfind(p_stack_lookup, p_lookup_block->lookup_horz ? x_size : y_size));

    case LOOKUP_LOOKUP:
        p_lookup_block->lookup_horz = (y_size == 1);
        allow_wild_match = TRUE; /* SKS 24apr96 - I think I'd changed this around 1.23 time, but then h/vlookup would have been wrong before too */
        break;

    case LOOKUP_MATCH:
        p_lookup_block->lookup_horz = (y_size == 1);
        break;
    }

    limit = p_lookup_block->lookup_horz ? x_size : y_size;

    while(p_lookup_block->ix < limit)
    {
        S32 compare_res;

        { /* get next data item and compare to target */
        EV_DATA ev_data;
        (void) array_range_index(&ev_data,
                                 &p_stack_lookup->arg1,
                                 p_lookup_block->lookup_horz ? p_lookup_block->ix : 0,
                                 p_lookup_block->lookup_horz ? 0 : p_lookup_block->ix,
                                 EM_CONST);
        compare_res = ss_data_compare(&ev_data, &p_lookup_block->target_data, FALSE, allow_wild_match); /* SKS 24apr96 notes that it's the 2nd string that can be wild*/
        ss_data_free_resources(&ev_data);
        } /*block*/

        if(p_lookup_block->all_occs)
        {
            if(0 == compare_res)
            {
                STATUS status = STATUS_OK;

                if(!data_is_array_range(&p_lookup_block->result_data))
                    status = ss_array_make(&p_lookup_block->result_data, 0, 0);

                if(status_ok(status))
                {
                    S32 x_size_out, y_size_out;

                    array_range_sizes(&p_stack_lookup->arg2, &x_size_out, &y_size_out);
                    if(status_ok(status = ss_array_element_make(&p_lookup_block->result_data, x_size_out - 1, p_lookup_block->n_found)))
                    {
                        S32 ix;

                        for(ix = 0; ix < x_size_out; ++ix)
                        {
                            EV_DATA ev_data_temp;
                            (void) array_range_index(&ev_data_temp, &p_stack_lookup->arg2, ix, p_lookup_block->ix, EM_CONST);
                            status_assert(ss_data_resource_copy(
                                                ss_array_element_index_wr(&p_lookup_block->result_data,
                                                                          ix,
                                                                          p_lookup_block->n_found),
                                                &ev_data_temp));
                            ss_data_free_resources(&ev_data_temp);
                        }
                    }

                    p_lookup_block->n_found += 1;
                }

                if(status_fail(status))
                {
                    ss_data_free_resources(&p_lookup_block->result_data);
                    ev_data_set_error(&p_lookup_block->result_data, status);
                    lookup_res = 0;
                    break;
                }
            }
        }
        else
        {
            if((0 == compare_res)
               ||
               (p_lookup_block->match && (p_lookup_block->match != compare_res)))
            {
                p_lookup_block->n_found = 1;
                p_lookup_block->ix_match = p_lookup_block->ix;
            }

            if(compare_res == p_lookup_block->match)
            {
                lookup_res = 1;
                break;
            }
        }

        p_lookup_block->ix += 1;

        /* check the time */
        if(monotime_diff(time_started) > BACKGROUND_SLICE)
        {
            lookup_res = -1;
            break;
        }
    }

    return((lookup_res < 0) ? lookup_res : p_lookup_block->n_found);
}

/******************************************************************************
*
* finish off a lookup
*
******************************************************************************/

static void
lookup_finish(
    P_EV_DATA p_ev_data_res,
    P_STACK_LOOKUP p_stack_lookup)
{
    P_LOOKUP_BLOCK p_lookup_block = p_stack_lookup->p_lookup_block;

    if(p_lookup_block->all_occs)
    {
        /* transfer ownership to p_ev_data_res */
        *p_ev_data_res = p_lookup_block->result_data;
        ev_data_set_blank(&p_lookup_block->result_data);
    }
    else
    {
        switch(p_lookup_block->lookup_id)
        {
        default: default_unhandled();
#if CHECKING
        case LOOKUP_HLOOKUP:
        case LOOKUP_VLOOKUP:
#endif
            {
            EV_DATA ev_data;
            (void) array_range_index(&ev_data,
                                     &p_stack_lookup->arg1,
                                     p_lookup_block->lookup_horz ? p_lookup_block->ix_match : p_stack_lookup->arg2.arg.integer - 1,
                                     p_lookup_block->lookup_horz ? p_stack_lookup->arg2.arg.integer - 1 : p_lookup_block->ix_match,
                                     EM_ANY);
            status_assert(ss_data_resource_copy(p_ev_data_res, &ev_data));
            ss_data_free_resources(&ev_data);
            break;
            }

        case LOOKUP_LOOKUP:
            {
            EV_DATA ev_data;
            (void) array_range_index(&ev_data,
                                     &p_stack_lookup->arg2,
                                     p_lookup_block->lookup_horz ? p_lookup_block->ix_match : 0,
                                     p_lookup_block->lookup_horz ? 0 : p_lookup_block->ix_match,
                                     EM_ANY);
            status_assert(ss_data_resource_copy(p_ev_data_res, &ev_data));
            ss_data_free_resources(&ev_data);
            break;
            }

        case LOOKUP_MATCH:
            ev_data_set_integer(p_ev_data_res, p_lookup_block->ix_match + 1);
            break;
        }
    }
}

/******************************************************************************
*
* set the value of a cell
*
******************************************************************************/

_Check_return_
static STATUS
poke_cell(
    _InRef_     PC_EV_SLR p_ev_slr,
    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_SLR p_ev_slr_cur)
{
    STATUS status = STATUS_OK;

    if(!ev_slr_equal(p_ev_slr, p_ev_slr_cur))
    {
        P_EV_CELL p_ev_cell;
        S32 res;

        /* can't overwrite formula cells */
        if((res = ev_travel(&p_ev_cell, p_ev_slr)) > 0 && !p_ev_cell->parms.data_only)
            status = create_error(EVAL_ERR_UNEXFORMULA);
        else
        {
            if(res > 0)
                ev_cell_free_resources(p_ev_cell);

            if(status_ok(status = ev_make_cell(p_ev_slr, p_ev_data)))
                cell_add_to_ranges_affected(p_ev_slr);

            if(status_ok(status))
                ev_todo_add_slr(p_ev_slr);
        }
    }

    return(status);
}

_Check_return_
static STATUS
poke_output(
    P_EV_DATA p_ev_data_out,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    P_EV_DATA p_ev_data_in,
    _InRef_     PC_EV_SLR p_ev_slr_cur)
{
    STATUS status = STATUS_OK;

    switch(p_ev_data_out->did_num)
    {
    case RPN_DAT_SLR:
        status = poke_cell(&p_ev_data_out->arg.slr, p_ev_data_in, p_ev_slr_cur);
        break;

    case RPN_DAT_RANGE:
        {
        EV_SLR ev_slr = p_ev_data_out->arg.range.s;

        ev_slr.col += EV_COL_PACK(ix);
        ev_slr.row += (EV_ROW) iy;

        if( (ix >= 0) && (iy >= 0) &&
            (ev_slr_col(&ev_slr) < ev_slr_col(&p_ev_data_out->arg.range.e)) &&
            (ev_slr_row(&ev_slr) < ev_slr_row(&p_ev_data_out->arg.range.e)) )
            status = poke_cell(&ev_slr, p_ev_data_in, p_ev_slr_cur);
        else
            status = create_error(EVAL_ERR_SUBSCRIPT);
        break;
        }

    case RPN_DAT_ARRAY:
        {
        if((ix >= 0) && (iy >= 0) && (ix < p_ev_data_out->arg.ev_array.x_size) && (iy < p_ev_data_out->arg.ev_array.y_size))
        {
            P_EV_DATA p_ev_data = (P_EV_DATA) ss_array_element_index_borrow(p_ev_data_out, ix, iy);

            switch(p_ev_data->did_num)
            {
            case RPN_DAT_SLR:
                /* SKS 04sep95 allows set_value to poke arrays e.g. set_value({a1,b1,c1,d1},{1,2,3,4}) */
                /* the important case is obviously when using index as l-value e.g. set_value(index(a11d16,1,1,4,row),a2d2) */
                status = poke_cell(&p_ev_data->arg.slr, p_ev_data_in, p_ev_slr_cur);
                break;

            default:
                /* the case where we get here normally is when doing set_value(a1,value,ix,iy) - see SETVALUE */
                ss_data_free_resources(p_ev_data);
                status_assert(ss_data_resource_copy(p_ev_data, p_ev_data_in));
                break;
            }
        }
        else
            status = create_error(EVAL_ERR_SUBSCRIPT);
        break;
        }

    default: default_unhandled(); break;
    }

    return(status);
}

/******************************************************************************
*
* custom function control statement processing
*
******************************************************************************/

_Check_return_
extern S32
process_control(
    _InVal_     S32 action,
    P_EV_DATA args[],
    _InVal_     S32 n_args,
    _InVal_     S32 eval_stack_base)
{
    /* save cell on which control statement encountered */
    EV_SLR current_slot = stack_base[stack_offset].slr;

    switch(action)
    {
    case CONTROL_GOTO:
        custom_jmp(&args[0]->arg.slr, 0, eval_stack_base - 1);
        break;

    case CONTROL_RESULT:
        custom_result(args[0], 0);
        break;

    case CONTROL_WHILE:
        if(args[0]->arg.integer)
        {
            /* while condition is true - start while:
             * clear stack and switch to while loop
             */
            stack_set(eval_stack_base);

            /* switch to control loop state */
            stack_base[stack_offset].stack_flags.type = CONTROL_LOOP;
            stack_base[stack_offset].data.stack_control_loop.control_type = CONTROL_WHILE;
            stack_base[stack_offset].data.stack_control_loop.origin_slot = current_slot;
        }
        else
        {
            /* while condition is false - continue
             * execution at next endwhile
             */
            process_control_search(EVS_CNT_WHILE,
                                   EVS_CNT_ENDWHILE,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   1,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADLOOPNEST);
        }
        break;

    /* found endwhile statement; pop last while
     * from stack and continue there
     */
    case CONTROL_ENDWHILE:
        {
        P_STACK_ENTRY p_stack_entry;

        if(NULL == (p_stack_entry = stack_back_search_loop(CONTROL_WHILE)))
            custom_result(NULL, EVAL_ERR_BADLOOPNEST);
        else
            custom_jmp(&p_stack_entry->data.stack_control_loop.origin_slot, 0, stack_offset(p_stack_entry - 1));

        break;
    }

    /* if elseif encountered as result of if(FALSE)
     * fall thru to if to check argument, otherwise
     * skip to endif
     */
    case CONTROL_ELSEIF:
        {
        if(!stack_back_search(stack_offset, EXECUTING_MACRO)->data.stack_executing_custom.elseif)
        {
            process_control_search(EVS_CNT_IFC,
                                   EVS_CNT_ENDIF,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   1,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADIFNEST);
            break;
        }
        }

        /*FALLTHRU*/

    case CONTROL_IF:
        {
        P_STACK_ENTRY p_stack_entry = stack_back_search(stack_offset, EXECUTING_MACRO);

        p_stack_entry->data.stack_executing_custom.elseif = 0;

        /* if true, skip to next statement */
        if(args[0]->arg.integer)
            custom_jmp(&current_slot, 1, eval_stack_base - 1);
        else
        {
            /* after if(FALSE), look for:
             * endif:  continue normally
             * else:   continue normally
             * elseif: try again
             */
            if(process_control_search(EVS_CNT_IFC,
                                      EVS_CNT_ENDIF,
                                      EVS_CNT_ELSE,
                                      EVS_CNT_ELSEIF,
                                      &current_slot,
                                      1,
                                      eval_stack_base - 1,
                                      EVAL_ERR_BADIFNEST) == EVS_CNT_ELSEIF)
                p_stack_entry->data.stack_executing_custom.elseif = 1;
        }

        break;
        }

    /* when else encountered
     * when executing, skip upto the endif - we
     * must have already had an if(TRUE) or elseif(TRUE)
     */
    case CONTROL_ELSE:
        process_control_search(EVS_CNT_IFC,
                               EVS_CNT_ENDIF,
                               EVS_CNT_NONE,
                               EVS_CNT_NONE,
                               &current_slot,
                               1,
                               eval_stack_base - 1,
                               EVAL_ERR_BADIFNEST);
        break;

    /* ENDIF encountered when executing - ignore by
     * moving onto next statement
     */
    case CONTROL_ENDIF:
        custom_jmp(&current_slot, 1, eval_stack_base - 1);
        break;

    case CONTROL_REPEAT:
        /* start repeat by resetting stack, then
         * pushing current position etc
         */
        stack_set(eval_stack_base);

        /* switch to control loop state */
        stack_base[stack_offset].stack_flags.type = CONTROL_LOOP;
        stack_base[stack_offset].data.stack_control_loop.control_type = CONTROL_REPEAT;
        stack_base[stack_offset].data.stack_control_loop.origin_slot = current_slot;
        break;

    case CONTROL_UNTIL:
        {
        P_STACK_ENTRY p_stack_entry;

        if(NULL == (p_stack_entry = stack_back_search_loop(CONTROL_REPEAT)))
            custom_result(NULL, EVAL_ERR_BADLOOPNEST);
        else if(!args[0]->arg.integer)
            custom_jmp(&p_stack_entry->data.stack_control_loop.origin_slot, 1, eval_stack_base - 1);
        else
            custom_jmp(&current_slot, 1, stack_offset(p_stack_entry - 1));
        break;
        }

    case CONTROL_FOR:
        {
        S32 res;
        STACK_CONTROL_LOOP stack_control_loop;

        stack_control_loop.control_type = CONTROL_FOR;
        stack_control_loop.origin_slot = current_slot;
        stack_control_loop.end = args[2]->arg.fp;

        if(n_args > 3)
            stack_control_loop.step = args[3]->arg.fp;
        else
            stack_control_loop.step = 1.0;

        if((res = name_make(&stack_control_loop.h_name, ev_slr_docno(&stack_base[stack_offset].slr), &args[0]->arg.string, args[1], NULL)) < 0)
            custom_result(NULL, res);
        else if(!process_control_for_cond(&stack_control_loop, 0))
            process_control_search(EVS_CNT_FOR,
                                   EVS_CNT_NEXT,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   1,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADLOOPNEST);
        else
        {
            /* clear stack and switch to for loop */
            stack_set(eval_stack_base - 1);

            /* removing FOR args made room for this */
            stack_offset += 1;

            stack_base[stack_offset].slr = stack_base[stack_offset-1].slr;
            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
            stack_base[stack_offset].stack_flags.type = CONTROL_LOOP;

            stack_base[stack_offset].data.stack_control_loop = stack_control_loop;
        }

        break;
        }

    case CONTROL_NEXT:
        {
        P_STACK_ENTRY p_stack_entry;

        if(NULL == (p_stack_entry = stack_back_search_loop(CONTROL_FOR)))
            custom_result(NULL, EVAL_ERR_BADLOOPNEST);
        else if(process_control_for_cond(&p_stack_entry->data.stack_control_loop, 1))
            custom_jmp(&p_stack_entry->data.stack_control_loop.origin_slot, 1, eval_stack_base - 1);
        else
            custom_jmp(&current_slot, 1, stack_offset(p_stack_entry - 1));

        break;
        }

    case CONTROL_BREAK:
        {
        S32 loop_count;
        P_STACK_ENTRY p_stack_entry;

        if(n_args)
            loop_count = (S32) args[0]->arg.integer;
        else
            loop_count = 1;

        loop_count = MAX(1, loop_count);

        p_stack_entry = &stack_base[stack_offset];
        while(loop_count--)
            if(NULL == (p_stack_entry = stack_back_search(PtrDiffElemU32(p_stack_entry, stack_base), CONTROL_LOOP)))
                break;

        if(NULL == p_stack_entry)
            custom_result(NULL, EVAL_ERR_BADLOOPNEST);
        else
        {
            S32 block_start, block_end;
            EV_SLR slr;

            switch(p_stack_entry->data.stack_control_loop.control_type)
            {
            case CONTROL_REPEAT:
                block_start = EVS_CNT_REPEAT;
                block_end   = EVS_CNT_UNTIL;
                break;

            case CONTROL_FOR:
                block_start = EVS_CNT_FOR;
                block_end   = EVS_CNT_NEXT;
                break;

            case CONTROL_WHILE:
            default:
                block_start = EVS_CNT_WHILE;
                block_end   = EVS_CNT_ENDWHILE;
                break;
            }

            slr = p_stack_entry->data.stack_control_loop.origin_slot;
            slr.row += 1;
            process_control_search(block_start,
                                   block_end,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &slr,
                                   1,
                                   stack_offset(p_stack_entry - 1),
                                   EVAL_ERR_BADLOOPNEST);
        }

        break;
        }

    case CONTROL_CONTINUE:
        {
        P_STACK_ENTRY p_stack_entry = stack_back_search(stack_offset, CONTROL_LOOP);

        if(NULL == p_stack_entry)
            custom_result(NULL, EVAL_ERR_BADLOOPNEST);
        else
        {
            S32 block_start, block_end;

            switch(p_stack_entry->data.stack_control_loop.control_type)
            {
            case CONTROL_REPEAT:
                block_start = EVS_CNT_REPEAT;
                block_end   = EVS_CNT_UNTIL;
                break;

            case CONTROL_FOR:
                block_start = EVS_CNT_FOR;
                block_end   = EVS_CNT_NEXT;
                break;

            case CONTROL_WHILE:
            default:
                block_start = EVS_CNT_WHILE;
                block_end   = EVS_CNT_ENDWHILE;
                break;
            }

            process_control_search(block_start,
                                   block_end,
                                   EVS_CNT_NONE,
                                   EVS_CNT_NONE,
                                   &current_slot,
                                   0,
                                   eval_stack_base - 1,
                                   EVAL_ERR_BADLOOPNEST);
        }
        break;
        }
    }

    return(NEW_STATE);
}

/******************************************************************************
*
* process a for condition
*
******************************************************************************/

static S32
process_control_for_cond(
    P_STACK_CONTROL_LOOP p_stack_control_loop,
    _InVal_     S32 step)
{
    const ARRAY_INDEX name_num = name_def_find(p_stack_control_loop->h_name);
    S32 res = 0;

    if(name_num >= 0)
    {
        const P_EV_NAME p_ev_name = array_ptr(&name_def.h_table, EV_NAME, name_num);

        if(RPN_DAT_REAL == p_ev_name->def_data.did_num)
        {
            if(step)
                p_ev_name->def_data.arg.fp += p_stack_control_loop->step;

            if(p_stack_control_loop->step >= 0)
                res = !(p_ev_name->def_data.arg.fp > p_stack_control_loop->end);
            else
                res = !(p_ev_name->def_data.arg.fp < p_stack_control_loop->end);
        }
    }

    return(res);
}

/******************************************************************************
*
* search forwards for control statements - the
* search stops on a block_end type, but nested
* blocks are counted as the search proceeds
* block_end_maybe1/2 are optional extra stop points
*
******************************************************************************/

static S32
process_control_search(
    _InVal_     S32 block_start,
    _InVal_     S32 block_end,
    _InVal_     S32 block_end_maybe1,
    _InVal_     S32 block_end_maybe2,
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     EV_ROW offset,
    _InVal_     S32 stack_after,
    _InVal_     S32 error)
{
    S32 found_type, nest, found, res;
    EV_ROW last_row;
    EV_SLR slr;

    slr      = *p_ev_slr;
    nest     = found = found_type = 0;
    last_row = ev_numrow(ev_slr_docno(&slr));

    while(!found && ++slr.row < last_row)
    {
        P_EV_CELL p_ev_cell;

        if(ev_travel(&p_ev_cell, &slr) > 0)
        {
            if(p_ev_cell->parms.control == (unsigned) block_start)
                ++nest;
            else if(p_ev_cell->parms.control == (unsigned) block_end)
            {
                if(nest)
                    --nest;
                else
                {
                    found_type = p_ev_cell->parms.control;
                    found = 1;
                }
            }
            else if(!nest)
            {
                if(p_ev_cell->parms.control == (unsigned) block_end_maybe1 ||
                   p_ev_cell->parms.control == (unsigned) block_end_maybe2)
                {
                    found_type = p_ev_cell->parms.control;
                    found = 1;
                }
            }
        }
    }

    if(!found)
    {
        custom_result(NULL, error);
        res = error;
    }
    else
    {
        EV_ROW jmp_offset;

        /* elseifs must be evaluated */
        if(found_type == EVS_CNT_ELSEIF)
            jmp_offset = 0;
        else
            jmp_offset = offset;

        custom_jmp(&slr, jmp_offset, stack_after);
        res = found_type;
    }

    return(res);
}

/******************************************************************************
*
* function called to recalc document
*
* call as often as possible; it will return as often as is reasonable
*
******************************************************************************/

#ifndef __cplusplus

#include <signal.h>

typedef void (__cdecl * P_PROC_SIGNAL) (
    _In_        int sig);

#include <setjmp.h>

static jmp_buf ev_recalc_jmp_buf;

#define JMPVAL_FPERROR 0x1994 /* SKS 09jan94 EVAL_ERR_FPERROR is too wide to pass out */

static void __cdecl
ev_recalc_signal_handler(
    _In_        int sig)
{
    /* reset signal trapping */
    signal(sig, (P_PROC_SIGNAL) ev_recalc_signal_handler);

#if WINDOWS
    if(sig == SIGFPE)
        _fpreset();
#endif

    longjmp(ev_recalc_jmp_buf, JMPVAL_FPERROR);
}

#endif /* __cplusplus */

/******************************************************************************
*
* backtrack up the stack to find the
* most recent cell we were calculating,
* set the error into the cell and go
* to calculate its dependents
*
******************************************************************************/

static S32 eval_rpn_stack_at = -1;

static S32
ev_recalc_pass_next_slot(void)
{
    STATUS status = todo_next_slr(&stack_base[stack_offset].slr);

    if(status_ok(status))
    {
        if(status)
        {
#if TRACE_ALLOWED
            eval_trace(TEXT("*******************<ev_recalc>*********************"));
#endif

            stack_base[stack_offset].stack_flags = stack_flags_zero;
            stack_base[stack_offset].stack_flags.type = CALC_SLOT;
            return(0);
        }
    }
    else
    {
        ev_recalc_status(status);
        todo_exit();
    }

    /* nothing to do - go home */
    stack_zap();
    return(1);
}

static void
ev_recalc_pass_calc_slot(void)
{
    /* called to start recalc of a cell */
    eval_rpn_stack_at = stack_offset;

    /* flip to end state */
    stack_base[stack_offset].stack_flags.type = END_CALC;
    stack_base[stack_offset].data.stack_in_calc.did_calc = 0;

    if(stack_base[stack_offset].slr.circ)
    {
        cell_set_error(&stack_base[stack_offset].slr, /*create_error*/(EVAL_ERR_CIRC));
        stack_base[stack_offset].stack_flags.calcederror = 1;
    }
    else
    {
        stack_base[stack_offset].data.stack_in_calc.travel_res = ev_travel(&stack_base[stack_offset].data.stack_in_calc.eval_block.p_ev_cell, &stack_base[stack_offset].slr);

        if(stack_base[stack_offset].data.stack_in_calc.travel_res > 0
           &&
           !stack_base[stack_offset].data.stack_in_calc.eval_block.p_ev_cell->parms.data_only)
        {
            stack_base[stack_offset].data.stack_in_calc.did_calc = 1;
            stack_base[stack_offset].data.stack_in_calc.eval_block.offset = 0;
            stack_base[stack_offset].data.stack_in_calc.eval_block.slr = stack_base[stack_offset].slr;
            stack_base[stack_offset].data.stack_in_calc.eval_block.in_dbase = FALSE;

            eval_rpn(stack_offset);
        }
    }
}

static void
ev_recalc_pass_in_eval(void)
{
    /* called during recalc of a cell after eval_rpn
     * has released control for some reason
     */
    P_STACK_IN_EVAL p_stack_in_eval = &stack_base[stack_offset].data.stack_in_eval;

    eval_trace(TEXT("<IN_EVAL>"));

    /* reload cell pointer */
    (void) ev_travel(&stack_base[p_stack_in_eval->stack_offset].data.stack_in_calc.eval_block.p_ev_cell, &stack_base[stack_offset].slr);

    /* remove IN_EVAL state and continue with recalc */
    stack_offset -= 1;

    eval_rpn(p_stack_in_eval->stack_offset);
}

static void
ev_recalc_pass_end_calc(void)
{
    /* called when recalculation of a cell is over */
    BOOL had_custom_result = 0;
    BOOL custom_sheet = ev_doc_check_custom(ev_slr_docno(&stack_base[stack_offset].slr));
    S32 need_redraw = 0;

    /* did we actually get a result ? */
    if(stack_base[stack_offset].data.stack_in_calc.did_calc)
    {
        /* check if the result has changed and needs redrawing */
        EV_DATA ev_data_old;
        ev_data_from_ev_cell(&ev_data_old, stack_base[stack_offset].data.stack_in_calc.eval_block.p_ev_cell);
        need_redraw = ss_data_compare(&ev_data_old, &stack_base[stack_offset].data.stack_in_calc.result_data, FALSE, FALSE);

        /* store result in cell */
        ev_cell_free_resources(stack_base[stack_offset].data.stack_in_calc.eval_block.p_ev_cell);
        ev_cell_constant_from_data(stack_base[stack_offset].data.stack_in_calc.eval_block.p_ev_cell,
                                   &stack_base[stack_offset].data.stack_in_calc.result_data);

        /* on error in custom function document, return custom function result error */
        if(custom_sheet && ev_data_is_error(&stack_base[stack_offset].data.stack_in_calc.result_data))
        {
            custom_result(NULL, stack_base[stack_offset].data.stack_in_calc.result_data.arg.ev_error.status);
            had_custom_result = 1;
        }
    }

    ss_data_free_resources(&stack_base[stack_offset].data.stack_in_calc.result_data); /* SKS 28sep95 moved here from below where it was most definitely wrong */

    if(!had_custom_result)
    {
        if(!custom_sheet && (need_redraw || stack_base[stack_offset].stack_flags.calcederror))
            cell_add_to_ranges_affected(&stack_base[stack_offset].slr);

        /* remove cell's entry from the todo list */
        if(!custom_sheet)
            (void) todo_remove_slr();

        /* continue with what we were doing previously */
        if(!stack_offset)
            stack_base[stack_offset].stack_flags.type = NEXT_SLOT;
        else
            stack_offset -= 1;
    }

    /*ss_data_free_resources(&stack_base[stack_offset].data.stack_in_calc.result); ^^^*/
    eval_rpn_stack_at = -1;
}

static void
ev_recalc_pass_control_loop(void)
{
    /* called to start a loop; the loop data
     * is pushed onto the stack; execution continues
     * at the cell after the origin cell
     */
    P_STACK_ENTRY p_stack_entry = stack_back_search(stack_offset, EXECUTING_MACRO);

    if(NULL == p_stack_entry)
        stack_base[stack_offset].stack_flags.type = INTERNAL_ERROR;
    else
        custom_sequence(p_stack_entry);
}

static void
ev_recalc_pass_dbase_function(void)
{
    /* called to start a database condition
     * eval_rpn checks that stack is available
     * needs 1 stack entry
     */
    P_STACK_DBASE p_stack_dbase = &stack_base[stack_offset].data.stack_dbase;
    P_EV_CELL p_ev_cell;

    /* I think all this lot can move into RPN to the DBASE condition there */
    /* set up conditional string ready for munging */
    if(ev_travel(&p_ev_cell, &p_stack_dbase->dbase_slot) > 0)
    {
        stack_offset += 1;

        stack_base[stack_offset].slr = p_stack_dbase->dbase_slot;
        stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
        stack_base[stack_offset].stack_flags.type = DBASE_CALC;

        stack_base[stack_offset].data.stack_in_calc.eval_block.p_ev_cell = p_ev_cell;
        stack_base[stack_offset].data.stack_in_calc.eval_block.offset = p_stack_dbase->cond_pos;
        stack_base[stack_offset].data.stack_in_calc.eval_block.slr = p_stack_dbase->dbase_slot;
        stack_base[stack_offset].data.stack_in_calc.eval_block.in_dbase = TRUE;
        stack_base[stack_offset].data.stack_in_calc.eval_block.dbase_stack = stack_offset - 1;

        /* go to evaluate condition */
        eval_rpn(stack_offset);
    }
}

static void
ev_recalc_pass_dbase_calc(void)
{
    /* receives result of database condition calculation
     * after each cell in database function
     */
    P_STACK_ENTRY p_stack_entry_dbase = &stack_base[stack_offset - 1];
    BOOL dbase_finished = 0;
    P_STACK_DBASE p_stack_dbase = &p_stack_entry_dbase->data.stack_dbase;

    /* work out state of condition */
    if( (RPN_DAT_REAL == arg_normalise(&stack_base[stack_offset].data.stack_in_calc.result_data, EM_REA, NULL, NULL, NULL))
        &&
        (stack_base[stack_offset].data.stack_in_calc.result_data.arg.fp != 0.0) )
    {
        EV_DATA ev_data;
        trace_2(TRACE_MODULE_EVAL, TEXT("DBASE processing entry: ") S32_TFMT TEXT(", ") S32_TFMT, p_stack_dbase->ix, p_stack_dbase->iy);
        dbase_array_index(&ev_data, &p_stack_dbase->arg0, p_stack_dbase, EM_CONST);
        /* go process the data item */
        array_range_proc_item(p_stack_dbase->p_stat_block, &ev_data);
        ss_data_free_resources(&ev_data);
    }
    else
        trace_2(TRACE_MODULE_EVAL, TEXT("DBASE failed entry: ") S32_TFMT TEXT(", ") S32_TFMT, p_stack_dbase->ix, p_stack_dbase->iy);

    ss_data_free_resources(&stack_base[stack_offset].data.stack_in_calc.result_data);

    {
    S32 x_size, y_size;

    array_range_sizes(&p_stack_dbase->arg0, &x_size, &y_size);
    p_stack_dbase->ix += 1;
    if(p_stack_dbase->ix >= x_size)
    {
        p_stack_dbase->ix = 0;
        p_stack_dbase->iy += 1;

        if(p_stack_dbase->iy >= y_size)
        {
            EV_DATA ev_data_result;

            /* remove DBASE_CALC state from stack */
            stack_offset -= 1;

            consume_bool(array_range_proc_finish(&ev_data_result, p_stack_dbase->p_stat_block, NULL));
            assert(0 == p_stack_dbase->p_stat_block->pass); /* don't support multiple-pass functions with database */
            dbase_function_block_dispose(p_stack_dbase);

            /* pop previous state (overwrites DBASE_FUNCTION state)
             * push dbase result
             * this leaves copies of flags & slr in [-1]
            */
            memcpy32(&stack_base[stack_offset], &stack_base[stack_offset-1], sizeof32(STACK_ENTRY));
            stack_base[stack_offset-1].stack_flags.type = DATA_ITEM;
            stack_base[stack_offset-1].data.stack_data_item.data = ev_data_result;
            dbase_finished = 1;
        }
    }
    } /*block*/

    /* evaluate condition again */
    if(!dbase_finished)
        eval_rpn(stack_offset);
}

static void
ev_recalc_pass_lookup_happening(
    _InVal_     MONOTIME time_started)
{
    S32 res = lookup_process(&stack_base[stack_offset].data.stack_lookup, time_started);

    if(res >= 0)
    {
        EV_DATA ev_data;

        if(!res)
            ev_data_set_error(&ev_data, EVAL_ERR_LOOKUP);
        else
            lookup_finish(&ev_data, &stack_base[stack_offset].data.stack_lookup);

        lookup_block_dispose(&stack_base[stack_offset].data.stack_lookup);

        /* pop previous state
         * push lookup result
         * this leaves copies of flags & slr in [-1]
        */
        memcpy32(&stack_base[stack_offset], &stack_base[stack_offset-1], sizeof32(STACK_ENTRY));
        stack_base[stack_offset-1].stack_flags.type = DATA_ITEM;
        stack_base[stack_offset-1].data.stack_data_item.data = ev_data;
    }
}

static void
ev_recalc_pass_macro_complete(void)
{
    /* called as result of RESULT function
     * or error during macro evaluation
     */
    S32 custom_over, n_args;
    STACK_ENTRY stack_entry_res, stack_entry_state;

    custom_over = 0;
    n_args = stack_base[stack_offset].data.stack_executing_custom.n_args;

    if(stack_base[stack_offset].data.stack_executing_custom.in_array)
    {
        ++stack_base[stack_offset].data.stack_executing_custom.x_pos;
        if(stack_base[stack_offset].data.stack_executing_custom.x_pos >= stack_base[stack_offset-1].data.stack_data_item.data.arg.ev_array.x_size)
        {
            stack_base[stack_offset].data.stack_executing_custom.x_pos  = 0;
            stack_base[stack_offset].data.stack_executing_custom.y_pos += 1;
            if(stack_base[stack_offset].data.stack_executing_custom.y_pos >= stack_base[stack_offset-1].data.stack_data_item.data.arg.ev_array.y_size)
                custom_over = 1;
        }
    }
    else
        custom_over = 1;

    /* macro may need re-calling for next array element */
    if(!custom_over)
    {
        stack_base[stack_offset].data.stack_executing_custom.next_slot =
        stack_base[stack_offset].data.stack_executing_custom.custom_slot;
        stack_base[stack_offset].stack_flags.type = EXECUTING_MACRO;
        return;
    }

    /* pop macro result */
    stack_offset -= 1;
    memcpy32(&stack_entry_res, &stack_base[stack_offset], sizeof32(STACK_ENTRY));

    /* pop previous state */
    stack_offset -= 1;
    memcpy32(&stack_entry_state, &stack_base[stack_offset], sizeof32(STACK_ENTRY));

    /* pop macro arguments from stack */
    stack_set(stack_offset - 1 - n_args);

    /* push macro result */
    stack_offset += 1;
    memcpy32(&stack_base[stack_offset], &stack_entry_res, sizeof32(STACK_ENTRY));

    /* set state */
    stack_offset += 1;
    memcpy32(&stack_base[stack_offset], &stack_entry_state, sizeof32(STACK_ENTRY));
}

static void
ev_recalc_pass_processing_array(void)
{
    /* the semantic routine is called
     * for each element in the argument arrays
     */
    EV_SPLIT_EXEC_DATA ev_split_exec_data;
    EV_DATA arg_data[EV_MAX_ARGS];
    P_EV_DATA args_in[EV_MAX_ARGS];
    P_P_EV_DATA p_p_ev_data;
    S32 ix, typec, max_x, max_y;
    STACK_PROCESSING_ARRAY stack_processing_array;
    PC_EV_TYPE p_ev_type;
    P_EV_DATA p_ev_data_res;

    if(stack_base[stack_offset].data.stack_processing_array.x_pos >= stack_base[stack_offset-1].data.stack_data_item.data.arg.ev_array.x_size)
    {
        stack_base[stack_offset].data.stack_processing_array.x_pos  = 0;
        stack_base[stack_offset].data.stack_processing_array.y_pos += 1;

        /* have we completed array ? */
        if(stack_base[stack_offset].data.stack_processing_array.y_pos >= stack_base[stack_offset-1].data.stack_data_item.data.arg.ev_array.y_size)
        {
            S32 n_args = stack_base[stack_offset].data.stack_processing_array.n_args; /* SKS 01may95 */
            STACK_ENTRY stack_entry_res, stack_entry_state;

            stack_offset -= 1;
            memcpy32(&stack_entry_res, &stack_base[stack_offset], sizeof32(STACK_ENTRY));

            stack_offset -= 1;
            memcpy32(&stack_entry_state, &stack_base[stack_offset], sizeof32(STACK_ENTRY));

            /* remove array arguments from stack */
            stack_set(stack_offset - 1 - n_args);

            /* push result of array fuddling on stack;
             * assume we can push since we just popped
            */
            stack_offset += 1;
            memcpy32(&stack_base[stack_offset], &stack_entry_res, sizeof32(STACK_ENTRY));

            stack_offset += 1;
            memcpy32(&stack_base[stack_offset], &stack_entry_state, sizeof32(STACK_ENTRY));
            return;
        }
    }

    /* make a copy of processing_array data */
    stack_processing_array = stack_base[stack_offset].data.stack_processing_array;

    stack_base[stack_offset].data.stack_processing_array.x_pos += 1;

    /* get the arguments and array pointers */
    for(ix = 0, p_p_ev_data = args_in, typec = stack_processing_array.type_count,
        p_ev_type = stack_processing_array.arg_types;
        ix < stack_processing_array.n_args;
        ++ix, ++p_p_ev_data)
    {
        *p_p_ev_data = stack_index_ptr_data(stack_processing_array.stack_base,
                                            stack_processing_array.n_args - ix - 1);

        /* replace stack array pointer with
         * pointer to relevant array element
         */
        if(p_ev_type && (*p_ev_type & EM_ARY))
            ev_split_exec_data.args[ix] = *p_p_ev_data;
        else
        {
            if(RPN_DAT_RANGE == (*p_p_ev_data)->did_num)
            {
                EV_SLR slr = (*p_p_ev_data)->arg.range.s;

                slr.col += EV_COL_PACK(stack_processing_array.x_pos);
                slr.row += (EV_ROW)    stack_processing_array.y_pos;

                ev_slr_deref(&arg_data[ix], &slr);
                ev_split_exec_data.args[ix] = &arg_data[ix];
            }
            else
            {
                /* don't give a pointer into the array itself,
                 * otherwise someone will poo on it
                 */
                ss_array_element_read(&arg_data[ix], *p_p_ev_data, stack_processing_array.x_pos, stack_processing_array.y_pos);
                ev_split_exec_data.args[ix] = &arg_data[ix];
            }
        }

        if(typec > 1)
        {
            ++p_ev_type;
            --typec;
        }
    }

    /* get pointer to result element */
    p_ev_data_res = ss_array_element_index_wr(&stack_base[stack_offset-1].data.stack_data_item.data,
                                              stack_processing_array.x_pos,
                                              stack_processing_array.y_pos);

    /* call semantic routine with array
     * elements as arguments instead of arrays
     */
    {
    STATUS status;

    if((status = args_check(stack_processing_array.n_args, ev_split_exec_data.args,
                            stack_processing_array.type_count,
                            stack_processing_array.arg_types,
                            p_ev_data_res,
                            &max_x,
                            &max_y,
                            NULL /* we can't be processing an array inside a dbase - it'll have been removed already */)) == 0)
    {
        if(OBJECT_ID_NONE != stack_processing_array.object_id)
        {
            ev_split_exec_data.object_table_index = stack_processing_array.object_table_index;
            ev_split_exec_data.n_args = stack_processing_array.n_args;
            ev_split_exec_data.p_ev_data_res = p_ev_data_res; /* optimise? */
            ev_split_exec_data.p_cur_slr = &stack_base[stack_offset].slr;
            if(STATUS_MODULE_NOT_FOUND == object_call_id_load(P_DOCU_NONE, T5_MSG_SS_RPN_EXEC, &ev_split_exec_data, stack_processing_array.object_id))
                status = STATUS_NOT_AVAILABLE;
        }
        else
            status = create_error(EVAL_ERR_UNEXARRAY);
    }
    else if(status > 0)
        status = create_error(EVAL_ERR_NESTEDARRAY);

    /* if function returned an array, correct and complain */
    if(data_is_array_range(p_ev_data_res))
        status = create_error(EVAL_ERR_NESTEDARRAY);

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }

    /* free temporary resources owned by arguments */
    for(ix = 0; ix < stack_processing_array.n_args; ++ix)
        ss_data_free_resources(&arg_data[ix]);
    } /*block*/
}

static void
ev_recalc_pass_alert_input(void)
{
    S32 res = -1;
    EV_DATA ev_data;

    ev_data_set_integer(&ev_data, 0);

    switch(stack_base[stack_offset].data.stack_alert_input.alert_input)
    {
    case RPN_FNV_ALERT:
        if((res = ev_alert_poll()) >= 0)
        {
            ev_data_set_integer(&ev_data, (S32) res);
            ev_alert_close();
        }
        break;

    case RPN_FNV_INPUT:
        {
        UCHARZ result_buffer[BUF_EV_LONGNAMLEN];

        if((res = ev_input_poll(ustr_bptr(result_buffer), EV_LONGNAMLEN)) >= 0)
        {
            EV_DATA string_data;

            /* get rid of input box */
            ev_input_close();

            if(status_ok(ss_string_make_ustr(&string_data, ustr_bptr(result_buffer))))
            {
                P_STACK_ALERT_INPUT p_stack_alert_input = &stack_base[stack_offset].data.stack_alert_input;
                S32 name_res;
                EV_HANDLE name_key;
                EV_STRINGC ev_stringc;

                ev_stringc.uchars = uchars_bptr(p_stack_alert_input->ustr_name_id); /* loan */
                ev_stringc.size   = p_stack_alert_input->name_id_len;

                if((name_res = name_make(&name_key, ev_slr_docno(&stack_base[stack_offset].slr), &ev_stringc, &string_data, NULL)) < 0)
                    ev_data_set_error(&ev_data, name_res);
                else
                    ev_data_set_integer(&ev_data, (S32) res);

                ss_data_free_resources(&string_data);
            }
        }
        break;
        }
    }

    if(res >= 0)
    {
        /* pop previous state
         * push alert/input result
         * this leaves copies of flags & slr in [-1]
        */
        memcpy32(&stack_base[stack_offset], &stack_base[stack_offset-1], sizeof32(STACK_ENTRY));
        stack_base[stack_offset-1].stack_flags.type = DATA_ITEM;
        stack_base[stack_offset-1].data.stack_data_item.data = ev_data;
    }
}

static void
ev_recalc_pass_setvalue(void)
{
    EV_DATA ev_data_result;
    P_STACK_SETVALUE p_stack_setvalue = &stack_base[stack_offset].data.stack_setvalue;
    BOOL sub_array_mode = p_stack_setvalue->n_args > 2;
    STATUS status = STATUS_OK;

    /* initialise output so we can free OK */
    ev_data_set_blank(&ev_data_result);

    /* special array indirection mode with first parm == SLR and extra indices supplied */
    if(sub_array_mode && (RPN_DAT_SLR == p_stack_setvalue->ev_data_arg_0.did_num))
    {
        P_EV_CELL p_ev_cell;
        S32 res;
        if((res = ev_travel(&p_ev_cell, &p_stack_setvalue->ev_data_arg_0.arg.slr)) > 0 && !p_ev_cell->parms.data_only)
            status = create_error(EVAL_ERR_UNEXFORMULA);
        else
        {
            S32 x_size, y_size;
            EV_DATA temp_data;

            ev_data_set_blank(&temp_data);

            if(res > 0)
            {
                /* claim the data from the cell for ourselves */
                ev_data_from_ev_cell(&temp_data, p_ev_cell);
                temp_data.local_data = 1;
                if(RPN_DAT_ARRAY != temp_data.did_num)
                    ss_data_free_resources(&temp_data);
                p_ev_cell->parms.did_num = RPN_DAT_BLANK;
            }

            array_range_sizes(&p_stack_setvalue->ev_data_arg_1, &x_size, &y_size);
            x_size = (0 == x_size) ? 0 : x_size - 1;
            y_size = (0 == y_size) ? 0 : y_size - 1;
            status_consume(array_expand(&temp_data, p_stack_setvalue->ix_x + x_size, p_stack_setvalue->ix_y + y_size));

            if(status_ok(status = ev_make_cell(&p_stack_setvalue->ev_data_arg_0.arg.slr, &temp_data)))
                cell_add_to_ranges_affected(&p_stack_setvalue->ev_data_arg_0.arg.slr);

            ev_todo_add_slr(&p_stack_setvalue->ev_data_arg_0.arg.slr);

            /* make setvalue target the array itself */
            p_stack_setvalue->ev_data_arg_0 = temp_data;
            p_stack_setvalue->ev_data_arg_0.local_data = 0;
        }
    }

    if(status_ok(status))
    {
        switch(p_stack_setvalue->ev_data_arg_0.did_num)
        {
        case RPN_DAT_RANGE:
        case RPN_DAT_ARRAY:
            {
            BOOL first = 1;
            S32 x_size, y_size, ix_x, ix_y, x_size_in, y_size_in, ix_s = 0, iy_s = 0;

            array_range_sizes(&p_stack_setvalue->ev_data_arg_0, &x_size, &y_size);
            array_range_sizes(&p_stack_setvalue->ev_data_arg_1, &x_size_in, &y_size_in);

            if(sub_array_mode)
            {
                ix_s = p_stack_setvalue->ix_x - 1;
                iy_s = p_stack_setvalue->ix_y - 1;
                x_size_in = MAX(1, x_size_in);
                y_size_in = MAX(1, y_size_in);
                x_size = MIN(x_size, ix_s + x_size_in);
                y_size = MIN(y_size, iy_s + y_size_in);
            }

            for(ix_y = iy_s; ix_y < y_size; ++ix_y)
            {
                for(ix_x = ix_s; ix_x < x_size; ++ix_x)
                {
                    EV_DATA poke_data;
                    STATUS status_t;

                    switch(p_stack_setvalue->ev_data_arg_1.did_num)
                    {
                    case RPN_DAT_RANGE:
                    case RPN_DAT_ARRAY:
                        {
                        S32 ix_x_in = ix_x - ix_s;
                        S32 ix_y_in = ix_y - iy_s;

                        if((ix_x_in < x_size_in) && (ix_y_in < y_size_in))
                            (void) array_range_index(&poke_data, &p_stack_setvalue->ev_data_arg_1, ix_x_in, ix_y_in, EM_ANY);
                        else
                            ev_data_set_blank(&poke_data);
                        break;
                        }

                    default:
                        poke_data = p_stack_setvalue->ev_data_arg_1;
                        poke_data.local_data = 0;
                        break;
                    }

                    status_t = poke_output(&p_stack_setvalue->ev_data_arg_0,
                                           ix_x,
                                           ix_y,
                                           &poke_data,
                                           &stack_base[stack_offset].slr);
                    if(status_ok(status))
                        status = status_t;

                    if(first)
                    {
                        status_assert(ss_data_resource_copy(&ev_data_result, &poke_data));
                        first = 0;
                    }

                    ss_data_free_resources(&poke_data);
                }
            }

            break;
            }

        default:
            if(status_ok(status = poke_output(&p_stack_setvalue->ev_data_arg_0,
                                              p_stack_setvalue->ix_x - 1,
                                              p_stack_setvalue->ix_y - 1,
                                              &p_stack_setvalue->ev_data_arg_1,
                                              &stack_base[stack_offset].slr)))
                status_assert(ss_data_resource_copy(&ev_data_result, &p_stack_setvalue->ev_data_arg_1));
            break;
        }
    }

    if(status_fail(status))
    {
        /* free anything we might have saved */
        ss_data_free_resources(&ev_data_result);
        ev_data_set_error(&ev_data_result, status);
    }

    dispose_setvalue_block(p_stack_setvalue);

    /* pop previous state
     * push setvalue result
     * this leaves copies of flags & slr in [-1]
     */
    memcpy32(&stack_base[stack_offset], &stack_base[stack_offset-1], sizeof32(STACK_ENTRY));
    stack_base[stack_offset-1].stack_flags.type = DATA_ITEM;
    stack_base[stack_offset-1].data.stack_data_item.data = ev_data_result;
}

static void
ev_recalc_pass_array_range_arg(void)
{
    /* called for each argument */
    P_ARRAY_RANGE_BLOCK p_array_range_block = stack_base[stack_offset].data.stack_array_range.p_array_range_block;
    BOOL switch_on = FALSE;
    STATUS status = STATUS_OK;

    while(p_array_range_block->arg_ix < p_array_range_block->n_args)
    {
        P_EV_DATA p_ev_data_arg = &p_array_range_block->args[p_array_range_block->arg_ix];

        switch(p_array_range_block->type = p_ev_data_arg->did_num)
        {
        case RPN_DAT_ARRAY:
            if(status_ok(status = array_scan_init(&p_array_range_block->array_scan_block, p_ev_data_arg)))
                switch_on = TRUE;
            break;

        case RPN_DAT_RANGE:
            if(status_ok(status = range_scan_init(&p_array_range_block->range_scan_block, &p_ev_data_arg->arg.range)))
                switch_on = TRUE;
            break;

        case RPN_DAT_FIELD:
            if(status_ok(status = field_scan_init(&p_array_range_block->array_scan_block, p_ev_data_arg)))
                switch_on = TRUE;
            break;

        default:
            array_range_proc_item(&p_array_range_block->stat_block, p_ev_data_arg);
            break;
        }

        p_array_range_block->arg_ix += 1;

        if(switch_on || status_fail(status))
            break;
    }

    /* switch to ARRAY_RANGE state */
    if(switch_on)
        stack_base[stack_offset].stack_flags.type = ARRAY_RANGE;
    /* finished - successfully or not */
    else if(p_array_range_block->arg_ix >= p_array_range_block->n_args
            ||
            status_fail(status))
    {
        EV_DATA ev_data;
        BOOL pop = TRUE;

        if(status_fail(status))
            ev_data_set_error(&ev_data, status);
        else
            pop = array_range_proc_finish(&ev_data, &p_array_range_block->stat_block, &p_array_range_block->arg_ix);

        if(pop)
        {
            /* pop previous state */
            array_range_block_dispose(&stack_base[stack_offset].data.stack_array_range);

            memcpy32(&stack_base[stack_offset], &stack_base[stack_offset-1], sizeof32(STACK_ENTRY));
            stack_base[stack_offset-1].stack_flags.type = DATA_ITEM;
            stack_base[stack_offset-1].data.stack_data_item.data = ev_data;
        }
    }
    /* not possible... */
    else
        assert0();
}

static void
ev_recalc_pass_array_range(
    _InVal_     MONOTIME time_started)
{
    P_ARRAY_RANGE_BLOCK p_array_range_block = stack_base[stack_offset].data.stack_array_range.p_array_range_block;
    BOOL at_end = FALSE;

    for(;;)
        {
        EV_DATA element_data;

        switch(p_array_range_block->type)
        {
        case RPN_DAT_RANGE:
            if(range_scan_element(&p_array_range_block->range_scan_block, &element_data, EM_CONST) == RPN_FRM_END)
                at_end = TRUE;
            break;

        case RPN_DAT_FIELD:
            if(field_scan_element(&p_array_range_block->array_scan_block, &element_data, EM_CONST) == RPN_FRM_END)
                at_end = TRUE;
            break;

        case RPN_DAT_ARRAY:
            if(array_scan_element(&p_array_range_block->array_scan_block, &element_data, EM_CONST) == RPN_FRM_END)
                at_end = TRUE;
            break;

        default: default_unhandled();
            at_end = TRUE;
            break;
        }

        if(at_end)
            break;

        array_range_proc_item(&p_array_range_block->stat_block, &element_data);
        ss_data_free_resources(&element_data);

#if TRACE_ALLOWED
        if(monotime_diff(time_started) > (trace_is_on()
                                          ? BACKGROUND_SLICE * 10
                                          : BACKGROUND_SLICE))
            break;
#else
        if(monotime_diff(time_started) > BACKGROUND_SLICE)
            break;
#endif
    }

    if(at_end)
        stack_base[stack_offset].stack_flags.type = ARRAY_RANGE_ARG;
}

_Check_return_
static BOOL
ev_recalc_pass_start(void)
{
    if(global_flags.blown || (NULL == stack_base))
    {
        /* give up right away with no stack */
        if(stack_check_n(1) < 0)
            return(FALSE);
        PTR_ASSERT(stack_base);

        /* if evaluator has been disturbed,
         * clear the stack and restart
         */
        stack_zap();

        /* tell the world there's something to do */
        ev_recalc_status(array_elements(&h_todo_list));

        /* get trees ready for action */
        tree_sort_all();

        trace_0(TRACE_MODULE_EVAL, TEXT("**************<recalc restarting after blow-up>*************"));

        global_flags.blown = 0;
        stack_base[stack_offset].stack_flags = stack_flags_zero;
        stack_base[stack_offset].stack_flags.type = NEXT_SLOT;
    }
    else
    {
        eval_trace(TEXT("-------- recalc continuing -------"));

        /* reload cell pointer when recalc continues */
        switch(stack_base[stack_offset].stack_flags.type)
        {
        case END_CALC:
            (void) ev_travel(&stack_base[stack_offset].data.stack_in_calc.eval_block.p_ev_cell, &stack_base[stack_offset].slr);
            break;

        default:
            break;
        }
    }

    /* lock trees */
    global_flags.lock = 1;

    return(TRUE);
}

static void
ev_recalc_pass_end(void)
{
    ARRAY_INDEX n_todo;

    if((n_todo = array_elements(&h_todo_list)) == 0
       &&
       array_elements(&h_needs_recalc) == 0)
    {
        ev_recalc_stop();

        {
        EV_DOCNO ev_docno_cur = ev_current_docno();

        if(ev_docno_cur != DOCNO_NONE)
            status_assert(maeve_event(p_docu_from_ev_docno(ev_docno_cur), T5_MSG_CUR_CHANGE_BEFORE, P_DATA_NONE));

        { /* send out affected ranges */
        const ARRAY_INDEX n_ranges_affected = array_elements(&h_ranges_affected);
        ARRAY_INDEX i;

        for(i = DOCNO_FIRST; i < n_ranges_affected; ++i)
        {
            const P_EV_RANGE p_ev_range = array_ptr(&h_ranges_affected, EV_RANGE, i);

            ev_uref_change_range(p_ev_range);
            ev_redraw_slot_range(p_ev_range);

#if TRACE_ALLOWED
            if(DOCNO_NONE != ev_slr_docno(&p_ev_range->s))
                if_constant(tracing(TRACE_MODULE_EVAL))
                {
                    TCHARZ tstr_buf[32 + BUF_EV_LONGNAMLEN];
                    EV_SLR ev_slr_e;
                    ev_trace_slr_tstr_buf(tstr_buf, elemof32(tstr_buf), TEXT("affected range: $$:"), &p_ev_range->s);
                    ev_slr_e = p_ev_range->e;
                    ev_slr_e.col -= 1;
                    ev_slr_e.row -= 1;
                    ev_trace_slr_tstr_buf(tstr_buf + tstrlen32(tstr_buf), elemof32(tstr_buf) - tstrlen32(tstr_buf), TEXT("$$"), &ev_slr_e);
                    trace_v0(TRACE_MODULE_EVAL, tstr_buf);
                }
#endif
        }

        al_array_dispose(&h_ranges_affected);
        } /*block*/

        if(ev_docno_cur != DOCNO_NONE)
            status_assert(maeve_event(p_docu_from_docno(ev_docno_cur), T5_MSG_CUR_CHANGE_AFTER, P_DATA_NONE));

        } /*block*/

    } /*fi*/

    ev_recalc_status(n_todo);

    global_flags.lock = 0;
}

extern void
ev_recalc(void)
{
    MONOTIME time_started;
#ifndef __cplusplus
    P_PROC_SIGNAL oldfpe = NULL;
    int jmpval;
#endif

    if(!ev_recalc_pass_start())
        return;

    time_started = monotime();

#ifndef __cplusplus

    oldfpe = signal(SIGFPE, (P_PROC_SIGNAL) ev_recalc_signal_handler);

    /* catch FP errors etc. */
    if((jmpval = setjmp(ev_recalc_jmp_buf)) != 0)
    {
        reportf(TEXT("*** ev_recalc setjmp returned from signal handler ***"));
#ifndef __cplusplus
        signal(SIGFPE, oldfpe); /* restore old handler */
#endif /* __cplusplus */
        assert(eval_rpn_stack_at >= 0);
        stack_set(eval_rpn_stack_at);
        ev_data_set_error(&stack_base[eval_rpn_stack_at].data.stack_in_calc.result_data,
            (jmpval == JMPVAL_FPERROR) ? EVAL_ERR_FPERROR : jmpval);
        /*ev_recalc_pass_end();*/
        global_flags.lock = 0; /* only the essentials */
        return;
    }

#endif /* __cplusplus */

    for(;;)
    {
        /* continue according to state on stack */
        switch(stack_base[stack_offset].stack_flags.type)
        {
        case NEXT_SLOT:
            if(ev_recalc_pass_next_slot()) goto break_loop;
            break;

        case CALC_SLOT:
            ev_recalc_pass_calc_slot();
            break;

        case IN_EVAL:
            ev_recalc_pass_in_eval();
            break;

        case END_CALC:
            ev_recalc_pass_end_calc();
            break;

        case CONTROL_LOOP:
            ev_recalc_pass_control_loop();
            break;

        case DBASE_FUNCTION:
            ev_recalc_pass_dbase_function();
            break;

        case DBASE_CALC:
            ev_recalc_pass_dbase_calc();
            break;

        case LOOKUP_HAPPENING:
            ev_recalc_pass_lookup_happening(time_started);
            break;

        case MACRO_COMPLETE:
            ev_recalc_pass_macro_complete();
            break;

        case EXECUTING_MACRO:
            /* whilst executing macro, select
             * next sequential cell for evaluation
             */
            custom_sequence(&stack_base[stack_offset]);
            break;

        case PROCESSING_ARRAY:
            ev_recalc_pass_processing_array();
            break;

        case ALERT_INPUT:
            ev_recalc_pass_alert_input();
            break;

        case SETVALUE:
            ev_recalc_pass_setvalue();
            break;

        case ARRAY_RANGE_ARG:
            ev_recalc_pass_array_range_arg();
            break;

        case ARRAY_RANGE:
            ev_recalc_pass_array_range(time_started);
            break;

        /* all other states are treated as an error condition */
        case INTERNAL_ERROR:
        default:
            stack_zap();
            cell_set_error(&stack_base[stack_offset].slr, /*create_error*/(EVAL_ERR_INTERNAL));
            goto break_loop;
        }

        if(ev_doc_foreground())
            continue;

#if TRACE_ALLOWED
        if(monotime_diff(time_started) > (trace_is_on()
                                          ? BACKGROUND_SLICE * 10
                                          : BACKGROUND_SLICE))
            break;
#else
        if(monotime_diff(time_started) > BACKGROUND_SLICE)
            break;
#endif
    }

break_loop:;

    /* exit code */
    ev_recalc_pass_end();

#ifndef __cplusplus
    signal(SIGFPE, oldfpe);
#endif /* __cplusplus */
}

/******************************************************************************
*
* add cell to affected area list
*
******************************************************************************/

static ARRAY_INDEX
extend_ranges_affected(
    _InRef_     PC_EV_SLR p_ev_slr)
{
    ARRAY_INDEX n_ranges = array_elements(&h_ranges_affected);
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(2, sizeof32(EV_RANGE), TRUE);

    consume_ptr(al_array_extend_by(&h_ranges_affected, EV_RANGE, ((ARRAY_INDEX) ev_slr_docno(p_ev_slr) + 1) - n_ranges, &array_init_block, &status));
    status_assert(status);

    {
    const ARRAY_INDEX n_ranges_affected = array_elements(&h_ranges_affected);
    ARRAY_INDEX i;
    P_EV_RANGE p_ev_range_t;
    for(i = n_ranges, p_ev_range_t = array_ptr(&h_ranges_affected, EV_RANGE, i);
        i < n_ranges_affected;
        ++i, ++p_ev_range_t)
    {
        p_ev_range_t->s.docno = EV_DOCNO_PACK(DOCNO_NONE);
    }
    } /*block*/

    return(array_elements(&h_ranges_affected));
}

static void
cell_add_to_ranges_affected(
    _InRef_     PC_EV_SLR p_ev_slr)
{
    ARRAY_INDEX n_ranges = array_elements(&h_ranges_affected);

    if(n_ranges < ((ARRAY_INDEX) ev_slr_docno(p_ev_slr) + 1))
        n_ranges = extend_ranges_affected(p_ev_slr);

    if(n_ranges > (ARRAY_INDEX) ev_slr_docno(p_ev_slr))
    {
        const P_EV_RANGE p_ev_range = array_ptr(&h_ranges_affected, EV_RANGE, ev_slr_docno(p_ev_slr));

        if(DOCNO_NONE == ev_slr_docno(&p_ev_range->s))
        {
            p_ev_range->s = p_ev_range->e = *p_ev_slr;
            p_ev_range->e.col += 1;
            p_ev_range->e.row += 1;
        }
        else
        {
            p_ev_range->s.col = MIN(p_ev_range->s.col, p_ev_slr->col); /* equivalent SBF */
            p_ev_range->s.row = MIN(p_ev_range->s.row, p_ev_slr->row);
            p_ev_range->e.col = MAX(p_ev_range->e.col, p_ev_slr->col + 1); /* equivalent SBF */
            p_ev_range->e.row = MAX(p_ev_range->e.row, p_ev_slr->row + 1);
        }
    }
}

/******************************************************************************
*
* store an error in a cell and switch state
* to calculating the cell's dependents
*
******************************************************************************/

/*ncr*/
static STATUS
cell_set_error(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     STATUS error)
{
    P_EV_CELL p_ev_cell;

    if(ev_travel(&p_ev_cell, p_ev_slr) > 0)
    {
        ev_cell_free_resources(p_ev_cell);

        p_ev_cell->parms.did_num = RPN_DAT_ERROR;

        zero_struct(p_ev_cell->ev_constant.ev_error);
        p_ev_cell->ev_constant.ev_error.status = error;
        p_ev_cell->ev_constant.ev_error.type = ERROR_NORMAL;
    }

    return(error);
}

/******************************************************************************
*
* search back up stack for an
* entry of the given type
*
******************************************************************************/

_Check_return_
extern P_STACK_ENTRY
stack_back_search(
    _In_        S32 stack_level,
    _InVal_     S32 entry_type)
{
    /* search back down stack for the entry */
    stack_level -= 1;

    while(stack_level > 0)
    {
        P_STACK_ENTRY p_stack_entry = &stack_base[stack_level];

        if(p_stack_entry->stack_flags.type == (unsigned) entry_type)
            return(p_stack_entry);

        stack_level -= 1;
    }

    return(NULL);
}

/******************************************************************************
*
* search back up stack for a loop control entry of the given type
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_STACK_ENTRY
stack_back_search_loop(
    _InVal_     S32 loop_type)
{
    /* find the most recent loop */
    P_STACK_ENTRY p_stack_entry = stack_back_search(stack_offset, CONTROL_LOOP);

    if(NULL == p_stack_entry)
        return(NULL);

    if(p_stack_entry->data.stack_control_loop.control_type != loop_type)
        return(NULL);

    return(p_stack_entry);
}

/******************************************************************************
*
* free the stack itself
*
******************************************************************************/

extern void
stack_free(void)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(&stack_base));
    stack_size = 0;
    stack_offset = 0;
}

/******************************************************************************
*
* free any resources owned by a data item in a stack entry
*
******************************************************************************/

static void
stack_free_resources(
    P_STACK_ENTRY p_stack_entry)
{
    /* free resources allocated to data items on stack */
    switch(p_stack_entry->stack_flags.type)
    {
    case DATA_ITEM:
        ss_data_free_resources(&p_stack_entry->data.stack_data_item.data);
        break;

    /* free indirected lookup block */
    case LOOKUP_HAPPENING:
        lookup_block_dispose(&p_stack_entry->data.stack_lookup);
        break;

    /* free indirected stats block */
    case DBASE_FUNCTION:
        dbase_function_block_dispose(&p_stack_entry->data.stack_dbase);
        break;

    /* kill off an alert or input box */
    case ALERT_INPUT:
        switch(p_stack_entry->data.stack_alert_input.alert_input)
        {
        case RPN_FNV_ALERT:
            ev_alert_close();
            break;

        case RPN_FNV_INPUT:
            ev_input_close();
            break;

        default: default_unhandled(); break;
        }
        break;

    case SETVALUE:
        dispose_setvalue_block(&p_stack_entry->data.stack_setvalue);
        break;

    case ARRAY_RANGE_ARG:
    case ARRAY_RANGE:
        array_range_block_dispose(&p_stack_entry->data.stack_array_range);
        break;
    }
}

/******************************************************************************
*
* check and ensure that n spaces are available on the stack
*
******************************************************************************/

_Check_return_
extern STATUS
stack_grow(
    _InVal_     U32 n_spaces /* zero to minimise stack */)
{
    STATUS status = STATUS_OK;

    if((0 == n_spaces) || (stack_offset + n_spaces >= stack_size))
    {
        U32 old_stack_size = (NULL != stack_base) ? stack_size : 0;
        U32 new_stack_size;

        if(0 == n_spaces)
             new_stack_size = STACK_INC; /* reduce to minimum */
        else
             new_stack_size = STACK_INC * (div_round_ceil_u((stack_offset + 1U) + n_spaces, STACK_INC));

        if(new_stack_size != old_stack_size)
        {
            P_STACK_ENTRY old_stack_base = stack_base;
            P_STACK_ENTRY new_stack_base;

            if(NULL != (new_stack_base = al_ptr_realloc_elem(STACK_ENTRY, stack_base, new_stack_size, &status)))
            {
                stack_base = new_stack_base;
                stack_size = new_stack_size;

                /* make sure new stack contains nowt */
                if(NULL == old_stack_base)
                    stack_base->stack_flags.type = INTERNAL_ERROR;

#if TRACE_ALLOWED
                if(new_stack_base != old_stack_base)
                    trace_0(TRACE_OUT | TRACE_ANY, TEXT("!!!!!!!!!!!!!!!!!!!! stack moved !!!!!!!!!!!!!!!!!!"));
#endif

                trace_2(TRACE_OUT | TRACE_ANY,
                        TEXT("stack realloced, now: ") U32_TFMT TEXT(" elements, ") U32_TFMT TEXT(" bytes"),
                        new_stack_size,
                        new_stack_size * sizeof32(STACK_ENTRY));

                /* if we managed to shrink stack, do a tidy up */
#if RISCOS
                if(0 == n_spaces)
                    alloc_tidy_up();
#endif
            }
        }
    }

    return(status);
}

/******************************************************************************
*
* set stack to given level
*
******************************************************************************/

extern void
stack_set(
    _InVal_     U32 stack_level)
{
#if CHECKING
    if((S32) stack_level < 0)
        if(__myasserted(TEXT("stack_set"), __TFILE__, stack_level, TEXT("stack_level req ") S32_TFMT TEXT(" < 0"), stack_level))
            __crash_and_burn_here();

    if((S32) stack_offset < 0)
        if(__myasserted(TEXT("stack_set"), __TFILE__, stack_offset, TEXT("stack_offset req ") S32_TFMT TEXT(" < 0"), stack_offset))
            __crash_and_burn_here();
#endif

    while(stack_offset > stack_level)
    {
        /* blow TOS element */
        stack_free_resources(&stack_base[stack_offset]);

        stack_offset -= 1;
    }
}

/******************************************************************************
*
* blow away the stack completely
*
******************************************************************************/

extern void
stack_zap(void)
{
    if(stack_base)
    {
        for(;;)
        {
            /* blow TOS element */
            stack_free_resources(&stack_base[stack_offset]);

            if(0 == stack_offset)
                break;

            stack_offset -= 1;
        }

        stack_base[stack_offset].stack_flags.type = INTERNAL_ERROR;

        /* minimise stack */
        status_consume(stack_grow(0));
    }

    global_flags.blown = 1;
}

/* end of ev_eval.c */
