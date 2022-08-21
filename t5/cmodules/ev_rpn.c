/* ev_rpn.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines that traverse expression RPN strings */

/* MRJC February 1991 / May 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

OBJECT_PROTO(extern, object_ss); /* SKS 29apr14 don't call via generic object dispatch */

/******************************************************************************
*
* return length of expression
*
******************************************************************************/

_Check_return_
static S32
len_rpn(
    P_RPNSTATE p_rpnstate)
{
    S32 len;
    P_U8 rpn_start = p_rpnstate->pos;

    rpn_check(p_rpnstate);
    while(p_rpnstate->num != RPN_FRM_END)
        rpn_skip(p_rpnstate);

    len = PtrDiffBytesS32(p_rpnstate->pos, rpn_start) + (S32) sizeof32(EV_IDNO) /*term*/;

    return(len);
}

/******************************************************************************
*
* read a custom id from the rpn string
* and look it up in the custom table
*
* --out--
* < 0 error
* >=0 definition found OK
*
******************************************************************************/

static S32
read_check_custom_def(
    _InVal_     EV_HANDLE h_custom,
    P_ARRAY_INDEX custom_num,
    P_P_EV_CUSTOM p_p_ev_custom)
{
    if((*custom_num = custom_def_from_handle(h_custom)) >= 0)
    {
        *p_p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, *custom_num);

        if(!(*p_p_ev_custom)->flags.undefined)
        {
            status_return(ev_doc_error(ev_slr_docno(&(*p_p_ev_custom)->owner)));
            return(0);
        }
    }

    return(create_error(EVAL_ERR_CUSTOMUNDEF));
}

/******************************************************************************
*
* read S16 from rpn
*
******************************************************************************/

static S16
readS16(
    P_U8 ip_pos)
{
    S16 s16;

    read_from_rpn(&s16, ip_pos, sizeof32(S16));
    return(s16);
}

/******************************************************************************
*
* loop over the arguments on the stack, comparing
* them against the given type list
* errors are generated for mismatched arguments
* after argument type checking, array expansion is
* performed ready for auto-call of functions and
* macros with arrays
*
* arg_normalise keeps track of the maximum sizes of
* array arguments for those parameters which do not
* expect arrays to be passed
*
* then we loop over arguments and results to expand
* arrays and arguments read
*
* --out--
* < 0 error in arguments or array expansion;
*     error state is passed in result structure
* ==0 arguments processed OK, no arrays
* > 0 arguments processed OK, array sizes set
*
******************************************************************************/

_Check_return_
extern S32
args_check(
    _InVal_     S32 arg_count,
    P_P_SS_DATA args,
    _InVal_     S32 type_count,
    _In_reads_opt_(type_count) const PC_EV_TYPE p_arg_types,
    _OutRef_    P_SS_DATA p_ss_data_res,
    _OutRef_    P_S32 p_max_x,
    _OutRef_    P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase)
{
    BOOL args_ok = TRUE;

    *p_max_x = 0;
    *p_max_y = 0;

    ss_data_set_blank(p_ss_data_res);

    if(0 != arg_count)
    {
        S32 ix, typec;
        PC_EV_TYPE p_ev_type;

        typec = type_count;
        p_ev_type = p_arg_types;

        for(ix = 0; ix < arg_count; ++ix)
        {
            /* check/convert each argument */
            if(status_fail(
                arg_normalise(args[ix],
                              (EV_TYPE) (p_ev_type ? *p_ev_type
                                                   : EM_REA | EM_STR | EM_DAT),
                              p_max_x,
                              p_max_y,
                              p_stack_dbase)))
            {
                *p_ss_data_res = *args[ix]; /* copy error out */
                args_ok = FALSE;
                break;
            }

            if(typec > 1)
            {
                --typec;
                ++p_ev_type;
            }
        }

        /* if any arrays were passed, we loop over the
         * arguments and do any necessary expansions
        */
        if(args_ok && (*p_max_x || *p_max_y))
        {
            typec = type_count;
            p_ev_type = p_arg_types;

            /* loop over arguments */
            for(ix = 0; ix < arg_count; ++ix)
            {
                /* check for expected array too - this is a fault */
                if(p_ev_type && (*p_ev_type & EM_ARY))
                {
                    switch(ss_data_get_data_id(args[ix]))
                    {
                    case DATA_ID_RANGE:
                    case DATA_ID_ARRAY:
                    case DATA_ID_FIELD:
                        ss_data_set_error(p_ss_data_res, EVAL_ERR_NESTEDARRAY);
                        args_ok = FALSE;
                        break;
                    }
                }
                else if(status_fail(array_expand(args[ix], *p_max_x, *p_max_y)))
                {
                    *p_ss_data_res = *args[ix]; /* copy error out */
                    args_ok = FALSE;
                }

                if(!args_ok)
                    break;

                if(typec > 1)
                {
                    --typec;
                    ++p_ev_type;
                }
            }

            /* expand result */
            if(args_ok)
            {
                ss_data_set_blank(p_ss_data_res);

                if(array_expand(p_ss_data_res, *p_max_x, *p_max_y) < 0)
                    args_ok = 0;
            }
        }
    }

    return(args_ok ? ((*p_max_x || *p_max_y) ? 1 : 0) : p_ss_data_res->arg.ss_error.status);
}

/******************************************************************************
*
* return length of expression
*
******************************************************************************/

_Check_return_
extern S32
ev_len(
    _InRef_     PC_EV_CELL p_ev_cell)
{
    if(p_ev_cell->ev_parms.data_only)
        return(OVH_EV_CELL);
    else
    {
        RPNSTATE rpnstate;
        rpnstate_init(&rpnstate, p_ev_cell, 0);
        return(PtrDiffBytesS32(rpnstate.pos, p_ev_cell) + len_rpn(&rpnstate));
    }
}

