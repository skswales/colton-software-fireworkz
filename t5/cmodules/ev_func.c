/* ev_func.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Lookup function routines and less common function routines for evaluator */

/* JAD September 1994 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/******************************************************************************
*
* Lookup and reference functions
*
******************************************************************************/

/******************************************************************************
*
* STRING address(row:integer, col:integer {, abs:integer {, a1:boolean {, ext_doc:string}}})
*
******************************************************************************/

PROC_EXEC_PROTO(c_address)
{
    STATUS status = STATUS_OK;
    const ROW row = args[0]->arg.integer - 1;
    const COL col = args[1]->arg.integer - 1;
    BOOL abs_row = TRUE;
    BOOL abs_col = TRUE;
    BOOL a1 = TRUE;
    UCHARZ ustr_buf[BUF_EV_LONGNAMLEN];
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 32);
    quick_ublock_with_buffer_setup(quick_ublock);

    exec_func_ignore_parms();

    if(n_args > 2)
    {
        switch(args[2]->arg.integer)
        {
        default:
            break;

        case 2:
            abs_col = FALSE;
            break;

        case 3:
            abs_row = FALSE;
            break;

        case 4:
            abs_row = FALSE;
            abs_col = FALSE;
            break;
        }
    }

    if(n_args > 3)
        a1 = args[3]->arg.boolean;

    if(n_args > 4)
    {   /* prepend external reference */
        status = quick_ublock_uchars_add(&quick_ublock, args[4]->arg.string.uchars, args[4]->arg.string.size);
    }

    if(a1)
    {   /* A1 */
        if(status_ok(status))
            if(abs_col)
                status = quick_ublock_a7char_add(&quick_ublock, CH_DOLLAR_SIGN);

        if(status_ok(status))
        {
            U32 len = xtos_ustr_buf(ustr_buf, elemof32(ustr_buf), col, TRUE /*upper_case_slr*/);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }

        if(status_ok(status))
            if(abs_row)
                status = quick_ublock_a7char_add(&quick_ublock, CH_DOLLAR_SIGN);

        if(status_ok(status))
        {
            U32 len = xsnprintf(ustr_buf, elemof32(ustr_buf), S32_FMT, (S32) row + 1);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }
    }
    else
    {   /* R1C1 */
        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, 'R');

        if(status_ok(status))
        {
            U32 len = xsnprintf(ustr_buf, elemof32(ustr_buf), abs_row ? S32_FMT : "[" S32_FMT "]", (S32) row + 1);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }

        if(status_ok(status))
            status = quick_ublock_a7char_add(&quick_ublock, 'C');

        if(status_ok(status))
        {
            U32 len = xsnprintf(ustr_buf, elemof32(ustr_buf), abs_col ? S32_FMT : "[" S32_FMT "]", (S32) col + 1);
            status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(ustr_buf), len);
        }
    }

    if(status_ok(status))
    {
        status_assert(ss_string_make_uchars(p_ev_data_res, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock)));
    }
    else
    {
        ev_data_set_error(p_ev_data_res, status);
    }

    quick_ublock_dispose(&quick_ublock);
}

/******************************************************************************
*
* THING choose(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_choose)
{
    exec_func_ignore_parms();

    if((args[0]->arg.integer < 1) || (args[0]->arg.integer >= n_args))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_OUTOFRANGE);
        return;
    }

    status_assert(ss_data_resource_copy(p_ev_data_res, args[args[0]->arg.integer]));
}

/******************************************************************************
*
* INTEGER col / col(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_col)
{
    PC_EV_SLR p_ev_slr;
    S32 col_result;

    exec_func_ignore_parms();

    if(0 == n_args)
        p_ev_slr = p_cur_slr;
    else if(RPN_DAT_SLR == args[0]->did_num)
        p_ev_slr = &(args[0]->arg.slr);
    else if(RPN_DAT_RANGE == args[0]->did_num)
        p_ev_slr = &(args[0]->arg.range.s);
    else
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        return;
    }

    col_result = (S32) ev_slr_col(p_ev_slr) + 1;

    ev_data_set_integer(p_ev_data_res, col_result);
}

/******************************************************************************
*
* INTEGER number of columns in range/array
*
******************************************************************************/

