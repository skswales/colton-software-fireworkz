/* ev_dcom.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RPN to infix decompiler */

/* MRJC February 1991 / May 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

typedef struct DECOMPILER_CONTEXT
{
    EV_DOCNO docno;                     /* document number of decompile data */
    U8 _spare[3];
    ARRAY_HANDLE h_arg_stack;           /* decompiler argument stack */
    SYM_INF sym_inf;                    /* information about current rpn item */
    RPNSTATE rpnstate;                  /* rpn state */
}
DECOMPILER_CONTEXT, * P_DECOMPILER_CONTEXT;

static P_DECOMPILER_CONTEXT p_decompiler_context = NULL;

typedef struct DECOMPILER_STACK_ENTRY
{
    S32 len;
    P_UCHARS uchars;
}
DECOMPILER_STACK_ENTRY, * P_DECOMPILER_STACK_ENTRY;

/*ncr*/
static U32
ev_dec_range_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO this_ev_docno,
    _InRef_     PC_EV_RANGE p_ev_range);

/******************************************************************************
*
* dispose of a decompiler stack entry
*
******************************************************************************/

static void
decompiler_stack_dispose_entry(
    P_DECOMPILER_STACK_ENTRY p_decompiler_stack_entry)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_decompiler_stack_entry->uchars));
    p_decompiler_stack_entry->len = 0;
}

/******************************************************************************
*
* pop an output buffer (array handle) from the decompiler stack
*
******************************************************************************/

_Check_return_
static STATUS
pop_buf(
    P_DECOMPILER_STACK_ENTRY p_decompiler_stack_entry)
{
    ARRAY_INDEX n = array_elements(&p_decompiler_context->h_arg_stack);

    if(0 == n)
        return(STATUS_FAIL);

    *p_decompiler_stack_entry = *array_ptr(&p_decompiler_context->h_arg_stack, DECOMPILER_STACK_ENTRY, n - 1);

    al_array_shrink_by(&p_decompiler_context->h_arg_stack, -1);

    return(STATUS_OK);
}

/******************************************************************************
*
* output a popped string and dispose of it after
*
******************************************************************************/

_Check_return_
static STATUS
popped_str_out_to_buf(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_DECOMPILER_STACK_ENTRY p_decompiler_stack_entry)
{
    STATUS status = quick_ublock_uchars_add(p_quick_ublock, p_decompiler_stack_entry->uchars, p_decompiler_stack_entry->len);
    decompiler_stack_dispose_entry(p_decompiler_stack_entry);
    return(status);
}

/******************************************************************************
*
* pop a string from the decompiler stack and
* append it to the output buffer
*
******************************************************************************/

_Check_return_
static STATUS
pop_str_out_to_buf(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)
{
    STATUS status = STATUS_OK;
    DECOMPILER_STACK_ENTRY decompiler_stack_entry;

    if(status_ok(status = pop_buf(&decompiler_stack_entry)))
        status = popped_str_out_to_buf(p_quick_ublock, &decompiler_stack_entry);

    return(status);
}

/******************************************************************************
*
* push a buffer onto the stack
*
******************************************************************************/

_Check_return_
static STATUS
push_buf(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*pushed,disposed*/)
{
    STATUS status = STATUS_OK;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(20, sizeof32(DECOMPILER_STACK_ENTRY), FALSE);
    P_DECOMPILER_STACK_ENTRY p_decompiler_stack_entry;

    if(NULL == (p_decompiler_stack_entry = al_array_extend_by(&p_decompiler_context->h_arg_stack, DECOMPILER_STACK_ENTRY, 1, &array_init_block, &status)))
        return(status);

    {
    const S32 len = quick_ublock_bytes(p_quick_ublock);
    if(len)
    {
        if(NULL == (p_decompiler_stack_entry->uchars = al_ptr_alloc_bytes(P_UCHARS, len, &status)))
            return(status);
        memcpy32(p_decompiler_stack_entry->uchars, quick_ublock_uchars(p_quick_ublock), len);
    }
    else
        p_decompiler_stack_entry->uchars = NULL;

    p_decompiler_stack_entry->len = len;
    } /*block*/

    quick_ublock_dispose(p_quick_ublock);

    return(status);
}

