/* ev_matr.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Matrix function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/******************************************************************************
*
* Matrix functions
*
******************************************************************************/

/* M[j,k] = mat[n*j+k] - data stored by cols across row, then cols across next row ... */

#define _em(_array, _n_cols_in_row, _row_idx, _col_idx) ( \
    (_array) + (_row_idx) * (_n_cols_in_row))[_col_idx]

/* Conventionally, the entry in the (one-based)
 * i-th row and the j-th column of a matrix is referred to
 * as the i,j, (i,j), or (i,j)th entry of the matrix
 * Easiest for C to keep zero-based...
 */

#define _Aij(_array, _n_cols_in_row, _i_idx, _j_idx) \
    _em(_array, _n_cols_in_row, _i_idx, _j_idx)

/* So watch out when using array_range_index() to fill and
 * ss_array_element_index_wr() for result as those use x=col, y=row args
 */

/******************************************************************************
*
* evaluate the determinant of an m-square matrix
*
******************************************************************************/

_Check_return_
static STATUS
determinant(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    STATUS status = STATUS_OK;

    switch(m)
    {
    case 0:
        *dp = 1.0;
        break; /* not as silly a question as RJM first thought! (see Bloom, Linear Algebra and Geometry) */

    case 1:
        *dp = *ap;
        break;

    case 2:
#define a11 (_Aij(ap, 2, 0, 0))
#define a12 (_Aij(ap, 2, 0, 1))
#define a21 (_Aij(ap, 2, 1, 0))
#define a22 (_Aij(ap, 2, 1, 1))
        *dp = (a11 * a22) - (a12 * a21);
#undef a11
#undef a12
#undef a21
#undef a22
        break;

    case 3:
#define a11 (_Aij(ap, 3, 0, 0))
#define a12 (_Aij(ap, 3, 0, 1))
#define a13 (_Aij(ap, 3, 0, 2))
#define a21 (_Aij(ap, 3, 1, 0))
#define a22 (_Aij(ap, 3, 1, 1))
#define a23 (_Aij(ap, 3, 1, 2))
#define a31 (_Aij(ap, 3, 2, 0))
#define a32 (_Aij(ap, 3, 2, 1))
#define a33 (_Aij(ap, 3, 2, 2))
        *dp = + a11 * (a22 * a33 - a23 * a32)
              - a12 * (a21 * a33 - a23 * a31)
              + a13 * (a21 * a32 - a22 * a31);
#undef a11
#undef a12
#undef a13
#undef a21
#undef a22
#undef a23
#undef a31
#undef a32
#undef a33
        break;

    default:
        { /* recurse evaluating minors */
        P_F64 minor;
        U32 minor_m = m - 1;
        F64 minor_D;
        U32 col_idx, i_col_idx, o_col_idx;
        U32 i_row_idx, o_row_idx;

        *dp = 0.0;

        if(NULL == (minor = al_ptr_alloc_elem(F64, minor_m * minor_m, &status)))
            return(status); /* unable to determine */

        for(col_idx = 0; col_idx < m; ++col_idx)
        {
            /* build minor by removing all of top row and current column */
            for(i_col_idx = o_col_idx = 0; o_col_idx < minor_m; ++i_col_idx, ++o_col_idx)
            {
                if(i_col_idx == col_idx)
                    ++i_col_idx;

                for(i_row_idx = 1, o_row_idx = 0; o_row_idx < minor_m; ++i_row_idx, ++o_row_idx)
                {
                    _em(minor, minor_m, o_row_idx, o_col_idx) = _em(ap, m, i_row_idx, i_col_idx);
                }
            }

            status_break(status = determinant(minor, minor_m, &minor_D));

            if(col_idx & 1)
                minor_D = -minor_D; /* make into cofactor */

            *dp += (_em(ap, m, 0, col_idx) * minor_D);
        }

        al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));

        break;
        }
    }

    return(status);
}

/*
* NUMBER m_determ(square-array)
*/

