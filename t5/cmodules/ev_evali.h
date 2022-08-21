/* ev_evali.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Local header for evaluator */

/* MRJC January 1991 */

#ifndef __ev_evali_h
#define __ev_evali_h

#ifndef          __ev_eval_h
#include "cmodules/ev_eval.h"
#endif

#define ev_docno_from_p_docu(p_docu) ( (EV_DOCNO) \
    docno_from_p_docu(p_docu) )

#define p_docu_from_ev_docno(ev_docno) \
    p_docu_from_docno((DOCNO) ev_docno)

/*
named resources
*/

typedef struct EV_NAME
{
    EV_HANDLE handle;                   /* internal id allocated to name */
    UCHARZ ustr_name_id[BUF_EV_INTNAMLEN]; /* name of resource */
    EV_SLR owner;                       /* document that owns name definition */
    EV_DATA def_data;                   /* data defined by name */
    DEF_FLAGS flags;
    P_USTR ustr_description;            /* description of name data (owned by us) */
}
EV_NAME, * P_EV_NAME; typedef const EV_NAME * PC_EV_NAME;

/*
user defined custom functions
*/

typedef struct EV_CUSTOM_ARGS
{
    S32 n;                              /* number of arguments for custom */
    UCHARZ ustr_arg_name_id[EV_MAX_ARGS][BUF_EV_INTNAMLEN];
    EV_TYPE arg_types[EV_MAX_ARGS];
}
EV_CUSTOM_ARGS, * P_EV_CUSTOM_ARGS;

typedef struct EV_CUSTOM
{
    EV_HANDLE handle;                   /* internal id for custom */
    UCHARZ ustr_custom_id[BUF_EV_INTNAMLEN]; /* custom name */
    EV_SLR owner;                       /* document/slr of custom */
    DEF_FLAGS flags;
    P_EV_CUSTOM_ARGS args;
}
EV_CUSTOM, * P_EV_CUSTOM, ** P_P_EV_CUSTOM; typedef const EV_CUSTOM * PC_EV_CUSTOM;

/*
types for exec routines
*/

#define exec_func_ignore_parms() (void) ( \
    IGNOREPARM(args), \
    IGNOREPARM_InVal_(n_args), \
    IGNOREPARM_InoutRef_(p_ev_data_res), \
    IGNOREPARM_InRef_(p_cur_slr) )

/* symbol information */

typedef struct SYM_INF
{
    EV_IDNO did_num;            /* id number of symbol */
    U8 sym_cr;                  /* accumulated CR/LFs before symbol */
    U8 sym_space;               /* accumulated spaces before symbol */
    U8 sym_equals;              /* = before symbol */
}
SYM_INF, * P_SYM_INF;

typedef struct FUN_PARMS
{
    UBF ex_type         : 3;            /* exec, database, control, lookup */
    UBF no_exec         : 4;            /* parameter for functions with no exec routine */
    UBF control         : 4;            /* custom function control statements */
    UBF var             : 1;            /* function makes RPN variable */
    UBF nodep           : 2;            /* number of the function argument needs no dependency (will need uref though) */
    UBF self            : 1;            /* generate self dependency */
    UBF load_recalc     : 1;            /* recalc on load */
    UBF event_type      : 2;            /* event type */
#if defined(EV_IDNO_U16)
    UBF rpn_alias       : 10;           /* RPN number alias for (e.g.) database functions - an EV_IDNO MUST fit! */
    /*            =28 */
    UBF _spare          : 32-28;
#else
    UBF rpn_alias       : 8;            /* RPN number alias for (e.g.) database functions - an EV_IDNO MUST fit! */
    /*            =26 */
    UBF _spare          : 32-26;
#endif
}
FUN_PARMS;

/*
exec types
*/

enum EXEC_TYPES
{
    EXEC_EXEC = 0,
    EXEC_DBASE,
    EXEC_CONTROL,
    EXEC_LOOKUP,
    EXEC_ALERT,
    EXEC_SETVALUE,
    EXEC_ARRAY_RANGE
};

/*
parameters for non_exec functions
*/

 enum EXEC_CONTROL
{
    CONTROL_GOTO,
    CONTROL_RESULT,
    CONTROL_WHILE,
    CONTROL_ENDWHILE,
    CONTROL_REPEAT,
    CONTROL_UNTIL,
    CONTROL_FOR,
    CONTROL_NEXT,
    CONTROL_BREAK,
    CONTROL_CONTINUE,
    CONTROL_IF,
    CONTROL_ELSE,
    CONTROL_ELSEIF,
    CONTROL_ENDIF
};

 enum EXEC_DBASE
{
    DBASE_DAVG,
    DBASE_DCOUNT,
    DBASE_DCOUNTA,
    DBASE_DMAX,
    DBASE_DMIN,
    DBASE_DPRODUCT,
    DBASE_DSTD,
    DBASE_DSTDP,
    DBASE_DSUM,
    DBASE_DVAR,
    DBASE_DVARP
};

enum EXEC_LOOKUP
{
    LOOKUP_LOOKUP,
    LOOKUP_HLOOKUP,
    LOOKUP_VLOOKUP,
    LOOKUP_MATCH
};

enum EXEC_ARRAY_RANGE
{
    ARRAY_RANGE_AND = 1,
    ARRAY_RANGE_AVEDEV,
    ARRAY_RANGE_AVERAGE,
    ARRAY_RANGE_AVERAGEA,
    ARRAY_RANGE_COUNT,
    ARRAY_RANGE_COUNTA,
    ARRAY_RANGE_COUNTBLANK,
    ARRAY_RANGE_DEVSQ,
    ARRAY_RANGE_GEOMEAN,
    ARRAY_RANGE_HARMEAN,
    ARRAY_RANGE_IRR,
    ARRAY_RANGE_KURT,
    ARRAY_RANGE_MAX,
    ARRAY_RANGE_MAXA,
    ARRAY_RANGE_MEDIAN,
    ARRAY_RANGE_MIN,
    ARRAY_RANGE_MINA,
    ARRAY_RANGE_MIRR,
    ARRAY_RANGE_MULTINOMIAL,
    ARRAY_RANGE_NPV,
    ARRAY_RANGE_OR,
    ARRAY_RANGE_PRODUCT,
    ARRAY_RANGE_SKEW,
    ARRAY_RANGE_SKEW_P,
    ARRAY_RANGE_STD,
    ARRAY_RANGE_STDEVA,
    ARRAY_RANGE_STDEVPA,
    ARRAY_RANGE_STDP,
    ARRAY_RANGE_SUM,
    ARRAY_RANGE_SUMSQ,
    ARRAY_RANGE_VAR,
    ARRAY_RANGE_VARA,
    ARRAY_RANGE_VARP,
    ARRAY_RANGE_VARPA,
    ARRAY_RANGE_XOR
};

