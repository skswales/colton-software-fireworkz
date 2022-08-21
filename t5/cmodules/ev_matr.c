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
determinant_0(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    UNREFERENCED_PARAMETER(ap);
    UNREFERENCED_PARAMETER_InVal_(m);
    assert(m == 0);

    *dp = 1.0; /* not as silly a question as RJM first thought! (see Bloom, Linear Algebra and Geometry) */

    return(STATUS_OK);
}

_Check_return_
static STATUS
determinant_1(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    UNREFERENCED_PARAMETER_InVal_(m);
    assert(m == 1);

    *dp = *ap;

    return(STATUS_OK);
}

_Check_return_
static STATUS
determinant_2(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    UNREFERENCED_PARAMETER_InVal_(m);
    assert(m == 2);

#define a11 (_Aij(ap, 2, 0, 0))
#define a12 (_Aij(ap, 2, 0, 1))
#define a21 (_Aij(ap, 2, 1, 0))
#define a22 (_Aij(ap, 2, 1, 1))
    *dp = (a11 * a22) - (a12 * a21);
#undef a11
#undef a12
#undef a21
#undef a22
 
    return(STATUS_OK);
}

_Check_return_
static STATUS
determinant_3(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    UNREFERENCED_PARAMETER_InVal_(m);
    assert(m == 3);

#define a11 (_Aij(ap, 3, 0, 0))
#define a12 (_Aij(ap, 3, 0, 1))
#define a13 (_Aij(ap, 3, 0, 2))
#define a21 (_Aij(ap, 3, 1, 0))
#define a22 (_Aij(ap, 3, 1, 1))
#define a23 (_Aij(ap, 3, 1, 2))
#define a31 (_Aij(ap, 3, 2, 0))
#define a32 (_Aij(ap, 3, 2, 1))
#define a33 (_Aij(ap, 3, 2, 2))
    *dp = + a11 * ((a22 * a33) - (a23 * a32))
          - a12 * ((a21 * a33) - (a23 * a31))
          + a13 * ((a21 * a32) - (a22 * a31));
#undef a11
#undef a12
#undef a13
#undef a21
#undef a22
#undef a23
#undef a31
#undef a32
#undef a33
 
    return(STATUS_OK);
}

#define DETERMINANT_GAUSS_ELIMINATION 1

#if !defined(DETERMINANT_GAUSS_ELIMINATION)

_Check_return_
static STATUS
determinant(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp);

_Check_return_
static STATUS
determinant_minors(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    /* recurse evaluating minors */
    STATUS status = STATUS_OK;
    P_F64 minor;
    const U32 minor_m = m - 1;
    F64 minor_D;
    U32 col_idx, i_col_idx, o_col_idx;
    U32 i_row_idx, o_row_idx;

    *dp = 0.0;

    if(NULL == (minor = al_ptr_alloc_elem(F64, minor_m * minor_m, &status)))
        return(status); /* unable to determine */

    for(col_idx = 0; col_idx < m; ++col_idx)
    {
        F64 a1j = _em(ap, m, 0, col_idx);

        if(0.0 == a1j)
            continue;

        /* build minor by removing all of top row and current column */
        for(i_col_idx = o_col_idx = 0; o_col_idx < minor_m; ++i_col_idx, ++o_col_idx)
        {
            if(i_col_idx == col_idx)
                ++i_col_idx;

            for(i_row_idx = 1, o_row_idx = 0; o_row_idx < minor_m; ++i_row_idx, ++o_row_idx)
            {
                f64_copy(_em(minor, minor_m, o_row_idx, o_col_idx), _em(ap, m, i_row_idx, i_col_idx));
            }
        }

        status_break(status = determinant(minor, minor_m, &minor_D));

        if(col_idx & 1) /* cofactor has alternating signs */
            *dp -= (a1j * minor_D);
        else
            *dp += (a1j * minor_D);
    }

    al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));

    return(status);
}

#endif /* DETERMINANT_GAUSS_ELIMINATION */

#if defined(DETERMINANT_GAUSS_ELIMINATION)