PROC_EXEC_PROTO(c_cols)
{
    S32 cols_result;

    exec_func_ignore_parms();

    if(0 == n_args)
        cols_result = (S32) ev_numcol(ev_slr_docno(p_cur_slr));
    else
    {
        S32 x_size, y_size;
        array_range_sizes(args[0], &x_size, &y_size);
        cols_result = (S32) x_size;
    }

    ev_data_set_integer(p_ev_data_res, cols_result);
}

/******************************************************************************
*
* SLR|other index(array, x:number, y:number {, xsize:number, ysize:number})
*
* returns SLR if it can
*
******************************************************************************/

static void
c_index_common(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA array_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     S32 x_size_out,
    _InVal_     S32 y_size_out)
{
    if((x_size_out == 1) && (y_size_out == 1))
    {
        EV_DATA temp_data;
        (void) array_range_index(&temp_data, array_data, ix, iy, EM_ANY);
        status_assert(ss_data_resource_copy(p_ev_data_out, &temp_data));
        ss_data_free_resources(&temp_data);
        return;
    }

    if(status_ok(ss_array_make(p_ev_data_out, x_size_out, y_size_out)))
    {
        S32 ix_in, iy_in, ix_out, iy_out;

        for(iy_in = iy, iy_out = 0; iy_out < y_size_out; ++iy_in, ++iy_out)
        {
            for(ix_in = ix, ix_out = 0; ix_out < x_size_out; ++ix_in, ++ix_out)
            {
                EV_DATA temp_data;
                (void) array_range_index(&temp_data, array_data, ix_in, iy_in, EM_ANY);
                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ev_data_out, ix_out, iy_out), &temp_data));
                ss_data_free_resources(&temp_data);
            }
        }
    }
}

PROC_EXEC_PROTO(c_index)
{
    const PC_EV_DATA array_data = args[0];
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &x_size_in, &y_size_in);

    /* NB Fireworkz and PipeDream INDEX() has x, y args */
    if(0 == args[1]->arg.integer)
    {   /* zero column number -> whole row */
        ix = 0;
        x_size_out = x_size_in;
    }
    else
    {
        ix = args[1]->arg.integer - 1;
        x_size_out = 1;
    }

    if(0 == args[2]->arg.integer)
    {   /* zero row number -> whole column */
        iy = 0;
        y_size_out = y_size_in;
    }
    else
    {
        iy = args[2]->arg.integer - 1;
        y_size_out = 1;
    }

    /* get size out parameters */
    if(n_args > 4)
    {
        if(0 == args[3]->arg.integer)
        {   /* zero xsize -> all of row starting from column index x */
            x_size_out = x_size_in - ix;
        }
        else
            x_size_out = MAX(1, args[3]->arg.integer);

        if(0 == args[4]->arg.integer)
        {   /* zero ysize -> all of column starting from row index y */
            y_size_out = y_size_in - iy;
        }
        else
            y_size_out = MAX(1, args[4]->arg.integer);
    }

    /* check it's all in range */
    if( ix < 0                           ||
        iy < 0                           ||
        ix + x_size_out - 1 >= x_size_in ||
        iy + y_size_out - 1 >= y_size_in )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_INDEX);
        return;
    }

    c_index_common(p_ev_data_res, array_data, ix, iy, x_size_out, y_size_out);
}

