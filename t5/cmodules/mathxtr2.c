/* mathxtr2.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* More additional math routines */

/* SKS September 1991 */

#include "common/gflags.h"

#ifndef          __mathxtr2_h
#include "cmodules/mathxtr2.h"
#endif

#ifndef          __mathxtra_h
#include "cmodules/mathxtra.h"
#endif

#ifndef                   __mathnums_h
#include "cmodules/coltsoft/mathnums.h"
#endif

#undef em /* NB data_in_columns here !!! */

/*
linest() : multivariate linear fit using least squares
*/

/*
* Important notes:
* ----------------
*
* each matrix has all rows in a column stored contiguously to enable
* swift transposition, i.e. col1,row0 follows on from col0,rowM
*
*/

/******************************************************************************
*
* evaluate the determinant of an m-square matrix
*
******************************************************************************/

_Check_return_
static STATUS
data_in_columns_determinant(
    _In_reads_x_(m*m) PC_F64 A /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    STATUS status = STATUS_OK;

    switch(m)
    {
    case 0:
    case 1:
        *dp = 0.0;
        break; /* silly question in this case */

    case 2:
        *dp = (A[0 + (0*2)] * A[1 + (1*2)]) - (A[1 + (0*2)] * A[0 + (1*2)]);
        break;

    default:
        { /* recurse evaluating minors */
        P_F64 minorp;
        const U32 minor_m = m - 1;
        const U32 minor_nbytes_per_col = minor_m * sizeof32(*minorp);
        U32 col_idx, i_col_idx, o_col_idx;
        F64 minor_D;

        *dp = 0.0;

        if(NULL == (minorp = al_ptr_alloc_bytes(P_F64, minor_m * minor_nbytes_per_col, &status)))
            return(status); /* unable to determine */

        for(col_idx = 0; col_idx < m; ++col_idx)
        {
            /* build minor by removing all of
             * top row and current column
            */
            for(i_col_idx = o_col_idx = 0; o_col_idx < minor_m; ++i_col_idx, ++o_col_idx)
            {
                if(i_col_idx == col_idx)
                    ++i_col_idx;

                /* copy all the row data (bar the top row) in this column */
                memcpy32(&minorp[0 + (o_col_idx * minor_m)], /* to   row_idx 0, o_col_idx */
                              &A[1 + (i_col_idx * m)      ], /* from row_idx 1, i_col_idx */
                              minor_nbytes_per_col);
            }

            status_break(status = data_in_columns_determinant(minorp, minor_m, &minor_D));

            if(col_idx & 1)
                minor_D = -minor_D; /* make into cofactor */

            *dp += (A[0 + (col_idx * m)] * minor_D);
        }

        al_ptr_dispose(P_P_ANY_PEDANTIC(&minorp));

        break;
        }
    }

    return(status);
}

/******************************************************************************
*
* shuffle jth column of matrix A to/from vector C
*
******************************************************************************/

static ARRAY_HANDLE permute_tmp;

static inline void
linest_permute_for_cramer(
    P_F64 A /*[m][m]*/,
    P_F64 C /*[m]*/,
    _InVal_     U32 m,
    _InVal_     U32 j)
{
    const U32 n_elements = array_elements(&permute_tmp);
    const U32 n_bytes = n_elements * sizeof32(F64);
    P_F64 p_tmp = array_range(&permute_tmp, F64, 0, n_elements);
    P_F64 A_j = &A[0 + (j * m)];

    memcpy32(p_tmp, C,     n_bytes);
    memcpy32(C,     A_j,   n_bytes);
    memcpy32(A_j,   p_tmp, n_bytes);
}

/******************************************************************************
*
* Solve a system of m simultaneous linear equations
*
* Currently solve system trivially using Cramer's Rule
* (see Bloom, Linear Algebra and Geometry)
*
******************************************************************************/