/*
table of RPN items
*/

typedef struct RPNDEF
{
    EV_IDNO rpn_type;
    S8 n_args;
    S8 max_nargs;
    FUN_PARMS fun_parms;
    U8 category;
    U8 object_id;
    U16 object_table_index;
    PC_EV_TYPE arg_types;  /* pointer to argument flags */
#if CHECKING
    EV_IDNO own_did_num; /* entry's own index for cross-check */
#endif
}
RPNDEF; typedef const RPNDEF * PC_RPNDEF;

/*
RPN engine data block
*/

typedef struct RPNSTATE
{
    PC_EV_CELL p_ev_cell;   /* pointer to cell */
    EV_IDNO num;            /* current RPN token number */
#if !defined(EV_IDNO_U32)
    U8 _spare[4-sizeof(EV_IDNO)];
#endif
    P_U8 pos;               /* current position in RPN string */
    P_U8 p_rpn_start;       /* start of RPN string */
}
RPNSTATE, * P_RPNSTATE;

/*
comparison macros
*/

/*
ranges equal ?
*/

#define ev_range_equal(p_ev_range1, p_ev_range2) (        \
    (p_ev_range2)->s.docno == (p_ev_range1)->s.docno &&   \
    (p_ev_range2)->s.col == (p_ev_range1)->s.col &&       \
    (p_ev_range2)->s.row == (p_ev_range1)->s.row &&       \
    (p_ev_range2)->e.col == (p_ev_range1)->e.col &&       \
    (p_ev_range2)->e.row == (p_ev_range1)->e.row          )

/*
range a subset of another ?
*/

#define ev_range_in_range(p_ev_range1, p_ev_range2) (     \
    (p_ev_range2)->s.docno == (p_ev_range1)->s.docno &&   \
    (p_ev_range2)->s.col >= (p_ev_range1)->s.col &&       \
    (p_ev_range2)->s.row >= (p_ev_range1)->s.row &&       \
    (p_ev_range2)->e.col <= (p_ev_range1)->e.col &&       \
    (p_ev_range2)->e.row <= (p_ev_range1)->e.row          )

/*
ranges overlap ?
*/

#define ev_range_overlap(p_ev_range1, p_ev_range2) (      \
    (p_ev_range2)->s.docno == (p_ev_range1)->s.docno &&   \
    (p_ev_range2)->s.col <  (p_ev_range1)->e.col &&       \
    (p_ev_range2)->s.row <  (p_ev_range1)->e.row &&       \
    (p_ev_range2)->e.col >  (p_ev_range1)->s.col &&       \
    (p_ev_range2)->e.row >  (p_ev_range1)->s.row          )

/*
cell contained in range ?
*/

#define ev_slr_in_range(p_ev_range, p_ev_slr) (   \
    (p_ev_slr)->docno == (p_ev_range)->s.docno && \
    (p_ev_slr)->col >= (p_ev_range)->s.col &&     \
    (p_ev_slr)->row >= (p_ev_range)->s.row &&     \
    (p_ev_slr)->col <  (p_ev_range)->e.col &&     \
    (p_ev_slr)->row <  (p_ev_range)->e.row        )

/*
flags about global trees
*/

typedef struct GLOBAL_FLAGS
{
    UBF blown : 1;
    UBF lock : 1;
}
GLOBAL_FLAGS;

/*
modified list
*/

typedef struct NEEDS_RECALC
{
    EV_SLR ev_slr;
}
NEEDS_RECALC, * P_NEEDS_RECALC;

/*
recalc todo list
*/

typedef struct TODO_ENTRY
{
    EV_SLR slr;                 /* slr to be done */
    S32 node_distance;          /* distance from modified node */
}
TODO_ENTRY, * P_TODO_ENTRY;

/*
definition of the structure of various dependency lists
*/

typedef struct RANGE_USE
{
    EV_RANGE range_to;          /* range points to */
    EV_SLR slr_by;              /* cell containing range */
    U8 by_index;                /* index into cell by table */
    DEF_FLAGS flags;
    S32 node_distance;
}
RANGE_USE, * P_RANGE_USE; typedef const RANGE_USE * PC_RANGE_USE;

typedef U16 RANGE_INDEX; typedef RANGE_INDEX * P_RANGE_INDEX; typedef const RANGE_INDEX * PC_RANGE_INDEX;

#define p_range_use_from_p_ss_doc(p_ss_doc, ix) \
    array_ptr(&(p_ss_doc)->range_table.h_table, RANGE_USE, (ix))

typedef struct SLR_USE
{
    EV_SLR slr_to;              /* reference points to */
    EV_SLR slr_by;              /* cell containing reference */
    U8 by_index;                /* index into cell by table */
    DEF_FLAGS flags;
}
SLR_USE, * P_SLR_USE; typedef const SLR_USE * PC_SLR_USE;

#define p_slr_use_from_p_ss_doc(p_ss_doc, ix) \
    array_ptr(&(p_ss_doc)->slr_table.h_table, SLR_USE, (ix))

/*
structure of name use table
*/

typedef struct NAME_USE
{
    EV_NAME_REF name_to;
    EV_SLR slr_by;               /* cell containing use of name */
    DEF_FLAGS flags;
}
NAME_USE, * P_NAME_USE; typedef const NAME_USE * PC_NAME_USE;

/*
structure of custom function use table
*/

typedef struct CUSTOM_USE
{
    EV_HANDLE custom_to;        /* reference to custom.. */
    EV_SLR slr_by;              /* cell containing reference to custom */
    DEF_FLAGS flags;
}
CUSTOM_USE, * P_CUSTOM_USE; typedef const CUSTOM_USE * PC_CUSTOM_USE;

/*
event uses
*/

typedef struct EVENT_USE
{
    EVENT_TYPE event_type;
    EV_SLR slr_by;
    DEF_FLAGS flags;
}
EVENT_USE, * P_EVENT_USE; typedef const EVENT_USE * PC_EVENT_USE;

/*
array scanning data
*/

typedef struct ARRAY_SCAN_BLOCK
{
    EV_DATA ev_data;
    S32 x_size;
    S32 y_size;
    S32 x_pos;
    S32 y_pos;
}
ARRAY_SCAN_BLOCK, * P_ARRAY_SCAN_BLOCK;

/*
range scanning data
*/

typedef struct RANGE_SCAN_BLOCK
{
    EV_RANGE range;
    EV_COL col_size;
    EV_ROW row_size;
    EV_SLR pos;
    EV_SLR slr_of_result;
}
RANGE_SCAN_BLOCK, * P_RANGE_SCAN_BLOCK;

