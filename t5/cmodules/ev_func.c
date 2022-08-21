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
* STRING address(row:integer, col:integer {, abs:integer {, a1:Logical=TRUE {, ext_doc:string}}})
*
******************************************************************************/

PROC_EXEC_PROTO(c_address)
{
    STATUS status = STATUS_OK;
    const ROW row = ss_data_get_integer(args[0]) - 1;
    const COL col = ss_data_get_integer(args[1]) - 1;
    BOOL abs_row = TRUE;
    BOOL abs_col = TRUE;
    const bool a1 = (n_args > 3) ? ss_data_get_logical(args[3]) : true;
    UCHARZ ustr_buf[BUF_EV_LONGNAMLEN];
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 32);
    quick_ublock_with_buffer_setup(quick_ublock);

    exec_func_ignore_parms();

    if(n_args > 2)
    {
        switch(ss_data_get_integer(args[2]))
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

    if(n_args > 4)
    {   /* prepend external reference */
        status = quick_ublock_uchars_add(&quick_ublock, ss_data_get_string(args[4]), ss_data_get_string_size(args[4]));
    }

    if(a1)
    {   /* A1 */
        if(status_ok(status))
            if(abs_col)
                status = quick_ublock_a7char_add(&quick_ublock, CH_DOLLAR_SIGN);

        if(status_ok(status))
        {
            U32 len = xtos_ustr_buf(ustr_bptr(ustr_buf), elemof32(ustr_buf), col, TRUE /*upper_case_slr*/);
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
        status_assert(ss_string_make_uchars(p_ss_data_res, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock)));

    quick_ublock_dispose(&quick_ublock);

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* THING choose(list)
*
******************************************************************************/

PROC_EXEC_PROTO(c_choose)
{
    const S32 arg_number = ss_data_get_integer(args[0]);

    exec_func_ignore_parms();

    if( (arg_number < 1) || (arg_number >= n_args) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_OUTOFRANGE);

    status_assert(ss_data_resource_copy(p_ss_data_res, args[arg_number]));
}

/******************************************************************************
*
* INTEGER col / col(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_col)
{
    PC_EV_SLR p_ev_slr = p_cur_slr;
    S32 col_result;

    exec_func_ignore_parms();

    if(0 != n_args)
    {
        if(DATA_ID_SLR == ss_data_get_data_id(args[0]))
            p_ev_slr = &(args[0]->arg.slr);
        else if(DATA_ID_RANGE == ss_data_get_data_id(args[0]))
            p_ev_slr = &(args[0]->arg.range.s);
        else
            exec_func_status_return(p_ss_data_res, EVAL_ERR_UNEXARRAY);
    }

    col_result = (S32) ev_slr_col(p_ev_slr) + 1;

    ss_data_set_integer(p_ss_data_res, col_result);
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

    ss_data_set_integer(p_ss_data_res, cols_result);
}

/******************************************************************************
*
* SLR|other index(array, x:number, y:number {, x_size:number, y_size:number})
*
* returns SLR if it can
*
******************************************************************************/

static void
c_index_common(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA array_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     S32 x_size_out,
    _InVal_     S32 y_size_out)
{
    if((x_size_out == 1) && (y_size_out == 1))
    {
        SS_DATA temp_data;
        (void) array_range_index(&temp_data, array_data, ix, iy, EM_ANY);
        status_assert(ss_data_resource_copy(p_ss_data_out, &temp_data));
        ss_data_free_resources(&temp_data);
        return;
    }

    if(status_ok(ss_array_make(p_ss_data_out, x_size_out, y_size_out)))
    {
        S32 ix_in, iy_in, ix_out, iy_out;

        for(iy_in = iy, iy_out = 0; iy_out < y_size_out; ++iy_in, ++iy_out)
        {
            for(ix_in = ix, ix_out = 0; ix_out < x_size_out; ++ix_in, ++ix_out)
            {
                SS_DATA temp_data;
                (void) array_range_index(&temp_data, array_data, ix_in, iy_in, EM_ANY);
                status_assert(ss_data_resource_copy(ss_array_element_index_wr(p_ss_data_out, ix_out, iy_out), &temp_data));
                ss_data_free_resources(&temp_data);
            }
        }
    }
}

PROC_EXEC_PROTO(c_index)
{
    const PC_SS_DATA array_data = args[0];
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &x_size_in, &y_size_in);

    /* NB Fireworkz and PipeDream INDEX() has x, y args */
    if(ss_data_get_integer(args[1]) <= 0)
    {   /* zero column number -> whole row */
        ix = 0;
        x_size_out = x_size_in;
    }
    else
    {
        ix = ss_data_get_integer(args[1]) - 1;
        x_size_out = 1;
    }

    if(ss_data_get_integer(args[2]) <= 0)
    {   /* zero row number -> whole column */
        iy = 0;
        y_size_out = y_size_in;
    }
    else
    {
        iy = ss_data_get_integer(args[2]) - 1;
        y_size_out = 1;
    }

    /* get size out parameters */
    if(n_args > 4)
    {
        x_size_out = MAX(0, ss_data_get_integer(args[3]));
        if(0 == x_size_out)
        {   /* zero x_size -> all of row starting from column index x */
            x_size_out = x_size_in - ix;
        }

        y_size_out = MAX(0, ss_data_get_integer(args[4]));
        if(0 == y_size_out)
        {   /* zero y_size -> all of column starting from row index y */
            y_size_out = y_size_in - iy;
        }
    }

    /* check it's all in range */
    if( (ix < 0)                           ||
        (iy < 0)                           ||
        (ix + x_size_out - 1 >= x_size_in) ||
        (iy + y_size_out - 1 >= y_size_in) )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_INDEX);
    }

    c_index_common(p_ss_data_res, array_data, ix, iy, x_size_out, y_size_out);
}