/******************************************************************************
*
* backtrack up the stack to find the most recent cell we were calculating,
* set the error into the cell and go to calculate its dependents
*
******************************************************************************/

static S32
eval_backtrack_error(
    _InVal_     S32 error,
    _InVal_     U32 stack_at)
{
    stack_set(stack_at);

    ss_data_set_error(&stack_base[stack_at].data.stack_in_calc.result_data, error);

    return(SAME_STATE);
}

/******************************************************************************
*
* perform certain operations optimally without going through
* the general argument checking and processing
*
******************************************************************************/

static S32
eval_optimise(
    P_RPNSTATE p_rpnstate)
{
    S32 did_opt = 0;

    switch(p_rpnstate->num)
    {
    case RPN_BOP_PLUS:
        if(0 != (did_opt = two_nums_add_try(
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-0].data.stack_data_item.data)))
            stack_offset -= 1;
        break;

    case RPN_BOP_MINUS:
        if(0 != (did_opt = two_nums_subtract_try(
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-0].data.stack_data_item.data)))
            stack_offset -= 1;
        break;

    case RPN_BOP_DIVIDE:
        if(0 != (did_opt = two_nums_divide_try(
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-0].data.stack_data_item.data)))
            stack_offset -= 1;
        break;

    case RPN_BOP_TIMES:
        if(0 != (did_opt = two_nums_multiply_try(
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-1].data.stack_data_item.data,
                                    &stack_base[stack_offset-0].data.stack_data_item.data)))
            stack_offset -= 1;
        break;
    }

    return(did_opt);
}

/******************************************************************************
*
* wander along an rpn string and evaluate it
*
******************************************************************************/

