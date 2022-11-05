/* ev_fnstm.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Statistical function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "cmodules/mathxtr2.h" /* for linest() */

/******************************************************************************
*
* Statistical functions - Multivariate linear/logarithmic fit using least squares
*
******************************************************************************/

/******************************************************************************
*
* GROWTH(logest_result_data, known_x's) is pretty trivial
*
******************************************************************************/

PROC_EXEC_PROTO(c_growth)
{
    const PC_SS_DATA array_logest_data = args[0];
    const PC_SS_DATA array_known_x = args[1];
    S32 x_vars;
    S32 x, y;
    S32 err = 0;
    BOOL across_rows;

    exec_func_ignore_parms();

    array_range_sizes(array_logest_data, &x, &y);

    x_vars = x - 1;

    if( (y != 1 /*no stats*/) && (y != 3 /*stats*/) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);

    array_range_sizes(array_known_x, &x, &y);

    /* SKS after PD 4.12 28apr92 - allow TREND() and GROWTH() to receive data
     * in untransposed form i.e. more naturally matched to linest data
    */
    across_rows = (x == x_vars);

    if( !across_rows && (y != x_vars) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);

    if(status_ok(ss_array_make(p_ss_data_res, across_rows ? 1 : x, across_rows ? y : 1)))
    {
        S32 col, row;

        for(col = 0, row = 0; across_rows ? (row < y) : (col < x); across_rows ? ++row : ++col)
        {
            SS_DATA a_data;
            F64 product; /* NB. product computed carefully using logs */
            S32 ci;
            BOOL negative;
            P_SS_DATA elep;

            errno = 0;

            /* start with the constant */
            if(DATA_ID_REAL !=
                array_range_index(&a_data, array_logest_data,
                                  0, /* NB!*/
                                  0,
                                  EM_REA))
                status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

            /* if initial y data was all -ve then this is a possibility ... */
            negative = (ss_data_get_real(&a_data) < 0);

            product = fabs(ss_data_get_real(&a_data));

            if(product != 0.0)
            {
                product = log(product);
                assert(errno == 0); /* log(+ve) cannot fail ho ho */

                /* loop across a row/down a column multiplying product by coefficients ^ x variables */
                for(ci = 0; ci < x_vars; ++ci)
                {
                    SS_DATA x_data;

                    if(DATA_ID_REAL !=
                        array_range_index(&a_data, array_logest_data,
                                          ci + 1, /* NB. skip constant! */
                                          0,
                                          EM_REA))
                        status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                    if(DATA_ID_REAL !=
                        array_range_index(&x_data, array_known_x,
                                          across_rows ? ci : col, /* NB. extract from ci'th col (if across_rows) */
                                          across_rows ? row : ci, /* NB. extract from ci'th row (if down_columns) */
                                          EM_REA))
                        status_break(err = EVAL_ERR_MATRIX_NOT_NUMERIC);

                    product += log(ss_data_get_real(&a_data)) * ss_data_get_real(&x_data);
                }

                if(errno /* == EDOM, ERANGE */)
                    err = EVAL_ERR_BAD_LOG;

                status_break(err);

                product = exp(product); /* convert sum of products into a0 * PI(ai ** xi) */

                if(product == HUGE_VAL) /* don't test for underflow case */
                {
                    err = EVAL_ERR_OUTOFRANGE;
                    break;
                }
            }

            if(negative)
                product = -product;

            elep = ss_array_element_index_wr(p_ss_data_res, col, row);
            ss_data_set_real(elep, product);
        }
    } /*fi*/

    if(status_fail(err))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, err);
    }
}

/******************************************************************************
*
* linest(known_y's [, known_x's [, stats [, known_a's [, known_ye's ]]]])
*
* result is an array of {estimation parameters[;
*                        estimated errors for the above;
*                        chi-squared]}
*
******************************************************************************/

typedef struct FOR_FIRST_LINEST
{
    PC_F64 x; /* const */
    S32 x_vars;
    PC_F64 y; /* const */
    P_F64 a;
}
FOR_FIRST_LINEST, * P_FOR_FIRST_LINEST;

PROC_LINEST_DATA_GET_PROTO(static, ligp, client_handle, colID, row)
{
    P_FOR_FIRST_LINEST lidatap = (P_FOR_FIRST_LINEST) client_handle;

    switch(colID)
    {
    case LINEST_A_COLOFF:
        assert0();
        return(0.0);

    case LINEST_Y_COLOFF:
        return(lidatap->y[row]);

    default:
        {
        S32 coloff = colID - LINEST_X_COLOFF;
        return((lidatap->x + row * lidatap->x_vars) [coloff]);
        }
    }
}