typedef struct STAT_BLOCK
{
    enum EXEC_ARRAY_RANGE exec_array_range_id;
    EV_DATA running_data;
    S32 count;
    S32 count_a;
    S32 count_blank;

    /* AVEDEV (two pass) */
    F64 mean;
    S32 pass;

    /* DEVSQ/STD/STDP/VAR/VARP */
    F64 sum_x2;
    F64 shift_value;

    /* MEDIAN (two pass - also uses pass in AVEDEV data) */
    EV_DATA statistics_array;

    /* KURT/SKEW (also uses mean in AVEDEV data) */
    F64 M2, M3, M4; /* running sums of powers of differences from the mean */

    /* MULTINOMIAL */
    EV_DATA multinomial_product;

    /* NPV */
    F64 npv_rate;
    F64 last_npv_rate;

    /* IRR (multiple passes - also uses NPV data) */
    F64 r;
    F64 last_r; /* from previous iteration */
    F64 last_npv; /* ditto */
    S32 iteration_count;

    /* MIRR (also uses NPV data) */
    EV_DATA running_data_positive;
    F64 last_npv_rate_positive;
    F64 npv_rate_positive;

#if CHECKING /* has been useful for debug */
    EV_IDNO _function_id;
#if !defined(EV_IDNO_U32)
    U8 _spare[4-sizeof(EV_IDNO)];
#endif
#endif
}
STAT_BLOCK, * P_STAT_BLOCK;

typedef struct LOOKUP_BLOCK
{
    EV_DATA target_data;
    EV_DATA result_data;
    S32 lookup_id;
    S32 match;
    S32 ix;
    S32 n_found;
    S32 ix_match;
    BOOL all_occs;
    BOOL lookup_horz;
}
LOOKUP_BLOCK, * P_LOOKUP_BLOCK;

typedef struct ARRAY_RANGE_BLOCK
{
    EV_DATA args[EV_MAX_ARGS];
    S32 n_args;
    S32 arg_ix;
    STAT_BLOCK stat_block;
    EV_IDNO type;
#if !defined(EV_IDNO_U32)
    U8 _spare[4 - sizeof(EV_IDNO)];
#endif
    RANGE_SCAN_BLOCK range_scan_block;
    ARRAY_SCAN_BLOCK array_scan_block;
}
ARRAY_RANGE_BLOCK, * P_ARRAY_RANGE_BLOCK;

/*
evaluator stack definition
*/

typedef struct EVAL_BLOCK
{
    S32 offset;
    EV_SLR slr;
    P_EV_CELL p_ev_cell;
    BOOL in_dbase;
    S32 dbase_stack;
}
EVAL_BLOCK, * P_EVAL_BLOCK;

typedef struct STACK_IN_CALC
{
    EVAL_BLOCK eval_block;
    S32 travel_res;
    EV_DATA result_data;
    BOOL did_calc;
}
STACK_IN_CALC, * P_STACK_IN_CALC;

typedef struct STACK_IN_EVAL
{
    U32 stack_offset;
}
STACK_IN_EVAL, * P_STACK_IN_EVAL;

typedef struct STACK_DATA_ITEM
{
    EV_DATA data;
}
STACK_DATA_ITEM;

typedef struct STACK_CONTROL_LOOP
{
    S32 control_type;
    EV_SLR origin_slot;
    F64 step;
    F64 end;
    EV_HANDLE h_name;
}
STACK_CONTROL_LOOP, * P_STACK_CONTROL_LOOP;

typedef struct STACK_DBASE
{
    EV_SLR dbase_slot;
    EV_DATA arg0;
    S16 cond_pos;
    S32 ix;
    S32 iy;
    P_STAT_BLOCK p_stat_block;
}
STACK_DBASE, * P_STACK_DBASE;

typedef struct STACK_LOOKUP
{
    EV_DATA arg1;
    EV_DATA arg2;
    P_LOOKUP_BLOCK p_lookup_block;
}
STACK_LOOKUP, * P_STACK_LOOKUP;

typedef struct STACK_EXECUTING_CUSTOM
{
    S32 stack_base;
    S32 n_args;
    EV_HANDLE custom_num;
    EV_SLR custom_slot;
    EV_SLR next_slot;
    S32 x_pos;
    S32 y_pos;
    BOOL elseif;
    BOOL in_array;
}
STACK_EXECUTING_CUSTOM, * P_STACK_EXECUTING_CUSTOM;

typedef struct STACK_PROCESSING_ARRAY
{
    S32 stack_base;
    S32 n_args;
    OBJECT_ID object_id;
    U32 object_table_index;
    S32 x_pos;
    S32 y_pos;
    S32 type_count;
    PC_EV_TYPE arg_types;
}
STACK_PROCESSING_ARRAY, * P_STACK_PROCESSING_ARRAY;

typedef struct STACK_ALERT_INPUT
{
    S32 alert_input;
    S32 name_id_len;
    UCHARZ ustr_name_id[BUF_EV_INTNAMLEN];
}
STACK_ALERT_INPUT, * P_STACK_ALERT_INPUT;

typedef struct STACK_SETVALUE
{
    EV_DATA ev_data_arg_0;
    EV_DATA ev_data_arg_1;
    S32 n_args;
    S32 ix_x;
    S32 ix_y;
}
STACK_SETVALUE, * P_STACK_SETVALUE;

typedef struct STACK_ARRAY_RANGE
{
    P_ARRAY_RANGE_BLOCK p_array_range_block;
}
STACK_ARRAY_RANGE, * P_STACK_ARRAY_RANGE;

/*
flags for stack entry
*/

typedef struct STACK_FLAGS
{
    UBF type : 5;               /* type of stack entry */
    UBF calcederror : 1;        /* error has been stored in cell */
    UBF inmacro : 1;            /* we are inside a custom */
    UBF circsource : 1;         /* cell is source of circular reference */
}
STACK_FLAGS;

typedef struct STACK_ENTRY
{
    EV_SLR slr;                 /* cell to which entry relates */
    STACK_FLAGS stack_flags;

    union STACK_ENTRY_DATA
    {
        STACK_DATA_ITEM stack_data_item;
        STACK_IN_CALC stack_in_calc;
        STACK_IN_EVAL stack_in_eval;
        STACK_CONTROL_LOOP stack_control_loop;
        STACK_DBASE stack_dbase;
        STACK_LOOKUP stack_lookup;
        STACK_EXECUTING_CUSTOM stack_executing_custom;
        STACK_PROCESSING_ARRAY stack_processing_array;
        STACK_ALERT_INPUT stack_alert_input;
        STACK_SETVALUE stack_setvalue;
        STACK_ARRAY_RANGE stack_array_range;
    } data;
}
STACK_ENTRY, * P_STACK_ENTRY;

#define STACK_INC 32U /* size of stack increments */