/*ncr*/
extern S32 /* error code */
eval_rpn(
    _InVal_     U32 stack_at)
{
    RPNSTATE rpnstate;
    P_EVAL_BLOCK p_eval_block = &stack_base[stack_at].data.stack_in_calc.eval_block;
    P_STACK_DBASE p_stack_dbase = NULL;

    if(p_eval_block->in_dbase)
        p_stack_dbase = &stack_base[p_eval_block->dbase_stack].data.stack_dbase;

    rpnstate_init(&rpnstate, p_eval_block->p_ev_cell, p_eval_block->offset);

    /* we can't access p_stack_in_calc after this point: the stack may move!
     * we must indirect through stack_base each time
     */
    while(rpnstate.num != RPN_FRM_END)
    {
        /* check that we have stack available */
        if(stack_check_n(3) < 0)
            return(eval_backtrack_error(status_nomem(), stack_at));

        /* given that the stack may have moved, reload the pointers to structures within it */
        p_eval_block = &stack_base[stack_at].data.stack_in_calc.eval_block;

        if(NULL != p_stack_dbase)
            p_stack_dbase = &stack_base[p_eval_block->dbase_stack].data.stack_dbase;

        assert(rpnstate.num < ELEMOF_RPN_TABLE);
        switch(rpn_table[rpnstate.num].rpn_type)
        {
        default: default_unhandled();
            break;

        /* needs 1 stack entry */
        case RPN_DAT:
            {
            stack_offset += 1;

            stack_base[stack_offset].slr = p_eval_block->slr;
            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
            stack_base[stack_offset].stack_flags.type = DATA_ITEM;

            read_cur_sym(&rpnstate, &stack_base[stack_offset].data.stack_data_item.data);

            switch(ss_data_get_data_id(&stack_base[stack_offset].data.stack_data_item.data))
            {
            /* check for duff slrs and ranges */
            case DATA_ID_SLR:
                if(stack_base[stack_offset].data.stack_data_item.data.arg.slr.bad_ref)
                    ss_data_set_error(&stack_base[stack_offset].data.stack_data_item.data, EVAL_ERR_BADSLR);
                else if(stack_base[stack_offset].data.stack_data_item.data.arg.slr.ext_ref
                        &&
                        ev_doc_error(ev_slr_docno(&stack_base[stack_offset].data.stack_data_item.data.arg.slr)))
                    ss_data_set_error(&stack_base[stack_offset].data.stack_data_item.data,
                                   ev_doc_error(ev_slr_docno(&stack_base[stack_offset].data.stack_data_item.data.arg.slr)));
                break;

            case DATA_ID_RANGE:
                if(stack_base[stack_offset].data.stack_data_item.data.arg.range.s.bad_ref |
                   stack_base[stack_offset].data.stack_data_item.data.arg.range.e.bad_ref)
                    ss_data_set_error(&stack_base[stack_offset].data.stack_data_item.data, EVAL_ERR_BADSLR);
                else if((stack_base[stack_offset].data.stack_data_item.data.arg.range.s.ext_ref |
                         stack_base[stack_offset].data.stack_data_item.data.arg.range.e.ext_ref)
                        &&
                        ev_doc_error(ev_slr_docno(&stack_base[stack_offset].data.stack_data_item.data.arg.range.s)))
                    ss_data_set_error(&stack_base[stack_offset].data.stack_data_item.data,
                                   ev_doc_error(ev_slr_docno(&stack_base[stack_offset].data.stack_data_item.data.arg.range.s)));
                break;
            }

            break;
            }

        /* needs 1 stack entry */
        case RPN_FRM:
            switch(rpnstate.num)
            {
            /* conditional subexpression */
            case RPN_FRM_COND:
                {
                /* stack offset of conditional expression */
                stack_offset += 1;

                stack_base[stack_offset].slr = p_eval_block->slr;
                stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                stack_base[stack_offset].stack_flags.type = DATA_ITEM;

                ss_data_set_data_id(&stack_base[stack_offset].data.stack_data_item.data, RPN_FRM_COND);
                stack_base[stack_offset].data.stack_data_item.data.arg.cond_pos = (RPN_OFFSET) (rpnstate.pos - rpnstate.p_rpn_start + sizeof32(EV_IDNO) + sizeof32(S16));
                break;
                }

            case RPN_FRM_SKIPTRUE:
            case RPN_FRM_SKIPFALSE:
                {
                S32 res;
                P_SS_DATA argp;

                { /* check boolean value on bottom of stack */
                S32 this_stack_offset = (S32) *(rpnstate.pos + sizeof32(EV_IDNO));
                argp = stack_index_ptr_data(stack_offset, this_stack_offset);
                } /*block*/

                status_break(arg_normalise(argp, EM_REA, NULL, NULL, NULL));

                res = (ss_data_get_real(argp) != 0.0);

                if(rpnstate.num == RPN_FRM_SKIPTRUE)
                {
                    if(0 == res)
                        break;
                }
                else /* RPN_FRM_SKIPFALSE */
                {
                    if(0 != res)
                        break;
                }

                /* skip argument evaluation - push a dummy */
                stack_offset += 1;

                stack_base[stack_offset].slr = p_eval_block->slr;
                stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                stack_base[stack_offset].stack_flags.type = DATA_ITEM;

                ss_data_set_blank(&stack_base[stack_offset].data.stack_data_item.data);

                rpnstate.pos += readS16(rpnstate.pos + sizeof32(EV_IDNO) + 1);
                rpn_check(&rpnstate);
                continue;
                }

            /* most formatting bits are ignored in evaluation */
            case RPN_FRM_BRACKETS:
            case RPN_FRM_RETURN:
            case RPN_FRM_SPACE:
            case RPN_FRM_EQUALS:
            case RPN_FRM_END:
                break;

            default: default_unhandled();
                break;
            }
            break;

        /* macro local arguments
         * needs 1 stack entry
         */
        case RPN_LCL:
            {
            P_STACK_ENTRY p_stack_entry;

            stack_offset += 1;

            stack_base[stack_offset].slr = p_eval_block->slr;
            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
            stack_base[stack_offset].stack_flags.type = DATA_ITEM;

            /* default to local argument undefined */
            ss_data_set_error(&stack_base[stack_offset].data.stack_data_item.data, EVAL_ERR_LOCALUNDEF);

            /* look back up stack for macro call */
            if(NULL != (p_stack_entry = stack_back_search(stack_offset - 1, EXECUTING_MACRO)))
            {
                UCHARZ arg_name[BUF_EV_INTNAMLEN];
                P_U8 ip, op;
                P_EV_CUSTOM p_ev_custom;
                S32 arg_ix, i;
                P_SS_DATA p_ss_data;

                /* load up local name */
                ip = rpnstate.pos + sizeof32(EV_IDNO);
                op = arg_name;
                do
                    *op++ = *ip;
                while(CH_NULL != *ip++);

                arg_ix = -1;

                p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, p_stack_entry->data.stack_executing_custom.custom_num);

                for(i = 0; i < p_ev_custom->args->n; ++i)
                {
                    PC_USTR this_arg_name = ustr_bptr(p_ev_custom->args->ustr_arg_name_id[i]);

                    if(ustr_compare_equals_nocase(this_arg_name, ustr_bptr(arg_name)))
                    {
                        arg_ix = i;
                        break;
                    }
                }

                if(arg_ix >= 0 && arg_ix < p_stack_entry->data.stack_executing_custom.n_args)
                {
                    /* get pointer to data on stack */
                    p_ss_data = stack_index_ptr_data(p_stack_entry->data.stack_executing_custom.stack_base,
                                                 p_stack_entry->data.stack_executing_custom.n_args - arg_ix - 1);

                    /* if it's an expanded array, we must
                     * index the relevant array element
                    */
                    if(p_stack_entry->data.stack_executing_custom.in_array &&
                       data_is_array_range(p_ss_data))
                    {
                        SS_DATA temp_data;

                        (void) array_range_index(&temp_data,
                                                 p_ss_data,
                                                 p_stack_entry->data.stack_executing_custom.x_pos,
                                                 p_stack_entry->data.stack_executing_custom.y_pos,
                                                 p_ev_custom->args->arg_types[arg_ix]);

                        status_assert(ss_data_resource_copy(&stack_base[stack_offset].data.stack_data_item.data, &temp_data));
                        ss_data_free_resources(&temp_data);
                    }
                    else
                        status_assert(ss_data_resource_copy(&stack_base[stack_offset].data.stack_data_item.data, p_ss_data));
                }
            }
            break;
        }

        /* process a macro call
         * needs 3 stack entries
         * stack during macro is like this:
         * [argn]
         * .
         * [arg1]
         * [arg0]
         * [IN_EVAL]
         * [result]
         * [EXECUTING_MACRO]
         */
        case RPN_FNM:
            {
            S32 res, arg_count;
            ARRAY_INDEX custom_num;
            P_EV_CUSTOM p_ev_custom;

            /* read macro call argument count */
            arg_count = (S32) *(rpnstate.pos + sizeof32(EV_IDNO));

            /* read the macro id */
            if((res = read_check_custom_def(*(p_ev_custom_from_ev_cell(rpnstate.p_ev_cell, (S32) *(rpnstate.pos + sizeof32(EV_IDNO) + sizeof32(U8)))),
                                            &custom_num,
                                            &p_ev_custom)) >= 0)
            {
                P_SS_DATA args[EV_MAX_ARGS];
                S32 ix, max_x, max_y;
                SS_DATA custom_result_data;

                /* get pointer to each argument */
                for(ix = 0; ix < arg_count; ++ix)
                    args[ix] = stack_index_ptr_data(stack_offset, arg_count - ix - 1);

                /* check the argument types and look for an array */
                if((res = args_check(arg_count, args,
                                     p_ev_custom->args->n,
                                     p_ev_custom->args->n
                                        ? p_ev_custom->args->arg_types
                                        : NULL,
                                     &custom_result_data,
                                     &max_x, &max_y,
                                     p_stack_dbase)) >= 0)
                {
                    /* save current state */
                    rpn_skip(&rpnstate);

                    stack_offset += 1;

                    stack_base[stack_offset].slr = p_eval_block->slr;
                    stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                    stack_base[stack_offset].stack_flags.type = IN_EVAL;

                    stack_base[stack_at].data.stack_in_calc.eval_block.offset = PtrDiffBytesU32(rpnstate.pos, rpnstate.p_rpn_start);
                    stack_base[stack_offset].data.stack_in_eval.stack_offset = stack_at;

                    /* create macro result area on stack */
                    stack_offset += 1;

                    stack_base[stack_offset].slr = p_eval_block->slr;
                    stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                    stack_base[stack_offset].stack_flags.type = DATA_ITEM;

                    stack_base[stack_offset].data.stack_data_item.data = custom_result_data;

                    /* switch to executing the macro */
                    stack_offset += 1;

                    stack_base[stack_offset].slr = p_eval_block->slr;
                    stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                    stack_base[stack_offset].stack_flags.type = EXECUTING_MACRO;

                    stack_base[stack_offset].data.stack_executing_custom.n_args = arg_count;
                    stack_base[stack_offset].data.stack_executing_custom.stack_base = stack_offset - 3;
                    stack_base[stack_offset].data.stack_executing_custom.in_array = (max_x || max_y);
                    stack_base[stack_offset].data.stack_executing_custom.x_pos =
                    stack_base[stack_offset].data.stack_executing_custom.y_pos = 0;
                    stack_base[stack_offset].data.stack_executing_custom.custom_slot = p_ev_custom->owner;
                    stack_base[stack_offset].data.stack_executing_custom.custom_slot.row += 1;
                    stack_base[stack_offset].data.stack_executing_custom.next_slot =
                    stack_base[stack_offset].data.stack_executing_custom.custom_slot;
                    stack_base[stack_offset].data.stack_executing_custom.custom_num = custom_num;
                    stack_base[stack_offset].data.stack_executing_custom.elseif = 0;
                }
            }

            /* stack error instead of calling macro */
            if(res < 0)
            {
                /* remove macro arguments */
                stack_set(stack_offset - arg_count);

                stack_offset += 1;

                stack_base[stack_offset].slr = p_eval_block->slr;
                stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                stack_base[stack_offset].stack_flags.type = DATA_ITEM;

                ss_data_set_error(&stack_base[stack_offset].data.stack_data_item.data, res);
                break;
            }

            return(NEW_STATE);
        }

        /* needs 3 stack entries
         * format for stack_executing_custom.ntix is:
         * arg1, arg2, ... argn opr [# arg] [macro id]
         */
        case RPN_UOP:
        case RPN_BOP:
        case RPN_REL:
        case RPN_FN0:
        case RPN_FNF:
        case RPN_FNV:
            if(!eval_optimise(&rpnstate))
            {
                PC_RPNDEF p_rpndef;
                EV_SPLIT_EXEC_DATA ev_split_exec_data;
                S32 res, type_count, arg_count, ix, max_x, max_y;
                PC_EV_TYPE p_arg_types;
                SS_DATA func_result_data;

                assert(rpnstate.num < ELEMOF_RPN_TABLE);
                p_rpndef = &rpn_table[rpnstate.num];

                /* establish argument count */
                if((arg_count = p_rpndef->n_args) < 0)
                    arg_count = (S32) *(rpnstate.pos + sizeof32(EV_IDNO));

                if(NULL != (p_arg_types = p_rpndef->arg_types))
                    type_count = (S32) *p_arg_types++;
                else
                    type_count = 0;

                /* get pointer to each argument */
                for(ix = 0; ix < arg_count; ++ix)
                    ev_split_exec_data.args[ix] = stack_index_ptr_data(stack_offset, arg_count - ix - 1);

                /* check the argument types and look for an array */
                if((res = args_check(arg_count,
                                     ev_split_exec_data.args,
                                     type_count,
                                     p_arg_types,
                                     &func_result_data,
                                     &max_x,
                                     &max_y,
                                     p_stack_dbase)) > 0)
                {
                    /* save calculating cell state */
                    if(OBJECT_ID_NONE != p_rpndef->object_id)
                    {
                        /* save resume position */
                        rpn_skip(&rpnstate);

                        stack_offset += 1;

                        stack_base[stack_offset].slr = p_eval_block->slr;
                        stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                        stack_base[stack_offset].stack_flags.type = IN_EVAL;

                        stack_base[stack_at].data.stack_in_calc.eval_block.offset = PtrDiffBytesU32(rpnstate.pos, rpnstate.p_rpn_start);
                        stack_base[stack_offset].data.stack_in_eval.stack_offset = stack_at;

                        /* push result array */
                        stack_offset += 1;

                        stack_base[stack_offset].slr = p_eval_block->slr;
                        stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                        stack_base[stack_offset].stack_flags.type = DATA_ITEM;

                        stack_base[stack_offset].data.stack_data_item.data = func_result_data;

                        /* swap to processing array state */
                        stack_offset += 1;

                        stack_base[stack_offset].slr = p_eval_block->slr;
                        stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                        stack_base[stack_offset].stack_flags.type = PROCESSING_ARRAY;

                        stack_base[stack_offset].data.stack_processing_array.x_pos =
                        stack_base[stack_offset].data.stack_processing_array.y_pos = 0;
                        stack_base[stack_offset].data.stack_processing_array.n_args = arg_count;
                        stack_base[stack_offset].data.stack_processing_array.stack_base = stack_offset - 3;
                        /*stack_base[stack_offset].data.stack_processing_array.p_proc_exec = p_proc_exec;*/
                        stack_base[stack_offset].data.stack_processing_array.object_id = OBJECT_ID_UNPACK(p_rpndef->object_id);
                        stack_base[stack_offset].data.stack_processing_array.object_table_index = p_rpndef->object_table_index;
                        stack_base[stack_offset].data.stack_processing_array.type_count = type_count;
                        stack_base[stack_offset].data.stack_processing_array.arg_types = p_arg_types;
                        res = NEW_STATE;
                    }
                    else
                        /* has no exec routine - can't handle array processing */
                        res = create_error(EVAL_ERR_UNEXARRAY);
                }
                /* no array processing to do */
                else if(!res)
                {
                    switch(p_rpndef->fun_parms.ex_type)
                    {
                    case EXEC_EXEC:
                        {
                        ev_split_exec_data.object_table_index = p_rpndef->object_table_index;
                        ev_split_exec_data.n_args = arg_count; /* optimise? */
                        ev_split_exec_data.p_ss_data_res = &func_result_data;
                        ev_split_exec_data.p_cur_slr = &stack_base[stack_offset].slr;
#if defined(OBJECT_ID_SS_SPLIT) /* SKS 29apr14 direct call now we're all back together */
                        assert(OBJECT_ID_SS == OBJECT_ID_UNPACK(p_rpndef->object_id));
                        status_consume(object_ss(P_DOCU_NONE, T5_MSG_SS_RPN_EXEC, &ev_split_exec_data));
#else
                        assert(OBJECT_ID_NONE != OBJECT_ID_UNPACK(p_rpndef->object_id));
                        if(STATUS_MODULE_NOT_FOUND == object_call_id_load(P_DOCU_NONE, T5_MSG_SS_RPN_EXEC, &ev_split_exec_data, OBJECT_ID_UNPACK(p_rpndef->object_id)))
                            res = STATUS_NOT_AVAILABLE;    /* couldn't load module */
#endif
                        break;
                        }

                    /* needs 3 stack entries */
                    case EXEC_DBASE:
                        {
                        STATUS status;
                        STACK_DBASE stack_dbase;

                        if(NULL == (stack_dbase.p_stat_block = al_ptr_alloc_elem(STAT_BLOCK, 1, &status)))
                            res = status;
                        else
                        {   /* initialise database function data block */
                            stack_dbase.dbase_slot = stack_base[stack_offset].slr;
                            status_assert(ss_data_resource_copy(&stack_dbase.arg0, ev_split_exec_data.args[0]));
                            stack_dbase.cond_pos = (S16) ev_split_exec_data.args[1]->arg.cond_pos;
                            stack_dbase.ix = 0;
                            stack_dbase.iy = 0;
                            array_range_stat_block_init(stack_dbase.p_stat_block,
                                                        NULL,
                                                        NULL,
                                                        UBF_UNPACK(EV_IDNO, p_rpndef->fun_parms.rpn_alias),
                                                        ev_split_exec_data.args,
                                                        arg_count);

                            stack_set(stack_offset - arg_count);

                            /* save current state */
                            rpn_skip(&rpnstate);

                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = IN_EVAL;

                            stack_base[stack_at].data.stack_in_calc.eval_block.offset = PtrDiffBytesU32(rpnstate.pos, rpnstate.p_rpn_start);
                            stack_base[stack_offset].data.stack_in_eval.stack_offset = stack_at;

                            /* switch to dbase state */
                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = DBASE_FUNCTION;

                            stack_base[stack_offset].data.stack_dbase = stack_dbase;

                            res = NEW_STATE;
                        }
                        break;
                        }

                    case EXEC_CONTROL:
                        if(stack_base[stack_offset].stack_flags.inmacro)
                            res = process_control(p_rpndef->fun_parms.no_exec, ev_split_exec_data.args, arg_count, stack_at);
                        else
                        {
                            ss_data_free_resources(&func_result_data);
                            res = create_error(EVAL_ERR_BADCONTROL);
                        }
                        break;

                    case EXEC_LOOKUP:
                        {
                        STATUS status;
                        STACK_LOOKUP stack_lookup;

                        if(NULL == (stack_lookup.p_lookup_block = al_ptr_alloc_elem(LOOKUP_BLOCK, 1, &status)))
                            res = status;
                        else
                        {
                            S32 match;
                            BOOL all_occs = 0;

                            status_assert(ss_data_resource_copy(&stack_lookup.arg1, ev_split_exec_data.args[1]));
                            status_assert(ss_data_resource_copy(&stack_lookup.arg2, ev_split_exec_data.args[2]));

                            switch(p_rpndef->fun_parms.no_exec)
                            {
                            default: default_unhandled();
#if CHECKING
                            case LOOKUP_HLOOKUP:
                            case LOOKUP_VLOOKUP:
#endif
                                match = 1;
                                if(arg_count > 3)
                                    match = ss_data_get_integer(ev_split_exec_data.args[3]);
                                break;

                            case LOOKUP_LOOKUP:
                                match = 0;
                                if(arg_count > 3)
                                    all_occs = (BOOL) ss_data_get_integer(ev_split_exec_data.args[3]);
                                break;

                            case LOOKUP_MATCH:
                                match = ss_data_get_integer(ev_split_exec_data.args[2]);
                                break;
                            }

                            lookup_block_init(stack_lookup.p_lookup_block, ev_split_exec_data.args[0], p_rpndef->fun_parms.no_exec, match, all_occs);

                            /* blow away lookup arguments */
                            stack_set(stack_offset - arg_count);

                            /* save current state */
                            rpn_skip(&rpnstate);

                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = IN_EVAL;

                            stack_base[stack_at].data.stack_in_calc.eval_block.offset = PtrDiffBytesU32(rpnstate.pos, rpnstate.p_rpn_start);
                            stack_base[stack_offset].data.stack_in_eval.stack_offset = stack_at;

                            /* switch to lookup state */
                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = LOOKUP_HAPPENING;

                            stack_base[stack_offset].data.stack_lookup = stack_lookup;

                            res = NEW_STATE;
                        }

                        break;
                        }

                    case EXEC_ALERT:
                        {
                        STACK_ALERT_INPUT stack_alert_input;

                        stack_alert_input.alert_input = rpnstate.num;
                        switch(rpnstate.num)
                        {
                        case RPN_FNV_ALERT:
                            res = ev_alert(ev_slr_docno(&stack_base[stack_offset].slr),
                                           &ev_split_exec_data.args[0]->arg.string,
                                           &ev_split_exec_data.args[1]->arg.string,
                                           (arg_count > 2) ? &ev_split_exec_data.args[2]->arg.string : NULL);
                            break;

                        case RPN_FNV_INPUT:
                            /* save away name to receive result */
                            stack_alert_input.name_id_len = ss_data_get_string_size(ev_split_exec_data.args[1]);
                            memcpy32(stack_alert_input.ustr_name_id, ss_data_get_string(ev_split_exec_data.args[1]), stack_alert_input.name_id_len);
                            res = ev_input(ev_slr_docno(&stack_base[stack_offset].slr),
                                           &ev_split_exec_data.args[0]->arg.string,
                                           &ev_split_exec_data.args[2]->arg.string,
                                           (arg_count > 3) ? &ev_split_exec_data.args[3]->arg.string : NULL);
                            break;
                        }

                        if(res >= 0)
                        {
                            /* blow away arguments */
                            stack_set(stack_offset - arg_count);

                            /* save current state */
                            rpn_skip(&rpnstate);

                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = IN_EVAL;

                            stack_base[stack_at].data.stack_in_calc.eval_block.offset = PtrDiffBytesU32(rpnstate.pos, rpnstate.p_rpn_start);
                            stack_base[stack_offset].data.stack_in_eval.stack_offset = stack_at;

                            /* switch to lookup state */
                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = ALERT_INPUT;

                            stack_base[stack_offset].data.stack_alert_input = stack_alert_input;

                            res = NEW_STATE;
                        }

                        break;
                        }

                    /* needs two stack entries */
                    case EXEC_SETVALUE:
                        {
                        STACK_SETVALUE stack_setvalue;

                        status_assert(ss_data_resource_copy(&stack_setvalue.ss_data_arg_0, ev_split_exec_data.args[0]));
                        status_assert(ss_data_resource_copy(&stack_setvalue.ss_data_arg_1, ev_split_exec_data.args[1]));
                        stack_setvalue.n_args = arg_count;
                        stack_setvalue.ix_x = stack_setvalue.ix_y = 1;
                        if(arg_count > 2)
                            stack_setvalue.ix_x = ss_data_get_integer(ev_split_exec_data.args[2]);
                        if(arg_count > 3)
                            stack_setvalue.ix_y = ss_data_get_integer(ev_split_exec_data.args[3]);

                        /* blow away setvalue arguments */
                        stack_set(stack_offset - arg_count);

                        /* save current state */
                        rpn_skip(&rpnstate);

                        stack_offset += 1;

                        stack_base[stack_offset].slr = p_eval_block->slr;
                        stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                        stack_base[stack_offset].stack_flags.type = IN_EVAL;

                        stack_base[stack_at].data.stack_in_calc.eval_block.offset = PtrDiffBytesU32(rpnstate.pos, rpnstate.p_rpn_start);
                        stack_base[stack_offset].data.stack_in_eval.stack_offset = stack_at;

                        /* switch to SETVALUE state */
                        stack_offset += 1;

                        stack_base[stack_offset].slr = p_eval_block->slr;
                        stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                        stack_base[stack_offset].stack_flags.type = SETVALUE;

                        stack_base[stack_offset].data.stack_setvalue = stack_setvalue;

                        res = NEW_STATE;
                        break;
                        }

                    case EXEC_ARRAY_RANGE:
                        {
                        STATUS status;
                        STACK_ARRAY_RANGE stack_array_range;

                        if(NULL == (stack_array_range.p_array_range_block = al_ptr_alloc_elem(ARRAY_RANGE_BLOCK, 1, &status)))
                            res = status;
                        else
                        {
                            S32 i;
                            /* copy arguments across */
                            for(i = 0; i < arg_count; ++i)
                                status_assert(ss_data_resource_copy(&stack_array_range.p_array_range_block->args[i], ev_split_exec_data.args[i]));

                            array_range_stat_block_init(&stack_array_range.p_array_range_block->stat_block,
                                                        &stack_array_range.p_array_range_block->n_args,
                                                        &stack_array_range.p_array_range_block->arg_ix,
                                                        rpnstate.num,
                                                        ev_split_exec_data.args,
                                                        arg_count);

                            /* blow away arguments */
                            stack_set(stack_offset - arg_count);

                            /* save current state */
                            rpn_skip(&rpnstate);

                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = IN_EVAL;

                            stack_base[stack_at].data.stack_in_calc.eval_block.offset = PtrDiffBytesU32(rpnstate.pos, rpnstate.p_rpn_start);
                            stack_base[stack_offset].data.stack_in_eval.stack_offset = stack_at;

                            /* switch to lookup state */
                            stack_offset += 1;

                            stack_base[stack_offset].slr = p_eval_block->slr;
                            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                            stack_base[stack_offset].stack_flags.type = ARRAY_RANGE_ARG;

                            stack_base[stack_offset].data.stack_array_range = stack_array_range;

                            res = NEW_STATE;
                        }

                        break;
                        }

                    default: default_unhandled();
                        break;
                    }
                }

                /* have we switched state ? */
                if(res == NEW_STATE)
                    return(res);
                else if(status_fail(res))
                {
                    ss_data_free_resources(&func_result_data);
                    ss_data_set_error(&func_result_data, res);
                }

                /* remove function arguments from stack */
                stack_set(stack_offset - arg_count);

                /* push result onto stack */
                stack_offset += 1;

                stack_base[stack_offset].slr = p_eval_block->slr;
                stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
                stack_base[stack_offset].stack_flags.type = DATA_ITEM;

                stack_base[stack_offset].data.stack_data_item.data = func_result_data;
            }
            break;

        /* make an array from args on stack */
        case RPN_FNA:
            {
            S32 x_rpn_size, y_rpn_size, range_max_x = 0, range_max_y = 0, stack_total;
            SS_DATA ss_data_out;

            read_from_rpn(&x_rpn_size, rpnstate.pos + sizeof32(EV_IDNO),                 sizeof32(S32));
            read_from_rpn(&y_rpn_size, rpnstate.pos + sizeof32(EV_IDNO) + sizeof32(S32), sizeof32(S32));
            stack_total = x_rpn_size * y_rpn_size;

            { /* look for ranges in array data */
            S32 x, y;
            S32 ix = stack_total - 1;

            for(y = 0; y < y_rpn_size; ++y)
            {
                for(x = 0; x < x_rpn_size; ++x)
                {
                    P_SS_DATA p_ss_data = stack_index_ptr_data(stack_offset, ix);

                    if(DATA_ID_RANGE == ss_data_get_data_id(p_ss_data))
                    {
                        S32 x_size, y_size;
                        array_range_sizes(p_ss_data, &x_size, &y_size);
                        range_max_x = MAX(range_max_x, x_size);
                        range_max_y = MAX(range_max_y, y_size);
                    }
                    --ix;
                }
            }
            } /*block*/

            /* check for lopsidedness */
            if((range_max_x > 1 && x_rpn_size > 1)
               ||
               (range_max_y > 1 && y_rpn_size > 1))
               ss_data_set_error(&ss_data_out, EVAL_ERR_SUBSCRIPT);
            else
            {
                S32 x_array_size = MAX(x_rpn_size, range_max_x);
                S32 y_array_size = MAX(y_rpn_size, range_max_y);

                if(status_ok(ss_array_make(&ss_data_out, x_array_size, y_array_size)))
                {
                    S32 x, y;
                    S32 ix = stack_total - 1;

                    for(y = 0; y < y_rpn_size; ++y)
                    {
                        for(x = 0; x < x_rpn_size; ++x)
                        {
                            P_SS_DATA p_ss_data = stack_index_ptr_data(stack_offset, ix);

                            if(DATA_ID_RANGE == ss_data_get_data_id(p_ss_data))
                            {
                                S32 ixr, ixr_limit = MAX(range_max_y, range_max_x);

                                for(ixr = 0; ixr < ixr_limit; ++ixr)
                                {
                                    SS_DATA ss_data;

                                    (void) array_range_mono_index(&ss_data, p_ss_data, ixr, EM_ANY);

                                    status_assert(ss_data_resource_copy(
                                                        ss_array_element_index_wr(&ss_data_out,
                                                                               range_max_x > 1 ? ixr : x,
                                                                               range_max_y > 1 ? ixr : y),
                                                        &ss_data));

                                    ss_data_free_resources(&ss_data);
                                }
                            }
                            else
                                status_assert(ss_data_resource_copy(
                                                    ss_array_element_index_wr(&ss_data_out, x, y),
                                                    stack_index_ptr_data(stack_offset, ix)));
                            --ix;
                        }
                    }
                }
            }

            /* remove arguments */
            stack_set(stack_offset - stack_total);

            /* push result */
            stack_offset += 1;

            stack_base[stack_offset].slr = p_eval_block->slr;
            stack_base[stack_offset].stack_flags = stack_base[stack_offset-1].stack_flags;
            stack_base[stack_offset].stack_flags.type = DATA_ITEM;

            stack_base[stack_offset].data.stack_data_item.data = ss_data_out;

            break;
            }
        }

        rpn_skip(&rpnstate);
    }

    if(stack_base[stack_offset].stack_flags.type != DATA_ITEM)
        return(eval_backtrack_error(EVAL_ERR_INTERNAL, stack_at));
    else
        stack_base[stack_at].data.stack_in_calc.result_data = stack_base[stack_offset].data.stack_data_item.data;

    /* clear final result from stack */
    stack_offset -= 1;

    /* return complete code */
    return(SAME_STATE);
}

