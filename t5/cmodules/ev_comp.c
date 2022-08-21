/* ev_comp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* infix to RPN compiler */

/* MRJC May 1988 / January 1991 / May 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include <ctype.h> /* for "C"isalpha and friends */

/*
internal functions
*/

static void
out_to_rpn(
    _InVal_     U32 n_bytes,
    _In_reads_bytes_(n_bytes) PC_ANY p_data);

static S32
proc_func(
    /*_In_*/    PC_RPNDEF p_rpndef);

static void
proc_func_arg_maybe_blank(void);

static S32
proc_func_custom(
    P_EV_HANDLE p_h_custom);

static S32
proc_func_dbs(void);

_Check_return_
static S32
proc_func_if(
    _OutRef_    P_EV_IDNO which_if);

static void
rec_array_row(
    _InVal_     S32 cur_y,
    P_S32 x_size);

static void
rec_bterm(void);

static void
rec_b2term(void);

static void
rec_cterm(void);

static void
rec_dterm(void);

static void
rec_eterm(void);

static void
rec_expr(void);

static void
rec_fterm(void);

static void
rec_gterm(void);

static void
rec_lterm(void);

_Check_return_
static EV_IDNO
scan_check_next(
    _Out_opt_   P_SYM_INF p_sym_inf);

static void
scan_next_symbol(void);

/*ncr*/
static STATUS
set_compile_error(
    _InVal_     STATUS status);

/*
* compiler IO variables
*/

typedef struct COMPILER_CONTEXT
{
    EV_SLR ev_slr;                      /* cell being compiled */

    union COMPILER_CONTEXT_IP_POS
    {
        PC_U8 p_u8;
        PC_USTR ustr;
    } ip_pos;                           /* position in input */

    P_COMPILER_OUTPUT p_compiler_output;

    DOCU_NAME docu_name;                /* scanned external ref */
    UCHARZ ident[BUF_EV_INTNAMLEN];     /* scanned identifier */

    SS_DATA data_cur;                   /* details of symbol scanned */
    SYM_INF sym_inf;
    PC_USTR err_pos;                    /* position of error */
    S32 error;                          /* reason for non-compilation */
    S32 n_scanned;                      /* number of symbols scanned */

    S32 dbs_nest;                       /* database function nest count */
    S32 array_nest;                     /* array nest count */

    FUN_PARMS fun_parms;                /* things about compiled expression */
}
COMPILER_CONTEXT, * P_COMPILER_CONTEXT;

static COMPILER_CONTEXT compiler_context;

/******************************************************************************
*
* output byte to compiled expression
*
******************************************************************************/

static void
out_byte(
    _InVal_     U8 u8)
{
    out_to_rpn(sizeof32(U8), &u8);
}

/******************************************************************************
*
* output a custom function call
* to the compiled string
*
* the compiled string contains an index
* to a custom function table which is stored separately
* from the rpn string
*
******************************************************************************/

static void
out_custom_call(
    _InVal_     EV_HANDLE h_custom)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(EV_HANDLE), FALSE);
    ARRAY_INDEX ix_custom_call;
    P_EV_HANDLE p_h_custom;

    ix_custom_call = array_elements(&compiler_context.p_compiler_output->h_custom_calls);

    /* output custom index */
    out_byte((U8) ix_custom_call);

    if(NULL == (p_h_custom = al_array_extend_by(&compiler_context.p_compiler_output->h_custom_calls, EV_HANDLE, 1, &array_init_block, &status)))
        set_compile_error(status);
    else
        *p_h_custom = h_custom;
}

/******************************************************************************
*
* output a custom function definition
* to the compiled string
*
* the compiled string contains an index
* to a custom definition table which is stored separately
* from the rpn string
*
******************************************************************************/

static void
out_custom_def(
    _InVal_     EV_HANDLE h_custom)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(EV_HANDLE), FALSE);
    ARRAY_INDEX ix_custom_def;
    P_EV_HANDLE p_h_custom;

    ix_custom_def = array_elements(&compiler_context.p_compiler_output->h_custom_defs);

    /* output custom index */
    out_byte((U8) ix_custom_def);

    if(NULL == (p_h_custom = al_array_extend_by(&compiler_context.p_compiler_output->h_custom_defs, EV_HANDLE, 1, &array_init_block, &status)))
        set_compile_error(status);
    else
        *p_h_custom = h_custom;
}

/******************************************************************************
*
* output an event dependency
*
******************************************************************************/

static void
out_event(
    _InVal_     EVENT_TYPE event_type)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(EVENT_TYPE), FALSE);
    P_EVENT_TYPE p_event_type;

    if(NULL == (p_event_type = al_array_extend_by(&compiler_context.p_compiler_output->h_events, EVENT_TYPE, 1, &array_init_block, &status)))
        set_compile_error(status);
    else
        *p_event_type = event_type;
}

/******************************************************************************
*
* output id number to compiled expression
*
******************************************************************************/

static inline void
out_idno(
    _InVal_     EV_IDNO ev_idno)
{
#if defined(EV_IDNO_U32)
    U16 u16 = (U16) ev_idno;
    out_to_rpn(sizeof32(u16), &u16);
    u16 = 0xBEEFU;
    out_to_rpn(sizeof32(u16), &u16);
#else
    out_to_rpn(sizeof32(EV_IDNO), &ev_idno);
#endif
}

/******************************************************************************
*
* output id number with accumulated
* formatting information
*
******************************************************************************/

static void
out_idno_format(
    P_SYM_INF p_sym_inf)
{
    if(p_sym_inf->sym_equals)
        out_idno(RPN_FRM_EQUALS);

    if(p_sym_inf->sym_cr)
    {
        out_idno(RPN_FRM_RETURN);
        out_byte(p_sym_inf->sym_cr);
    }

    if(p_sym_inf->sym_space)
    {
        out_idno(RPN_FRM_SPACE);
        out_byte(p_sym_inf->sym_space);
    }

    out_idno(p_sym_inf->sym_idno);
}

/******************************************************************************
*
* output a reference to a name
* to the compiled string
*
* the compiled string contains an index
* to a name table which is stored separately
* from the rpn string
*
******************************************************************************/

static void
out_name(
    _InVal_     EV_HANDLE h_name,
    _InVal_     BOOL no_dep)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(2, sizeof32(EV_NAME_REF), FALSE);
    ARRAY_INDEX ix_name;
    P_EV_NAME_REF p_ev_name_ref;

    ix_name = array_elements(&compiler_context.p_compiler_output->h_names);

    /* output name index */
    out_byte((U8) ix_name);

    if(NULL == (p_ev_name_ref = al_array_extend_by(&compiler_context.p_compiler_output->h_names, EV_NAME_REF, 1, &array_init_block, &status)))
        set_compile_error(status);
    else
    {
        p_ev_name_ref->h_name = h_name;
        p_ev_name_ref->no_dep = no_dep;
    }
}

/******************************************************************************
*
* return current output position
*
******************************************************************************/

_Check_return_
static inline ARRAY_INDEX
out_pos(void)
{
    return(array_elements(&compiler_context.p_compiler_output->h_rpn));
}

/******************************************************************************
*
* output a range to the compiled string
*
* the compiled string contains an index
* to a range table which is stored separately
* from the rpn string
*
******************************************************************************/

static void
out_range(
    _InRef_     PC_EV_RANGE p_ev_range_in,
    _InVal_     BOOL no_dep)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(5, sizeof32(EV_RANGE), FALSE);
    ARRAY_INDEX ix_range;
    P_EV_RANGE p_ev_range;

    ix_range = array_elements(&compiler_context.p_compiler_output->h_ranges);

    /* output range index */
    out_byte((U8) ix_range);

    if(NULL == (p_ev_range = al_array_extend_by(&compiler_context.p_compiler_output->h_ranges, EV_RANGE, 1, &array_init_block, &status)))
        set_compile_error(status);
    else
    {
        *p_ev_range = *p_ev_range_in;
        p_ev_range->s.no_dep = p_ev_range->e.no_dep = no_dep;
    }
}

/******************************************************************************
*
* set skip offset into rpn string
* call out_pos to remember position
*
******************************************************************************/

static void
out_skip_set(
    _InVal_     ARRAY_INDEX skip_pos,
    _InVal_     S16 skip_count)
{
    const U32 n_bytes = sizeof32(S16);
    P_U8 rpn_pos = array_range(&compiler_context.p_compiler_output->h_rpn, U8, skip_pos, n_bytes);
    memcpy32(rpn_pos, &skip_count, n_bytes);
}

/******************************************************************************
*
* output an slr to the compiled string
*
* the compiled string contains an index
* to an slr table which is stored separately
* from the rpn string
*
******************************************************************************/