enum STACK_TYPES
{
    DATA_ITEM,
    CALC_SLOT,
    IN_EVAL,
    END_CALC,
    CONTROL_LOOP,
    DBASE_FUNCTION,
    DBASE_CALC,
    LOOKUP_HAPPENING,
    MACRO_COMPLETE,
    EXECUTING_MACRO,
    PROCESSING_ARRAY,
    SETVALUE,
    ALERT_INPUT,
    NEXT_SLOT,
    ARRAY_RANGE_ARG,
    ARRAY_RANGE,
    INTERNAL_ERROR
};

enum RECALC_STATES
{
    SAME_STATE = 0,
    NEW_STATE
};

/*
ev_comp.c external functions
*/

_Check_return_
extern STATUS
ident_validate(
    _In_z_      PC_USTR ident);

_Check_return_
extern S32
recog_slr_range(
    P_EV_DATA p_ev_data,
    _InoutRef_  P_DOCU_NAME p_docu_name,
    _InVal_     EV_DOCNO ev_docno_from,
    _In_z_      PC_USTR in_str);

/*
ev_dcom.c external functions
*/

/*
ev_eval.c
*/

extern P_STACK_ENTRY stack_base;

extern U32 stack_offset;

extern U32 stack_size;

extern GLOBAL_FLAGS global_flags;

/*
ev_eval.c external functions
*/

extern void
array_range_stat_block_init(
    _OutRef_    P_STAT_BLOCK p_stat_block,
    _OutRef_opt_ P_S32 p_n_args,
    _OutRef_opt_ P_S32 p_arg_ix,
    _InVal_     EV_IDNO function_id,
    P_EV_DATA p_args[],
    _InVal_     S32 arg_count);

extern void
dbase_array_index(
    P_EV_DATA p_ev_data_out,
    P_EV_DATA p_ev_data_in,
    P_STACK_DBASE p_stack_dbase,
    _InVal_     EV_TYPE ev_type);

extern void
lookup_block_init(
    _OutRef_    P_LOOKUP_BLOCK p_lookup_block,
    _InRef_opt_ PC_EV_DATA p_ev_data_target,
    _InVal_     S32 lookup_id,
    _InVal_     S32 match,
    _InVal_     BOOL all_occs);

_Check_return_
extern S32
process_control(
    _InVal_     S32 action,
    P_EV_DATA args[],
    _InVal_     S32 n_args,
    _InVal_     S32 eval_stack_base);

_Check_return_
extern P_STACK_ENTRY
stack_back_search(
    _In_        S32 stack_level,
    _InVal_     S32 entry_type);

extern void
stack_free(void);

_Check_return_
extern STATUS
stack_grow(
    _InVal_     U32 n_spaces);

_Check_return_
static inline STATUS
stack_check_n(
    _InVal_     U32 n_spaces)
{
    /* can current stack accomodate n_spaces more slots? remember that stack_base[stack_offset] is valid TOS */
    if(stack_offset + n_spaces < stack_size)
        return(STATUS_OK);

    return(stack_grow(n_spaces));
}

extern void
stack_set(
    _InVal_     U32 stack_level);

extern void
stack_zap(void);

/*
0=bottom element
1=next element up etc
*/

#define stack_index_ptr_data(base_offset, offset) ( \
    &stack_base[(base_offset)-(offset)].data.stack_data_item.data )

#define stack_offset(p_stack_entry) ( \
    (S32) ((p_stack_entry) - stack_base) )

/*
ev_exec.c external functions
*/

PROC_EXEC_PROTO(c_uplus);
PROC_EXEC_PROTO(c_uminus);
PROC_EXEC_PROTO(c_not);

PROC_EXEC_PROTO(c_and);
PROC_EXEC_PROTO(c_mul);
PROC_EXEC_PROTO(c_add);
PROC_EXEC_PROTO(c_sub);
PROC_EXEC_PROTO(c_div);
PROC_EXEC_PROTO(c_power);
PROC_EXEC_PROTO(c_or);

PROC_EXEC_PROTO(c_bop_concatenate);

PROC_EXEC_PROTO(c_eq);
PROC_EXEC_PROTO(c_gt);
PROC_EXEC_PROTO(c_gteq);
PROC_EXEC_PROTO(c_lt);
PROC_EXEC_PROTO(c_lteq);
PROC_EXEC_PROTO(c_neq);

PROC_EXEC_PROTO(c_if);

/*
ev_func.c external functions
*/

/*
lookup and reference functions
*/

PROC_EXEC_PROTO(c_address);
PROC_EXEC_PROTO(c_choose);
PROC_EXEC_PROTO(c_col);
PROC_EXEC_PROTO(c_cols);
/* NO c_hlookup: LOOKUP_HLOOKUP */
PROC_EXEC_PROTO(c_index);
/* NO c_lookup: LOOKUP_LOOKUP */
/* NO c_match: LOOKUP_MATCH */
PROC_EXEC_PROTO(c_odf_index);
PROC_EXEC_PROTO(c_row);
PROC_EXEC_PROTO(c_rows);
/* NO c_vlookup: LOOKUP_VLOOKUP */

/*
miscellaneous functions
*/

PROC_EXEC_PROTO(c_command);
/* NO c_countblank: ARRAY_RANGE_COUNTBLANK */
PROC_EXEC_PROTO(c_current_cell);
PROC_EXEC_PROTO(c_deref);
PROC_EXEC_PROTO(c_doubleclick);
PROC_EXEC_PROTO(c_even);
PROC_EXEC_PROTO(c_false);
PROC_EXEC_PROTO(c_flip);
PROC_EXEC_PROTO(c_isblank);
PROC_EXEC_PROTO(c_iserr);
PROC_EXEC_PROTO(c_iserror);
PROC_EXEC_PROTO(c_iseven);
PROC_EXEC_PROTO(c_islogical);
PROC_EXEC_PROTO(c_isna);
PROC_EXEC_PROTO(c_isnontext);
PROC_EXEC_PROTO(c_isnumber);
PROC_EXEC_PROTO(c_isodd);
PROC_EXEC_PROTO(c_isref);
PROC_EXEC_PROTO(c_istext);
PROC_EXEC_PROTO(c_na);
PROC_EXEC_PROTO(c_odd);
PROC_EXEC_PROTO(c_odf_type);
PROC_EXEC_PROTO(c_page);
PROC_EXEC_PROTO(c_pages);
PROC_EXEC_PROTO(c_set_name);
PROC_EXEC_PROTO(c_sort);
PROC_EXEC_PROTO(c_true);
PROC_EXEC_PROTO(c_type);
PROC_EXEC_PROTO(c_version);

/*
ev_fndat.c external functions (date and time)
*/