/******************************************************************************
*
* free all elements from the decompiler stack
*
******************************************************************************/

static void
decompiler_stack_dispose(void)
{
    DECOMPILER_STACK_ENTRY decompiler_stack_entry;

    /* free any items on the stack */
    while(status_ok(pop_buf(&decompiler_stack_entry)))
        decompiler_stack_dispose_entry(&decompiler_stack_entry);

    al_array_dispose(&p_decompiler_context->h_arg_stack);
}

/******************************************************************************
*
* decompile current symbol
*
* --out--
* length of string
*
******************************************************************************/

_Check_return_
static STATUS
decode_data_slr(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     EV_DOCNO ev_docno)
{
    UCHARZ ustr_buf[BUF_EV_LONGNAMLEN];
    U32 len = ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf), elemof32(ustr_buf), ev_docno, p_ev_slr);
    return(quick_ublock_uchars_add(p_quick_ublock, uchars_bptr(ustr_buf), len));
}

_Check_return_
static STATUS
decode_data_range(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_EV_RANGE p_ev_range,
    _InVal_     EV_DOCNO ev_docno)
{
    UCHARZ ustr_buf[BUF_EV_LONGNAMLEN];
    U32 len = ev_dec_range_ustr_buf(ustr_bptr(ustr_buf), elemof32(ustr_buf), ev_docno, p_ev_range);
    return(quick_ublock_uchars_add(p_quick_ublock, uchars_bptr(ustr_buf), len));
}

_Check_return_
static STATUS
decode_data_name(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     EV_DOCNO ev_docno)
{
    ARRAY_INDEX name_num = name_def_find(p_ss_data->arg.h_name);
    STATUS status = STATUS_OK;

    if(name_num >= 0)
    {
        const PC_EV_NAME p_ev_name = array_ptrc(&name_def.h_table, EV_NAME, name_num);

        if(ev_slr_docno(&p_ev_name->owner) != ev_docno)
        {
            UCHARZ buffer[BUF_EV_LONGNAMLEN];
            const U32 len = ev_write_docname_ustr_buf(ustr_bptr(buffer), EV_LONGNAMLEN, ev_slr_docno(&p_ev_name->owner), ev_docno);
            status = quick_ublock_uchars_add(p_quick_ublock, ustr_bptr(buffer), len);
        }

        if(status_ok(status))
            status = quick_ublock_ustr_add(p_quick_ublock, ustr_bptrc(p_ev_name->ustr_name_id));
    }

    return(status);
}