static void
out_slr(
    _InRef_     PC_EV_SLR p_ev_slr_in,
    _InVal_     BOOL no_dep /* set no_dep bit */,
    _InVal_     BOOL self /* self reference has no index in rpn */)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(10, sizeof32(EV_SLR), FALSE);
    P_EV_SLR p_ev_slr;

    if(!self)
    {
        /* output slr index */
        ARRAY_INDEX ix_slr = array_elements(&compiler_context.p_compiler_output->h_slrs);
        out_byte((U8) ix_slr);
    }

    if(NULL == (p_ev_slr = al_array_extend_by(&compiler_context.p_compiler_output->h_slrs, EV_SLR, 1, &array_init_block, &status)))
        set_compile_error(status);
    else
    {
        *p_ev_slr = *p_ev_slr_in;
        p_ev_slr->no_dep = no_dep;
    }
}

/******************************************************************************
*
* output string to RPN
* free string afterwards
*
******************************************************************************/

static void
out_string_free(
    P_SYM_INF p_sym_inf,
    _InoutRef_  P_SS_DATA p_ss_data)
{
    p_sym_inf->sym_idno = DATA_ID_STRING;
    out_idno_format(p_sym_inf);

    assert(ss_data_is_string(p_ss_data));
    PTR_ASSERT(ss_data_get_string(p_ss_data));

    assert(NULL == memchr(ss_data_get_string(p_ss_data), CH_NULL, ss_data_get_string_size(p_ss_data)));
    out_to_rpn(ss_data_get_string_size(p_ss_data), ss_data_get_string(p_ss_data));
    {
    U8 nullch = CH_NULL;
    out_to_rpn(1, &nullch);
    } /*block*/
    ss_data_free_resources(p_ss_data);
}

/******************************************************************************
*
* output signed 16 bit word to compiled expression
*
******************************************************************************/

static void
out_S16(
    _InVal_     S16 word)
{
    out_to_rpn(sizeof32(S16), &word);
}

/******************************************************************************
*
* output data to the rpn string
*
******************************************************************************/

static void
out_to_rpn(
    _InVal_     U32 n_bytes,
    _In_reads_bytes_(n_bytes) PC_ANY p_data)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(64, sizeof32(U8), FALSE);

    if(status_fail(al_array_add(&compiler_context.p_compiler_output->h_rpn, BYTE, n_bytes, &array_init_block, p_data)))
        set_compile_error(status_nomem());

    al_array_auto_compact_set(&compiler_context.p_compiler_output->h_rpn);
}

/******************************************************************************
*
* Grammar:
*
*   <expr>     := <aterm>  { <group6><aterm> }
*   <aterm>    := <bterm>  { <group5><bterm> }
*   <bterm>    := <cterm>  { <group4><cterm> }
*   <cterm>    := <dterm>  { <group3><dterm> }
*   <dterm>    := <eterm>  { <group2><eterm> }
*   <eterm>    := <fterm>  { <group1><fterm> }
*   <fterm>    := { <unary><fterm> } | <gterm>
*   <gterm>    := <lterm> | (<expr>)
*   <lterm>    := number        |
*                 date          |
*                 slr           |
*                 range         |
*                 string        |
*                 '{'<array>'}' |
*                 <function>    |
*                 <custom>      |
*                 <name>
*
*   <array>    := <row>  {;<row>}
*   <row>      := <item> {,<item>}
*   <item>     := number | slr | date | string | name
*
*   <function> := <ownid> | <owndb>(range, <expr>) | <ownfunc>(<args>)
*   <custom>    := <name>(<args>)
*
*   <name>     := {<external>} ident
*   <external> := [ ident ]
*   <args>     := {<expr>} {,<expr>}
*   <ownid>    := col | pi | row ...
*   <owndb>    := dsum | dmax | dmin ...
*   <ownfunc>  := abs | int | lookup | max | year ...
*
*   <unary>    := + | - | !
*   <group1>   := ^
*   <group2>   := * | /
*   <group3>   := + | -
*   <group4>   := = | <> | < | > | <= | >=
*   <group5>   := &
*   <group6>   := |
*
* examples:
*   number     9.9
*   slr        [ext]$A1
*   range      [ext]$A1$A10
*   string     "fred"
*   ident      abc_123
*   date       1.2.3 10:56
*
******************************************************************************/

/******************************************************************************
*
* recognise evaluator constants and formulae in string form;
* convert constants into internal binary and formulae into rpn
*
* --out--
* <0 error in processing input string
* =0 nothing processed
* >0 processed OK
*
******************************************************************************/

_Check_return_
extern STATUS
ev_compile(
    _OutRef_    P_COMPILER_OUTPUT p_compiler_output /* handles of compiler output */,
    _In_opt_z_  PC_USTR ustr_result     /* possible result */,
    _In_opt_z_  PC_USTR ustr_formula    /* possible formula or constant */,
    _InRef_     PC_EV_SLR p_ev_slr)
{
    STATUS status = STATUS_OK;

#if defined(EV_IDNO_U16)
    assert(RPN_END <= U16_MAX);
#else
    assert(RPN_END <= U8_MAX);
#endif

    zero_struct_ptr(p_compiler_output);
    zero_struct(compiler_context);

    /* save options */
    compiler_context.p_compiler_output = p_compiler_output;

    /* initialise parms for a constant */
    p_compiler_output->ev_parms.data_only = 1;
    p_compiler_output->ev_parms.data_id = UBF_PACK(DATA_ID_BLANK);
    ss_data_set_blank(&p_compiler_output->ss_data);

    if((NULL != ustr_formula) && (CH_NULL != PtrGetByte(ustr_formula)))
    {
#if 0
        /* check for single name */
        if(!ev_name_check(p_formula, ev_slr_docno(p_ev_slr)))
            return(create_error(EVAL_ERR_BADEXPR));
#endif

        /* check if we have a constant */
        if(status_fail(status = ss_recog_constant(&compiler_context.data_cur, ustr_formula)))
            return(status);
        else if(status > 0)
        {
            p_compiler_output->ss_data = compiler_context.data_cur;
            p_compiler_output->ev_parms.data_id = UBF_PACK(ss_data_get_data_id(&compiler_context.data_cur));
        }
        else
        {
            /* now try compiling the string */
            compiler_context.ev_slr = *p_ev_slr;
            compiler_context.ip_pos.ustr = ustr_formula;
            compiler_context.sym_inf.sym_idno = SYM_BLANK;

            name_init(&compiler_context.docu_name);

            /* recognise the expression */
            ev_doc_reuse_hold();
            rec_expr();
            ev_doc_reuse_release();

            /* set scanner position */
            p_compiler_output->chars_processed = PtrDiffBytesU32(compiler_context.ip_pos.ustr, ustr_formula);

            if(compiler_context.sym_inf.sym_idno != RPN_FRM_END)
            {
                ev_compiler_output_dispose(p_compiler_output);
                if(status_ok(status = compiler_context.error))
                    status = /*create_error*/(EVAL_ERR_BADEXPR);
            }
            else
            {
                out_idno(RPN_FRM_END);

                p_compiler_output->ev_parms.control = compiler_context.fun_parms.control;
                p_compiler_output->ev_parms.data_only = 0;
                if(compiler_context.fun_parms.var)
                    p_compiler_output->ev_parms.rpn_variable = 1;
                if(compiler_context.fun_parms.load_recalc)
                    p_compiler_output->load_recalc = 1;

                p_compiler_output->ev_parms.slr_n    = UBF_PACK(array_elements32(&p_compiler_output->h_slrs));
                p_compiler_output->ev_parms.range_n  = UBF_PACK(array_elements32(&p_compiler_output->h_ranges));
                p_compiler_output->ev_parms.name_n   = UBF_PACK(array_elements32(&p_compiler_output->h_names));
                p_compiler_output->ev_parms.custom_n = UBF_PACK(array_elements32(&p_compiler_output->h_custom_calls));
                p_compiler_output->ev_parms.event_n  = UBF_PACK(array_elements32(&p_compiler_output->h_events));

                status = 1;
            }
        }
    }

    /* if we were given a result, this overrides any result so far */
    if((NULL != ustr_result) && status_ok(status))
    {
        status = ss_recog_constant(&compiler_context.data_cur, ustr_result);
        if(status_done(status))
        {
            p_compiler_output->ss_data = compiler_context.data_cur;
            p_compiler_output->ev_parms.data_id = UBF_PACK(ss_data_get_data_id(&compiler_context.data_cur));
        }
        else if(STATUS_OK == status)
        {   /* don't give up if it's just a case of not recognising a wonky result */
            status = STATUS_DONE;
        }
    }

    return(status);
}

/******************************************************************************
*
* dispose of compiler output
*
******************************************************************************/

extern void
ev_compiler_output_dispose(
    _InoutRef_  P_COMPILER_OUTPUT p_compiler_output)
{
    al_array_dispose(&p_compiler_output->h_rpn);
    al_array_dispose(&p_compiler_output->h_slrs);
    al_array_dispose(&p_compiler_output->h_ranges);
    al_array_dispose(&p_compiler_output->h_names);
    al_array_dispose(&p_compiler_output->h_custom_calls);
    al_array_dispose(&p_compiler_output->h_custom_defs);
    al_array_dispose(&p_compiler_output->h_events);

    ss_data_free_resources(&p_compiler_output->ss_data);
}