PROC_LINEST_DATA_PUT_PROTO(static, lipp, client_handle, colID, row, value)
{
    P_FOR_FIRST_LINEST lidatap = (P_FOR_FIRST_LINEST) client_handle;

    switch(colID)
    {
    case LINEST_A_COLOFF:
        lidatap->a[row] = value;
        break;

    default: default_unhandled();
        break;
    }

    return(STATUS_OK);
}

typedef struct LINEST_ARRAY
{
    P_F64 val;
    S32 rows;
    S32 cols;
}
LINEST_ARRAY;

PROC_EXEC_PROTO(c_linest)
{
    static const LINEST_ARRAY empty = { NULL, 0, 0 };

    LINEST_ARRAY known_y;
    LINEST_ARRAY known_x;
    S32 stats;
    LINEST_ARRAY known_a;
    LINEST_ARRAY known_e;
    LINEST_ARRAY result_a;
    S32 data_in_cols;
    S32 y_items;
    S32 x_vars;
    STATUS status = STATUS_OK;
    FOR_FIRST_LINEST lidata;

    exec_func_ignore_parms();

    stats = 0;

    known_y  = empty;
    known_x  = empty;
    known_a  = empty;
    known_e  = empty;
    result_a = empty;

    switch(n_args)
    {
    default:
    case 5:
        /* check known_ye's is an array and get x and y sizes */
        array_range_sizes(args[4], &known_e.cols, &known_e.rows);

        /*FALLTHRU*/

    case 4:
        /* check known_a's is an array and get x and y sizes */
        array_range_sizes(args[3], &known_a.cols, &known_a.rows);

        /*FALLTHRU*/

    case 3:
         stats = (ss_data_get_real(args[2]) != 0.0);

        /*FALLTHRU*/

    case 2:
        /* check known_x's is an array and get x and y sizes */
        array_range_sizes(args[1], &known_x.cols, &known_x.rows);

        /*FALLTHRU*/

    case 1:
        /* check known_y's is an array and get x and y sizes */
        array_range_sizes(args[0], &known_y.cols, &known_y.rows);

        /* y data usually in a column, but allow for a row */
        data_in_cols = (known_y.rows > known_y.cols);
        y_items = data_in_cols ? known_y.rows : known_y.cols;
        break;
    }

    /* number of independent x variables (usually separate columns,
     * but can be separate rows if y data is so arranged)
    */
    if(n_args < 2)
        /* we always invent x if not supplied */
        x_vars = 1;
    else
        x_vars = (data_in_cols) ? known_x.cols : known_x.rows;

    /* simple to allocate y and x arrays together */
    if(NULL == (known_y.val = al_ptr_alloc_elem(F64, (U32) y_items * ((U32) x_vars + 1), &status)))
        goto endlabel;
    known_x.val = &known_y.val[y_items];

    result_a.cols = x_vars + 1;
    result_a.rows = stats ? 3 : 1;
    if(NULL == (result_a.val = al_ptr_alloc_elem(F64, result_a.cols * (U32) result_a.rows, &status)))
        goto endlabel;

    if(known_a.cols != 0)
    {
        if(known_a.cols > result_a.cols)
        {
            status = EVAL_ERR_MISMATCHED_MATRICES;
            goto endlabel;
        }

        if(NULL == (known_a.val = al_ptr_alloc_elem(F64, result_a.cols, &status)))
            goto endlabel;
    }

    if(known_e.rows != 0)
    {
        /* neatly covers both arrangements of y,e */
        if((known_e.rows > known_y.rows) /*|| (known_e.rows > known_y.rows)*/) /* wot???*/
        {
            status = EVAL_ERR_MISMATCHED_MATRICES;
            goto endlabel;
        }

        if(NULL == (known_e.val = al_ptr_alloc_elem(F64, y_items, &status)))
            goto endlabel;
    }

    if(status_ok(ss_array_make(p_ss_data_res, result_a.cols, result_a.rows)))
    {
        S32 i, j;
        SS_DATA ss_data;

        /* copy y data into working array */
        for(i = 0; i < y_items; ++i)
        {
            if(DATA_ID_REAL !=
                array_range_index(&ss_data, args[0],
                                  (data_in_cols) ? 0 : i,
                                  (data_in_cols) ? i : 0,
                                  EM_REA))
            {
                status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endlabel;
            }

            known_y.val[i] = ss_data_get_real(&ss_data);
        }

        /* copy x data into working array */
        for(i = 0; i < y_items; ++i)
        {
            if(n_args > 1)
            {
                for(j = 0; j < x_vars; ++j)
                {
                    if(DATA_ID_REAL !=
                        array_range_index(&ss_data, args[1],
                                          (data_in_cols) ? j : i,
                                          (data_in_cols) ? i : j,
                                          EM_REA))
                    {
                        status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                        goto endlabel;
                    }

                    (known_x.val + i * x_vars)[j] = ss_data_get_real(&ss_data);
                }
            }
            else
                known_x.val[i] = (F64) i + 1.0; /* make simple { 1.0, 2.0, 3.0 ... } */
        }

        /* <<< ignore the a data for the mo */
        if(known_a.val)
        { /*EMPTY*/ }

        /* copy (possibly partial) ye data into working array */
        if(known_e.val)
        {
            for(i = 0; i < y_items; ++i)
            {
                if(i < ((data_in_cols) ? known_e.rows : known_e.cols))
                {
                    if(DATA_ID_REAL !=
                        array_range_index(&ss_data, args[4],
                                          (data_in_cols) ? 0 : i,
                                          (data_in_cols) ? i : 0,
                                          EM_REA))
                    {
                        status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                        goto endlabel;
                    }

                    known_e.val[i] = ss_data_get_real(&ss_data);
                }
                else
                    known_e.val[i] = 1.0;
            }
        }

        /* and then ignore it! */

/* bodgey bit from here ... */

        lidata.x = known_x.val;
        lidata.x_vars = x_vars;

        lidata.y = known_y.val;

        lidata.a = result_a.val;

        if(status_fail(status = linest(ligp, lipp, (CLIENT_HANDLE) &lidata, x_vars, y_items)))
            goto endlabel;

/* ... to here */

        /* copy result a data from working array */

        for(j = 0; j < result_a.cols; ++j)
        {
            F64 res;
            P_SS_DATA elep;

            res = (result_a.val + 0 * result_a.cols)[j];

            elep = ss_array_element_index_wr(p_ss_data_res, j, 0); /* NB j,i */
            ss_data_set_real(elep, res);

            if(stats)
            {
                /* estimated estimation errors */
#if 1 /* until yielded */
                res = 0;
#else
                res = (result_a.val + 1 * result_a.cols)[j];
#endif

                elep = ss_array_element_index_wr(p_ss_data_res, j, 1); /* NB j,i */
                ss_data_set_real(elep, res);
            }
        }

        if(stats)
        {
            /* chi-squared */
            F64 chi_sq;
            P_SS_DATA elep;

            chi_sq = 0.0;

            elep = ss_array_element_index_wr(p_ss_data_res, 0, 2); /* NB j,i */
            ss_data_set_real(elep, chi_sq);
        }

    } /*fi*/

endlabel:;

    if(status_fail(status))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&known_e.val));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&known_a.val));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&result_a.val));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&known_y.val)); /* and x too */
}