_Check_return_
static STATUS
decode_data(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_SS_DATA p_ss_data,
    _InVal_     EV_DOCNO ev_docno)
{

    switch(ss_data_get_data_id(p_ss_data))
    {
    default:
#if CHECKING
        default_unhandled();
        /*FALLTHRU*/
    case DATA_ID_REAL:
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD8:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
    case DATA_ID_STRING:
    case DATA_ID_ARRAY:
    case DATA_ID_DATE:
    case DATA_ID_BLANK:
    case DATA_ID_ERROR:
#endif
        return(ss_decode_constant(p_quick_ublock, p_ss_data));

    case DATA_ID_SLR:
        return(decode_data_slr(p_quick_ublock, &p_ss_data->arg.slr, ev_docno));

    case DATA_ID_RANGE:
        return(decode_data_range(p_quick_ublock, &p_ss_data->arg.range, ev_docno));

    case DATA_ID_NAME:
        return(decode_data_name(p_quick_ublock, p_ss_data, ev_docno));

    /* the data in fields is implied - they have no value */
    case DATA_ID_FIELD:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* insert formatting space and returns into output buffer
*
******************************************************************************/

/*_Check_return_*/
static STATUS
dec_format_space(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)
{
    if(p_decompiler_context->sym_inf.sym_equals)
    {
        p_decompiler_context->sym_inf.sym_equals = 0;
        if(!g_ss_decompiler_options.initial_formula_equals)
            status_return(quick_ublock_a7char_add(p_quick_ublock, CH_EQUALS_SIGN));
    }

    while(p_decompiler_context->sym_inf.sym_cr)
    {
        --p_decompiler_context->sym_inf.sym_cr;
        if(g_ss_decompiler_options.cr)
            status_return(quick_ublock_a7char_add(p_quick_ublock, CR));
        if(g_ss_decompiler_options.lf)
            status_return(quick_ublock_a7char_add(p_quick_ublock, LF));
    }

    while(p_decompiler_context->sym_inf.sym_space)
    {
        --p_decompiler_context->sym_inf.sym_space;
        status_return(quick_ublock_a7char_add(p_quick_ublock, CH_SPACE));
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
quick_ublock_func_name_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR ustr)
{
    if(g_ss_decompiler_options.upper_case_function)
    {
        const U32 len = ustrlen32(ustr);
        U32 offset = 0;

        while(offset < len)
        {
            U32 bytes_of_char;
            UCS4 ucs4 = ustr_char_decode_off(ustr, offset, /*ref*/bytes_of_char);

            /*if(t5_ucs4_is_lowercase(ucs4))*/ /* almost certainly the case */
                ucs4 = t5_ucs4_uppercase(ucs4);

            status_return(quick_ublock_ucs4_add(p_quick_ublock, ucs4));

            offset += bytes_of_char;
        }

        return(STATUS_OK);
    }
    else
    {
        return(quick_ublock_ustr_add(p_quick_ublock, ustr));
    }
}

/******************************************************************************
*
* take current rpn token details and decompile, combining with arguments on
* the stack; op_buf contains new argument;
*
******************************************************************************/

_Check_return_
static STATUS
dec_rpn_token(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InoutRef_  P_BOOL p_arg_valid)
{
    STATUS status = STATUS_OK;
    PC_RPNDEF p_rpndef;

    assert(p_decompiler_context->rpnstate.num < ELEMOF_RPN_TABLE);
    p_rpndef = &rpn_table[p_decompiler_context->rpnstate.num];

    switch(p_rpndef->rpn_type)
    {
    default: default_unhandled();
        break;

    case RPN_DAT:
        {
        SS_DATA ss_data;

        /* read rpn argument */
        read_cur_sym(&p_decompiler_context->rpnstate, &ss_data);
        dec_format_space(p_quick_ublock);
        if(status_ok(status = decode_data(p_quick_ublock, &ss_data, p_decompiler_context->docno)))
            *p_arg_valid = 1;
        break;
        }

    case RPN_FRM:
        {
        switch(p_decompiler_context->sym_inf.sym_idno)
        {
        case RPN_FRM_BRACKETS: /* brackets must be added to top argument on stack */
            {
            dec_format_space(p_quick_ublock);
            if(status_ok(status = quick_ublock_a7char_add(p_quick_ublock, CH_LEFT_PARENTHESIS)))
            if(status_ok(status = pop_str_out_to_buf(p_quick_ublock)))
            if(status_ok(status = quick_ublock_a7char_add(p_quick_ublock, CH_RIGHT_PARENTHESIS)))
                *p_arg_valid = 1;
            break;
            }

        case RPN_FRM_SPACE:
            p_decompiler_context->sym_inf.sym_space = *(p_decompiler_context->rpnstate.pos + sizeof32(EV_IDNO));
            break;

        case RPN_FRM_RETURN:
            p_decompiler_context->sym_inf.sym_cr = *(p_decompiler_context->rpnstate.pos + sizeof32(EV_IDNO));
            break;

        case RPN_FRM_EQUALS:
            p_decompiler_context->sym_inf.sym_equals = 1;
            break;

        case RPN_FRM_END:
            break;

        case RPN_FRM_COND:
            {
            U32 size;
            dec_format_space(p_quick_ublock);
            size = PtrDiffBytesU32(p_decompiler_context->rpnstate.pos, p_decompiler_context->rpnstate.p_rpn_start)
                    + sizeof32(EV_IDNO)
                    + sizeof32(S16);
            status = ev_decompile(p_quick_ublock, p_decompiler_context->rpnstate.p_ev_cell, size, p_decompiler_context->docno);
            if(status_ok(status))
                *p_arg_valid = 1;
            break;
            }

        case RPN_FRM_SKIPTRUE:
        case RPN_FRM_SKIPFALSE:
            break;
        }

        break;
        }

    case RPN_LCL:
        {
        PC_USTR ustr_local_id;

        dec_format_space(p_quick_ublock);
        status_assert(quick_ublock_a7char_add(p_quick_ublock, CH_COMMERCIAL_AT));
        ustr_local_id = ustr_AddBytes(p_decompiler_context->rpnstate.pos, sizeof32(EV_IDNO));
        if(status_ok(status = quick_ublock_ustr_add(p_quick_ublock, ustr_local_id)))
            *p_arg_valid = 1;
        break;
        }

    case RPN_UOP:
        {
        PC_USTR ustr_fname;

        dec_format_space(p_quick_ublock);
        ustr_fname = func_name(p_decompiler_context->sym_inf.sym_idno);
        PTR_ASSERT(ustr_fname);
        status_assert(quick_ublock_func_name_add(p_quick_ublock, ustr_fname));
        if(status_ok(status = pop_str_out_to_buf(p_quick_ublock)))
            *p_arg_valid = 1;
        break;
        }

    case RPN_REL:
    case RPN_BOP:
        {
        DECOMPILER_STACK_ENTRY decompiler_stack_entry;

        if(status_ok(status = pop_buf(&decompiler_stack_entry)) && status_ok(status = pop_str_out_to_buf(p_quick_ublock)))
        {
            PC_USTR ustr_fname;

            dec_format_space(p_quick_ublock);
            ustr_fname = func_name(p_decompiler_context->sym_inf.sym_idno);
            PTR_ASSERT(ustr_fname);
            status_assert(quick_ublock_func_name_add(p_quick_ublock, ustr_fname));
            if(status_ok(status = popped_str_out_to_buf(p_quick_ublock, &decompiler_stack_entry)))
                *p_arg_valid = 1;
        }

        decompiler_stack_dispose_entry(&decompiler_stack_entry);
        break;
        }

    case RPN_FN0:
#if 1
        /* common code looks good to me for zero args function */
#else
        {
        PC_USTR ustr_fname;

        dec_format_space(p_quick_ublock);
        ustr_fname = func_name(p_decompiler_context->sym_inf.did_num);
        assert(ustr_fname);
        status = quick_ublock_func_name_add(p_quick_ublock, ustr_fname);
        if(g_ss_decompiler_options.zero_args_function_parentheses)
        {
            if(status_ok(status))
                status = quick_ublock_a7char_add(p_quick_ublock, CH_LEFT_PARENTHESIS);
            if(status_ok(status))
                status = quick_ublock_a7char_add(p_quick_ublock, CH_RIGHT_PARENTHESIS);
        }
        if(status_ok(status))
            *p_arg_valid = 1;
        break;
        }
#endif
    case RPN_FNF:
    case RPN_FNV:
    case RPN_FNM:
        {
        S32 narg, i;
        BOOL custom_call = (p_decompiler_context->sym_inf.sym_idno == RPN_FNM_CUSTOMCALL);

        dec_format_space(p_quick_ublock);

        /* work out number of arguments */
        if((narg = p_rpndef->n_args) < 0)
            narg = (S32) *(p_decompiler_context->rpnstate.pos + sizeof32(EV_IDNO));

#if TRACE_ALLOWED
        if(p_rpndef->rpn_type == RPN_FNF)
        {
            assert(p_rpndef->n_args >= 0);
        }
        else if(p_rpndef->rpn_type == RPN_FNV)
        {
            assert(p_rpndef->n_args < 0);
            assert(narg >= - (S32) p_rpndef->n_args - 1);
        }
#endif

        /* copy in custom/function name */
        if(custom_call)
        {
            EV_HANDLE h_custom;
            ARRAY_INDEX custom_num;
            P_EV_CUSTOM p_ev_custom;

            /* read custom id */
            h_custom = *(p_ev_custom_from_ev_cell(p_decompiler_context->rpnstate.p_ev_cell,
                                                  (S32) *(p_decompiler_context->rpnstate.pos + sizeof32(EV_IDNO) + sizeof32(U8))));
            custom_num = custom_def_find(h_custom);
            assert(custom_num >= 0);
            p_ev_custom = array_ptr(&custom_def.h_table, EV_CUSTOM, custom_num);

            if(ev_slr_docno(&p_ev_custom->owner) != p_decompiler_context->docno)
            {
                UCHARZ ustr_buf[BUF_EV_LONGNAMLEN];
                U32 len = ev_write_docname_ustr_buf(ustr_bptr(ustr_buf), EV_LONGNAMLEN, ev_slr_docno(&p_ev_custom->owner), p_decompiler_context->docno);
                status = quick_ublock_uchars_add(p_quick_ublock, uchars_bptr(ustr_buf), len);
            }

            if(status_ok(status))
                status = quick_ublock_func_name_add(p_quick_ublock, ustr_bptr(p_ev_custom->ustr_custom_id));
        }
        else
        {
            PC_USTR ustr_fname = func_name(p_decompiler_context->sym_inf.sym_idno);
            PTR_ASSERT(ustr_fname);
            status = quick_ublock_func_name_add(p_quick_ublock, ustr_fname);
        }

        if((narg || custom_call || (g_ss_decompiler_options.zero_args_function_parentheses && (p_rpndef->fun_parms.ex_type == EXEC_EXEC))) && status_ok(status))
            status = quick_ublock_a7char_add(p_quick_ublock, CH_LEFT_PARENTHESIS);

        if(narg && status_ok(status))
        {
            ARRAY_INDEX startsp;
            P_DECOMPILER_STACK_ENTRY p_decompiler_stack_entry;

            startsp = array_elements(&p_decompiler_context->h_arg_stack) - (ARRAY_INDEX) narg;
            p_decompiler_stack_entry = array_ptr(&p_decompiler_context->h_arg_stack, DECOMPILER_STACK_ENTRY, startsp);

            /* loop to add arguments */
            i = narg;
            while(i-- && status_ok(status))
            {
                status_break(status = popped_str_out_to_buf(p_quick_ublock, p_decompiler_stack_entry));

                if(i)
                    status = quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context.function_arg_sep);

                p_decompiler_stack_entry += 1;
            }
        }

        if((narg || custom_call || (g_ss_decompiler_options.zero_args_function_parentheses && (p_rpndef->fun_parms.ex_type == EXEC_EXEC))) && status_ok(status))
            status = quick_ublock_a7char_add(p_quick_ublock, CH_RIGHT_PARENTHESIS);

        if(narg)
            al_array_shrink_by(&p_decompiler_context->h_arg_stack, (ARRAY_INDEX) -narg);

        if(status_ok(status))
            *p_arg_valid = 1;
        break;
        }

    case RPN_FNA:
        {
        S32 ix, iy;
        S32 x_size, y_size;
        S32 n_elements;

        dec_format_space(p_quick_ublock);

        read_from_rpn(&x_size, p_decompiler_context->rpnstate.pos + sizeof32(EV_IDNO),                 sizeof32(S32));
        read_from_rpn(&y_size, p_decompiler_context->rpnstate.pos + sizeof32(EV_IDNO) + sizeof32(S32), sizeof32(S32));

        n_elements = x_size * y_size;

        status = quick_ublock_a7char_add(p_quick_ublock, CH_LEFT_CURLY_BRACKET);

        if(status_ok(status))
        {
            ARRAY_INDEX startsp;
            P_DECOMPILER_STACK_ENTRY p_decompiler_stack_entry;

            startsp = array_elements(&p_decompiler_context->h_arg_stack) - (ARRAY_INDEX) n_elements;

            p_decompiler_stack_entry = array_ptr(&p_decompiler_context->h_arg_stack, DECOMPILER_STACK_ENTRY, startsp);

            for(iy = 0; iy < y_size && status_ok(status); ++iy)
            {
                if(iy)
                    status = quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context.array_row_sep);

                for(ix = 0; ix < x_size && status_ok(status); ++ix)
                {
                    if(ix)
                        status_break(status = quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context.array_col_sep));

                    status = popped_str_out_to_buf(p_quick_ublock, p_decompiler_stack_entry);

                    p_decompiler_stack_entry += 1;
                }
            }
        }

        if(status_ok(status))
            status = quick_ublock_a7char_add(p_quick_ublock, CH_RIGHT_CURLY_BRACKET);

        al_array_shrink_by(&p_decompiler_context->h_arg_stack, (ARRAY_INDEX) -n_elements);

        if(status_ok(status))
            *p_arg_valid = 1;

        break;
        }
    }

    return(status);
}