_Check_return_
static STATUS
determinant_gauss_elimination(
    _In_reads_x_(m*m) PC_F64 ap_in /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    STATUS status = STATUS_OK;
    U32 curr_row_idx, row_idx, col_idx;
    P_F64 ap /*[m][m]*/;

    *dp = 1.0;

    /* take a copy as this routine trashes the array, which some callers don't want! */
    if(NULL == (ap = al_ptr_alloc_elem(F64, m * m, &status)))
        return(status); /* unable to determine */

    memcpy32(ap, ap_in, (m * m) * sizeof32(F64));

    /* for each row (i==curr_row_idx) */
    for(curr_row_idx = 0; curr_row_idx < m; ++curr_row_idx)
    {
        /* scale the whole row such that the element on the diagonal (Aii) is one */
        const F64 initial_Aii = _Aij(ap, m, curr_row_idx, curr_row_idx);
        F64 reciprocal_Aii;

        /* we are going to be dividing by Aii, so check validity */
        if(!isgreater(fabs(initial_Aii), F64_MIN))
        {
            *dp = 0.0; /* this is kosher */
            break;
        }

        *dp *= initial_Aii; /* determinant is scaled by this */

        reciprocal_Aii = 1.0 / initial_Aii;

        /* for each column in the current row, scale together */
        _Aij(ap, m, curr_row_idx, curr_row_idx) = 1.0; /* Aii := one */

        /* note that values to the left of the (i)th column in the current row (and those below) are already zero, so we can skip scaling */
        for(col_idx = curr_row_idx + 1; col_idx < m; ++col_idx)
        {
            _Aij(ap, m, curr_row_idx, col_idx) *= reciprocal_Aii;
        }

        /* for all succeding rows, subtract a multiple of this scaled current row to make the ith column zero */
        for(row_idx = curr_row_idx + 1; row_idx < m; ++row_idx)
        {
            F64 multiples;

            if(0.0 == _Aij(ap, m, row_idx, curr_row_idx))
                continue;

            multiples = _Aij(ap, m, row_idx, curr_row_idx); /* as _Aij(ap, m, curr_row_idx, curr_row_idx) is now one */

            _Aij(ap, m, row_idx, curr_row_idx) = 0.0; /* := zero */

            /* note that values to the left of the (i)th column in the current row are zero */
            for(col_idx = curr_row_idx + 1; col_idx < m; ++col_idx)
            {
                _Aij(ap, m, row_idx, col_idx) -= (multiples * _Aij(ap, m, curr_row_idx, col_idx));
            }
        }
    }

    /* should end up with row-echelon form array - lower triangle zero, all diagonal elements one */
    al_ptr_dispose(P_P_ANY_PEDANTIC(&ap));

    return(status);
}

#endif /* DETERMINANT_GAUSS_ELIMINATION */

_Check_return_
static STATUS
determinant(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    switch(m)
    {
    case 0:
        return(determinant_0(ap, m, dp));

    case 1:
        return(determinant_1(ap, m, dp));

    case 2:
        return(determinant_2(ap, m, dp));

    case 3:
        return(determinant_3(ap, m, dp));

    default:
#if defined(DETERMINANT_GAUSS_ELIMINATION)
        return(determinant_gauss_elimination(ap, m, dp));
#else
        return(determinant_minors(ap, m, dp));
#endif
    }
}

#if RISCOS
//#define POSSIBLE_VFP_SUPPORT 1
#endif

#if defined(POSSIBLE_VFP_SUPPORT)

#include "cmodules/riscos/vfp_double.h"