PROC_EXEC_PROTO(c_odf_index)
{
    const PC_SS_DATA array_data = args[0];
    S32 ix, iy, x_size_in, y_size_in, x_size_out, y_size_out;

    exec_func_ignore_parms();

    array_range_sizes(array_data, &x_size_in, &y_size_in);

    /* NB OpenDocument INDEX() has row, column args */
    if(ss_data_get_integer(args[1]) <= 0)
    {   /* zero row number -> whole column */
        iy = 0;
        y_size_out = y_size_in;
    }
    else
    {
        iy = ss_data_get_integer(args[1]) - 1;
        y_size_out = 1;
    }

    if(ss_data_get_integer(args[2]) <= 0)
    {   /* zero column number -> whole row */
        ix = 0;
        x_size_out = x_size_in;
    }
    else
    {
        ix = ss_data_get_integer(args[2]) - 1;
        x_size_out = 1;
    }

    /* check it's all in range */
    if( (ix < 0)                           ||
        (iy < 0)                           ||
        (ix + x_size_out - 1 >= x_size_in) ||
        (iy + y_size_out - 1 >= y_size_in) )
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_BAD_INDEX);
    }

    c_index_common(p_ss_data_res, array_data, ix, iy, x_size_out, y_size_out);
}

/******************************************************************************
*
* INTEGER row / row(slr|range)
*
******************************************************************************/

PROC_EXEC_PROTO(c_row)
{
    PC_EV_SLR p_ev_slr = p_cur_slr;
    S32 row_result;

    exec_func_ignore_parms();

    if(0 != n_args)
    {
        if(DATA_ID_SLR == ss_data_get_data_id(args[0]))
            p_ev_slr = &(args[0]->arg.slr);
        else if(DATA_ID_RANGE == ss_data_get_data_id(args[0]))
            p_ev_slr = &(args[0]->arg.range.s);
        else
            exec_func_status_return(p_ss_data_res, EVAL_ERR_UNEXARRAY);
    }

    row_result = (S32) ev_slr_row(p_ev_slr) + 1;

    ss_data_set_integer(p_ss_data_res, row_result);
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

    ss_data_set_integer(p_ss_data_res, rows_result);
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

    if(NULL != (p_u8 = al_array_alloc_U8(&h_commands, ss_data_get_string_size(args[0]), &array_init_block_u8, &status)))
    {
        EV_DOCNO ev_docno = p_stack_entry ? ev_slr_docno(&p_stack_entry->slr) : ev_slr_docno(p_cur_slr);
        memcpy32(p_u8, ss_data_get_string(args[0]), ss_data_get_string_size(args[0]));
        status_consume(command_array_handle_execute((DOCNO) ev_docno, h_commands)); /* error already reported */
        al_array_dispose(&h_commands);
    }

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* SLR current_cell
*
******************************************************************************/

PROC_EXEC_PROTO(c_current_cell)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ev_current_cell(&p_ss_data_res->arg.slr);
    ss_data_set_data_id(p_ss_data_res, DATA_ID_SLR);
}