/******************************************************************************
*
* return size of compiler output - does not include any cell overhead
*
******************************************************************************/

_Check_return_
extern S32
ev_compiler_output_size(
    _InRef_     P_COMPILER_OUTPUT p_compiler_output)
{
    return(OVH_EV_CELL
         + array_elements(&p_compiler_output->h_rpn) * sizeof32(U8)
         + array_elements(&p_compiler_output->h_slrs) * sizeof32(EV_SLR)
         + array_elements(&p_compiler_output->h_ranges) * sizeof32(EV_RANGE)
         + array_elements(&p_compiler_output->h_names) * sizeof32(EV_NAME_REF) /* SKS 22nov94 (was EV_HANDLE) to fix mineswee/c_game load problem */
         + array_elements(&p_compiler_output->h_custom_calls) * sizeof32(EV_HANDLE)
         + array_elements(&p_compiler_output->h_events) * sizeof32(EVENT_TYPE)
          );
}

/******************************************************************************
*
* check if string can be name, and then check if name is defined
*
* <0 bad name string
* =0 name string OK, not defined
* >0 name string OK & defined
*
******************************************************************************/

#if 0

_Check_return_
extern S32
ev_name_check(
    _In_z_      P_USTR ustr_name,
    _InVal_     EV_DOCNO ev_docno)
{
    S32 res;

    ustr_SkipSpaces(ustr_name); /* MRJC 28.4.92 */

    if((res = ident_validate(ustr_name)) >= 0)
    {
        if(ev_func_lookup(ustr_name) >= 0)
            res = -1;
        else
        {
            if(find_name_in_list(ev_docno, ustr_name) < 0)
                res = 0;
            else
                res = 1;
        }
    }

    return(res);
}

#endif

/******************************************************************************
*
* install compiler output in a new cell
*
******************************************************************************/

_Check_return_
extern STATUS
ev_cell_from_compiler_output(
    P_EV_CELL p_ev_cell,
    P_COMPILER_OUTPUT p_compiler_output,
    _InRef_opt_ PC_EV_SLR p_ev_slr_offset,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope)
{
    STATUS status = STATUS_OK;
    P_U8 p_u8;

    p_ev_cell->ev_parms = p_compiler_output->ev_parms;
    ev_cell_constant_from_data(p_ev_cell, &p_compiler_output->ss_data);

    if((NULL != p_ev_slr_offset) && p_compiler_output->ev_parms.slr_n)
    {
        const U32 n = p_ev_cell->ev_parms.slr_n;
        P_EV_SLR p_ev_slr = array_range(&p_compiler_output->h_slrs, EV_SLR, 0, n);
        U32 i;

        for(i = 0; i < n; i += 1, p_ev_slr += 1)
        {
            slr_offset_add(p_ev_slr, p_ev_slr_offset, p_ev_range_scope, FALSE, FALSE);
        }
    }

    p_u8 = (P_U8) p_ev_cell->slrs;

    if(p_ev_cell->ev_parms.slr_n)
    {
        const U32 n = p_compiler_output->ev_parms.slr_n;
        const U32 n_bytes = n * sizeof32(EV_SLR);
        memcpy32(p_u8, array_rangec(&p_compiler_output->h_slrs, EV_SLR, 0, n), n_bytes);
        p_u8 += n_bytes;
    }

    if((NULL != p_ev_slr_offset) && p_compiler_output->ev_parms.range_n)
    {
        const U32 n = p_ev_cell->ev_parms.range_n;
        P_EV_RANGE p_ev_range = array_range(&p_compiler_output->h_ranges, EV_RANGE, 0, n);
        U32 i;

        for(i = 0; i < n; i += 1, p_ev_range += 1)
        {
            slr_offset_add(&p_ev_range->s, p_ev_slr_offset, p_ev_range_scope, FALSE, FALSE);
            slr_offset_add(&p_ev_range->e, p_ev_slr_offset, p_ev_range_scope, FALSE, TRUE);
        }
    }

    if(p_ev_cell->ev_parms.range_n)
    {
        const U32 n = p_compiler_output->ev_parms.range_n;
        const U32 n_bytes = n * sizeof32(EV_RANGE);
        memcpy32(p_u8, array_rangec(&p_compiler_output->h_ranges, EV_RANGE, 0, n), n_bytes);
        p_u8 += n_bytes;
    }

    if(p_ev_cell->ev_parms.name_n)
    {
        const U32 n = p_compiler_output->ev_parms.name_n;
        const U32 n_bytes = n * sizeof32(EV_NAME_REF);
        memcpy32(p_u8, array_rangec(&p_compiler_output->h_names, EV_NAME_REF, 0, n), n_bytes);
        p_u8 += n_bytes;
    }

    if(p_ev_cell->ev_parms.custom_n)
    {
        const U32 n = p_compiler_output->ev_parms.custom_n;
        const U32 n_bytes = n * sizeof32(EV_HANDLE);
        memcpy32(p_u8, array_rangec(&p_compiler_output->h_custom_calls, EV_HANDLE, 0, n), n_bytes);
        p_u8 += n_bytes;
    }

    if(p_ev_cell->ev_parms.event_n)
    {
        const U32 n = p_compiler_output->ev_parms.event_n;
        const U32 n_bytes = n * sizeof32(EVENT_TYPE);
        memcpy32(p_u8, array_rangec(&p_compiler_output->h_events, EVENT_TYPE, 0, n), n_bytes);
        p_u8 += n_bytes;
    }

    if(array_elements(&p_compiler_output->h_rpn))
    {
        const U32 n = array_elements32(&p_compiler_output->h_rpn);
        const U32 n_bytes = n * sizeof32(U8);
        memcpy32(p_u8, array_rangec(&p_compiler_output->h_rpn, U8, 0, n), n_bytes);
    }

    return(status);
}

/******************************************************************************
*
* an internal or custom function call has been identified
*
******************************************************************************/

static void
func_call(
    P_SYM_INF p_sym_inf,
    _InVal_     EV_HANDLE h_custom)
{
    PC_RPNDEF p_rpndef;

    assert(p_sym_inf->sym_idno < ELEMOF_RPN_TABLE);
    p_rpndef = &rpn_table[p_sym_inf->sym_idno];

    if(p_sym_inf->sym_idno == RPN_FNF_IF)
    {
        /* if() */
        EV_IDNO which_if;
        S32 narg = proc_func_if(&which_if);

        if(narg >= 0)
        {
            p_sym_inf->sym_idno = which_if;
            out_idno_format(p_sym_inf);
        }
    }
    else if(p_rpndef->fun_parms.ex_type == EXEC_DBASE)
    {
        /* database functions */
        compiler_context.dbs_nest += 1;

        if(compiler_context.dbs_nest > 1)
            set_compile_error(EVAL_ERR_DBASENEST);
        else if(proc_func_dbs() >= 0)
            out_idno_format(p_sym_inf);

        compiler_context.dbs_nest -= 1;
    }
    else if(p_sym_inf->sym_idno == RPN_FNM_FUNCTION)
    {
        /* custom definitions */
        S32 narg;
        EV_HANDLE h_custom;

        if((narg = proc_func_custom(&h_custom)) >= 0)
        {
            out_idno_format(p_sym_inf);
            out_byte((U8) narg);
            out_custom_def(h_custom);
        }
    }
    else
    {
        /* other functions */
        S32 narg;

        if(p_rpndef->n_args < 0)
        {
            /* variable argument functions */
            narg = proc_func(p_rpndef);

            if(narg >= 0)
            {
                out_idno_format(p_sym_inf);
                out_byte((U8) narg);

                /* custom calls have their id tagged on t'end */
                if(p_sym_inf->sym_idno == RPN_FNM_CUSTOMCALL)
                    out_custom_call(h_custom);
            }
        }
        else if(p_rpndef->n_args == 0)
        {
            /* zero argument functions */
#if 1       /* allow parsing of hopefully empty function list */
            narg = proc_func(p_rpndef);

            if(narg == 0)
#endif
            {
                out_idno_format(p_sym_inf);
            }
        }
        else
        {
            /* fixed argument functions */
            narg = proc_func(p_rpndef);

            if(narg >= 0)
                out_idno_format(p_sym_inf);
        }
    }

    /* done at the end cos p_sym_inf->sym_idno may be adjusted */
    assert(p_sym_inf->sym_idno < ELEMOF_RPN_TABLE);
    p_rpndef = &rpn_table[p_sym_inf->sym_idno];

    /* output self dependencies when required */
    if(p_rpndef->fun_parms.self)
        out_slr(&compiler_context.ev_slr, 1, 1);

    /* check for event dependencies */
    if(p_rpndef->fun_parms.event_type != EV_EVENT_NONE)
        out_event(p_rpndef->fun_parms.event_type);

    /* accumulate flags */
    compiler_context.fun_parms.control = p_rpndef->fun_parms.control;
    compiler_context.fun_parms.var |= p_rpndef->fun_parms.var;
    compiler_context.fun_parms.load_recalc |= p_rpndef->fun_parms.load_recalc;
}