PROC_EXEC_PROTO(c_odf_index)
{
    const PC_EV_DATA array_data = args[0];
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &x_size_in, &y_size_in);

    /* NB OpenDocument INDEX() has row, column args */
    if(0 == args[1]->arg.integer)
    {   /* zero row number -> whole column */
        iy = 0;
        y_size_out = y_size_in;
    }
    else
    {
        iy = args[1]->arg.integer - 1;
        y_size_out = 1;
    }

    if(0 == args[2]->arg.integer)
    {   /* zero column number -> whole row */
        ix = 0;
        x_size_out = x_size_in;
    }
    else
    {
        ix = args[2]->arg.integer - 1;
        x_size_out = 1;
    }

    /* check it's all in range */
    if( ix < 0                           ||
        iy < 0                           ||
        ix + x_size_out - 1 >= x_size_in ||
        iy + y_size_out - 1 >= y_size_in )
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_INDEX);
        return;
    }

    c_index_common(p_ev_data_res, array_data, ix, iy, x_size_out, y_size_out);
}

/******************************************************************************
*
* INTEGER row / row(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_row)
{
    PC_EV_SLR p_ev_slr;
    S32 row_result;

    exec_func_ignore_parms();

    if(0 == n_args)
        p_ev_slr = p_cur_slr;
    else if(RPN_DAT_SLR == args[0]->did_num)
        p_ev_slr = &(args[0]->arg.slr);
    else if(RPN_DAT_RANGE == args[0]->did_num)
        p_ev_slr = &(args[0]->arg.range.s);
    else
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_UNEXARRAY);
        return;
    }

    row_result = (S32) ev_slr_row(p_ev_slr) + 1;

    ev_data_set_integer(p_ev_data_res, row_result);
}

/******************************************************************************
*
* INTEGER number of rows in range/array
*
******************************************************************************/

PROC_EXEC_PROTO(c_rows)
{
    S32 rows_result;

    exec_func_ignore_parms();

    if(0 == n_args)
        rows_result = (S32) ev_numrow(ev_slr_docno(p_cur_slr));
    else
    {
        S32 x_size, y_size;
        array_range_sizes(args[0], &x_size, &y_size);
        rows_result = (S32) y_size;
    }

    ev_data_set_integer(p_ev_data_res, rows_result);
}

/******************************************************************************
*
* Miscellaneous functions
*
******************************************************************************/

/******************************************************************************
*
* command(string)
*
******************************************************************************/

PROC_EXEC_PROTO(c_command)
{
    STATUS status = STATUS_OK;
    P_STACK_ENTRY p_stack_entry = NULL;
    P_STACK_ENTRY p_stack_entry_i = &stack_base[stack_offset];
    ARRAY_HANDLE h_commands;
    P_U8 p_u8;

    exec_func_ignore_parms();

    /* command context in custom function must be topmost caller, not the function itself */
    while(NULL != (p_stack_entry_i = stack_back_search(PtrDiffElemU32(p_stack_entry_i, stack_base), EXECUTING_MACRO)))
        p_stack_entry = p_stack_entry_i;

    if(NULL != (p_u8 = al_array_alloc_U8(&h_commands, args[0]->arg.string.size, &array_init_block_u8, &status)))
    {
        EV_DOCNO ev_docno = p_stack_entry ? ev_slr_docno(&p_stack_entry->slr) : ev_slr_docno(p_cur_slr);
        memcpy32(p_u8, args[0]->arg.string.uchars, args[0]->arg.string.size);
        status_consume(command_array_handle_execute((DOCNO) ev_docno, h_commands)); /* error already reported */
        al_array_dispose(&h_commands);
    }

    if(status_fail(status))
        ev_data_set_error(p_ev_data_res, status);
}

/******************************************************************************
*
* SLR current_cell
*
******************************************************************************/

PROC_EXEC_PROTO(c_current_cell)
{
    exec_func_ignore_parms();

    ev_current_cell(&p_ev_data_res->arg.slr);
    p_ev_data_res->did_num = RPN_DAT_SLR;
}

/******************************************************************************
*
* deref(slr)
*
******************************************************************************/