/******************************************************************************
*
* deref(slr)
*
******************************************************************************/

PROC_EXEC_PROTO(c_deref)
{
    exec_func_ignore_parms();

    if(DATA_ID_SLR == ss_data_get_data_id(args[0]))
        ev_slr_deref(p_ss_data_res, &args[0]->arg.slr);
    else
        status_assert(ss_data_resource_copy(p_ss_data_res, args[0]));
}

/******************************************************************************
*
* SLR double_click
*
******************************************************************************/

PROC_EXEC_PROTO(c_doubleclick)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ev_double_click(&p_ss_data_res->arg.slr, p_cur_slr);
    ss_data_set_data_id(p_ss_data_res, DATA_ID_SLR);

    if(DOCNO_NONE == ev_slr_docno(&p_ss_data_res->arg.slr))
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ODF_NA);
}

/******************************************************************************
*
* NUMBER even(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_even)
{
    bool negate_result = false;
    F64 f64 = ss_data_get_real(args[0]);
    F64 even_result;

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negate_result = true;
    }

    even_result = 2.0 * ceil(f64 * 0.5);

    /* always round away from zero (Excel) */
    if(negate_result)
        even_result = -even_result;

    ss_data_set_real_try_integer(p_ss_data_res, even_result);
}

/******************************************************************************
*
* LOGICAL false
*
******************************************************************************/

PROC_EXEC_PROTO(c_false)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ss_data_set_logical(p_ss_data_res, false);
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

    status_assert(ss_data_resource_copy(p_ss_data_res, args[0]));
    data_ensure_constant(p_ss_data_res);

    if(ss_data_is_array(p_ss_data_res))
    {
        array_range_sizes(p_ss_data_res, &x_size, &y_size);
        y_half = y_size / 2;
        y_swap = y_size - 1;
        for(iy = 0; iy < y_half; ++iy, y_swap -= 2)
        {
            for(ix = 0; ix < x_size; ++ix)
            {
                SS_DATA temp_data;
                temp_data = *ss_array_element_index_wr(p_ss_data_res, ix, iy + y_swap);
                *ss_array_element_index_wr(p_ss_data_res, ix, iy + y_swap) =
                *ss_array_element_index_wr(p_ss_data_res, ix, iy);
                *ss_array_element_index_wr(p_ss_data_res, ix, iy) = temp_data;
            }
        }
    }
}

/******************************************************************************
*
* LOGICAL isxxx(value)
*
******************************************************************************/

PROC_EXEC_PROTO(c_isblank)
{
    bool isblank_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_BLANK:
        isblank_result = true;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isblank_result);
}

PROC_EXEC_PROTO(c_iserr)
{
    bool iserr_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_ERROR:
        iserr_result = (EVAL_ERR_ODF_NA != args[0]->arg.ss_error.status);
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, iserr_result);
}

PROC_EXEC_PROTO(c_iserror)
{
    bool iserror_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_ERROR:
        iserror_result = true;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, iserror_result);
}