/******************************************************************************
*
* read argument for current rpn token into supplied symbol structure
*
******************************************************************************/

extern void
read_cur_sym(
    P_RPNSTATE p_rpnstate,
    P_SS_DATA p_ss_data)
{
    PC_U8 p_rpn_content = p_rpnstate->pos + sizeof32(EV_IDNO);

    p_ss_data->local_data = 0;

    switch(p_ss_data->data_id = p_rpnstate->num)
    {
    case DATA_ID_REAL:
        read_from_rpn(&p_ss_data->arg.fp, p_rpn_content, sizeof32(F64));
        return;

    case DATA_ID_LOGICAL:
        p_ss_data->arg.boolean = (BOOL) PtrGetByte(p_rpn_content); /* U8: 0 or 1 */
        return;

    case DATA_ID_WORD8:
        {
        S8 s8;
        read_from_rpn(&s8, p_rpn_content, sizeof32(S8));
        p_ss_data->arg.integer = (S32) s8;
        return;
        }

    case DATA_ID_WORD16:
        {
        S16 s16;
        read_from_rpn(&s16, p_rpn_content, sizeof32(S16));
        p_ss_data->arg.integer = (S32) s16;
        return;
        }

    case DATA_ID_WORD32:
        read_from_rpn(&p_ss_data->arg.integer, p_rpn_content, sizeof32(S32));
        return;

    case DATA_ID_SLR:
        p_ss_data->arg.slr = *(p_ev_slr_from_ev_cell(p_rpnstate->p_ev_cell, (S32) *p_rpn_content));
        return;

    case DATA_ID_RANGE:
        p_ss_data->arg.range = *(p_ev_range_from_ev_cell(p_rpnstate->p_ev_cell, (S32) *p_rpn_content));
        return;

    case DATA_ID_STRING:
        p_ss_data->arg.string.uchars = (PC_UCHARS) p_rpn_content;
        p_ss_data->arg.string.size = ustrlen32(p_ss_data->arg.string.uchars); /* string in RPN can't contain CH_NULL */
        return;

    case DATA_ID_DATE:
        read_from_rpn(&p_ss_data->arg.ss_date, p_rpn_content, sizeof32(SS_DATE));
        return;

    case DATA_ID_NAME:
        p_ss_data->arg.h_name = (p_ev_name_from_ev_cell(p_rpnstate->p_ev_cell, (S32) *p_rpn_content))->h_name;
        return;

    case DATA_ID_BLANK:
        return;
    }
}