PROC_EXEC_PROTO(c_deref)
{
    exec_func_ignore_parms();

    if(RPN_DAT_SLR == args[0]->did_num)
        ev_slr_deref(p_ev_data_res, &args[0]->arg.slr);
    else
        status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
}

/******************************************************************************
*
* SLR double_click
*
******************************************************************************/

PROC_EXEC_PROTO(c_doubleclick)
{
    exec_func_ignore_parms();

    ev_double_click(&p_ev_data_res->arg.slr, p_cur_slr);

    if(DOCNO_NONE == ev_slr_docno(&p_ev_data_res->arg.slr))
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NA);
        return;
    }

    p_ev_data_res->did_num = RPN_DAT_SLR;
}

/******************************************************************************
*
* NUMBER even(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_even)
{
    BOOL negate_result = FALSE;
    F64 f64 = args[0]->arg.fp;
    F64 even_result;

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negate_result = TRUE;
    }

    even_result = 2.0 * ceil(f64 * 0.5);

    /* always round away from zero (Excel) */
    if(negate_result)
        even_result = -even_result;

    ev_data_set_real_ti(p_ev_data_res, even_result);
}

/******************************************************************************
*
* BOOLEAN false
*
******************************************************************************/

PROC_EXEC_PROTO(c_false)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, FALSE);
}

/******************************************************************************
*
* ARRAY flip(array)
*
******************************************************************************/

PROC_EXEC_PROTO(c_flip)
{
    S32 x_size, y_size;
    S32 y_half;
    S32 ix, iy;
    S32 y_swap;

    exec_func_ignore_parms();

    status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
    data_ensure_constant(p_ev_data_res);

    if(ev_data_is_error(p_ev_data_res))
        return;

    if(RPN_DAT_ARRAY == p_ev_data_res->did_num)
    {
        array_range_sizes(p_ev_data_res, &x_size, &y_size);
        y_half = y_size / 2;
        y_swap = y_size - 1;
        for(iy = 0; iy < y_half; ++iy, y_swap -= 2)
        {
            for(ix = 0; ix < x_size; ++ix)
            {
                EV_DATA temp_data;
                temp_data = *ss_array_element_index_wr(p_ev_data_res, ix, iy + y_swap);
                *ss_array_element_index_wr(p_ev_data_res, ix, iy + y_swap) =
                *ss_array_element_index_wr(p_ev_data_res, ix, iy);
                *ss_array_element_index_wr(p_ev_data_res, ix, iy) = temp_data;
            }
        }
    }
}

/******************************************************************************
*
* BOOLEAN isxxx(value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_isblank)
{
    BOOL isblank_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_BLANK:
        isblank_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isblank_result);
}

PROC_EXEC_PROTO(c_iserr)
{
    BOOL iserr_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_ERROR:
        iserr_result = (EVAL_ERR_ODF_NA != args[0]->arg.ev_error.status);
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, iserr_result);
}

PROC_EXEC_PROTO(c_iserror)
{
    BOOL iserror_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_ERROR:
        iserror_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, iserror_result);
}

static void
iseven_isodd_calc(
    _InoutRef_  P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA arg0,
    _InVal_     BOOL test_iseven)
{
    BOOL is_even = FALSE;

    switch(arg0->did_num)
    {
    case RPN_DAT_REAL:
        {
        F64 f64 = arg0->arg.fp;
        if(f64 < 0.0)
            f64 = -f64;
        f64 = floor(f64); /* NB truncate (Excel) */
        is_even = (f64 == (2.0 * floor(f64 * 0.5))); /* exactly divisible by two? */
        ev_data_set_boolean(p_ev_data_out, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
        break;
        }

    case RPN_DAT_BOOL8: /* more useful? */
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        {
        S32 s32 = arg0->arg.integer;
        if(s32 < 0)
            s32 = -s32;
        is_even = (0 == (s32 & 1)); /* bottom bit clear -> number is even */
        ev_data_set_boolean(p_ev_data_out, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
        break;
        }

#if 0 /* more pedantic? */
    case RPN_DAT_BOOL8:
        ev_data_set_error(p_ev_data_out, EVAL_ERR_UNEXNUMBER);
        break;
#endif

    default: default_unhandled();
        ev_data_set_boolean(p_ev_data_out, FALSE);
        break;
    }
}