/******************************************************************************
*
* oh god its logest. what a pathetic excuse of a routine
* when all you have to do is to tell the user to transform
* his data in an intelligent manner in the first place
* and use linest... I blame Excel personally
*
* it can't even fit y = k * m ^ -x !
*
******************************************************************************/

PROC_EXEC_PROTO(c_logest)
{
    SS_DATA y_data;
    SS_DATA a_data = { DATA_ID_BLANK };
    SS_DATA ss_data;
    P_SS_DATA elep;
    P_SS_DATA targs0, targs3;
    F64 val;
    S32 x, y;
    S32 col, row;
    S32 err = 0;
    S32 sign = 0; /* not yet set */

    exec_func_ignore_parms();

    /* perform quick preprocess of a data, taking logs */

    if(n_args > 3)
    {
        array_range_sizes(args[0], &x, &y);

        if(DATA_ID_RANGE == ss_data_get_data_id(args[3]))
        {
            if(status_fail(ss_array_make(&a_data, x, y)))
                return;
        }
        else
            status_assert(ss_data_resource_copy(&a_data, args[3]));

        for(col = 0; col < x; ++col)
        {
            if(DATA_ID_REAL != array_range_index(&ss_data, args[3], col, 0, EM_REA))
            {
                err = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endlabel2;
            }

            val = ss_data_get_real(&ss_data);
            /* do sign forcing before y transform */
            if(col == 0)
                sign = (val < 0) ? -1 : +1;
            ss_data_set_real(&ss_data, log(val));

            *ss_array_element_index_wr(&a_data, col, 0) = ss_data;
        }
    }

    /* perform quick preprocess of y data, taking logs */

    array_range_sizes(args[0], &x, &y);

    if(DATA_ID_RANGE == ss_data_get_data_id(args[0]))
    {
        if(status_fail(ss_array_make(&y_data, x, y)))
            return;
    }
    else
        status_assert(ss_data_resource_copy(&y_data, args[0]));

    /* simple layout-independent transform of y data */
    for(col = 0; col < x; ++col)
    {
        for(row = 0; row < y; ++row)
        {
            if(DATA_ID_REAL != array_range_index(&ss_data, args[0], col, row, EM_REA))
            {
                err = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endlabel;
            }

            if(ss_data_get_real(&ss_data) < 0.0)
            {
                if(!sign)
                    sign = -1;
                else if(sign != -1)
                {
                    err = EVAL_ERR_MIXED_SIGNS;
                    goto endlabel;
                }

                ss_data_set_real(&ss_data, log(-ss_data_get_real(&ss_data)));
            }
            else if(ss_data_get_real(&ss_data) > 0.0)
            {
                if(!sign)
                    sign = +1;
                else if(sign != +1)
                {
                    err = EVAL_ERR_MIXED_SIGNS;
                    goto endlabel;
                }

                ss_data_set_real(&ss_data, log(ss_data_get_real(&ss_data)));
            }
            /* else 0.0 ... interesting case ... not necessarily wrong but very hard */

            /* poke back transformed data */
            elep = ss_array_element_index_wr(&y_data, col, row);
            ss_data_set_real(elep, ss_data_get_real(&ss_data));
        }
    }

    /* ask MRJC whether this is legal */
    targs0  = args[0];
    args[0] = &y_data;

    if(n_args > 3)
    {
        targs3  = args[3];
        args[3] = &a_data;
    }
    else
        targs3  = NULL;

    /* call my friend to do the hard work */
    c_linest(args, n_args, p_ss_data_res, p_cur_slr);

    args[0] = targs0;

    if(targs3)
        args[3] = targs3;

    /* and transform first row of coefficients in situ using exp, and sign the constant if needed */

    if(ss_data_is_array(p_ss_data_res))
    {
        array_range_sizes(p_ss_data_res, &x, &y);

        for(col = 0; col < x; ++col)
        {
            elep = ss_array_element_index_wr(p_ss_data_res, col, 0);
            assert(ss_data_is_real(elep));
            val = exp(ss_data_get_real(elep));
            if((col == 0) && (sign == -1))
                val = -val;
            ss_data_set_real(elep, val);
        }
    }

endlabel:;

    ss_data_free_resources(&y_data);

endlabel2:;

    ss_data_free_resources(&a_data);

    exec_func_status_return(p_ss_data_res, err);
}