_Check_return_
static STATUS
determinant_3_vfp(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    UNREFERENCED_PARAMETER_InVal_(m);
    assert(m == 3);

#define a11 (_Aij(ap, 3, 0, 0))
#define a12 (_Aij(ap, 3, 0, 1))
#define a13 (_Aij(ap, 3, 0, 2))
#define a21 (_Aij(ap, 3, 1, 0))
#define a22 (_Aij(ap, 3, 1, 1))
#define a23 (_Aij(ap, 3, 1, 2))
#define a31 (_Aij(ap, 3, 2, 0))
#define a32 (_Aij(ap, 3, 2, 1))
#define a33 (_Aij(ap, 3, 2, 2))

    VFP_DOUBLE nums[3][3];
    VFP_DOUBLE determ, temp /*, product_l, product_r*/;
#define n11 (nums[0][0])
#define n12 (nums[0][1])
#define n13 (nums[0][2])
#define n21 (nums[1][0])
#define n22 (nums[1][1])
#define n23 (nums[1][2])
#define n31 (nums[2][0])
#define n32 (nums[2][1])
#define n33 (nums[2][2])
#if 1 /* simple flat copy loop */
    PC_F64 p_f64_in = &a11;
    P_VFP_DOUBLE p_vfp_double_out = &n11;
    int i;

    for(i = 0; i < 3*3; ++i)
        vfp_double_copy_from_f64(p_vfp_double_out++, p_f64_in++);
#else
    vfp_double_copy_from_f64(&n11, &a11);
    vfp_double_copy_from_f64(&n12, &a12);
    vfp_double_copy_from_f64(&n13, &a13);
    vfp_double_copy_from_f64(&n21, &a21);
    vfp_double_copy_from_f64(&n22, &a22);
    vfp_double_copy_from_f64(&n23, &a23);
    vfp_double_copy_from_f64(&n31, &a31);
    vfp_double_copy_from_f64(&n32, &a32);
    vfp_double_copy_from_f64(&n33, &a33);
#endif

#undef a11
#undef a12
#undef a13
#undef a21
#undef a22
#undef a23
#undef a31
#undef a32
#undef a33

    /* determinant := */

    /* + [n11 * ((n22 * n33) - (n23 * n32))] */
#if 1 /* product - product */
    temp = vfp_double_mul_sub_mul(/*lhs*/ &n22, &n33,
                                  /*rhs*/ &n23, &n32);
#else
    product_l /*22_33*/ = vfp_double_mul(&n22, &n33);
    product_r /*23_32*/ = vfp_double_mul(&n23, &n32);
    temp = vfp_double_sub(&product_l /*22_33*/, &product_r /*23_32*/);
#endif
    determ = vfp_double_mul(&n11, &temp);

#if 1 /* turn this second term around for MLA */
    /* + [n12 * ((n23 * n31) - (n21 * n33))] */
#if 1 /* product - product */
    temp = vfp_double_mul_sub_mul(/*lhs*/ &n23, &n31,
                                  /*rhs*/ &n21, &n33);
#else
    product_l /*23_31*/ = vfp_double_mul(&n23, &n31);
    product_r /*21_33*/ = vfp_double_mul(&n21, &n33);
    temp = vfp_double_sub(&product_l /*23_31*/, &product_r /*21_33*/);
#endif
    determ = vfp_double_mla(&n12, &temp, &determ);
#else
    /* - [n12 * ((n21 * n33) - (n23 * n31))] */
    product_l /*21_33*/ = vfp_double_mul(&n21, &n33);
    product_r /*23_31*/ = vfp_double_mul(&n23, &n31);
    temp = vfp_double_sub(&product_l /*21_33*/, &product_r /*23_31*/);
    temp = vfp_double_mul(&n12, &temp);
    determ = vfp_double_sub(&determ, &temp);
#endif

    /* + [n13 * ((n21 * n32) - (n22 * n31))] */
#if 1 /* product - product */
    temp = vfp_double_mul_sub_mul(/*lhs*/ &n21, &n32,
                                  /*rhs*/ &n22, &n31);
#else
    product_l /*21_32*/ = vfp_double_mul(&n21, &n32);
    product_r /*22_31*/ = vfp_double_mul(&n22, &n31);
    temp = vfp_double_sub(&product_l /*21_32*/, &product_r /*22_31*/);
#endif
    determ = vfp_double_mla(&n13, &temp, &determ);

#undef n11
#undef n12
#undef n13
#undef n21
#undef n22
#undef n23
#undef n31
#undef n32
#undef n33

    vfp_double_copy_to_f64(dp, &determ);

    return(STATUS_OK);
}

_Check_return_
static STATUS
determinant_vfp(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp);

_Check_return_
static STATUS
determinant_vfp_minors(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    /* recurse evaluating minors (VFP is active) */
    VFP_DOUBLE vfp_determ;
    STATUS status = STATUS_OK;
    P_F64 minor;
    const U32 minor_m = m - 1;
    F64 minor_D;
    U32 col_idx, i_col_idx, o_col_idx;
    U32 i_row_idx, o_row_idx;
    F64 nums[5*5]; /* don't really need to allocate for small ones */

    f64_copy_words(dp, &g_real_zero); /* *dp = 0.0; saves a FPA MVFD in this VFP code */
    vfp_determ = vfp_double_from_f64(dp);

    if(minor_m <= 5)
        minor = nums;
    else if(NULL == (minor = al_ptr_alloc_elem(F64, minor_m * minor_m, &status)))
        return(status); /* unable to determine */

    for(col_idx = 0; col_idx < m; ++col_idx)
    {
        /* F64 a1j = _em(ap, m, 0, col_idx); */
        const VFP_DOUBLE vfp_a1j = vfp_double_from_f64(&/*a1j*/_em(ap, m, 0, col_idx));

        if(0 == vfp_double_compare_zero(&vfp_a1j)) /* -/0/+ */
            continue;

        /* build minor by removing all of top row and current column */
        for(i_col_idx = o_col_idx = 0; o_col_idx < minor_m; ++i_col_idx, ++o_col_idx)
        {
            if(i_col_idx == col_idx)
                ++i_col_idx;

            for(i_row_idx = 1, o_row_idx = 0; o_row_idx < minor_m; ++i_row_idx, ++o_row_idx)
            {
                f64_copy(_em(minor, minor_m, o_row_idx, o_col_idx), _em(ap, m, i_row_idx, i_col_idx));
            }
        }

        status_break(status = determinant_vfp(minor, minor_m, &minor_D));

        {
        const VFP_DOUBLE vfp_minor_D = vfp_double_from_f64(&minor_D);
        const VFP_DOUBLE vfp_cofactor = vfp_double_mul(&vfp_a1j, &vfp_minor_D);

        if(col_idx & 1) /* cofactor has alternating signs */
            vfp_determ = vfp_double_sub(&vfp_determ, &vfp_cofactor);
        else
            vfp_determ = vfp_double_add(&vfp_determ, &vfp_cofactor);
        } /*block*/
    }

    if(minor != nums)
        al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));

    vfp_double_copy_to_f64(dp, &vfp_determ);

    return(status);
}