PROC_EXEC_PROTO(c_iseven)
{
    exec_func_ignore_parms();

    iseven_isodd_calc(p_ev_data_res, args[0], TRUE /*->test_ISEVEN*/);
}

PROC_EXEC_PROTO(c_islogical)
{
    BOOL islogical_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_BOOL8:
        islogical_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, islogical_result);
}

PROC_EXEC_PROTO(c_isna)
{
    BOOL isna_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_ERROR:
        isna_result = (EVAL_ERR_ODF_NA == args[0]->arg.ev_error.status);
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isna_result);
}

PROC_EXEC_PROTO(c_isnontext)
{
    BOOL isnontext_result = TRUE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_STRING:
        isnontext_result = FALSE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isnontext_result);
}

PROC_EXEC_PROTO(c_isnumber)
{
    BOOL isnumber_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_REAL:
    /*case RPN_DAT_BOOL8:*/ /* indeed! that's a LOGICAL for Excel */
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
        isnumber_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isnumber_result);
}

PROC_EXEC_PROTO(c_isodd)
{
    exec_func_ignore_parms();

    iseven_isodd_calc(p_ev_data_res, args[0], FALSE /*->test_ISODD*/);
}

PROC_EXEC_PROTO(c_isref)
{
    BOOL isref_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_SLR:
    case RPN_DAT_RANGE:
        isref_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, isref_result);
}

PROC_EXEC_PROTO(c_istext)
{
    BOOL istext_result = FALSE;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_STRING:
        istext_result = TRUE;
        break;

    default:
        break;
    }

    ev_data_set_boolean(p_ev_data_res, istext_result);
}

/******************************************************************************
*
* ERROR na
*
******************************************************************************/

PROC_EXEC_PROTO(c_na)
{
    exec_func_ignore_parms();

    ev_data_set_error(p_ev_data_res, EVAL_ERR_ODF_NA);
}

/******************************************************************************
*
* NUMBER odd(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_odd)
{
    BOOL negate_result = FALSE;
    F64 f64 = args[0]->arg.fp;

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negate_result = TRUE;
    }

    f64 = (2.0 * ceil((f64 + 1.0) * 0.5)) - 1.0;

    /* always round away from zero (Excel) */
    if(negate_result)
        f64 = -f64;

    ev_data_set_real_ti(p_ev_data_res, f64);
}

/******************************************************************************
*
* INTEGER page(ref, n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_page)
{
    S32 page_result;
    BOOL xy = (n_args < 2) ? TRUE : (0 != args[1]->arg.integer);
    STATUS status = ev_page_slr(&args[0]->arg.slr, xy);

    exec_func_ignore_parms();

    if(status_fail(status))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    page_result = (S32) status + 1;

    ev_data_set_integer(p_ev_data_res, page_result);
}

/******************************************************************************
*
* INTEGER pages(n)
*
******************************************************************************/

PROC_EXEC_PROTO(c_pages)
{
    S32 pages_result;
    BOOL xy = (n_args < 1) ? TRUE : (0 != args[0]->arg.integer);
    STATUS status = ev_page_last(ev_slr_docno(p_cur_slr), xy);

    exec_func_ignore_parms();

    if(status_fail(status))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    pages_result = (S32) status;

    ev_data_set_integer(p_ev_data_res, pages_result);
}