/******************************************************************************
*
* skip to next rpn token
*
******************************************************************************/

/*ncr*/
extern EV_IDNO
rpn_skip(
    P_RPNSTATE p_rpnstate)
{
    assert(p_rpnstate->num < ELEMOF_RPN_TABLE);
    if(p_rpnstate->num < ELEMOF_RPN_TABLE)
    {
        /* work out how to skip symbol */
        p_rpnstate->pos += sizeof32(EV_IDNO);

        switch(rpn_table[p_rpnstate->num].rpn_type)
        {
        default: default_unhandled();
            break;

        case RPN_DAT:
            switch(p_rpnstate->num)
            {
            default: default_unhandled();
                break;

            case DATA_ID_REAL:
                p_rpnstate->pos += sizeof32(F64);
                break;

            case DATA_ID_LOGICAL:
                p_rpnstate->pos += sizeof32(U8);
                break;

            case DATA_ID_WORD8:
                p_rpnstate->pos += sizeof32(S8);
                break;

            case DATA_ID_WORD16:
                p_rpnstate->pos += sizeof32(S16);
                break;

            case DATA_ID_WORD32:
                p_rpnstate->pos += sizeof32(S32);
                break;

            case DATA_ID_SLR:
                p_rpnstate->pos += sizeof32(U8);
                break;

            case DATA_ID_RANGE:
                p_rpnstate->pos += sizeof32(U8);
                break;

            case DATA_ID_STRING:
                p_rpnstate->pos += strlen32p1(p_rpnstate->pos);
                break;

            case DATA_ID_DATE:
                p_rpnstate->pos += sizeof32(SS_DATE);
                break;

            case DATA_ID_NAME:
                p_rpnstate->pos += sizeof32(U8);
                break;

            case DATA_ID_BLANK:
                break;
            }
            break;

        case RPN_FRM:
            switch(p_rpnstate->num)
            {
            default: default_unhandled();
                break;

            case RPN_FRM_SPACE:
            case RPN_FRM_RETURN:
                /* skip cr/space argument */
                p_rpnstate->pos += 1;
                break;

            case RPN_FRM_COND:
                /* skip entire conditional rpn */
                p_rpnstate->pos += readS16(p_rpnstate->pos);
                break;

            case RPN_FRM_SKIPTRUE:
            case RPN_FRM_SKIPFALSE:
                /* skip skip argument */
                p_rpnstate->pos += 1 + sizeof32(S16);
                break;

            case RPN_FRM_EQUALS:
            case RPN_FRM_BRACKETS:
            case RPN_FRM_END:
                break;
            }
            break;

        case RPN_LCL:
            while(*p_rpnstate->pos)
                ++p_rpnstate->pos;
            ++p_rpnstate->pos;
            break;

        case RPN_UOP:
        case RPN_BOP:
        case RPN_REL:
            break;

        case RPN_FN0:
        case RPN_FNF:
            break;

        case RPN_FNV:
            /* skip argument count */
            ++p_rpnstate->pos;
            break;

        case RPN_FNM:
            /* skip argument count and macro id */
            p_rpnstate->pos += (1 + sizeof32(U8));
            break;

        case RPN_FNA:
            /* skip x, y counts */
            p_rpnstate->pos += sizeof32(S32) * 2;
            break;
        }
    }

    rpn_check(p_rpnstate);

    return(p_rpnstate->num);
}

/******************************************************************************
*
* initialise rpnstate block
*
******************************************************************************/

extern void
rpnstate_init(
    _OutRef_    P_RPNSTATE p_rpnstate,
    _InRef_     PC_EV_CELL p_ev_cell,
    _InVal_     S32 offset)
{
    p_rpnstate->p_ev_cell = p_ev_cell;
    p_rpnstate->p_rpn_start = p_rpn_from_ev_cell(p_ev_cell);
    p_rpnstate->pos = p_rpnstate->p_rpn_start + offset;

    rpn_check(p_rpnstate);
}

/* end of ev_rpn.c */