PROC_EXEC_PROTO(c_age);
PROC_EXEC_PROTO(c_date);
PROC_EXEC_PROTO(c_datevalue);
PROC_EXEC_PROTO(c_day);
PROC_EXEC_PROTO(c_dayname);
PROC_EXEC_PROTO(c_days_360);
PROC_EXEC_PROTO(c_edate);
PROC_EXEC_PROTO(c_eomonth);
PROC_EXEC_PROTO(c_hour);
PROC_EXEC_PROTO(c_minute);
PROC_EXEC_PROTO(c_month);
PROC_EXEC_PROTO(c_monthdays);
PROC_EXEC_PROTO(c_monthname);
PROC_EXEC_PROTO(c_now);
PROC_EXEC_PROTO(c_second);
PROC_EXEC_PROTO(c_time);
PROC_EXEC_PROTO(c_timevalue);
PROC_EXEC_PROTO(c_today);
PROC_EXEC_PROTO(c_weekday);
PROC_EXEC_PROTO(c_weeknumber);
PROC_EXEC_PROTO(c_year);

/*
NO ev_fndba.c (database)
*/

/* NO c_davg: ARRAY_RANGE_AVG */
/* NO c_dcount: ARRAY_RANGE_COUNT */
/* NO c_dcounta: ARRAY_RANGE_COUNTA */
/* NO c_dmax: ARRAY_RANGE_MAX */
/* NO c_dmin: ARRAY_RANGE_MIN */
/* NO c_dproduct: ARRAY_RANGE_PRODUCT */
/* NO c_dstd: ARRAY_RANGE_STD */
/* NO c_dstdp: ARRAY_RANGE_STDP */
/* NO c_dsum: ARRAY_RANGE_SUM */
/* NO c_dvar: ARRAY_RANGE_VAR */
/* NO c_dvarp: ARRAY_RANGE_VARP */

/*
ev_fneng.c external functions (engineering)
*/

PROC_EXEC_PROTO(c_besseli);
PROC_EXEC_PROTO(c_besselj);
PROC_EXEC_PROTO(c_besselk);
PROC_EXEC_PROTO(c_bessely);
PROC_EXEC_PROTO(c_delta);
PROC_EXEC_PROTO(c_erf);
PROC_EXEC_PROTO(c_erfc);
PROC_EXEC_PROTO(c_gestep);

/*
ev_fnfin.c external functions (financial)
*/

PROC_EXEC_PROTO(c_cterm);
PROC_EXEC_PROTO(c_db);
PROC_EXEC_PROTO(c_ddb);
PROC_EXEC_PROTO(c_fv);
PROC_EXEC_PROTO(c_fvschedule);
/* NO c_irr: ARRAY_RANGE_IRR */
/* NO c_mirr: ARRAY_RANGE_MIRR */
PROC_EXEC_PROTO(c_nper);
/* NO c_npv: ARRAY_RANGE_NPV */
PROC_EXEC_PROTO(c_odf_fv);
PROC_EXEC_PROTO(c_odf_irr);
PROC_EXEC_PROTO(c_odf_pmt);
PROC_EXEC_PROTO(c_pmt);
PROC_EXEC_PROTO(c_pv);
PROC_EXEC_PROTO(c_rate);
PROC_EXEC_PROTO(c_sln);
PROC_EXEC_PROTO(c_syd);
PROC_EXEC_PROTO(c_term);

/*
ev_fnsta.c external functions (statistical)
*/

/* NO c_avedev: ARRAY_RANGE_AVEDEV */
/* NO c_averagea: ARRAY_RANGE_AVERAGEA */
/* NO c_avg: ARRAY_RANGE_AVG */
PROC_EXEC_PROTO(c_beta);
PROC_EXEC_PROTO(c_bin);
PROC_EXEC_PROTO(c_combin);
/* NO c_count: ARRAY_RANGE_COUNT */
/* NO c_counta: ARRAY_RANGE_COUNTA */
/* NO c_devsq: ARRAY_RANGE_DEVSQ */
PROC_EXEC_PROTO(c_frequency);
PROC_EXEC_PROTO(c_gammaln);
/* NO c_geomean: ARRAY_RANGE_GEOMEAN */
PROC_EXEC_PROTO(c_grand);
/* NO c_harmean: ARRAY_RANGE_HARMEAN */
/* NO c_kurt: ARRAY_RANGE_KURT */
PROC_EXEC_PROTO(c_listcount);
/* NO c_max: ARRAY_RANGE_MAX */
/* NO c_median: ARRAY_RANGE_MEDIAN */
/* NO c_min: ARRAY_RANGE_MIN */
PROC_EXEC_PROTO(c_permut);
PROC_EXEC_PROTO(c_rand);
PROC_EXEC_PROTO(c_randbetween);
PROC_EXEC_PROTO(c_rank);
/* NO c_skew: ARRAY_RANGE_SKEW */
/* NO c_skew_p: ARRAY_RANGE_SKEW_P */
PROC_EXEC_PROTO(c_spearman);
/* NO c_std: ARRAY_RANGE_STD */
/* NO c_stdp: ARRAY_RANGE_STDP */
/* NO c_var: ARRAY_RANGE_VAR */
/* NO c_varp: ARRAY_RANGE_VARP */

extern void
binomial_coefficient_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return integer or fp or error */
    _InVal_     S32 n,
    _InVal_     S32 k);

/*
ev_fnstb.c external functions (statistical)
*/

PROC_EXEC_PROTO(c_combina);
PROC_EXEC_PROTO(c_fisher);
PROC_EXEC_PROTO(c_fisherinv);
PROC_EXEC_PROTO(c_gamma);
PROC_EXEC_PROTO(c_large);
PROC_EXEC_PROTO(c_mode_sngl);
PROC_EXEC_PROTO(c_percentile_exc);
PROC_EXEC_PROTO(c_percentile_inc);
PROC_EXEC_PROTO(c_percentrank_exc);
PROC_EXEC_PROTO(c_percentrank_inc);
PROC_EXEC_PROTO(c_quartile_exc);
PROC_EXEC_PROTO(c_quartile_inc);
PROC_EXEC_PROTO(c_rank_eq);
PROC_EXEC_PROTO(c_small);
PROC_EXEC_PROTO(c_standardize);
PROC_EXEC_PROTO(c_trimmean);

_Check_return_
extern F64
median_calc_span(
    _InRef_     PC_EV_DATA p_ev_data,
    _InVal_     S32 y_start /*incl*/,
    _InVal_     S32 y_end   /*excl*/);

_Check_return_
extern BOOL
statistics_value_next(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy,
    _InRef_     S32 x_size,
    _InRef_     S32 y_size);

/*
ev_fnstc.c external functions (statistical - continuous distributions)
*/