_Check_return_
static STATUS
determinant_vfp(
    _In_reads_x_(m*m) PC_F64 ap /*[m][m]*/,
    _InVal_     U32 m,
    _OutRef_    P_F64 dp)
{
    assert(m >= 3);

    if(m == 3)
        return(determinant_3_vfp(ap, m, dp));

   return(determinant_vfp_minors(ap, m, dp));
}

#endif /* POSSIBLE_VFP_SUPPORT */

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
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MATRIX_NOT_SQUARE);

    if(x_size == 0)
    {
        ss_data_set_integer(p_ss_data_res, 1); /* yes, really */
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

#if defined(POSSIBLE_VFP_SUPPORT)
        bool f_using_vfp = false;

        if(m >= 3)
            f_using_vfp = vfp_context_ensure();
#endif

        assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

        /* load up the matrix (a) */
        for(i = 0; i < m; ++i)
        {
            for(j = 0; j < m; ++j)
            {
                SS_DATA ss_data;

                if(DATA_ID_REAL != array_range_index(&ss_data, args[0], j, i, EM_REA)) /* NB j,i */
                    status_break(status = EVAL_ERR_MATRIX_NOT_NUMERIC);

                f64_copy(_Aij(a, m, i, j), ss_data.arg.fp);
            }
        }

#if defined(POSSIBLE_VFP_SUPPORT)
        if(status_ok(status = (f_using_vfp ? determinant_vfp : determinant)(a, m, &m_determ_result)))
#else
        if(status_ok(status = determinant(a, m, &m_determ_result)))
#endif
        {
            ss_data_set_real(p_ss_data_res, m_determ_result);
        }

        if(a != nums)
            al_ptr_dispose(P_P_ANY_PEDANTIC(&a));

#if defined(POSSIBLE_VFP_SUPPORT)
        if(f_using_vfp)
            vfp_context_destroy();
#endif
    }

    if(status_fail(status))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
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
#if defined(POSSIBLE_VFP_SUPPORT)
    bool f_using_vfp = false;
#endif

    exec_func_ignore_parms();

    /* get x and y sizes */
    array_range_sizes(args[0], &x_size, &y_size);

    if(x_size != y_size)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MATRIX_NOT_SQUARE);

    m = x_size;

    if(status_fail(ss_array_make(p_ss_data_res, m, m)))
        return;

    if(NULL == (a = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*a)), &status)))
        goto endpoint;

    assert(&_Aij(a, m, m - 1, m - 1) + 1 == &a[m * m]);

    if(NULL == (adj = al_ptr_alloc_bytes(P_F64, m * (m * sizeof32(*adj)), &status)))
        goto endpoint;

#if defined(POSSIBLE_VFP_SUPPORT)
    if(m >= 3)
        f_using_vfp = vfp_context_ensure();
#endif

    /* load up the matrix (a) */
    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            SS_DATA ss_data;

            if(DATA_ID_REAL != array_range_index(&ss_data, args[0], j, i, EM_REA)) /* NB j,i */
            {
                status = EVAL_ERR_MATRIX_NOT_NUMERIC;
                goto endpoint;
            }

            f64_copy(_Aij(a, m, i, j), ss_data.arg.fp);
        }
    }

#if defined(POSSIBLE_VFP_SUPPORT)
    if(status_fail(status = (f_using_vfp ? determinant_vfp : determinant)(a, m, &D)))
#else
    if(status_fail(status = determinant(a, m, &D)))