_Check_return_
static STATUS
linest_solve_system(
    P_F64 A /*[m][m]*/ /*abused but preserved*/,
    P_F64 C /*[m]*/ /*abused but preserved*/,
    P_F64 X /*[m]*/ /*out*/,
    _InVal_     U32 m)
{
    STATUS status = STATUS_OK;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(F64), 0);
    F64 det_A;
    U32 j;

    status_return(data_in_columns_determinant(A, m, &det_A));

    if(det_A == 0.0)
        return(EVAL_ERR_MATRIX_SINGULAR); /* insoluble */

    if(NULL == al_array_alloc(&permute_tmp, F64, m, &array_init_block, &status))
        return(status);

    for(j = 0; j < m; ++j)
    {
        F64 det_B_j;

        /* swap the jth column of matrix A with the C vector */
        linest_permute_for_cramer(A, C, m, j);

        status_break(status = data_in_columns_determinant(A, m, &det_B_j));

        X[j] = det_B_j / det_A;

        /* restore A, C */
        linest_permute_for_cramer(A, C, m, j);
    }

    al_array_dispose(&permute_tmp);

    return(status);
}

/*
Solve the system of simultaneous linear equations AX=C
by the method of linear least squares: square up to the
normal equations by multiplying both sides by transpose(A)
*/

_Check_return_
extern STATUS
linest(
    _InRef_     P_PROC_LINEST_DATA_GET p_proc_get,
    _InRef_     P_PROC_LINEST_DATA_PUT p_proc_put,
    _InVal_     CLIENT_HANDLE client_handle,
    _InVal_     U32 ext_m /* number of independent x variables */,
    _InVal_     U32 n /* number of data points */)
{
    U32 m = ext_m + 1;
    P_F64 M /*[m][m]*/; /* transpose(A).A */
    P_F64 V /*[m]*/; /* transpose(A).C */
    P_F64 X /*[m]*/; /* result vector */
    U32 i, j, r;
    F64 c_r;
    F64 a_ir, a_jr;
    STATUS status;
                                          /* X,  V,  M */
    if(NULL == (X = al_ptr_alloc_bytes(P_F64, (1 + 1 + m) * m * sizeof32(*X), &status)))
        return(status);

    V = X + m;
    M = V + m;

    /* fill V vector (transpose(A).C) */
    for(i = 0; i < m; ++i)
    {
        /* Vi = sum_over_r(a_ir * c_r) */
        P_F64 v_i = &V[i];

        for(*v_i = 0.0, r = 0; r < n; ++r)
        {
            a_ir = (i == 0)
                      ? 1.0
                      : ((* p_proc_get) (client_handle, (LINEST_X_COLOFF-1) + i, r));

            c_r = ((* p_proc_get) (client_handle, LINEST_Y_COLOFF, r));

            *v_i += a_ir * c_r;
        }
    }

    /* fill M matrix (transpose(A).A) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            /* Mij = sum_over_r(a_ir * a_jr) */
            P_F64 m_ij = &M[i + (j * m)];

            for(*m_ij = 0.0, r = 0; r < n; ++r)
            {
                a_ir = (i == 0)
                          ? 1.0
                          : ((* p_proc_get) (client_handle, (LINEST_X_COLOFF-1) + i, r));

                if(i == j)
                    a_jr = a_ir;
                else
                    a_jr = (j == 0)
                              ? 1.0
                              : ((* p_proc_get) (client_handle, (LINEST_X_COLOFF-1) + j, r));

                *m_ij += a_ir * a_jr;
            }
        }
    }

    if(status_ok(status = linest_solve_system(M, V, X, m)))
    {
        STATUS put_status;

        /* output the result vector */
        for(i = 0; i < m; ++i)
            if(status_fail(put_status = (* p_proc_put) (client_handle, LINEST_A_COLOFF, i, X[i])))
                if(status_ok(status))
                    status = put_status;
                /* but keep looping anyway? */
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&X));

    return(status);
}

/* end of mathxtr2.c */