PROC_EXEC_PROTO(c_beta_dist);
PROC_EXEC_PROTO(c_beta_inv);
PROC_EXEC_PROTO(c_chisq_dist);
PROC_EXEC_PROTO(c_chisq_dist_rt);
PROC_EXEC_PROTO(c_chisq_inv);
PROC_EXEC_PROTO(c_chisq_inv_rt);
PROC_EXEC_PROTO(c_chisq_test);
PROC_EXEC_PROTO(c_confidence_norm);
PROC_EXEC_PROTO(c_confidence_t);
PROC_EXEC_PROTO(c_expon_dist);
PROC_EXEC_PROTO(c_F_dist);
PROC_EXEC_PROTO(c_F_dist_rt);
PROC_EXEC_PROTO(c_F_inv);
PROC_EXEC_PROTO(c_F_inv_rt);
PROC_EXEC_PROTO(c_F_test);
PROC_EXEC_PROTO(c_gamma_dist);
PROC_EXEC_PROTO(c_gamma_inv);
PROC_EXEC_PROTO(c_lognorm_dist);
PROC_EXEC_PROTO(c_lognorm_inv);
PROC_EXEC_PROTO(c_norm_dist);
PROC_EXEC_PROTO(c_norm_s_dist);
PROC_EXEC_PROTO(c_norm_inv);
PROC_EXEC_PROTO(c_norm_s_inv);
PROC_EXEC_PROTO(c_odf_betadist);
PROC_EXEC_PROTO(c_odf_tdist);
PROC_EXEC_PROTO(c_phi);
PROC_EXEC_PROTO(c_t_dist);
PROC_EXEC_PROTO(c_t_dist_rt);
PROC_EXEC_PROTO(c_t_dist_2t);
PROC_EXEC_PROTO(c_t_inv);
PROC_EXEC_PROTO(c_t_inv_2t);
PROC_EXEC_PROTO(c_t_test);
PROC_EXEC_PROTO(c_weibull_dist);
PROC_EXEC_PROTO(c_z_test);

/*
ev_fnstd.c external functions (statistical - discrete distributions)
*/

PROC_EXEC_PROTO(c_binom_dist);
PROC_EXEC_PROTO(c_binom_dist_range);
PROC_EXEC_PROTO(c_binom_inv);
PROC_EXEC_PROTO(c_hypgeom_dist);
PROC_EXEC_PROTO(c_negbinom_dist);
PROC_EXEC_PROTO(c_poisson_dist);

/*
ev_fnstm.c external functions (statistical - multivariate fit)
*/

PROC_EXEC_PROTO(c_growth);
PROC_EXEC_PROTO(c_linest);
PROC_EXEC_PROTO(c_logest);
PROC_EXEC_PROTO(c_trend);

/*
ev_fnstp.c external functions (statistical - paired statistics)
*/

PROC_EXEC_PROTO(c_correl);
PROC_EXEC_PROTO(c_covariance_p);
PROC_EXEC_PROTO(c_covariance_s);
PROC_EXEC_PROTO(c_forecast);
PROC_EXEC_PROTO(c_frequency);
PROC_EXEC_PROTO(c_intercept);
PROC_EXEC_PROTO(c_pearson);
PROC_EXEC_PROTO(c_prob);
PROC_EXEC_PROTO(c_rsq);
PROC_EXEC_PROTO(c_slope);
PROC_EXEC_PROTO(c_steyx);

_Check_return_
extern BOOL
statistics_paired_values_next(
    _OutRef_    P_EV_DATA p_ev_data_out_a,
    _OutRef_    P_EV_DATA p_ev_data_out_b,
    _InRef_     PC_EV_DATA p_ev_data_in_a,
    _InRef_     PC_EV_DATA p_ev_data_in_b,
    _InoutRef_  P_S32 p_ix,
    _InoutRef_  P_S32 p_iy,
    _InRef_     S32 x_size,
    _InRef_     S32 y_size);

/*
ev_fnstr.c external functions (string)
*/

PROC_EXEC_PROTO(c_char);
PROC_EXEC_PROTO(c_clean);
PROC_EXEC_PROTO(c_code);
PROC_EXEC_PROTO(c_dollar);
PROC_EXEC_PROTO(c_exact);
PROC_EXEC_PROTO(c_find);
PROC_EXEC_PROTO(c_fixed);
PROC_EXEC_PROTO(c_formula_text);
PROC_EXEC_PROTO(c_join);
PROC_EXEC_PROTO(c_left);
PROC_EXEC_PROTO(c_length);
PROC_EXEC_PROTO(c_lower);
PROC_EXEC_PROTO(c_mid);
PROC_EXEC_PROTO(c_n);
PROC_EXEC_PROTO(c_proper);
PROC_EXEC_PROTO(c_rept);
PROC_EXEC_PROTO(c_replace);
PROC_EXEC_PROTO(c_reverse);
PROC_EXEC_PROTO(c_right);
PROC_EXEC_PROTO(c_string);
PROC_EXEC_PROTO(c_substitute);
PROC_EXEC_PROTO(c_t);
PROC_EXEC_PROTO(c_text);
PROC_EXEC_PROTO(c_trim);
PROC_EXEC_PROTO(c_upper);
PROC_EXEC_PROTO(c_value);

/*
ev_help.c
*/

_Check_return_
extern STATUS /* EV_IDNO or error */
arg_normalise(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags,
    _InoutRef_opt_ P_S32 p_max_x,
    _InoutRef_opt_ P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase);

_Check_return_
extern STATUS
array_expand(
    P_EV_DATA p_ev_data,
    _InVal_     S32 max_x,
    _InVal_     S32 max_y);

/*ncr*/
extern EV_IDNO
array_range_index(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy,
    _InVal_     EV_TYPE types);

/*ncr*/
extern EV_IDNO
array_range_mono_index(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 mono_ix,
    _InVal_     EV_TYPE types);

extern void
array_range_sizes(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _OutRef_    P_S32 p_x_size,
    _OutRef_    P_S32 p_y_size);

extern void
array_range_mono_size(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _OutRef_    P_S32 p_mono_size);

_Check_return_
extern S32
array_scan_element(
    _InoutRef_  P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags);

_Check_return_
extern STATUS
array_scan_init(
    _OutRef_    P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _InRef_     PC_EV_DATA p_ev_data);

_Check_return_
extern STATUS
array_sort(
    P_EV_DATA p_ev_data,
    _InVal_     U32 x_index);

extern void
data_ensure_constant(
    P_EV_DATA p_ev_data);

_Check_return_
extern BOOL
data_is_array_range(
    _InRef_     PC_EV_DATA p_ev_data);

_Check_return_
extern S32
field_scan_element(
    _InoutRef_  P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags);

_Check_return_
extern STATUS
field_scan_init(
    _OutRef_    P_ARRAY_SCAN_BLOCK p_array_scan_block,
    _InRef_     PC_EV_DATA p_ev_data);