#endif
        goto endpoint;

    /* we are going to be dividing by D, so check validity */
    if(!isgreater(fabs(D), F64_MIN) /*0.0 == D*/)
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

                    f64_copy(_Aij(minor, minor_m, out_i, out_j), _Aij(a, m, in_i, in_j));
                }
            }

#if defined(POSSIBLE_VFP_SUPPORT)
            if(status_fail(status = (f_using_vfp ? determinant_vfp : determinant)(minor, minor_m, &minor_D)))
#else
            if(status_fail(status = determinant(minor, minor_m, &minor_D)))
#endif
                goto endpoint;

            f64_copy(_Aij(adj, m, i, j), minor_D);
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

    { /* copy out the inverse(A) == adjunct(A) / determinant(A) */
#if defined(POSSIBLE_VFP_SUPPORT)
    const VFP_DOUBLE vfp_D = vfp_double_from_f64(&D); /* copy once - trivial, so doesn't matter if VFP not being used! */
#endif

    for(i = 0; i < m; ++i)
    {
        for(j = 0; j < m; ++j)
        {
            const P_SS_DATA p_ss_data = ss_array_element_index_wr(p_ss_data_res, j, i); /* NB j,i */
            const PC_F64 p_Aji = &_Aij(adj, m, j, i); /* NB j,i here too for transpose step 3 above */
#if defined(POSSIBLE_VFP_SUPPORT)
            if(f_using_vfp)
            {
                const VFP_DOUBLE vfp_Aji = vfp_double_from_f64(p_Aji);
                const VFP_DOUBLE vfp_temp = vfp_double_div(&vfp_Aji, &vfp_D);
                ss_data_set_real_from_vfp_double(p_ss_data, vfp_temp);
                continue;
            }
#endif
            ss_data_set_real(p_ss_data, *p_Aji / D);
        }
    }
    } /*block*/

endpoint:

#if defined(POSSIBLE_VFP_SUPPORT)
    if(f_using_vfp)
        vfp_context_destroy();
#endif

    al_ptr_dispose(P_P_ANY_PEDANTIC(&minor));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&adj));
    al_ptr_dispose(P_P_ANY_PEDANTIC(&a));

    if(status_fail(status))
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
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

    if(x_size[0] != y_size[1]) /* whinge about dimensions */
        exec_func_status_return(p_ss_data_res, EVAL_ERR_MISMATCHED_MATRICES);

    if(status_ok(ss_array_make(p_ss_data_res, x_size[1], y_size[0])))
    {
        S32 ix, iy;

        for(ix = 0; ix < x_size[1]; ++ix)
        {
            for(iy = 0; iy < y_size[0]; ++iy)
            {
                F64 product = 0.0;
                S32 elem;
                P_SS_DATA elep;

                for(elem = 0; elem < x_size[0]; elem++)
                {
                    SS_DATA ss_data[2];

                    (void) array_range_index(&ss_data[0], args[0], elem, iy, EM_REA);
                    (void) array_range_index(&ss_data[1], args[1], ix, elem, EM_REA);

                    if( ss_data_is_real(&ss_data[0]) && ss_data_is_real(&ss_data[1]) )
                    {
                        product += ss_data_get_real(&ss_data[0]) * ss_data_get_real(&ss_data[1]);
                    }
                    else
                    {
                        ss_data_free_resources(p_ss_data_res);
                        ss_data_set_error(p_ss_data_res, EVAL_ERR_MATRIX_NOT_NUMERIC);
                        return;
                    }
                }

                elep = ss_array_element_index_wr(p_ss_data_res, ix, iy);
                ss_data_set_real(elep, product);
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
    const S32 n = ss_data_get_integer(args[0]);

    exec_func_ignore_parms();

    if(status_ok(ss_array_make(p_ss_data_res, n, n)))
    {
        S32 ix, iy;

        for(ix = 0; ix < n; ++ix)
        {
            for(iy = 0; iy < n; ++iy)
            {
                const P_SS_DATA elep = ss_array_element_index_wr(p_ss_data_res, ix, iy);

                ss_data_set_integer(elep, (ix == iy) ? 1 : 0);
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
    if(status_ok(ss_array_make(p_ss_data_res, y_size, x_size)))
    {
        for(ix = 0; ix < x_size; ++ix)
        {
            for(iy = 0; iy < y_size; ++iy)
            {
                SS_DATA temp_data;

                (void) array_range_index(&temp_data, args[0], ix, iy, EM_ANY);

                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ss_data_res, iy, ix), &temp_data));

                ss_data_free_resources(&temp_data);
            }
        }
    }
}

/* end of ev_matr.c */