/******************************************************************************
*
* TREND(linest_result_data, known_x's) is pretty trivial
*
******************************************************************************/

PROC_EXEC_PROTO(c_trend)
{
    const PC_SS_DATA array_linest_data = args[0];
    const PC_SS_DATA array_known_x = args[1];
    S32 x_vars;
    S32 x, y;
    S32 err = 0;
    BOOL across_rows;

    exec_func_ignore_parms();

    array_range_sizes(array_linest_data, &x, &y);

    x_vars = x - 1;

    if( (y != 1 /*no stats*/) && (y != 3 /*stats*/) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);

    array_range_sizes(array_known_x, &x, &y);

    /* SKS after PD 4.12 28apr92 - allow TREND() and GROWTH() to receive data
     * in untransposed form i.e. more naturally matched to linest data
    */
    across_rows = (x == x_vars);

    if( !across_rows && (y != x_vars) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MATRIX_WRONG_SIZE);

    if(status_ok(ss_array_make(p_ss_data_res, across_rows ? 1 : x, across_rows ? y : 1)))
    {
        S32 col, row;

        for(col = 0, row = 0; across_rows ? (row < y) : (col < x); across_rows ? ++row : ++col)
        {
            SS_DATA a_data;
            F64 sum;
            S32 ci;
            P_SS_DATA elep;

            /* start with the constant */
            if(DATA_ID_REAL !=
                array_range_index(&a_data, array_linest_data,
                                  0, /* NB!*/
                                  0,
                                  EM_REA))
            {
                err = EVAL_ERR_MATRIX_NOT_NUMERIC;
                break;
            }

            sum = ss_data_get_real(&a_data);

            /* loop across a row/down a column summing coefficients * x variables */
            for(ci = 0; ci < x_vars; ++ci)
            {
                SS_DATA x_data;

                if(DATA_ID_REAL !=
                    array_range_index(&a_data, array_linest_data,
                                      ci + 1, /* NB. skip constant! */
                                      0,
                                      EM_REA))
                {
                    err = EVAL_ERR_MATRIX_NOT_NUMERIC;
                    break;
                }

                if(DATA_ID_REAL !=
                    array_range_index(&x_data, array_known_x,
                                      across_rows ? ci : col, /* NB. extract from ci'th col (if across_rows) */
                                      across_rows ? row : ci, /* NB. extract from ci'th row (if down_columns) */
                                      EM_REA))
                {
                    err = EVAL_ERR_MATRIX_NOT_NUMERIC;
                    break;
                }

                sum += ss_data_get_real(&a_data) * ss_data_get_real(&x_data);
            }

            status_break(err);

            elep = ss_array_element_index_wr(p_ss_data_res, col, row);
            ss_data_set_real(elep, sum);
        }
    } /*fi*/

    if(status_fail(err))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, err);
    }
}

/* end of ev_fnstm.c */