extern void
name_deref(
    P_EV_DATA p_ev_data,
    _InVal_     EV_HANDLE h_name);

_Check_return_
extern S32
range_next(
    _InRef_     PC_EV_RANGE p_ev_range,
    _InoutRef_  P_EV_SLR p_ev_slr);

_Check_return_
extern STATUS
range_scan_init(
    _OutRef_    P_RANGE_SCAN_BLOCK p_range_scan_block,
    _InRef_     PC_EV_RANGE p_ev_range);

_Check_return_
extern S32
range_scan_element(
    _InoutRef_  P_RANGE_SCAN_BLOCK p_range_scan_block,
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     EV_TYPE type_flags);

extern void
slr_offset_add(
    _InoutRef_  P_EV_SLR p_ev_slr,
    _InRef_     PC_EV_SLR p_ev_slr_offset,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope,
    _InVal_     BOOL use_abs,
    _InVal_     BOOL end_coord);

#if TRACE_ALLOWED

extern void
ev_trace_slr_tstr_buf(
    _Out_writes_z_(elemof_tstr_buf) PTSTR tstr_buf,
    _InVal_     U32 elemof_tstr_buf,
    _In_z_      PCTSTR tstr,
    _InRef_     PC_EV_SLR p_ev_slr);

#endif

_Check_return_ _Success_(return)
extern BOOL
two_nums_add_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors);

_Check_return_ _Success_(return)
extern BOOL
two_nums_divide_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors);

_Check_return_ _Success_(return)
extern BOOL
two_nums_multiply_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors);

_Check_return_ _Success_(return)
extern BOOL
two_nums_subtract_try(
    _OutRef_    P_EV_DATA p_ev_data_res,
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL propogate_errors);

/*
ev_math.c external functions
*/

/*
mathematical functions
*/

PROC_EXEC_PROTO(c_abs);
PROC_EXEC_PROTO(c_exp);
PROC_EXEC_PROTO(c_fact);
PROC_EXEC_PROTO(c_int);
PROC_EXEC_PROTO(c_ln);
PROC_EXEC_PROTO(c_odf_log10);
PROC_EXEC_PROTO(c_mod);
PROC_EXEC_PROTO(c_sgn);
PROC_EXEC_PROTO(c_sqr);
/* NO c_sum: ARRAY_RANGE_SUM */

extern void
factorial_calc(
    _OutRef_    P_EV_DATA p_ev_data_out, /* may return integer or fp or error */
    _InVal_     S32 n);

extern void
product_between_calc(
    _InoutRef_  P_EV_DATA p_ev_data_res, /* denotes integer or fp; may return integer or fp */
    _InVal_     S32 start,
    _InVal_     S32 end);

_Check_return_
extern STATUS
status_from_errno(void);

/*
ev_matb.c external functions (mathematical)
*/

PROC_EXEC_PROTO(c_ceiling);
PROC_EXEC_PROTO(c_factdouble);
PROC_EXEC_PROTO(c_floor);
PROC_EXEC_PROTO(c_log);
PROC_EXEC_PROTO(c_mround);
PROC_EXEC_PROTO(c_odf_int);
PROC_EXEC_PROTO(c_quotient);
PROC_EXEC_PROTO(c_round);
PROC_EXEC_PROTO(c_rounddown);
PROC_EXEC_PROTO(c_roundup);
PROC_EXEC_PROTO(c_seriessum);
/* NO c_sumsq: ARRAY_RANGE_SUMSQ */
PROC_EXEC_PROTO(c_sumproduct);
PROC_EXEC_PROTO(c_sum_x2my2);
PROC_EXEC_PROTO(c_sum_x2py2);
PROC_EXEC_PROTO(c_sum_xmy2);
PROC_EXEC_PROTO(c_trunc);

extern void
round_common(
    P_EV_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_EV_DATA p_ev_data_res,
    _InVal_     U32 rpn_did_num);

/*
ev_matr.c external functions (matrix functions)
*/

PROC_EXEC_PROTO(c_m_determ);
PROC_EXEC_PROTO(c_m_inverse);
PROC_EXEC_PROTO(c_m_mult);
PROC_EXEC_PROTO(c_m_unit);
PROC_EXEC_PROTO(c_transpose);

/*
ev_mcpx.c external functions (complex number)
*/

PROC_EXEC_PROTO(c_c_acos);
PROC_EXEC_PROTO(c_c_acosh);
PROC_EXEC_PROTO(c_c_acosec);
PROC_EXEC_PROTO(c_c_acosech);
PROC_EXEC_PROTO(c_c_acot);
PROC_EXEC_PROTO(c_c_acoth);
PROC_EXEC_PROTO(c_c_add);
PROC_EXEC_PROTO(c_c_asec);
PROC_EXEC_PROTO(c_c_asech);
PROC_EXEC_PROTO(c_c_asin);
PROC_EXEC_PROTO(c_c_asinh);
PROC_EXEC_PROTO(c_c_atan);
PROC_EXEC_PROTO(c_c_atanh);
PROC_EXEC_PROTO(c_c_complex);
PROC_EXEC_PROTO(c_c_conjugate);
PROC_EXEC_PROTO(c_c_cos);
PROC_EXEC_PROTO(c_c_cosh);
PROC_EXEC_PROTO(c_c_cosec);
PROC_EXEC_PROTO(c_c_cosech);
PROC_EXEC_PROTO(c_c_cot);
PROC_EXEC_PROTO(c_c_coth);
PROC_EXEC_PROTO(c_c_div);
PROC_EXEC_PROTO(c_c_exp);
PROC_EXEC_PROTO(c_c_imaginary);
PROC_EXEC_PROTO(c_c_ln);
PROC_EXEC_PROTO(c_c_log_10);
PROC_EXEC_PROTO(c_c_log_2);
PROC_EXEC_PROTO(c_c_mul);
PROC_EXEC_PROTO(c_c_power);
PROC_EXEC_PROTO(c_c_radius);
PROC_EXEC_PROTO(c_c_real);
PROC_EXEC_PROTO(c_c_round);
PROC_EXEC_PROTO(c_c_sec);
PROC_EXEC_PROTO(c_c_sech);
PROC_EXEC_PROTO(c_c_sin);
PROC_EXEC_PROTO(c_c_sinh);
PROC_EXEC_PROTO(c_c_sqrt);
PROC_EXEC_PROTO(c_c_sub);
PROC_EXEC_PROTO(c_c_tan);
PROC_EXEC_PROTO(c_c_tanh);
PROC_EXEC_PROTO(c_c_theta);

PROC_EXEC_PROTO(c_odf_complex);

/*
ev_fntri.c external functions (trigonometrical)
*/