PROC_EXEC_PROTO(c_m_determ)
{
    F64 m_determ_result;
    S32 x_size, y_size;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &x_size, &y_size);

    if(x_size != y_size)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_NOT_SQUARE);
        return;
    }

    if(x_size == 0)
    {
        ev_data_set_integer(p_ev_data_res, 1); /* yes, really */
        return;
    }

    {
    STATUS status = STATUS_OK;
    S32 m = x_size;
    F64 nums[3*3]; /* don't really need to allocate for small ones */
    P_F64 a /*[m][m]*/;

    if(m <= 3)
        a = nums;
    else
        a = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*a)), &status);

    if(NULL != a)
    {
        S32 i, j;

        assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

        /* load up the matrix (a) */
        for(i = 0; i < m; ++i)
        {
            for(j = 0; j < m; ++j)
            {
                EV_DATA ev_data;

                if(RPN_DAT_REAL != array_range_index(&ev_data, args[0], j, i, EM_REA)) /* NB j,i */
                    status_break(status = EVAL_ERR_MATRIX_NOT_NUMERIC);

                _Aij(a, m, i, j) = ev_data.arg.fp;
            }
        }

        if(status_ok(status = determinant(a, m, &m_determ_result)))
        {
            ev_data_set_real_ti(p_ev_data_res, m_determ_result);
        }

        if(a != nums)
            al_ptr_dispose(P_P_ANY_PEDANTIC(&a));
    }

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
    } /*block*/
}

/*
inverse of square matrix A is 1/det * adjunct(A)
*/

PROC_EXEC_PROTO(c_m_inverse)
{
    STATUS status = STATUS_OK;
    S32 x_size, y_size;
    U32 m, minor_m;
    P_F64 a /*[m][m]*/ = NULL;
    P_F64 adj /*[m][m]*/ = NULL;
    P_F64 minor /*[minor_m][minor_m]*/ = NULL;
    U32 i, j;
    F64 D, minor_D;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &x_size, &y_size);

    if(x_size != y_size)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_NOT_SQUARE);
        return;
    }

    m = x_size;

    if(status_fail(ss_array_make(p_ev_data_res, m, m)))
        return;

    if(NULL == (a = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*a)), &status)))
        goto endpoint;

    assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

    if(NULL == (adj = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*adj)), &status)))
        goto endpoint;

    /* load up the matrix (a) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            EV_DATA ev_data;

            if(RPN_DAT_REAL != array_range_index(&ev_data, args[0], j, i, EM_REA)) /* NB j,i */
            {
                status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endpoint;
            }

            _Aij(a, m, i, j) = ev_data.arg.fp;
        }
    }

    if(status_fail(status = determinant(a, m, &D)))
        goto endpoint;

    if(0.0 == D)
    {
        status = EVAL_ERR_MATRIX_SINGULAR;
        goto endpoint;
    }

    /* compute adjunct(A) */

    /* step 1 - compute matrix of minors */
    minor_m = m - 1;

    if(NULL == (minor = al_ptr_alloc_bytes(P_F64, minor_m * (minor_m * sizeof32(*minor)), &status)))
        goto endpoint;

    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            /* build minor(i,j) by removing all of current row and current column */
            U32 in_i, in_j, out_i, out_j;

            for(in_i = 0; in_i < m; ++in_i)
            {
                if(in_i == i) continue;

                out_i = in_i;

                if(out_i > i) --out_i;

                for(in_j = 0; in_j < m; ++in_j)
                {
                    if(in_j == j) continue;

                    out_j = in_j;

                    if(out_j > j) --out_j;

                    _Aij(minor, minor_m, out_i, out_j) = _Aij(a, m, in_i, in_j);
                }
            }

            if(status_fail(status = determinant(minor, minor_m, &minor_D)))
                goto endpoint;

            _Aij(adj, m, i, j) = minor_D;
       }
    }

    /* step 2 - convert to matrix of cofactors (multiply by +/- checkerboard) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            if(0 != ((i & 1) ^ (j & 1)))
                _Aij(adj, m, i, j) = - _Aij(adj, m, i, j);
        }
    }

    /* step 3 - transpose the matrix of cofactors => adjunct(A) (simple to do this in formation of result) */

    /* copy out the inverse(A) == adjunct(A) / determinant(A) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            P_EV_DATA p_ev_data = ss_array_element_index_wr(p_ev_data_res, j, i); /* NB j,i */
            ev_data_set_real(p_ev_data, _Aij(adj, m, j, i) / D); /* NB j,i here too for transpose step 3 above */
        }
    }