/******************************************************************************
*
* convert range to string
*
* --out--
* length of string range
*
******************************************************************************/

/*ncr*/
extern U32
ev_dec_range_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO this_ev_docno,
    _InRef_     PC_EV_RANGE p_ev_range)
{
    EV_SLR slr;
    U32 len;

    len = ev_dec_slr_ustr_buf(ustr_buf, elemof_buffer, this_ev_docno, &p_ev_range->s);

    if(g_ss_decompiler_options.range_colon_separator)
        len += xsnprintf(PtrAddBytes(P_U8Z, ustr_buf, len), elemof_buffer - len, "%c", CH_COLON);

    slr = p_ev_range->e;
    slr.docno = EV_DOCNO_PACK(this_ev_docno);
    slr.col -= 1;
    slr.row -= 1;
    len += ev_dec_slr_ustr_buf(PtrAddBytes(P_USTR, ustr_buf, len), elemof_buffer - len, this_ev_docno, &slr);

    return(len);
}

/******************************************************************************
*
* convert SLR to string
*
* --out--
* length of string slr
*
******************************************************************************/

/*ncr*/
extern U32
ev_dec_slr_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO this_ev_docno,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    P_U8 op_pos = (P_U8) ustr_buf;

    if((ev_slr_docno(p_ev_slr) != this_ev_docno) || p_ev_slr->ext_ref)
        op_pos += ev_write_docname_ustr_buf((P_USTR) op_pos, elemof_buffer, ev_slr_docno(p_ev_slr), this_ev_docno);

    if(p_ev_slr->bad_ref)
        *op_pos++ = CH_PERCENT_SIGN;

    if(p_ev_slr->abs_col)
        *op_pos++ = CH_DOLLAR_SIGN;

    op_pos += xtos_ustr_buf((P_USTR) op_pos, elemof_buffer - PtrDiffBytesU32(op_pos, ustr_buf), ev_slr_col(p_ev_slr), g_ss_decompiler_options.upper_case_slr);

    if(p_ev_slr->abs_row)
        *op_pos++ = CH_DOLLAR_SIGN;

    op_pos += xsnprintf(op_pos, elemof_buffer - PtrDiffBytesU32(op_pos, ustr_buf), S32_FMT, (S32) ev_slr_row(p_ev_slr) + 1);

    return(PtrDiffBytesU32(op_pos, ustr_buf));
}