/******************************************************************************
*
* validate an identifier
*
* --out--
* <0  error
* >=0 length of ident
*
******************************************************************************/

_Check_return_
extern STATUS
ident_validate(
    _In_z_      PC_USTR ident,
    _InVal_     BOOL relaxed)
{
    PC_USTR pos = ident;
    U32 used_in;
    S32 ident_len;
    BOOL had_digit = FALSE;
    BOOL digit_ok = FALSE;

    PtrSkipSpaces(PC_USTR, pos); /* MRJC 24.3.92 - skip lead spaces */

    /* MRJC 8.11.94 - skip lead ? */
    while(PtrGetByte(pos) == CH_QUESTION_MARK) /* database field */
        ustr_IncByte(pos);

    used_in = 0;

    for(;;)
    {
        U32 bytes_of_char;
        UCS4 ucs4 = uchars_char_decode_off(pos, used_in, bytes_of_char);
        SBCHAR ident_ch;

        if(!ucs4_is_sbchar(ucs4))
            break;

        ident_ch = (SBCHAR) ucs4;

        if(0 == used_in)
        {   /* "XID_Start" */
            if(sbchar_isalpha(ident_ch)) /* Latin-1, no remapping (for document portability) */
            { /*EMPTY*/ }
            else if(CH_UNDERSCORE == ident_ch) /* SKS 21jul14 better Excel compatibility */
            {
                digit_ok = TRUE; /* digits in identifiers can occur only after an underscore */
            }
            else
            {
                break;
            }
        }
        else
        {   /* "XID_Continue" */
            if(sbchar_isalpha(ident_ch))
            { /*EMPTY*/ }
            else if(sbchar_isdigit(ident_ch))
            {
                had_digit = TRUE;

                if(relaxed && !digit_ok && (2 < used_in))
                    digit_ok = TRUE;
            }
            else if(CH_UNDERSCORE == ident_ch)
            {
                digit_ok = TRUE; /* digits in identifiers can occur only after an underscore */
            }
            else if(CH_FULL_STOP == ident_ch)
            {
                digit_ok = TRUE; /* digits in identifiers can also occur after an dot (e.g. t.dist.2t) */
            }
            else
            {
                break;
            }
        }

        used_in += bytes_of_char;
    }

    ustr_IncBytes(pos, used_in);

    PtrSkipSpaces(PC_USTR, pos); /* MRJC 24.3.92 - skip trail spaces */

    ident_len = PtrDiffBytesS32(pos, ident);

    if( (CH_NULL != PtrGetByte(pos)) ||
        (0 == ident_len)             ||
        (ident_len > EV_INTNAMLEN)   ||
        (had_digit && !digit_ok)     )
        return(/*create_error*/(EVAL_ERR_BADIDENT));

    return(ident_len);
}

/******************************************************************************
*
* process a custom definition argument in the form "name:type"
*
******************************************************************************/

_Check_return_
static STATUS
proc_custom_argument(
    _InRef_     P_SS_STRINGC text_in,
    /*filled*/  P_USTR name_out,
    _OutRef_    P_EV_TYPE p_ev_type)
{
    const U32 uchars_n = text_in->size;
    const PC_UCHARS uchars = text_in->uchars;
    U32 uchars_idx = 0;
    U32 outidx = 0;

    /* default to accepting real, string, date */
    *p_ev_type = EM_REA | EM_STR | EM_DAT;

    while((uchars_idx < uchars_n) && (outidx < EV_INTNAMLEN))
    {
        U32 bytes_of_char;
        UCS4 ucs4 = uchars_char_decode_off(uchars, uchars_idx, bytes_of_char);
        SBCHAR ident_ch;

        if(!ucs4_is_sbchar(ucs4))
            break;

        ident_ch = (SBCHAR) ucs4;

        if(0 == uchars_idx)
        {   /* "XID_Start" */
            if(sbchar_isalpha(ident_ch)) /* Latin-1, no remapping (for document portability) */
            {
                ident_ch = sbchar_tolower(ident_ch);
            }
            else if(CH_UNDERSCORE != ident_ch) /* SKS 21jul14 better Excel compatibility */
            {
                break;
            }
        }
        else
        {   /* "XID_Continue" */
            if(sbchar_isalpha(ident_ch))
            {
                ident_ch = sbchar_tolower(ident_ch);
            }
            else if((CH_UNDERSCORE != ident_ch) && !sbchar_isdigit(ident_ch))
            {
                break;
            }
        }

        outidx += uchars_char_encode_off(name_out, EV_INTNAMLEN, outidx, ident_ch);

        uchars_idx += bytes_of_char;
    }

    assert(outidx < BUF_EV_INTNAMLEN);
    PtrPutByteOff(name_out, outidx, CH_NULL);

    status_return(ident_validate(name_out, FALSE));

    /* is there a type following? (actually a list of types is acceptable) */
    if((uchars_idx < uchars_n) && (CH_COLON == PtrGetByteOff(uchars, uchars_idx)))
    {
        EV_TYPE type_list = 0;

        ++uchars_idx;

        while(uchars_idx < uchars_n)
        {
            UCHARZ type_id[BUF_EV_INTNAMLEN]; /* really A7STR */
            EV_TYPE type_res;

            outidx = 0;

            uchars_idx += ss_string_skip_internal_whitespace_uchars(uchars, uchars_n, uchars_idx);

            while((uchars_idx < uchars_n) && (outidx < EV_INTNAMLEN))
            {
                U32 bytes_of_char;
                UCS4 ucs4 = uchars_char_decode_off(uchars, uchars_idx, bytes_of_char);
                SBCHAR type_ch;

                if(!ucs4_is_sbchar(ucs4))
                    break;

                type_ch = (U8) ucs4;

                if(!sbchar_isalpha(type_ch)) /* ASCII, no remapping (type names are never translated) */
                    break;

                type_id[outidx++] = (A7CHAR) /*"C"*/tolower(type_ch);

                assert(1 == bytes_of_char);
                uchars_idx += bytes_of_char;
            }

            type_id[outidx] = CH_NULL;

            if(0 == (type_res = type_name_lookup(ustr_bptr(type_id))))
                return(create_error(EVAL_ERR_ARGCUSTTYPE));

            type_list |= type_res;

            uchars_idx += ss_string_skip_internal_whitespace_uchars(uchars, uchars_n, uchars_idx);

            if((uchars_idx < uchars_n) && (PtrGetByteOff(uchars, uchars_idx) == g_ss_recog_context.function_arg_sep))
            {     
                ++uchars_idx;
                continue;
            }

            break;
        }

        *p_ev_type = type_list;
    }

    assert(uchars_idx == uchars_n);

    return(STATUS_OK);
}

/******************************************************************************
*
* process a function with arguments
*
* --out--
* <0  error encountered
* >=0 number of arguments processed
*
******************************************************************************/