endpoint:

    al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&adj));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&a));

    if(status_fail(status))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
}

/******************************************************************************
*
*  Matrix Multiply
*
*  arg[0] is of dimensions a*b
*
*  arg[1] is of dimensions b*c
*
*  return matrix of dimensions a*c
*
******************************************************************************/

PROC_EXEC_PROTO(c_m_mult)
{
    S32 x_size[2];
    S32 y_size[2];

    exec_func_ignore_parms();

    /* check it is an array and get x and y sizes */
    array_range_sizes(args[0], &x_size[0], &y_size[0]);
    array_range_sizes(args[1], &x_size[1], &y_size[1]);

    if(x_size[0] != y_size[1])
    {   /* whinge about dimensions */
        ev_data_set_error(p_ev_data_res, EVAL_ERR_MISMATCHED_MATRICES);
        return;
    }

    if(status_ok(ss_array_make(p_ev_data_res, x_size[1], y_size[0])))
    {
        S32 ix, iy;

        for(ix = 0; ix < x_size[1]; ++ix)
        {
            for(iy = 0; iy < y_size[0]; ++iy)
            {
                F64 product = 0.0;
                S32 elem;
                P_EV_DATA elep;

                for(elem = 0; elem < x_size[0]; elem++)
                {
                    EV_DATA data[2];

                    (void) array_range_index(&data[0], args[0], elem, iy, EM_REA);
                    (void) array_range_index(&data[1], args[1], ix, elem, EM_REA);

                    if((RPN_DAT_REAL == data[0].did_num) && (RPN_DAT_REAL == data[1].did_num))
                    {
                        product += data[0].arg.fp * data[1].arg.fp;
                    }
                    else
                    {
                        ss_data_free_resources(p_ev_data_res);
                        ev_data_set_error(p_ev_data_res, EVAL_ERR_MATRIX_NOT_NUMERIC);
                        return;
                    }
                }

                elep = ss_array_element_index_wr(p_ev_data_res, ix, iy);
                ev_data_set_real_ti(elep, product);
            }
        }
    }
}

/******************************************************************************
*
*  Unit Matrix
*
*  return unit matrix of dimensions n*n
*
******************************************************************************/

PROC_EXEC_PROTO(c_m_unit)
{
    const S32 n = args[0]->arg.integer;

    exec_func_ignore_parms();

    if(status_ok(ss_array_make(p_ev_data_res, n, n)))
    {
        S32 ix, iy;

        for(ix = 0; ix < n; ++ix)
        {
            for(iy = 0; iy < n; ++iy)
            {
                const P_EV_DATA elep = ss_array_element_index_wr(p_ev_data_res, ix, iy);

                ev_data_set_integer(elep, (ix == iy) ? 1 : 0);
            }
        }
    }
}

PROC_EXEC_PROTO(c_transpose)
{
    S32 x_size, y_size;
    S32 ix, iy;

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &x_size, &y_size);

    /* make a y-dimension by x-dimension result array and swap elements */
    if(status_ok(ss_array_make(p_ev_data_res, y_size, x_size)))
    {
        for(ix = 0; ix < x_size; ++ix)
        {
            for(iy = 0; iy < y_size; ++iy)
            {
                EV_DATA temp_data;

                (void) array_range_index(&temp_data, args[0], ix, iy, EM_ANY);

                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_res, iy, ix), &temp_data));

                ss_data_free_resources(&temp_data);
            }
        }
    }
}

/* end of ev_matr.c */