static void
iseven_isodd_calc_real(
    _InoutRef_  P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA arg0,
    _InVal_     bool test_iseven)
{
    bool is_even;
    F64 f64 = ss_data_get_real(arg0);
    if(f64 < 0.0)
        f64 = -f64;
    f64 = floor(f64); /* NB truncate (Excel) */
    is_even = (f64 == (2.0 * floor(f64 * 0.5))); /* exactly divisible by two? */
    ss_data_set_logical(p_ss_data_out, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
}

static void
iseven_isodd_calc_integer(
    _InoutRef_  P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA arg0,
    _InVal_     bool test_iseven)
{
    bool is_even;
    S32 s32 = ss_data_get_integer(arg0);
    if(s32 < 0)
        s32 = -s32;
    is_even = (0 == (s32 & 1)); /* bottom bit clear -> number is even */
    ss_data_set_logical(p_ss_data_out, (test_iseven ? is_even /* test for iseven() */ : !is_even /* test for isodd() */));
}

static void
iseven_isodd_calc(
    _InoutRef_  P_SS_DATA p_ss_data_out,
    _InRef_     PC_SS_DATA arg0,
    _InVal_     bool test_iseven)
{
    switch(ss_data_get_data_id(arg0))
    {
    case DATA_ID_REAL:
        iseven_isodd_calc_real(p_ss_data_out, arg0, test_iseven);
        break;

    case DATA_ID_LOGICAL: /* more useful? */
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
        iseven_isodd_calc_integer(p_ss_data_out, arg0, test_iseven);
        break;

#if 0 /* more pedantic? */
    case DATA_ID_LOGICAL:
        ss_data_set_error(p_ss_data_out, EVAL_ERR_UNEXNUMBER);
        break;
#endif

    default: default_unhandled();
        ss_data_set_logical(p_ss_data_out, false);
        break;
    }
}

PROC_EXEC_PROTO(c_iseven)
{
    exec_func_ignore_parms();

    iseven_isodd_calc(p_ss_data_res, args[0], true /*->test_ISEVEN*/);
}

PROC_EXEC_PROTO(c_islogical)
{
    bool islogical_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_LOGICAL:
        islogical_result = true;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, islogical_result);
}

PROC_EXEC_PROTO(c_isna)
{
    bool isna_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_ERROR:
        isna_result = (EVAL_ERR_ODF_NA == args[0]->arg.ss_error.status);
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isna_result);
}

PROC_EXEC_PROTO(c_isnontext)
{
    bool isnontext_result = true;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_STRING:
        isnontext_result = false;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isnontext_result);
}

PROC_EXEC_PROTO(c_isnumber)
{
    bool isnumber_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_REAL:
    /*case DATA_ID_LOGICAL:*/ /* indeed! that's a LOGICAL for Excel */
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE: /* as it can be converted to an Excel serial number */
        isnumber_result = true;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isnumber_result);
}

PROC_EXEC_PROTO(c_isodd)
{
    exec_func_ignore_parms();

    iseven_isodd_calc(p_ss_data_res, args[0], false /*->test_ISODD*/);
}

PROC_EXEC_PROTO(c_isref)
{
    bool isref_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_SLR:
    case DATA_ID_RANGE:
        isref_result = true;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, isref_result);
}

PROC_EXEC_PROTO(c_istext)
{
    bool istext_result = false;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_STRING:
        istext_result = true;
        break;

    default:
        break;
    }

    ss_data_set_logical(p_ss_data_res, istext_result);
}

/******************************************************************************
*
* ERROR na
*
******************************************************************************/

PROC_EXEC_PROTO(c_na)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ss_data_set_error(p_ss_data_res, EVAL_ERR_ODF_NA);
}

/******************************************************************************
*
* LOGICAL not() function is equivalent OpenDocument / Microsoft Excel function
*
******************************************************************************/

PROC_EXEC_PROTO(c_not)
{
    c_uop_not(args, n_args, p_ss_data_res, p_cur_slr);
}

/******************************************************************************
*
* NUMBER odd(number)
*
******************************************************************************/

PROC_EXEC_PROTO(c_odd)
{
    bool negate_result = false;
    F64 f64 = ss_data_get_real(args[0]);

    exec_func_ignore_parms();

    if(f64 < 0.0)
    {
        f64 = -f64;
        negate_result = true;
    }

    f64 = (2.0 * ceil((f64 + 1.0) * 0.5)) - 1.0;

    /* always round away from zero (Excel) */
    if(negate_result)
        f64 = -f64;

    ss_data_set_real_try_integer(p_ss_data_res, f64);
}

/******************************************************************************
*
* INTEGER page(ref {, n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_page)
{
    S32 page_result;
    const BOOL y_flag = (n_args > 1) ? (ss_data_get_integer(args[1]) != 0) : TRUE;
    STATUS status = ev_page_slr(&args[0]->arg.slr, y_flag);

    exec_func_ignore_parms();

    exec_func_status_return(p_ss_data_res, status);

    page_result = (S32) status + 1;

    ss_data_set_integer(p_ss_data_res, page_result);
}

/******************************************************************************
*
* INTEGER pages({n})
*
******************************************************************************/