/******************************************************************************
*
* decode a data item to text
*
******************************************************************************/

_Check_return_
extern STATUS
ss_data_decode(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_SS_DATA p_ss_data,
    _InVal_     EV_DOCNO ev_docno)
{
    STATUS status;

    status = decode_data(p_quick_ublock, p_ss_data, ev_docno);

    return(status);
}

/******************************************************************************
*
* given a pointer to an expression cell, return a textual form for it
*
******************************************************************************/

_Check_return_
extern STATUS
ev_cell_decode(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_EV_CELL p_ev_cell,
    _InVal_     EV_DOCNO ev_docno)
{
    STATUS status = STATUS_OK;

    if(p_ev_cell->ev_parms.data_only)
    {
        SS_DATA ss_data;
        ss_data_from_ev_cell(&ss_data, p_ev_cell);
        status = ss_data_decode(p_quick_ublock, &ss_data, ev_docno);
    }
    else
    {
        assert(0 == quick_ublock_bytes(p_quick_ublock)); /* NB ev_decompile won't append as you may think */

        status = ev_decompile(p_quick_ublock, p_ev_cell, 0, ev_docno);

        if(status_ok(status) && g_ss_decompiler_options.initial_formula_equals)
        {   /* nasty - insert a leading equals sign */
            if(status_ok(status = quick_ublock_a7char_add(p_quick_ublock, CH_EQUALS_SIGN)))
            {
                P_UCHARS uchars = quick_ublock_uchars_wr(p_quick_ublock);
                const U32 n_bytes = quick_ublock_bytes(p_quick_ublock);
                memmove32(uchars_AddBytes_wr(uchars, 1), uchars, n_bytes - 1);
                PtrPutByte(uchars, CH_EQUALS_SIGN);
            }
        }
    }

    return(status);
}