/******************************************************************************
*
* define and set value of a name
*
* VALUE set_name("name", value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_set_name)
{
    S32 res;
    EV_HANDLE name_key, name_num;

    exec_func_ignore_parms();

    if((res = name_make(&name_key, ev_slr_docno(p_cur_slr), &args[0]->arg.string, args[1], NULL)) < 0)
    {
        ev_data_set_error(p_ev_data_res, res);
        return;
    }

    name_num = name_def_find(name_key);

    status_assert(ss_data_resource_copy(p_ev_data_res, &array_ptr(&name_def.h_table, EV_NAME, name_num)->def_data));
}

/******************************************************************************
*
* ARRAY sort(array {, column_index})
*
******************************************************************************/

PROC_EXEC_PROTO(c_sort)
{
    U32 x_index = 0;
    STATUS status;

    exec_func_ignore_parms();

    if(n_args > 1)
        x_index = (U32) args[1]->arg.integer; /* array_sort() does range checking */ /* NB NOT -1 - see printed documentation */

    status_assert(ss_data_resource_copy(p_ev_data_res, args[0]));
    data_ensure_constant(p_ev_data_res);

    if(ev_data_is_error(p_ev_data_res))
        return;

    if(status_fail(status = array_sort(p_ev_data_res, x_index)))
    {
        ss_data_free_resources(p_ev_data_res);
        ev_data_set_error(p_ev_data_res, status);
    }
}

/******************************************************************************
*
* BOOLEAN true
*
******************************************************************************/

PROC_EXEC_PROTO(c_true)
{
    exec_func_ignore_parms();

    ev_data_set_boolean(p_ev_data_res, TRUE);
}

/******************************************************************************
*
* STRING return type of argument
*
******************************************************************************/

PROC_EXEC_PROTO(c_type)
{
    EV_TYPE type;
    PC_A7STR a7str_type;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    default: default_unhandled();
#if CHECKING
    case RPN_DAT_REAL:
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
#endif
        type = EM_REA;
        break;

    case RPN_DAT_SLR:
        type = EM_SLR;
        break;

    case RPN_DAT_STRING:
        type = EM_STR;
        break;

    case RPN_DAT_DATE:
        type = EM_DAT;
        break;

    case RPN_DAT_RANGE:
    case RPN_DAT_ARRAY:
    case RPN_DAT_FIELD:
        type = EM_ARY;
        break;

    case RPN_DAT_BLANK:
        type = EM_BLK;
        break;

    case RPN_DAT_ERROR:
        type = EM_ERR;
        break;
    }

    a7str_type = type_from_flags(type);
    PTR_ASSERT(a7str_type);

    status_assert(ss_string_make_ustr(p_ev_data_res, (PC_USTR) a7str_type)); /* U is superset of A7 */
}

/******************************************************************************
*
* NUMBER return OpenDocument / Microsoft Excel compatible type of argument
*
******************************************************************************/

PROC_EXEC_PROTO(c_odf_type)
{
    S32 odf_type_result;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    default: default_unhandled();
#if CHECKING
    case RPN_DAT_REAL:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE: /* Excel stores these as real numbers; we can convert if required */
    case RPN_DAT_BLANK: /* yup */
#endif
        odf_type_result = 1;
        break;

    case RPN_DAT_STRING:
        odf_type_result = 2;
        break;

    case RPN_DAT_BOOL8:
        odf_type_result = 4;
        break;

    case RPN_DAT_ERROR:
        odf_type_result = 16;
        break;

    case RPN_DAT_RANGE:
    case RPN_DAT_ARRAY:
    case RPN_DAT_FIELD:
        odf_type_result = 64;
        break;
    }

    ev_data_set_integer(p_ev_data_res, odf_type_result);
}

/******************************************************************************
*
* REAL return the version number
*
******************************************************************************/

PROC_EXEC_PROTO(c_version)
{
    exec_func_ignore_parms();

    ev_data_set_real(p_ev_data_res, tstrtod(resource_lookup_tstr(MSG_SKEL_VERSION), NULL));
}

/* end of ev_func.c */