PROC_EXEC_PROTO(c_acos);
PROC_EXEC_PROTO(c_acosec);
PROC_EXEC_PROTO(c_acosech);
PROC_EXEC_PROTO(c_acosh);
PROC_EXEC_PROTO(c_acot);
PROC_EXEC_PROTO(c_acoth);
PROC_EXEC_PROTO(c_asec);
PROC_EXEC_PROTO(c_asech);
PROC_EXEC_PROTO(c_asin);
PROC_EXEC_PROTO(c_asinh);
PROC_EXEC_PROTO(c_atan);
PROC_EXEC_PROTO(c_atanh);
PROC_EXEC_PROTO(c_atan_2);
PROC_EXEC_PROTO(c_cos);
PROC_EXEC_PROTO(c_cosh);
PROC_EXEC_PROTO(c_cosec);
PROC_EXEC_PROTO(c_cosech);
PROC_EXEC_PROTO(c_cot);
PROC_EXEC_PROTO(c_coth);
PROC_EXEC_PROTO(c_deg);
PROC_EXEC_PROTO(c_pi);
PROC_EXEC_PROTO(c_rad);
PROC_EXEC_PROTO(c_sec);
PROC_EXEC_PROTO(c_sech);
PROC_EXEC_PROTO(c_sin);
PROC_EXEC_PROTO(c_sinh);
PROC_EXEC_PROTO(c_tan);
PROC_EXEC_PROTO(c_tanh);

/*
ev_name.c
*/

extern DEPTABLE custom_def;

extern DEPTABLE name_def;

/*
ev_name.c external functions
*/

_Check_return_
extern ARRAY_INDEX
custom_def_find(
    _InVal_     EV_HANDLE handle);

extern void
custom_list_sort(void);

_Check_return_
extern ARRAY_INDEX
ensure_custom_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_custom_name);

_Check_return_
extern ARRAY_INDEX
ensure_name_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_name);

_Check_return_
extern EV_HANDLE
find_custom_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_custom_name);

_Check_return_
extern EV_HANDLE
find_name_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_name);

_Check_return_
extern ARRAY_INDEX
name_def_find(
    _InVal_     EV_HANDLE handle);

extern void
name_free_resources(
    _InoutRef_  P_EV_NAME p_ev_name);

extern void
name_list_sort(void);

_Check_return_
extern S32
name_make(
    P_EV_HANDLE p_ev_handle,
    _InVal_     EV_DOCNO ev_docno,
    _InRef_     PC_EV_STRINGC p_name,
    _InRef_     PC_EV_DATA p_ev_data_in,
    _In_opt_z_  PC_USTR ustr_description);

/*
ev_rpn.c external functions
*/

_Check_return_
extern S32
args_check(
    _InVal_     S32 arg_count,
    P_P_EV_DATA args,
    _InVal_     S32 type_count,
    _In_reads_opt_(type_count) const PC_EV_TYPE p_arg_types,
    _OutRef_    P_EV_DATA p_ev_data_res,
    _OutRef_    P_S32 p_max_x,
    _OutRef_    P_S32 p_max_y,
    P_STACK_DBASE p_stack_dbase);

/*ncr*/
extern S32
eval_rpn(
    _InVal_     U32 stack_at);

extern void
read_cur_sym(
    P_RPNSTATE p_rpnstate,
    P_EV_DATA p_ev_data);

#define read_from_rpn(to, from, size) \
    memcpy32((to), (from), (size))

#if defined(EV_IDNO_U16)
#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif
static inline void
rpn_check(
    _InoutRef_  P_RPNSTATE p_rpnstate)
{
    p_rpnstate->num = (EV_IDNO) readval_U16(p_rpnstate->pos);
#if defined(EV_IDNO_U32)
    assert(0xBEEFU == readval_U16(p_rpnstate->pos + 2));
#endif
}
#else
#define rpn_check(p_rpnstate) ( \
    (p_rpnstate)->num = (EV_IDNO) *((p_rpnstate)->pos))
#endif

/*ncr*/
extern EV_IDNO
rpn_skip(
    P_RPNSTATE p_rpnstate);

extern void
rpnstate_init(
    _OutRef_    P_RPNSTATE p_rpnstate,
    _InRef_     PC_EV_CELL p_ev_cell,
    _InVal_     S32 offset);

/*
ev_tabl.c
*/

extern const RPNDEF * const rpn_table;

/*
ev_tabl.c external functions
*/

_Check_return_
extern S32
ev_func_lookup(
    _In_z_      PC_USTR ustr_id);

_Check_return_
_Ret_maybenull_z_
extern PC_USTR
func_name(
    _InVal_     EV_IDNO did_num);

_Check_return_
_Ret_maybenull_z_
extern PC_A7STR
type_from_flags(
    _InVal_     EV_TYPE type);

_Check_return_
extern EV_TYPE
type_lookup(
    _In_z_      PC_USTR ustr_id);

/*
ev_todo.c
*/

#define TODO_SORTED_NOT  0
#define TODO_SORTED_SLR  1
#define TODO_SORTED_NODE 2

extern ARRAY_HANDLE h_todo_list;

extern ARRAY_HANDLE h_needs_recalc;

extern void
todo_exit(void);

_Check_return_ _Success_(return > 0)
extern STATUS
todo_next_slr(
    _OutRef_    P_EV_SLR p_ev_slr);

/*ncr*/
extern S32 /* any left ? */
todo_remove_slr(void);

/*
ev_tree.c
*/

extern DEPTABLE custom_use_deptable;
extern DEPTABLE event_use_deptable;
extern DEPTABLE name_use_deptable;

/*
ev_tree.c external functions
*/

PROC_BSEARCH_PROTO(extern, compare_range_index, RANGE_INDEX, RANGE_INDEX);

_Check_return_
extern ARRAY_INDEX
search_for_custom_use(
    _InVal_     EV_HANDLE h_custom);

_Check_return_
extern ARRAY_INDEX
search_for_name_use(
    _InVal_     EV_HANDLE h_name);

_Check_return_
extern ARRAY_INDEX
search_for_slr_use(
    _InRef_     PC_EV_SLR p_ev_slr);

extern void
tree_get_dependent_docs(
    _InVal_     EV_DOCNO ev_docno,
    P_DOCU_DEP_SUP p_docu_dep_sup);

extern void
tree_get_supporting_docs(
    _InVal_     EV_DOCNO ev_docno_in,
    P_DOCU_DEP_SUP p_docu_dep_sup);

/*ncr*/
extern S32
tree_sort(
    P_DEPTABLE p_deptable,
    _InRef_     P_PROC_ELEMENT_DELETED p_proc_element_deleted,
    _InRef_opt_ P_PROC_BSEARCH p_proc_bsearch);

extern void
tree_sort_all(void);

extern void
tree_sort_events(void);

/*
link_ev.c
*/

#endif /* __ev_evali_h */

/* end of ev_evali.h */