/* decode cell contents as it would appear in the formula line i.e. with alternate/foreign UI if wanted */

_Check_return_
extern STATUS
ev_cell_decode_ui(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_EV_CELL p_ev_cell,
    _InVal_     EV_DOCNO ev_docno)
{
    STATUS status;
    SS_DECOMPILER_OPTIONS ss_decompiler_options = g_ss_decompiler_options;
    SS_RECOG_CONTEXT ss_recog_context;

    ss_recog_context_push(&ss_recog_context);

    if(global_preferences.ss_alternate_formula_style)
    {   /* Alternate formula style (Excel-ise) the decompiler output */
        g_ss_decompiler_options.lf = 0; /* prune these out for now */
        g_ss_decompiler_options.cr = 0;
        g_ss_decompiler_options.initial_formula_equals = 1;
        g_ss_decompiler_options.range_colon_separator = 1;
        g_ss_decompiler_options.upper_case_function = 1;
        g_ss_decompiler_options.upper_case_slr = 1;
        g_ss_decompiler_options.zero_args_function_parentheses = 1;
    }

    status = ev_cell_decode(p_quick_ublock, p_ev_cell, ev_docno);

    ss_recog_context_pull(&ss_recog_context);

    g_ss_decompiler_options = ss_decompiler_options;

    return(status);
}