PROC_EXEC_PROTO(c_pages)
{
    S32 pages_result;
    const BOOL y_flag = (0 != n_args) ? (ss_data_get_integer(args[0]) != 0) : TRUE;
    STATUS status = ev_page_last(ev_slr_docno(p_cur_slr), y_flag);

    exec_func_ignore_parms();

    exec_func_status_return(p_ss_data_res, status);

    pages_result = (S32) status;

    ss_data_set_integer(p_ss_data_res, pages_result);
}

/******************************************************************************
*
* NUMBER power(a, b) is equivalent OpenDocument / Microsoft Excel function
*
******************************************************************************/

PROC_EXEC_PROTO(c_power)
{
    c_bop_power(args, n_args, p_ss_data_res, p_cur_slr);
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
    EV_HANDLE name_key;
    ARRAY_INDEX name_num;

    exec_func_ignore_parms();

    res = name_make(&name_key, ev_slr_docno(p_cur_slr), &args[0]->arg.string, args[1], NULL);
    exec_func_status_return(p_ss_data_res, res);

    name_num = name_def_from_handle(name_key);
    assert(name_num >= 0);

    status_assert(ss_data_resource_copy(p_ss_data_res, &array_ptr(&name_def_deptable.h_table, EV_NAME, name_num)->def_data));
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
        x_index = (U32) ss_data_get_integer(args[1]); /* NB NOT -1 - SORT() is zero-based (see printed Fireworkz documentation) */

    status_assert(ss_data_resource_copy(p_ss_data_res, args[0]));
    data_ensure_constant(p_ss_data_res);

    if(ss_data_is_error(p_ss_data_res))
        return;

    if(status_fail(status = array_sort(p_ss_data_res, x_index))) /* array_sort() does range checking */
    {
        ss_data_free_resources(p_ss_data_res);
        ss_data_set_error(p_ss_data_res, status);
    }
}

/******************************************************************************
*
* LOGICAL true
*
******************************************************************************/

PROC_EXEC_PROTO(c_true)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ss_data_set_logical(p_ss_data_res, true);
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

    switch(ss_data_get_data_id(args[0]))
    {
    default: default_unhandled();
#if CHECKING
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
#endif
        type = EM_REA;
        break;

    case DATA_ID_DATE:
        type = EM_DAT;
        break;

    case DATA_ID_STRING:
        type = EM_STR;
        break;

    case DATA_ID_BLANK:
        type = EM_BLK;
        break;

    case DATA_ID_ERROR:
        type = EM_ERR;
        break;

    case DATA_ID_SLR:
        type = EM_SLR;
        break;

    case DATA_ID_ARRAY:
    case DATA_ID_RANGE:
    case DATA_ID_FIELD:
        type = EM_ARY;
        break;
    }

    a7str_type = type_name_from_type_flags(type);
    PTR_ASSERT(a7str_type);

    status_assert(ss_string_make_ustr(p_ss_data_res, (PC_USTR) a7str_type)); /* U is superset of A7 */
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

    switch(ss_data_get_data_id(args[0]))
    {
    default: default_unhandled();
#if CHECKING
    case DATA_ID_REAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_DATE: /* Excel stores these as real numbers; we can convert if required */
    case DATA_ID_BLANK: /* yup */
#endif
        odf_type_result = 1;
        break;

    case DATA_ID_STRING:
        odf_type_result = 2;
        break;

    case DATA_ID_LOGICAL:
        odf_type_result = 4;
        break;

    case DATA_ID_ERROR:
        odf_type_result = 16;
        break;

    case DATA_ID_ARRAY:
    case DATA_ID_RANGE:
    case DATA_ID_FIELD:
        odf_type_result = 64;
        break;
    }

    ss_data_set_integer(p_ss_data_res, odf_type_result);
}

/******************************************************************************
*
* REAL return the version number
*
******************************************************************************/

PROC_EXEC_PROTO(c_version)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ss_data_set_real(p_ss_data_res, tstrtod(resource_lookup_tstr(MSG_SKEL_VERSION), NULL));
}

/* end of ev_func.c */