static S32
proc_func(
    /*_In_*/    PC_RPNDEF p_rpndef)
{
    S32 narg;

    /* have we any arguments at all ? */
    if(scan_check_next(NULL) != SYM_OBRACKET)
    {
        /* function allowed to be totally devoid ? */
        if( ((RPN_FN0 == p_rpndef->rpn_type)             /*n_args == 0*/) ||
            ((RPN_FNV == p_rpndef->rpn_type) && (p_rpndef->n_args == -1)) )
            return(0);

        return(set_compile_error(EVAL_ERR_FUNARGS));
    }
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    narg = 0;

    /* loop over function arguments */
    if(scan_check_next(NULL) != SYM_CBRACKET)
    {
        S32 nodep = p_rpndef->fun_parms.nodep;

        for(;;)
        {
            /* argument is non-dependent when nodep == 1 */
            compiler_context.fun_parms.nodep = (nodep == 1);
            if(nodep > 0)
                nodep -= 1;

            proc_func_arg_maybe_blank();
            narg += 1;

            if(scan_check_next(NULL) != SYM_COMMA)
                break;
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
        }
    }

    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_compile_error(EVAL_ERR_CBRACKETS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    /* functions with fixed number of arguments must have exactly the correct number of arguments */
    if(p_rpndef->n_args >= 0)
    {
        if(narg != p_rpndef->n_args)
            return(set_compile_error(EVAL_ERR_FUNARGS));
    }
    /* functions with variable number of arguments must have at least (-p_rpndef->n_args - 1) arguments */
    else if(narg < - (S32) p_rpndef->n_args - 1)
        return(set_compile_error(EVAL_ERR_FUNARGS));

    return(narg);
}

/******************************************************************************
*
* process a function argument which may be blank
*
******************************************************************************/

static void
proc_func_arg_maybe_blank(void)
{
    SYM_INF sym_inf;

    (void) scan_check_next(&sym_inf);

    if(sym_inf.sym_idno == SYM_COMMA ||
       sym_inf.sym_idno == SYM_CBRACKET)
    {
        sym_inf.sym_idno = DATA_ID_BLANK;
        out_idno_format(&sym_inf);
    }
    else
        rec_expr();
}

/******************************************************************************
*
* process a custom definition of the form:
* function("name:type", "arg1:type", ... "argn:type")
*
******************************************************************************/

static S32
proc_func_custom(
    P_EV_HANDLE p_h_custom)
{
    SYM_INF sym_inf;
    ARRAY_INDEX custom_num;
    UCHARZ custom_name[BUF_EV_INTNAMLEN];
    EV_TYPE dummy_type;
    P_EV_CUSTOM p_ev_custom;
    S32 narg;
    STATUS res;

    /* must start with opening bracket */
    if(scan_check_next(NULL) != SYM_OBRACKET)
        return(set_compile_error(EVAL_ERR_FUNARGS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    /* scan first argument which must be custom name */
    if(DATA_ID_STRING != scan_check_next(&sym_inf))
        return(set_compile_error(EVAL_ERR_ARGTYPE));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    /* extract custom name from first argument */
    if(status_fail(
        res = proc_custom_argument(&compiler_context.data_cur.arg.string,
                                   ustr_bptr(custom_name),
                                   &dummy_type)))
        return(set_compile_error(res));

    /* does the custom already exist ? */
    if((custom_num = find_custom_in_list(ev_slr_docno(&compiler_context.ev_slr), ustr_bptr(custom_name))) >= 0)
    {
        p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, custom_num);
        if(!p_ev_custom->flags.undefined)
        {   /* SKS 24sep97 allow redefinition in the same cell */
            if( (ev_slr_col(&p_ev_custom->owner) != ev_slr_col(&compiler_context.ev_slr)) ||
                (ev_slr_row(&p_ev_custom->owner) != ev_slr_row(&compiler_context.ev_slr)) )
                return(set_compile_error(EVAL_ERR_MACROEXISTS));
        }
    }
    /* ensure custom is in list */
    else if(status_fail(custom_num = ensure_custom_in_list(ev_slr_docno(&compiler_context.ev_slr), ustr_bptr(custom_name))))
        return(set_compile_error((STATUS) custom_num));
    else
        p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, custom_num);

    /* send custom identidier home */
    *p_h_custom = p_ev_custom->handle;

    /* store custom name (first arg to function()) in rpn */
    out_string_free(&sym_inf, &compiler_context.data_cur);

    /* now loop over arguments, commas etc. storing
     * the results in the custom definition structure
     * as well as sending them to the rpn for decompilation
     * closing bracket will stop argument scanning
     */

    res = STATUS_OK;
    narg = 0;

    while((scan_check_next(NULL) == SYM_COMMA) && (narg < EV_MAX_ARGS))
    {
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

        if(DATA_ID_STRING != scan_check_next(&sym_inf))
        {
            res = -1;
            break;
        }
        else
            compiler_context.sym_inf.sym_idno = SYM_BLANK;

        /* extract argument names and types */
        if(status_fail(
            res = proc_custom_argument(&compiler_context.data_cur.arg.string,
                                       ustr_bptr(p_ev_custom->args->ustr_arg_name_id[narg]),
                                       &p_ev_custom->args->arg_types[narg])))
            break;

        /* output argument text to rpn */
        out_string_free(&sym_inf, &compiler_context.data_cur);

        ++narg;
    }

    p_ev_custom->args->n = narg;

    if(status_fail(res))
        return(set_compile_error(res));

    /* must finish with closing bracket */
    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_compile_error(EVAL_ERR_CBRACKETS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    /* return number of args to function() */
    return(narg + 1);
}

/******************************************************************************
*
* process a database function
* <datab> := <identd> (range, conditional)
*
******************************************************************************/

static S32
proc_func_dbs(void)
{
    ARRAY_INDEX dbase_start;

    if(scan_check_next(NULL) != SYM_OBRACKET)
        return(set_compile_error(EVAL_ERR_FUNARGS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    proc_func_arg_maybe_blank();

    /* munge the comma */
    if(scan_check_next(NULL) != SYM_COMMA)
        return(set_compile_error(EVAL_ERR_FUNARGS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    out_idno(RPN_FRM_COND);
    dbase_start = out_pos();
    /* reserve space for condition length */
    out_S16(0);

    /* scan conditional */
    rec_expr();

    /* put on the end */
    out_idno(RPN_FRM_END);

    /* update condition length */
    out_skip_set(dbase_start, (S16) (out_pos() - dbase_start));

    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_compile_error(EVAL_ERR_CBRACKETS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    return(0);
}

/******************************************************************************
*
* process an if function
*
* --out--
* <0  error encountered
* >=0 OK (n_args)
*
******************************************************************************/

_Check_return_
static S32 /* n_args encountered */
proc_func_if(
    _OutRef_    P_EV_IDNO which_if)
{
    ARRAY_INDEX skip_post, skip_posf;

    /* assume two or three-part if */
    *which_if = RPN_FNF_IF;

    if(scan_check_next(NULL) != SYM_OBRACKET)
        return(set_compile_error(EVAL_ERR_FUNARGS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    proc_func_arg_maybe_blank();

    if(scan_check_next(NULL) != SYM_COMMA)
    {
        /* check for single argument if */
        if(scan_check_next(NULL) == SYM_CBRACKET)
        {
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            *which_if = RPN_FNF_IFC;
            return(1);
        }
        else
            return(set_compile_error(EVAL_ERR_BADEXPR));
    }
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    skip_post = out_pos();
    out_idno(RPN_FRM_SKIPFALSE);
    /* boolean stack offset = 0 */
    out_byte(0);
    out_S16(0);

    /* get next argument */
    proc_func_arg_maybe_blank();

    out_skip_set(skip_post + sizeof32(EV_IDNO) + 1, (S16) (out_pos() - skip_post));

    skip_posf = out_pos();
    out_idno(RPN_FRM_SKIPTRUE);
    /* boolean stack offset = 1 */
    out_byte(1);
    out_S16(0);

    if(scan_check_next(NULL) != SYM_COMMA)
        return(set_compile_error(EVAL_ERR_BADEXPR));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    /* next argument */
    proc_func_arg_maybe_blank();

    out_skip_set(skip_posf + sizeof32(EV_IDNO) + 1, (S16) (out_pos() - skip_posf));

    if(scan_check_next(NULL) != SYM_CBRACKET)
        return(set_compile_error(EVAL_ERR_CBRACKETS));
    else
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

    /* we did the full three arguments */
    return(3);
}

/******************************************************************************
*
* <array> := <row> {;<row>}
*
******************************************************************************/

static void
rec_array(
    P_S32 x_size,
    P_S32 y_size)
{
    S32 cur_y;

    cur_y = 0;
    *x_size = 0;

    do  {
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        rec_array_row(cur_y, x_size);
        ++cur_y;
    }
    while(scan_check_next(NULL) == SYM_SEMICOLON);

    *y_size = cur_y;
}

/******************************************************************************
*
* <row> := <data> {,<data>}
*
******************************************************************************/

static void
rec_array_row(
    _InVal_     S32 cur_y,
    P_S32 x_size)
{
    S32 cur_x;

    cur_x = 0;

    do  {
        EV_IDNO next;

        compiler_context.sym_inf.sym_idno = SYM_BLANK;

        next = scan_check_next(NULL);

        if(next == SYM_COMMA     ||
           next == SYM_SEMICOLON ||
           next == SYM_CARRAY)
            out_idno(DATA_ID_BLANK);
        else
            rec_expr();
        ++cur_x;
    }
    while(scan_check_next(NULL) == SYM_COMMA);

    /* check array x dimensions, and pad with
     * blanks if necessary
     */
    if(!compiler_context.error)
    {
        if(!cur_y)
            *x_size = cur_x;
        else if(cur_x > *x_size)
            set_compile_error(EVAL_ERR_ARRAYEXPAND);
        else if(cur_x < *x_size)
        {
            S32 i;

            for(i = cur_x; i < *x_size; ++i)
                out_idno(DATA_ID_BLANK);
        }
    }
}

/******************************************************************************
*
* recognise &
*
******************************************************************************/

static void
rec_aterm(void)
{
    SYM_INF sym_inf;

    rec_bterm();

    while(scan_check_next(&sym_inf) == RPN_BOP_AND)
    {
        ARRAY_INDEX skip_pos, skip_parm;

        skip_pos = out_pos();
        out_idno(RPN_FRM_SKIPFALSE);
        /* boolean stack offset = 0 */
        out_byte(0);
        skip_parm = out_pos();
        out_S16(0);

        if(!compiler_context.error)
        {
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            rec_bterm();
            out_idno_format(&sym_inf);

            /* write in skip parameter */
            out_skip_set(skip_parm, (S16) (out_pos() - skip_pos - sizeof32(EV_IDNO)));
        }
    }
    return;
}

/******************************************************************************
*
* recognise =, <>, <, >, <=, >=
*
******************************************************************************/

static void
rec_bterm(void)
{
    SYM_INF sym_inf;

    rec_b2term();

    for(;;)
    {
        switch(scan_check_next(&sym_inf))
        {
        case RPN_REL_EQUALS:
        case RPN_REL_NOTEQUAL:
        case RPN_REL_LT:
        case RPN_REL_GT:
        case RPN_REL_LTEQUAL:
        case RPN_REL_GTEQUAL:
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            break;

        default:
            return;
        }

        rec_b2term();

        out_idno_format(&sym_inf);
    }
}

/******************************************************************************
*
* recognise &&
*
******************************************************************************/

static void
rec_b2term(void)
{
    SYM_INF sym_inf;

    rec_cterm();

    for(;;)
    {
        switch(scan_check_next(&sym_inf))
        {
        case RPN_BOP_CONCATENATE:
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            break;

        default:
            return;
        }

        rec_cterm();

        out_idno_format(&sym_inf);
    }
}

/******************************************************************************
*
* recognise +, -
*
******************************************************************************/

static void
rec_cterm(void)
{
    SYM_INF sym_inf;

    rec_dterm();

    for(;;)
    {
        switch(scan_check_next(&sym_inf))
        {
        case RPN_BOP_PLUS:
        case RPN_BOP_MINUS:
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            break;

        default:
            return;
        }

        rec_dterm();

        out_idno_format(&sym_inf);
    }
}

/******************************************************************************
*
* recognise *, /
*
******************************************************************************/

static void
rec_dterm(void)
{
    SYM_INF sym_inf;

    rec_eterm();

    for(;;)
    {
        switch(scan_check_next(&sym_inf))
        {
        case RPN_BOP_TIMES:
        case RPN_BOP_DIVIDE:
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            break;

        default:
            return;
        }

        rec_eterm();

        out_idno_format(&sym_inf);
    }
}

/******************************************************************************
*
* recognise ^
*
******************************************************************************/

static void
rec_eterm(void)
{
    SYM_INF sym_inf;

    rec_fterm();

    while(scan_check_next(&sym_inf) == RPN_BOP_POWER)
    {
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        rec_fterm();
        out_idno_format(&sym_inf);
    }

    return;
}

/******************************************************************************
*
* recognise expression - works by recursive descent
*
* recognise |
*
******************************************************************************/

static void
rec_expr(void)
{
    SYM_INF sym_inf;

    rec_aterm();

    while(scan_check_next(&sym_inf) == RPN_BOP_OR)
    {
        ARRAY_INDEX skip_pos, skip_parm;

        skip_pos = out_pos();
        out_idno(RPN_FRM_SKIPTRUE);
        /* boolean stack offset = 0 */
        out_byte(0);
        skip_parm = out_pos();
        out_S16(0);

        if(!compiler_context.error)
        {
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            rec_aterm();
            out_idno_format(&sym_inf);

            out_skip_set(skip_parm, (S16) (out_pos() - skip_pos - sizeof32(EV_IDNO)));
        }
    }

    return;
}

/******************************************************************************
*
* recognise unary +, -, !
*
******************************************************************************/

static void
rec_fterm(void)
{
    SYM_INF sym_inf;

    switch(scan_check_next(&sym_inf))
    {
    case RPN_BOP_PLUS:
        sym_inf.sym_idno = RPN_UOP_PLUS;
        goto proc_op;

    case RPN_BOP_MINUS:
        sym_inf.sym_idno = RPN_UOP_MINUS;

        /*FALLTHRU*/

    case RPN_UOP_NOT:
    proc_op:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        rec_fterm();
        out_idno_format(&sym_inf);
        return;

    default:
        rec_gterm();
        return;
    }
}

/******************************************************************************
*
* recognise lterm or brackets
*
******************************************************************************/

static void
rec_gterm(void)
{
    SYM_INF sym_inf;

    if(scan_check_next(&sym_inf) == SYM_OBRACKET)
    {
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        rec_expr();
        if(scan_check_next(NULL) != SYM_CBRACKET)
            set_compile_error(EVAL_ERR_CBRACKETS);
        else
        {
            compiler_context.sym_inf.sym_idno = SYM_BLANK;
            sym_inf.sym_idno = RPN_FRM_BRACKETS;
            out_idno_format(&sym_inf);
        }
    }
    else
        rec_lterm();
}

/******************************************************************************
*
*   <lterm> := number        |
*              date          |
*              slr           |
*              range         |
*              string        |
*              '{'<array>'}' |
*              <function>    |
*              <custom>      |
*              <name>
*
******************************************************************************/

static void
rec_lterm(void)
{
    SYM_INF sym_inf;

    switch(scan_check_next(&sym_inf))
    {
    case DATA_ID_REAL:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_to_rpn(sizeof32(F64), &compiler_context.data_cur.arg.fp);
        break;

    case DATA_ID_LOGICAL:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_byte((U8) compiler_context.data_cur.arg.boolean);
        break;

    case DATA_ID_WORD8:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_byte((U8) (S8) ss_data_get_integer(&compiler_context.data_cur));
        break;

    case DATA_ID_WORD16:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_S16((S16) ss_data_get_integer(&compiler_context.data_cur));
        break;

    case DATA_ID_WORD32:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_to_rpn(sizeof32(S32), &compiler_context.data_cur.arg.integer);
        break;

    case DATA_ID_SLR:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_slr(&compiler_context.data_cur.arg.slr, compiler_context.fun_parms.nodep, 0);
        break;

    case DATA_ID_RANGE:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_range(&compiler_context.data_cur.arg.range, compiler_context.fun_parms.nodep);
        break;

    case DATA_ID_STRING:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_string_free(&sym_inf, &compiler_context.data_cur);
        break;

    case DATA_ID_DATE:
        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        out_idno_format(&sym_inf);
        out_to_rpn(sizeof32(SS_DATE), ss_data_get_date(&compiler_context.data_cur));
        break;

    case SYM_OARRAY:
        {
        S32 x_size, y_size;

        compiler_context.array_nest += 1;

        if(compiler_context.array_nest == 1)
            rec_array(&x_size, &y_size);

        compiler_context.array_nest -= 1;

        if(compiler_context.array_nest)
        {
            set_compile_error(EVAL_ERR_NESTEDARRAY);
            break;
        }

        if(scan_check_next(NULL) != SYM_CARRAY)
        {
            set_compile_error(EVAL_ERR_CARRAY);
            break;
        }

        /* array has been scanned */
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

        /* output array details to compiled expression */
        sym_inf.sym_idno = RPN_FNA_MAKEARRAY;
        out_idno_format(&sym_inf);

        /* output array sizes */
        out_to_rpn(sizeof32(S32), &x_size);
        out_to_rpn(sizeof32(S32), &y_size);

        break;
        }

    case RPN_LCL_ARGUMENT:
        {
        P_U8 ci;

        out_idno_format(&sym_inf);

        ci = compiler_context.ident;
        do
            out_byte(*ci);
        while(*ci++);

        compiler_context.sym_inf.sym_idno = SYM_BLANK;
        break;
        }

    case SYM_TAG:
        {
        EV_DOCNO refto_ev_docno;
        UCHARZ ident[BUF_EV_INTNAMLEN];

        refto_ev_docno = ev_establish_docno_from_docu_name(&compiler_context.docu_name, ev_slr_docno(&compiler_context.ev_slr));
        xstrkpy(ident, sizeof32(ident), compiler_context.ident);

        /* we scanned the tag */
        compiler_context.sym_inf.sym_idno = SYM_BLANK;

        /* lookup id as internal function */
        if(name_is_blank(&compiler_context.docu_name))
        {
            S32 int_func = ev_func_lookup(ustr_bptr(ident));

            if(int_func >= 0)
            {
                sym_inf.sym_idno = (EV_IDNO) int_func;
                func_call(&sym_inf, 0);
                break;
            }
         }

        name_dispose(&compiler_context.docu_name);

        /* is it a custom function call ? */
        if(scan_check_next(NULL) == SYM_OBRACKET)
        {
            ARRAY_INDEX custom_num;
            P_EV_CUSTOM p_ev_custom;

            /* must be a custom - establish reference */
            custom_num = ensure_custom_in_list(refto_ev_docno, ustr_bptr(ident));

            if(status_fail(custom_num))
            {
                set_compile_error((STATUS) custom_num);
                break;
            }

            p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, custom_num);
            sym_inf.sym_idno = RPN_FNM_CUSTOMCALL;
            func_call(&sym_inf, p_ev_custom->handle);
        }
        else
        {
            ARRAY_INDEX name_num;
            PC_EV_NAME p_ev_name;

            /* MRJC changed emphasis 12.12.94 so name must exist first
             * unless it's a custom function document
             * or it's a database field name
             * 28.2.95: allow names if they refer to an external document
             */
            if( ev_doc_check_is_custom(ev_slr_docno(&compiler_context.ev_slr))
                ||
                (ev_slr_docno(&compiler_context.ev_slr) != refto_ev_docno)
                ||
                (ident[0] == CH_QUESTION_MARK) /* database field */ )
                name_num = ensure_name_in_list(refto_ev_docno, ustr_bptr(ident));
            else
                name_num = find_name_in_list(refto_ev_docno, ustr_bptr(ident));

            if(name_num < 0)
            {
                set_compile_error(name_num);
                break;
            }

            p_ev_name = array_ptrc(&name_def_deptable.h_table, EV_NAME, name_num);
            sym_inf.sym_idno = DATA_ID_NAME;
            out_idno_format(&sym_inf);
            out_name(p_ev_name->handle, compiler_context.fun_parms.nodep);

            compiler_context.fun_parms.var = 1;
        }

        break;
        }

    default:
        set_compile_error(EVAL_ERR_BADEXPR);
        break;
    }
}

/******************************************************************************
*
* read an external document reference
* caller strips spaces
*
* --out--
* number of chars read inc []
*
******************************************************************************/

_Check_return_
static S32
recog_extref(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PC_USTR in_ustr)
{
    PC_USTR ustr = in_ustr;
    STATUS status;

    if(PtrGetByte(ustr) == CH_LEFT_SQUARE_BRACKET)
        if(status_ok(status = name_read_ustr(p_docu_name, ustr_AddBytes(ustr, 1), CH_RIGHT_SQUARE_BRACKET)))
            return(1 + (S32) status);

    name_init(p_docu_name);
    return(0);
}

/******************************************************************************
*
* scan an identifier
*
* --out--
* length of in_str recognised, copied (lowercased) to id_out
*
******************************************************************************/

_Check_return_
static STATUS
recog_ident(
    P_U8 id_out,
    _In_z_      PC_USTR in_str)
{
    PC_USTR ci = in_str;
    U32 used_in = 0;
    U32 outidx = 0;

    /* copy identifier to co, ready for lookup */
    while(outidx < EV_INTNAMLEN)
    {
        U32 bytes_of_char;
        UCS4 ucs4 = uchars_char_decode_off(ci, used_in, bytes_of_char);
        SBCHAR ident_ch;

        if(!ucs4_is_sbchar(ucs4))
            break;

        ident_ch = (SBCHAR) ucs4;

        if(0 == used_in)
        {   /* "XID_Start" */
            if(sbchar_isalpha(ident_ch)) /* Latin-1, no remapping (for document portability) */
            {
                ident_ch = sbchar_tolower(ident_ch);
            }
            else if((CH_UNDERSCORE != ident_ch) && (CH_QUESTION_MARK /* database field */ != ident_ch)) /* SKS 21jul14 better Excel compatibility */
            {
                break;
            }
        }
        else
        {   /* "XID_Continue" */
            if(sbchar_isalpha(ident_ch)) /* Latin-1, no remapping (for document portability) */
            {
                ident_ch = sbchar_tolower(ident_ch);
            }
            else if((CH_UNDERSCORE != ident_ch) && !sbchar_isdigit(ident_ch) && (CH_FULL_STOP != ident_ch))
            {
                break;
            }
        }

        outidx += uchars_char_encode_off((P_UCHARS) id_out, EV_INTNAMLEN, outidx, ident_ch);
        assert(outidx <= EV_INTNAMLEN);

        used_in += bytes_of_char;
    }

    if(0 == outidx)
        return(0);

   id_out[outidx] = CH_NULL;

   status_return(ident_validate((PC_USTR) id_out, TRUE));

   return(used_in);
}

/******************************************************************************
*
* read a cell reference
*
* --out--
* =0 no slr found
* >0 number of chars read
*
******************************************************************************/

static S32
recog_slr(
    _OutRef_    P_EV_SLR p_ev_slr,
    _In_z_      PC_USTR in_str)
{
    S32 res, col_temp;
    U32 row_temp;
    PC_USTR pos = in_str;
    P_U8Z epos;

    zero_struct_ptr(p_ev_slr);

    if(PtrGetByte(pos) == CH_PERCENT_SIGN)
    {
        ustr_IncByte(pos);
        p_ev_slr->bad_ref = 1;
    }

    if(PtrGetByte(pos) == CH_DOLLAR_SIGN)
    {
        ustr_IncByte(pos);
        p_ev_slr->abs_col = 1;
    }

    if(((res = stox(pos, &col_temp)) == 0) || (col_temp >= EV_MAX_COL))
        return(0);

    p_ev_slr->col = EV_COL_PACK(col_temp);

    ustr_IncBytes(pos, res);

    if(PtrGetByte(pos) == CH_DOLLAR_SIGN)
    {
        ustr_IncByte(pos);
        p_ev_slr->abs_row = 1;
    }

    /* stop things like + and - being munged */
    if(!sbchar_isdigit(PtrGetByte(pos)))
        return(0);

    row_temp = fast_strtoul((PC_U8Z) pos, &epos);

    if((PC_USTR) epos == pos)
        return(0);

    row_temp -= 1;

    if(row_temp > (U32) EV_MAX_ROW)
        return(0);

    p_ev_slr->row = (ROW) row_temp;

    return(PtrDiffBytesU32(epos, in_str));
}

/******************************************************************************
*
* read an slr or range
*
* --out--
* =0 no slr or range found
* >0 number of chars read
*
******************************************************************************/

_Check_return_
extern S32
recog_slr_range(
    P_SS_DATA p_ss_data,
    _InoutRef_  P_DOCU_NAME p_docu_name,
    _InVal_     EV_DOCNO ev_docno_from,
    _In_z_      PC_USTR in_str)
{
    PC_USTR pos = in_str;
    S32 len = 0;

    S32 len_ext_1 = 0;
    EV_SLR s_slr;
    S32 len_slr_1;

    S32 len_colon = 0;

    S32 len_ext_2 = 0;
    EV_SLR e_slr;
    S32 len_slr_2;

    /* don't check for an external id here if one is supplied by caller */
    if(name_is_blank(p_docu_name))
        if(0 != (len_ext_1 = recog_extref(p_docu_name, pos)))
            ustr_IncBytes(pos, len_ext_1);

    len_slr_1 = recog_slr(&s_slr, pos);

    if(len_slr_1 > 0)
    {
        ustr_IncBytes(pos, len_slr_1);

        s_slr.docno = EV_DOCNO_PACK(ev_establish_docno_from_docu_name(p_docu_name, ev_docno_from));

        /* were we passed an external id ? */
        if(!name_is_blank(p_docu_name))
            s_slr.ext_ref = 1;

        /* allow foreign range input */
        if(CH_COLON == PtrGetByte(pos))
        {
            ustr_IncByte(pos);
            len_colon = 1;
        }

        /* if there is an external id, check for a second one, and allow it if it's the same */
        if(!name_is_blank(p_docu_name))
        {
            DOCU_NAME docu_name_temp;

            if(0 != (len_ext_2 = recog_extref(&docu_name_temp, pos)))
            {
                if(0 == name_compare(p_docu_name, &docu_name_temp, TRUE))
                {
                    ustr_IncBytes(pos, len_ext_2); /* SKS 26may95 after 1.22 - failed to recognise [sht1]a1[sht1]b1 */
                }
                /* else don't increment pos - next SLR test will fail and just return first SLR */

                name_dispose(&docu_name_temp);
            }
        }

        /* check for another SLR to make range */
        len_slr_2 = recog_slr(&e_slr, pos);

        if(len_slr_2 > 0)
        {
            /* set up range (i,i,i,i->i,i,e,e) */
            e_slr.col += 1;
            e_slr.row += 1;
            e_slr.docno = s_slr.docno; /* equivalent UBF */

            if((s_slr.col < e_slr.col) && (s_slr.row < e_slr.row))
            {
                p_ss_data->arg.range.s = s_slr;
                p_ss_data->arg.range.e = e_slr;
                ss_data_set_data_id(p_ss_data, DATA_ID_RANGE);

                len = (len_ext_1 + len_slr_1) + len_colon + (len_ext_2 + len_slr_2); /* all components for range */
            }
            /* else bad range - ignore everything here */
        }
        else
        {
            if((CH_LEFT_PARENTHESIS == PtrGetByte(pos)) || (sbchar_isalpha(PtrGetByte(pos))))
            {   /* probably a function name now we are more relaxed */
                /* treat as bad SLR - ignore everything here */
            }
            else
            {   /* stick with the SLR we obtained */
                p_ss_data->arg.slr = s_slr;
                ss_data_set_data_id(p_ss_data, DATA_ID_SLR);

                len = len_ext_1 + len_slr_1; /* all components for SLR */
            }
        } /*fi*/

    }
    /* else bad SLR - ignore everything here */
    /*fi*/

    return(len);
}

/******************************************************************************
*
* return information about the next symbol - scan a new symbol if we have used up the stored one
*
******************************************************************************/

_Check_return_
static EV_IDNO
scan_check_next(
    _Out_opt_   P_SYM_INF p_sym_inf)
{
    if(compiler_context.sym_inf.sym_idno == SYM_BLANK)
    {
        compiler_context.err_pos = compiler_context.ip_pos.ustr;
        scan_next_symbol();
        compiler_context.n_scanned += 1;
    }

    if(NULL != p_sym_inf)
        *p_sym_inf = compiler_context.sym_inf;

    return(compiler_context.sym_inf.sym_idno);
}

/******************************************************************************
*
* scan next symbol from the text input
*
******************************************************************************/

static void
scan_next_symbol(void)
{
    STATUS res;

    /* accumulate line separators and spaces (in roughly the same order as decompiler will output) */
    compiler_context.sym_inf.sym_equals = 0;
    compiler_context.sym_inf.sym_cr = 0;
    compiler_context.sym_inf.sym_space = 0;

    for(;;)
    {
        const U8 u8 = *compiler_context.ip_pos.p_u8;

        if((LF == u8) || (CR == u8))
        {
            ++compiler_context.ip_pos.p_u8;
            if(*compiler_context.ip_pos.p_u8 == (u8 ^ (LF ^ CR))) /* consume a CR,LF or LF,CR pair as one */
                ++compiler_context.ip_pos.p_u8;
            ++compiler_context.sym_inf.sym_cr;
            compiler_context.sym_inf.sym_space = 0; /* reset so trailing spaces on one line don't get counted as leading spaces on the next */
            continue;
        }

        if(CH_SPACE == u8)
        {
            ++compiler_context.ip_pos.p_u8;
            ++compiler_context.sym_inf.sym_space;
            continue;
        }

        /* look for EXCEL type equals sign */
        if(CH_EQUALS_SIGN == u8)
        {   /* NB only before first symbol */
            if(0 == compiler_context.n_scanned)
            {
                ++compiler_context.ip_pos.p_u8;
                /*compiler_context.sym_inf.sym_equals = 0;*/ /* and then ignore it! */
                continue;
            }
        }

        break; /* none of the above matched on this pass */
    }

    /* try to scan a data item */
    if( (*compiler_context.ip_pos.p_u8 != CH_MINUS_SIGN__BASIC) &&
        (*compiler_context.ip_pos.p_u8 != CH_PLUS_SIGN)  )
    {
        /* check for date and/or time */
        /*SKS 12apr95 moved from below ss_recog_number call to be consistent with foreign ss_recog_constant recog sequence changes */
        if((res = ss_recog_date_time(&compiler_context.data_cur, compiler_context.ip_pos.ustr)) > 0)
            compiler_context.ip_pos.p_u8 += res;
        /* check for number */
        else if((res = ss_recog_number(&compiler_context.data_cur, compiler_context.ip_pos.ustr)) > 0)
            compiler_context.ip_pos.p_u8 += res;
        /* read a string ? */
        else if((res = ss_recog_string(&compiler_context.data_cur, compiler_context.ip_pos.ustr)) > 0)
            compiler_context.ip_pos.p_u8 += res;
        /* check for Logical */
        else if((res = ss_recog_logical(&compiler_context.data_cur, compiler_context.ip_pos.ustr)) > 0)
            compiler_context.ip_pos.p_u8 += res;

        if(res > 0)
        {
            compiler_context.sym_inf.sym_idno = ss_data_get_data_id(&compiler_context.data_cur);
            return;
        }
    }

    /* clear external ref buffer */
    name_dispose(&compiler_context.docu_name);

    if((res = recog_extref(&compiler_context.docu_name, compiler_context.ip_pos.ustr)) > 0)
        compiler_context.ip_pos.p_u8 += res;

    /* check for slr or range */
    if((res = recog_slr_range(&compiler_context.data_cur,
                              &compiler_context.docu_name,
                              ev_slr_docno(&compiler_context.ev_slr),
                              compiler_context.ip_pos.ustr)) > 0)
    {
        compiler_context.ip_pos.p_u8 += res;
        compiler_context.fun_parms.var = 1;
        compiler_context.sym_inf.sym_idno = ss_data_get_data_id(&compiler_context.data_cur);
        if(!name_is_blank(&compiler_context.docu_name))
            name_dispose(&compiler_context.docu_name);
        return;
    }

    /* check for special symbols */
    switch(*compiler_context.ip_pos.p_u8)
    {
    case CH_NULL:
        compiler_context.sym_inf.sym_idno = RPN_FRM_END;
        return;

    case CH_LEFT_PARENTHESIS:
        ++compiler_context.ip_pos.p_u8;
        compiler_context.sym_inf.sym_idno = SYM_OBRACKET;
        return;

    case CH_RIGHT_PARENTHESIS:
        ++compiler_context.ip_pos.p_u8;
        compiler_context.sym_inf.sym_idno = SYM_CBRACKET;
        return;

    case CH_LEFT_CURLY_BRACKET:
        ++compiler_context.ip_pos.p_u8;
        compiler_context.sym_inf.sym_idno = SYM_OARRAY;
        return;

    case CH_RIGHT_CURLY_BRACKET:
        ++compiler_context.ip_pos.p_u8;
        compiler_context.sym_inf.sym_idno = SYM_CARRAY;
        return;

    /* local name */
    case CH_COMMERCIAL_AT:
        ++compiler_context.ip_pos.p_u8;
        if((res = recog_ident(compiler_context.ident, compiler_context.ip_pos.ustr)) <= 0)
        {
            set_compile_error(res ? res : EVAL_ERR_BADIDENT);
            return;
        }

        compiler_context.ip_pos.p_u8 += res;
        compiler_context.sym_inf.sym_idno = RPN_LCL_ARGUMENT;
        return;

    default:
        break;
    }

    if(PtrGetByte(compiler_context.ip_pos.ustr) == g_ss_recog_context.function_arg_sep)
    {
        ++compiler_context.ip_pos.p_u8;
        compiler_context.sym_inf.sym_idno = SYM_COMMA;
        return;
    }

    if(PtrGetByte(compiler_context.ip_pos.ustr) == g_ss_recog_context.array_row_sep)
    {
        ++compiler_context.ip_pos.p_u8;
        compiler_context.sym_inf.sym_idno = SYM_SEMICOLON;
        return;
    }

    /* scan an id (Latin-1, no remapping (for document portability)) */
    if(sbchar_isalpha(*compiler_context.ip_pos.p_u8)
       ||
       (*compiler_context.ip_pos.p_u8 == CH_QUESTION_MARK) /* database field */
       ||
       !name_is_blank(&compiler_context.docu_name))
    {
        if((res = recog_ident(compiler_context.ident, compiler_context.ip_pos.ustr)) <= 0)
        {
            set_compile_error(res ? res : EVAL_ERR_BADIDENT);
            name_dispose(&compiler_context.docu_name);
            return;
        }

        compiler_context.ip_pos.p_u8 += res;
        compiler_context.sym_inf.sym_idno = SYM_TAG;
        return;
    }

    /* must be an operator or a dud */
    if(sbchar_ispunct(*compiler_context.ip_pos.p_u8))
    {
        U8 cur_ch = *compiler_context.ip_pos.p_u8;
        P_U8 co = compiler_context.ident;
        S32 count = 0;

        while(count < EV_INTNAMLEN)
        {
            *co++  = cur_ch;
            ++count;

            cur_ch = *(++compiler_context.ip_pos.p_u8);

            switch(cur_ch)
            {
            case CH_GREATER_THAN_SIGN:
            case CH_LESS_THAN_SIGN:
            case CH_EQUALS_SIGN:
                continue;

            default:
                break;
            }

            break;
        }

        *co = CH_NULL;

        {
        S32 i = ev_func_lookup((PC_USTR) compiler_context.ident);
        compiler_context.sym_inf.sym_idno = (EV_IDNO) i;
        if(i >= 0)
            return;
        } /*block*/
    }

    set_compile_error(EVAL_ERR_BADEXPR);
}

/******************************************************************************
*
* set compiler error condition
*
******************************************************************************/

/*ncr*/
static STATUS
set_compile_error(
    _InVal_     STATUS status)
{
    compiler_context.sym_inf.sym_idno = SYM_BAD;

    if(0 == compiler_context.error)
        compiler_context.error = status;

    return(STATUS_FAIL);
}

/* end of ev_comp.c */