/******************************************************************************
*
* decompile from RPN to plain text
*
******************************************************************************/

_Check_return_
extern STATUS
ev_decompile(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*output,NOT appended*/,
    _InRef_     PC_EV_CELL p_ev_cell,
    _InVal_     S32 offset,
    _InVal_     EV_DOCNO ev_docno)
{
    DECOMPILER_CONTEXT decompiler_context;
    P_DECOMPILER_CONTEXT p_decompiler_context_old;
    STATUS status = STATUS_OK;

    /* set up decompiler context */
    p_decompiler_context_old = p_decompiler_context;
    p_decompiler_context = &decompiler_context;

    /* save initial data */
    p_decompiler_context->docno = ev_docno;

    rpnstate_init(&p_decompiler_context->rpnstate, p_ev_cell, offset);

    /* clear stack info */
    p_decompiler_context->h_arg_stack = 0;

    /* set up symbol info */
    p_decompiler_context->sym_inf.sym_space = 0;
    p_decompiler_context->sym_inf.sym_cr = 0;
    p_decompiler_context->sym_inf.sym_equals = 0;
    p_decompiler_context->sym_inf.sym_idno = p_decompiler_context->rpnstate.num;

    while(status_ok(status))
    {
        BOOL arg_valid = FALSE;

        if(status_fail(status = dec_rpn_token(p_quick_ublock, &arg_valid)))
            break;

        if((p_decompiler_context->sym_inf.sym_idno = rpn_skip(&p_decompiler_context->rpnstate)) == RPN_FRM_END)
            break;

        if(arg_valid)
            status = push_buf(p_quick_ublock);
    }

    /* stack must be all used up */
    if(array_elements(&p_decompiler_context->h_arg_stack) && status_ok(status))
        status = create_error(EVAL_ERR_BADRPN);

    /* release decompiler stack and contents */
    decompiler_stack_dispose();

    /* restore old context */
    p_decompiler_context = p_decompiler_context_old;

    return(status);
}

/* end of ev_dcom.c */
