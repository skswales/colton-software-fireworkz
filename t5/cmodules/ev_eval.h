/* ev_eval.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Evaluator external header file */

/* MRJC April 1991 / May 1992; JAD September 1994 */

#ifndef __ev_eval_h
#define __ev_eval_h

#define EV_MAX_ARGS 25          /* maximum number of arguments */

/*
definition of expression parameters
*/

typedef struct EV_PARMS
{
    UBF data_only : 1;                  /* cell contains no RPN */
    UBF rpn_variable : 1;               /* RPN is variable */
    UBF control : 4;                    /* control statement ? */
    UBF data_id : 4;                    /* type of data in result constant */
    UBF event_n : 6;                    /* number of event uses in table */
    UBF slr_n : 8;                      /* number of SLR uses in table */
    UBF range_n : 8;                    /* number of range uses in table */
    UBF name_n : 8;                     /* number of name uses in table */
    UBF custom_n : 8;                   /* number of custom calls in table */
    UBF style_handle_autoformat : 8;    /* style handle applied by mrofmun autoformat */
    /*          = 56 */
}
EV_PARMS, * P_EV_PARMS;

/* control statement types */

enum ev_control_statement_types
{
    EVS_CNT_NONE   = 1,
    EVS_CNT_IFC       ,
    EVS_CNT_ELSE      ,
    EVS_CNT_ELSEIF    ,
    EVS_CNT_ENDIF     ,
    EVS_CNT_WHILE     ,
    EVS_CNT_ENDWHILE  ,
    EVS_CNT_REPEAT    ,
    EVS_CNT_UNTIL     ,
    EVS_CNT_FOR       ,
    EVS_CNT_NEXT
};

/*
structure of a name reference
*/

typedef struct EV_NAME_REF
{
    EV_HANDLE h_name;
    BOOL no_dep;
}
EV_NAME_REF, * P_EV_NAME_REF;

/*
JAD : ob_ss & ob_sspt want this, so now it's here
*/

typedef void (* P_PROC_EXEC) (
    P_SS_DATA args[EV_MAX_ARGS],
    _InVal_     S32 n_args,
    _InoutRef_  P_SS_DATA p_ss_data_res,
    _InRef_     PC_EV_SLR p_cur_slr);

#define PROC_EXEC_PROTO(_proc_name) \
extern void \
_proc_name( \
    P_SS_DATA args[EV_MAX_ARGS], \
    _InVal_     S32 n_args,      \
    _InoutRef_  P_SS_DATA p_ss_data_res, \
    _InRef_     PC_EV_SLR p_cur_slr)

/*
evaluator's view of a cell
*/

typedef struct EV_CELL
{
    EV_PARMS ev_parms;

    SS_CONSTANT ss_constant;

#define OVH_EV_CELL offsetof32(EV_CELL, slrs)

    EV_SLR slrs[1];             /* array of supporting slrs */
    EV_RANGE ranges[1];         /* array of supporting ranges */
    EV_HANDLE names[1];         /* array of supporting names */
}
EV_CELL, * P_EV_CELL, ** P_P_EV_CELL; typedef const EV_CELL * PC_EV_CELL;

/*
macros to extract addresses of things in cell
*/

#define p_ev_slr_from_ev_cell(p_ev_cell, index) ( (P_EV_SLR) \
    ((P_U8) (p_ev_cell) \
    + OVH_EV_CELL \
    + sizeof32(EV_SLR) * (index)))

#define p_ev_range_from_ev_cell(p_ev_cell, index) ( (P_EV_RANGE) \
    ((P_U8) (p_ev_cell) \
    + OVH_EV_CELL \
    + sizeof32(EV_SLR) * (p_ev_cell)->ev_parms.slr_n \
    + sizeof32(EV_RANGE) * (index)))

#define p_ev_name_from_ev_cell(p_ev_cell, index) ( (P_EV_NAME_REF) \
    ((P_U8) (p_ev_cell) \
    + OVH_EV_CELL \
    + sizeof32(EV_SLR) * (p_ev_cell)->ev_parms.slr_n \
    + sizeof32(EV_RANGE) * (p_ev_cell)->ev_parms.range_n \
    + sizeof32(EV_NAME_REF) * (index)))

#define p_ev_custom_from_ev_cell(p_ev_cell, index) ( (P_EV_HANDLE) \
    ((P_U8) (p_ev_cell) \
    + OVH_EV_CELL \
    + sizeof32(EV_SLR) * (p_ev_cell)->ev_parms.slr_n \
    + sizeof32(EV_RANGE) * (p_ev_cell)->ev_parms.range_n \
    + sizeof32(EV_NAME_REF) * (p_ev_cell)->ev_parms.name_n \
    + sizeof32(EV_HANDLE) * (index)))

#define p_ev_event_from_ev_cell(p_ev_cell, index) ( (P_EV_HANDLE) \
    ((P_U8) (p_ev_cell) \
    + OVH_EV_CELL \
    + sizeof32(EV_SLR) * (p_ev_cell)->ev_parms.slr_n \
    + sizeof32(EV_RANGE) * (p_ev_cell)->ev_parms.range_n \
    + sizeof32(EV_NAME_REF) * (p_ev_cell)->ev_parms.name_n \
    + sizeof32(EV_HANDLE) * (p_ev_cell)->ev_parms.custom_n \
    + sizeof32(EVENT_TYPE) * (index)))

#define p_rpn_from_ev_cell(p_ev_cell) ( (P_U8) \
    ((p_ev_event_from_ev_cell((p_ev_cell), (p_ev_cell)->ev_parms.event_n))) )

/*
event types
*/

enum EV_EVENT_TYPES
{
    EV_EVENT_NONE,
    EV_EVENT_DOUBLECLICK,
    EV_EVENT_CURRENT_CELL
};

typedef S32 EVENT_TYPE; typedef EVENT_TYPE * P_EVENT_TYPE;

/*
compiler output data
*/

typedef struct COMPILER_OUTPUT
{
    ARRAY_HANDLE h_rpn;                 /* rpn string out */
    ARRAY_HANDLE h_slrs;                /* cells referred to in RPN (supporters) */
    ARRAY_HANDLE h_ranges;              /* ranges referred to in RPN (supporters) */
    ARRAY_HANDLE h_names;               /* names referred to in RPN (supporters) - see ev_name_ref structure */
    ARRAY_HANDLE h_custom_calls;        /* custom functions referred to in RPN */
    ARRAY_HANDLE h_custom_defs;         /* custom functions defined in RPN */
    ARRAY_HANDLE h_events;              /* event uses in RPN */
    SS_DATA ss_data;                    /* data to be stored in cell */
    EV_PARMS ev_parms;                  /* things about rpn */
    S32 chars_processed;                /* number of characters processed from input string */
    UBF load_recalc : 1;                /* must recalc on load */
}
COMPILER_OUTPUT, * P_COMPILER_OUTPUT;

/*
evaluator resource enumeration
*/

enum ev_resource_types
{
    EV_RESO_ENGINEER    = 0, /* Yuk. These ones are in Config files: e.g. Function-S:0 for Function->Complex menu (now Engineering) */
    EV_RESO_DATABASE    = 1,
    EV_RESO_DATE        = 2,
    EV_RESO_FINANCE     = 3,
    EV_RESO_MATHS       = 4,
    EV_RESO_MATRIX      = 5,
    EV_RESO_MISC        = 6,
    EV_RESO_STATS       = 7,
    EV_RESO_STRING      = 8,
    EV_RESO_TRIG        = 9,
    EV_RESO_CONTROL     = 10,
    EV_RESO_LOOKUP      = 11,
    EV_RESO_CUSTOM      = 12,
    EV_RESO_LOGICAL     = 13,
    EV_RESO_COMPAT      = 14,

    EV_RESO_NAMES,          /* not in Config file */
    EV_RESO_ALL_VISIBLE,    /* SKS 11apr93 */
    EV_RESO_NOTME           
};

typedef struct EV_RESOURCE
{
    S32 category;
    EV_DOCNO ev_docno_to;
    EV_DOCNO ev_docno_from;
    S32 item_no;
}
EV_RESOURCE, * P_EV_RESOURCE;

typedef struct RESOURCE_SPEC
{
    ARRAY_HANDLE_USTR h_id_ustr;
    ARRAY_HANDLE_USTR h_definition_ustr;
    U32 n_args;
    U32 max_additional_args;
    P_USTR ustr_description; /* owned by us */
}
RESOURCE_SPEC, * P_RESOURCE_SPEC;

/*
rpn atomic numbers
*/

#if defined(EV_IDNO_U16) && CHECKING
#define EV_IDNO_U16_FORCE /* forces all non-primitive RPN numbers up above 256 */
#endif

enum RPN_NUMBERS
{
    /* start after externally visible numbers */
#if defined(EV_IDNO_U16_FORCE)
    /* local argument */
    RPN_LCL_ARGUMENT = RPN_DAT_NEXT_NUMBER + 4*64U,
#else
    /* local argument */
    RPN_LCL_ARGUMENT = RPN_DAT_NEXT_NUMBER,
#endif

    /* format information */
    RPN_FRM_BRACKETS    ,
    RPN_FRM_SPACE       ,
    RPN_FRM_RETURN      ,
    RPN_FRM_EQUALS      ,
    RPN_FRM_END         ,
    RPN_FRM_COND        ,
    RPN_FRM_SKIPFALSE   ,
    RPN_FRM_SKIPTRUE    ,

    /* unary operators */
    RPN_UOP_NOT         ,
    RPN_UOP_MINUS       ,
    RPN_UOP_PLUS        ,

    /* binary operators */
    RPN_BOP_AND         ,
    RPN_BOP_DIVIDE      ,
    RPN_BOP_MINUS       ,
    RPN_BOP_OR          ,
    RPN_BOP_PLUS        ,
    RPN_BOP_POWER       ,
    RPN_BOP_TIMES       ,
    RPN_BOP_CONCATENATE ,

    /* binary relational operators */
    RPN_REL_EQUALS      ,
    RPN_REL_GT          ,
    RPN_REL_GTEQUAL     ,
    RPN_REL_LT          ,
    RPN_REL_LTEQUAL     ,
    RPN_REL_NOTEQUAL    ,

    /* functions */
    RPN_FNF_ABS         ,
    RPN_FNF_ACOS        ,
    RPN_FNF_ACOSEC      ,
    RPN_FNF_ACOSECH     ,
    RPN_FNF_ACOSH       ,
    RPN_FNF_ACOT        ,
    RPN_FNF_ACOTH       ,
    RPN_FNF_AGE         ,
    RPN_FNV_ALERT       ,
    RPN_FNV_AND         ,
    RPN_FNF_ASEC        ,
    RPN_FNF_ASECH       ,
    RPN_FNF_ASIN        ,
    RPN_FNF_ASINH       ,
    RPN_FNF_ATAN        ,
    RPN_FNF_ATAN_2      ,
    RPN_FNF_ATANH       ,
    RPN_FNV_AVEDEV      ,
    RPN_FNV_AVG         ,

    RPN_FNF_BETA        ,
    RPN_FNF_BIN         ,
    RPN_FNV_BREAK       ,

    RPN_FNF_C_ACOS      ,
    RPN_FNF_C_ACOSEC    ,
    RPN_FNF_C_ACOSECH   ,
    RPN_FNF_C_ACOSH     ,
    RPN_FNF_C_ACOT      ,
    RPN_FNF_C_ACOTH     ,
    RPN_FNF_C_ADD       ,
    RPN_FNF_C_ASEC      ,
    RPN_FNF_C_ASECH     ,
    RPN_FNF_C_ASIN      ,
    RPN_FNF_C_ASINH     ,
    RPN_FNF_C_ATAN      ,
    RPN_FNF_C_ATANH     ,
    RPN_FNF_C_CONJUGATE ,
    RPN_FNF_C_COS       ,
    RPN_FNF_C_COSEC     ,
    RPN_FNF_C_COSECH    ,
    RPN_FNF_C_COSH      ,
    RPN_FNF_C_COT       ,
    RPN_FNF_C_COTH      ,
    RPN_FNF_C_DIV       ,
    RPN_FNF_C_EXP       ,
    RPN_FNF_C_LN        ,
    RPN_FNF_C_MUL       ,
    RPN_FNF_C_POWER     ,
    RPN_FNF_C_RADIUS    ,
    RPN_FNF_C_SEC       ,
    RPN_FNF_C_SECH      ,
    RPN_FNF_C_SIN       ,
    RPN_FNF_C_SINH      ,
    RPN_FNF_C_SUB       ,
    RPN_FNF_C_TAN       ,
    RPN_FNF_C_TANH      ,
    RPN_FNF_C_THETA     ,

    RPN_FNV_CEILING     ,
    RPN_FNF_CHAR        ,
    RPN_FNV_CHOOSE      ,
    RPN_FNF_CLEAN       ,
    RPN_FNF_CODE        ,
    RPN_FNV_COL         ,
    RPN_FNV_COLS        ,
    RPN_FNF_COMBIN      ,
    RPN_FNF_COMMAND     ,
    RPN_FN0_CONTINUE    ,
    RPN_FNF_COS         ,
    RPN_FNF_COSEC       ,
    RPN_FNF_COSECH      ,
    RPN_FNF_COSH        ,
    RPN_FNF_COT         ,
    RPN_FNF_COTH        ,
    RPN_FNV_COUNT       ,
    RPN_FNV_COUNTA      ,
    RPN_FNF_CTERM       ,
    RPN_FN0_CURRENT_CELL,

    RPN_FNF_DATE        ,
    RPN_FNF_DATEVALUE   ,
    RPN_FNF_DAVG        ,
    RPN_FNF_DAY         ,
    RPN_FNV_DAYNAME     ,
    RPN_FNV_DAYS_360    ,
    RPN_FNF_DCOUNT      ,
    RPN_FNF_DCOUNTA     ,
    RPN_FNV_DDB         ,
    RPN_FNF_DEG         ,
    RPN_FNF_DEREF       ,
    RPN_FNV_DEVSQ       ,
    RPN_FNF_DMAX        ,
    RPN_FNF_DMIN        ,
    RPN_FNV_DOLLAR      ,
    RPN_FN0_DOUBLECLICK ,
    RPN_FNF_DPRODUCT    ,
    RPN_FNF_DSTD        ,
    RPN_FNF_DSTDP       ,
    RPN_FNF_DSUM        ,
    RPN_FNF_DVAR        ,
    RPN_FNF_DVARP       ,

    RPN_FN0_ELSE        ,
    RPN_FNF_ELSEIF      ,
    RPN_FN0_ENDIF       ,
    RPN_FN0_ENDWHILE    ,
    RPN_FNF_EVEN        ,
    RPN_FNF_EXACT       ,
    RPN_FNF_EXP         ,

    RPN_FNF_FACT        ,
    RPN_FN0_FALSE       ,
    RPN_FNV_FIND        ,
    RPN_FNV_FIXED       ,
    RPN_FNF_FLIP        ,
    RPN_FNV_FLOOR       ,
    RPN_FNV_FOR         ,
    RPN_FNF_FORMULA_TEXT,
    RPN_FNM_FUNCTION    ,
    RPN_FNF_FV          ,
    RPN_FNV_ODF_FV,

    RPN_FNF_GAMMALN     ,
    RPN_FNV_GEOMEAN     ,
    RPN_FNF_GOTO        ,
    RPN_FNV_GRAND       ,
    RPN_FNF_GROWTH      ,

    RPN_FNV_HARMEAN     ,
    RPN_FNV_HLOOKUP     ,
    RPN_FNF_HOUR        ,

    RPN_FNF_IF          ,
    RPN_FNF_IFC         ,
    RPN_FNV_INDEX       ,
    RPN_FNV_INPUT       ,
    RPN_FNF_INT         ,
    RPN_FNF_IRR         ,
    RPN_FNF_ISBLANK     ,
    RPN_FNF_ISERR       ,
    RPN_FNF_ISERROR     ,
    RPN_FNF_ISEVEN      ,
    RPN_FNF_ISLOGICAL   ,
    RPN_FNF_ISNA        ,
    RPN_FNF_ISNONTEXT   ,
    RPN_FNF_ISNUMBER    ,
    RPN_FNF_ISODD       ,
    RPN_FNF_ISREF       ,
    RPN_FNF_ISTEXT      ,

    RPN_FNV_JOIN        ,

    RPN_FNV_KURT        ,

    RPN_FNF_LARGE       ,
    RPN_FNV_LEFT        ,
    RPN_FNF_LENGTH      ,
    RPN_FNV_LINEST      ,
    RPN_FNF_LISTCOUNT   ,
    RPN_FNF_LN          ,
    RPN_FNV_LOG         ,
    RPN_FNV_LOGEST      ,
    RPN_FNV_LOOKUP      ,
    RPN_FNF_LOWER       ,

    RPN_FNF_M_DETERM    ,
    RPN_FNF_M_INVERSE   ,
    RPN_FNF_M_MULT      ,

    RPN_FNF_MATCH       ,
    RPN_FNV_MAX         ,
    RPN_FNV_MEDIAN      ,
    RPN_FNF_MID         ,
    RPN_FNV_MIN         ,
    RPN_FNF_MINUTE      ,
    RPN_FNF_MIRR        ,
    RPN_FNF_MOD         ,
    RPN_FNF_MODE_SNGL   ,
    RPN_FNF_MONTH       ,
    RPN_FNF_MONTHDAYS   ,
    RPN_FNF_MONTHNAME   ,
    RPN_FNF_MROUND      ,

    RPN_FNF_N           ,
    RPN_FN0_NEXT        ,
    RPN_FNF_NOT         ,
    RPN_FN0_NOW         ,
    RPN_FNV_NPER        ,
    RPN_FNF_NPV         ,

    RPN_FNF_ODD         ,
    RPN_FNV_OR          ,

    RPN_FNV_PAGE        ,
    RPN_FNV_PAGES       ,
    RPN_FNF_PERCENTILE_INC,
    RPN_FNF_PERMUT      ,
    RPN_FN0_PI          ,
    RPN_FNF_PMT         ,
    RPN_FNF_ODF_PMT,
    RPN_FNF_POWER       ,
    RPN_FNV_PRODUCT     ,
    RPN_FNF_PROPER      ,
    RPN_FNF_PV          ,

    RPN_FNF_QUARTILE_INC,

    RPN_FNF_RAD         ,
    RPN_FNV_RAND        ,
    RPN_FNF_RANDBETWEEN ,
    RPN_FNV_RANK        ,
    RPN_FNV_RANK_EQ     ,
    RPN_FNF_RATE        ,
    RPN_FN0_REPEAT      ,
    RPN_FNF_REPLACE     ,
    RPN_FNF_REPT        ,
    RPN_FNF_RESULT      ,
    RPN_FNF_REVERSE     ,
    RPN_FNV_RIGHT       ,
    RPN_FNV_ROUND       ,
    RPN_FNF_ROUNDDOWN   ,
    RPN_FNF_ROUNDUP     ,
    RPN_FNV_ROW         ,
    RPN_FNV_ROWS        ,

    RPN_FNF_SEC         ,
    RPN_FNF_SECH        ,
    RPN_FNF_SECOND      ,
    RPN_FNF_SET_NAME    ,
    RPN_FNV_SET_VALUE   ,
    RPN_FNF_SGN         ,
    RPN_FNF_SIN         ,
    RPN_FNF_SINH        ,
    RPN_FNV_SKEW        ,
    RPN_FNF_SLN         ,
    RPN_FNF_SMALL       ,
    RPN_FNV_SORT        ,
    RPN_FNF_SPEARMAN    ,
    RPN_FNF_SQR         ,
    RPN_FNF_STANDARDIZE ,
    RPN_FNV_STD         ,
    RPN_FNV_STDP        ,
    RPN_FNV_STRING      ,
    RPN_FNV_SUBSTITUTE  ,
    RPN_FNV_SUM         ,
    RPN_FNV_SUMSQ       ,
    RPN_FNF_SYD         ,

    RPN_FNF_T           ,
    RPN_FNF_TAN         ,
    RPN_FNF_TANH        ,
    RPN_FNF_TERM        ,
    RPN_FNF_TEXT        ,
    RPN_FNF_TIME        ,
    RPN_FNF_TIMEVALUE   ,
    RPN_FN0_TODAY       ,
    RPN_FNF_TRANSPOSE   ,
    RPN_FNF_TREND       ,
    RPN_FNF_TRIM        ,
    RPN_FNF_TRIMMEAN    ,
    RPN_FN0_TRUE        ,
    RPN_FNV_TRUNC       ,
    RPN_FNF_TYPE        ,

    RPN_FNF_UNTIL       ,
    RPN_FNF_UPPER       ,

    RPN_FNF_VALUE       ,
    RPN_FNV_VAR         ,
    RPN_FNV_VARP        ,
    RPN_FN0_VERSION     ,
    RPN_FNV_VLOOKUP     ,

    RPN_FNV_WEEKDAY     ,
    RPN_FNF_WEEKNUMBER  ,
    RPN_FNF_WHILE       ,

    RPN_FNF_YEAR        ,

#if 1 /* New functions should go here (in the order they appear in RPNDEF and in messages) until we have another big sort-out */

    RPN_FNV_SUMPRODUCT,
    RPN_FNF_SUM_X2MY2,
    RPN_FNF_SUM_X2PY2,
    RPN_FNF_SUM_XMY2,
    RPN_FNF_ODF_TYPE,
    RPN_FNF_COUNTBLANK,
    RPN_FNF_COVARIANCE_P,
    RPN_FNF_PEARSON,
    RPN_FNF_RSQ,
    RPN_FNF_SLOPE,
    RPN_FNF_INTERCEPT,
    RPN_FNF_FORECAST,
    RPN_FNF_STEYX,
    RPN_FNV_SKEW_P,
    RPN_FNF_BESSELJ,
    RPN_FNF_BESSELY,
    RPN_FNV_AVERAGEA,
    RPN_FNV_MAXA,
    RPN_FNV_MINA,
    RPN_FNV_STDEVA,
    RPN_FNV_STDEVPA,
    RPN_FNV_VARA,
    RPN_FNV_VARPA,
    RPN_FNV_PERCENTRANK_INC,
    RPN_FNV_ERF,
    RPN_FNF_ERFC,
    RPN_FNF_COVARIANCE_S,
    RPN_FNV_XOR,
    RPN_FNV_C_COMPLEX,
    RPN_FNV_ADDRESS,
    RPN_FNV_DB,
    RPN_FNF_FVSCHEDULE,
    RPN_FNF_QUOTIENT,
    RPN_FNV_EXPON_DIST,
    RPN_FNF_COMBINA,
    RPN_FNF_FACTDOUBLE,
    RPN_FNF_SERIESSUM,
    RPN_FNV_HYPGEOM_DIST,
    RPN_FNF_WEIBULL_DIST,
    RPN_FNV_POISSON_DIST,
    RPN_FNF_BINOM_DIST,
    RPN_FNV_BINOM_DIST_RANGE,
    RPN_FNF_FISHER,
    RPN_FNF_FISHERINV,
    RPN_FNV_MULTINOMIAL,
    RPN_FNF_BESSELI,
    RPN_FNF_BESSELK,
    RPN_FNV_NORM_DIST,
    RPN_FNV_NORM_S_DIST,
    RPN_FNV_LOGNORM_DIST,
    RPN_FNF_PHI,
    RPN_FNV_DELTA,
    RPN_FNV_GESTEP,
    RPN_FNF_ODF_INT,
    RPN_FNF_FREQUENCY,
    RPN_FNF_M_UNIT,
    RPN_FNV_ODF_IRR,
    RPN_FNF_NORM_INV,
    RPN_FNF_NORM_S_INV,
    RPN_FNF_ODF_LOG10,
    RPN_FNF_GAMMA,
    RPN_FNV_NEGBINOM_DIST,
    RPN_FNV_ODF_INDEX,
    RPN_FNV_GAMMA_DIST,
    RPN_FNF_GAMMA_INV,
    RPN_FNV_BETA_DIST,
    RPN_FNV_ODF_BETADIST,
    RPN_FNV_BETA_INV,
    RPN_FNF_CORREL,
    RPN_FN0_NA,
    RPN_FNV_CHISQ_DIST,
    RPN_FNF_CHISQ_INV,
    RPN_FNF_CHISQ_DIST_RT,
    RPN_FNF_CHISQ_INV_RT,
    RPN_FNV_F_DIST,
    RPN_FNF_F_INV,
    RPN_FNF_F_DIST_RT,
    RPN_FNF_F_INV_RT,
    RPN_FNV_T_DIST,
    RPN_FNF_T_INV,
    RPN_FNF_T_DIST_2T,
    RPN_FNF_T_INV_2T,
    RPN_FNF_T_DIST_RT,
    RPN_FNF_ODF_TDIST,
    RPN_FNF_CONFIDENCE_NORM,
    RPN_FNF_CONFIDENCE_T,
    RPN_FNF_PERCENTILE_EXC,
    RPN_FNF_QUARTILE_EXC,
    RPN_FNF_BINOM_INV,
    RPN_FNV_Z_TEST,
    RPN_FNV_PROB,
    RPN_FNF_CHISQ_TEST,
    RPN_FNF_F_TEST,
    RPN_FNF_T_TEST,

    RPN_FNF_ODF_IMABS,
    RPN_FNF_ODF_IMAGINARY,
    RPN_FNF_ODF_IMARGUMENT,
    RPN_FNF_ODF_IMCONJUGATE,
    RPN_FNF_ODF_IMCOS,
    RPN_FNF_ODF_IMDIV,
    RPN_FNF_ODF_IMEXP,
    RPN_FNF_ODF_IMLN,
    RPN_FNF_ODF_IMLOG10,
    RPN_FNF_ODF_IMLOG2,
    RPN_FNF_ODF_IMPOWER,
    RPN_FNF_ODF_IMPRODUCT,
    RPN_FNF_ODF_IMREAL,
    RPN_FNF_ODF_IMSIN,
    RPN_FNF_ODF_IMSQRT,
    RPN_FNF_ODF_IMSUB,
    RPN_FNF_ODF_IMSUM,

    RPN_FNF_C_IMAGINARY,
    RPN_FNF_C_REAL,
    RPN_FNV_C_ROUND,
    RPN_FNF_C_SQRT,

    RPN_FNV_ODF_COMPLEX,
    RPN_FNF_LOGNORM_INV,
    RPN_FNV_PERCENTRANK_EXC,

    RPN_FNF_EDATE, /* 2.01 */
    RPN_FNF_EOMONTH, /* 2.01 */

    RPN_FNF_ODF_MOD, /* 2.20 */

    RPN_FNF_BIN2DEC, /* 2.24 */
    RPN_FNV_BIN2HEX, /* 2.24 */
    RPN_FNV_BIN2OCT, /* 2.24 */
    RPN_FNV_DEC2BIN, /* 2.24 */
    RPN_FNV_DEC2HEX, /* 2.24 */
    RPN_FNV_DEC2OCT, /* 2.24 */
    RPN_FNV_HEX2BIN, /* 2.24 */
    RPN_FNF_HEX2DEC, /* 2.24 */
    RPN_FNV_HEX2OCT, /* 2.24 */
    RPN_FNV_OCT2BIN, /* 2.24 */
    RPN_FNF_OCT2DEC, /* 2.24 */
    RPN_FNV_OCT2HEX, /* 2.24 */

    RPN_FNF_BITAND, /* 2.24 */
    RPN_FNF_BITLSHIFT, /* 2.24 */
    RPN_FNF_BITOR, /* 2.24 */
    RPN_FNF_BITRSHIFT, /* 2.24 */
    RPN_FNF_BITXOR, /* 2.24 */

    RPN_FNV_BASE, /* 2.24 */
    RPN_FNF_DECIMAL, /* 2.24 */
    RPN_FNV_GCD, /* 2.24 */
    RPN_FNV_LCM, /* 2.24 */

    RPN_FNF_ISOWEEKNUM, /* 2.24 */
    RPN_FNF_DAYS, /* 2.24 */

#endif /* end of any new functions */

    RPN_END_BUILT_IN,

    /* custom functions */
    RPN_FNM_CUSTOMCALL  = RPN_END_BUILT_IN,

    /* make array */
    RPN_FNA_MAKEARRAY   ,

#define ELEMOF_RPN_TABLE (RPN_FNA_MAKEARRAY + 1)

    /* special symbol constants - these symbols
     * don't appear in the final rpn string
    */

    SYM_BAD             ,
    SYM_BLANK           ,
    SYM_OBRACKET        ,
    SYM_CBRACKET        ,
    SYM_COMMA           ,
    SYM_OARRAY          ,
    SYM_CARRAY          ,
    SYM_SEMICOLON       ,
    SYM_TAG

#define RPN_END SYM_TAG

};

/*
types of rpn atoms
*/

enum RPN_TYPES
{
    RPN_DAT     = 0,        /* data */
    RPN_CON     ,           /* constant cells */
    RPN_LCL     ,           /* local variables */
    RPN_EXT     ,           /* external cells */
    RPN_FRM     ,           /* formatting information */
    RPN_UOP     ,           /* unary operator */
    RPN_BOP     ,           /* binary operator */
    RPN_REL     ,           /* relational operator */
    RPN_FN0     ,           /* function, no arguments */
    RPN_FNF     ,           /* function, fixed number of args */
    RPN_FNV     ,           /* function, variable args */
    RPN_FNM     ,           /* function, custom function call, variable args */
    RPN_FNA     ,           /* function, make array, variable args */

    RPN_INV                 /* invalid, for checking */
};

/*
type mask bits
*/

enum type_mask_bits
{
    EM_REA      = 1U,
    EM_SLR      = 2U,      /* if not set, SLRs are dereferenced first */
    EM_STR      = 4U,
    EM_DAT      = 8U,
    EM_ARY      = 16U,     /* if not set, operator called for each element */
    EM_BLK      = 32U,     /* if not set, blanks converted to real 0 */
    EM_CDX      = 128U,    /* conditional subexpression */
    EM_ERR      = 256U,
    EM_INT      = 512U,    /* function wants integers */

    EM_LOGICAL  = (EM_REA                                                       | EM_INT),
    EM_CONST    = (EM_REA          | EM_STR | EM_DAT          | EM_BLK | EM_ERR | EM_INT),
    EM_ANY      = (EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_ERR | EM_INT)
};

#if 1
typedef U32 EV_TYPE; typedef EV_TYPE * P_EV_TYPE; typedef const EV_TYPE * PC_EV_TYPE; /* could be U16 but we don't build huge arrays of them or structures containing them so stop masking */
#else
typedef S16 EV_TYPE; typedef EV_TYPE * P_EV_TYPE; typedef const EV_TYPE * PC_EV_TYPE; /* 2.30 and earlier */
#endif

typedef struct EV_SPLIT_EXEC_DATA
{
    U32         object_table_index;  /* index into our table */
    P_SS_DATA   args[EV_MAX_ARGS];
    S32         n_args;
    P_SS_DATA   p_ss_data_res;
    PC_EV_SLR   p_cur_slr;
}
EV_SPLIT_EXEC_DATA, * P_EV_SPLIT_EXEC_DATA;

/*
evaluator external routines
*/

#define ev_docno_from_p_docu(p_docu) ( (EV_DOCNO) \
    docno_from_p_docu(p_docu) )

#define p_docu_from_ev_docno(ev_docno) \
    p_docu_from_docno((DOCNO) ev_docno)

/*
ev_comp.c
*/

_Check_return_
extern STATUS
ev_compile(
    _OutRef_    P_COMPILER_OUTPUT p_compiler_output /* handles of compiler output */,
    _In_opt_z_  PC_USTR ustr_result,
    _In_opt_z_  PC_USTR ustr_formula,
    _InRef_     PC_EV_SLR p_ev_slr);

extern void
ev_compiler_output_adjust(
    _InoutRef_  P_COMPILER_OUTPUT p_compiler_output,
    _InRef_     PC_EV_SLR p_ev_slr_offset,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope);

extern void
ev_compiler_output_dispose(
    _InoutRef_  P_COMPILER_OUTPUT p_compiler_output);

_Check_return_
extern S32
ev_compiler_output_size(
    _InRef_     P_COMPILER_OUTPUT p_compiler_output);

_Check_return_
extern STATUS
ev_cell_from_compiler_output(
    P_EV_CELL p_ev_cell,
    P_COMPILER_OUTPUT p_compiler_output,
    _InRef_opt_ PC_EV_SLR p_ev_slr_offset,
    _InRef_opt_ PC_EV_RANGE p_ev_range_scope);

/*
ev_dcom.c
*/

/*ncr*/
extern U32
ev_dec_slr_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO this_ev_docno,
    _InRef_     PC_EV_SLR p_ev_slr);

_Check_return_
extern STATUS
ev_cell_decode(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_EV_CELL p_ev_cell,
    _InVal_     EV_DOCNO ev_docno);

_Check_return_
extern STATUS
ev_cell_decode_ui(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_EV_CELL p_ev_cell,
    _InVal_     EV_DOCNO ev_docno);

_Check_return_
extern STATUS
ss_data_decode(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    P_SS_DATA p_ss_data,
    _InVal_     EV_DOCNO ev_docno_from);

_Check_return_
extern STATUS
ev_decompile(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*output,NOT appended*/,
    _InRef_     PC_EV_CELL p_ev_cell,
    _InVal_     S32 offset,
    _InVal_     EV_DOCNO ev_docno);

_Check_return_
extern STATUS
quick_ublock_func_name_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR ustr);

/*
ev_eval.c
*/

extern void
ev_recalc(void);

/*
ev_help.c
*/

extern void
ss_data_from_ev_cell(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_EV_CELL p_ev_cell);

extern void
ev_exp_copy(
    _OutRef_    P_EV_CELL p_ev_cell_out,
    _InRef_     PC_EV_CELL p_ev_cell_in);

extern void
ev_exp_refs_adjust(
    _InoutRef_  P_EV_CELL p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr_offset /* adjustment to slrs */);

extern void
ev_cell_constant_from_data(
    _OutRef_    P_EV_CELL p_ev_cell,
    _InoutRef_  P_SS_DATA p_ss_data);

extern void
ev_cell_free_resources(
    P_EV_CELL p_ev_cell);

/*ncr*/ _Ret_notnull_
extern P_SS_DATA
ev_slr_deref(
    _OutRef_    P_SS_DATA p_ss_data,
    _InRef_     PC_EV_SLR p_ev_slr);

/*
link_ev.c
*/

_Check_return_
extern STATUS
ev_alert(
    _InVal_     EV_DOCNO ev_docno,
    _InRef_     PC_SS_STRINGC p_ss_string_message,
    _InRef_     PC_SS_STRINGC p_ss_string_button_1,
    _InRef_     PC_SS_STRINGC p_ss_string_button_2);

extern void
ev_alert_close(void);

_Check_return_
extern STATUS
ev_alert_poll(void);

_Check_return_
extern STATUS
ev_input(
    _InVal_     EV_DOCNO ev_docno,
    _InRef_     PC_SS_STRINGC p_ss_string_message,
    _InRef_     PC_SS_STRINGC p_ss_string_button_1,
    _InRef_     PC_SS_STRINGC p_ss_string_button_2);

extern void
ev_input_close(void);

_Check_return_
extern STATUS
ev_input_poll(
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer);

#define ev_doc_auto_calc() ( \
    !global_preferences.ss_calc_manual )

#define ev_doc_check_is_custom(docno) ( \
    ev_p_ss_doc_from_docno(docno)->is_custom )

#define ev_doc_error(docno) ( \
    ev_doc_is_thunk(docno) ? EVAL_ERR_CANTEXTREF : 0 )

#define ev_doc_foreground() ( \
    global_preferences.ss_calc_foreground )

#define ev_doc_is_thunk(docno) ( \
    !(p_docu_from_docno(docno)->flags.has_data) )

#define ev_doc_modify(docno) ( \
    docu_modify(p_docu_from_docno(docno)) )

#define ev_doc_reuse_hold() \
    docno_reuse_hold()

#define ev_doc_reuse_release() \
    docno_reuse_release()

extern void
ev_recog_constant_using_autoformat(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InVal_     EV_DOCNO ev_docno,
    _In_z_      PC_USTR ustr);

extern void
ev_current_cell(
    _OutRef_    P_EV_SLR p_ev_slr);

_Check_return_
extern EV_DOCNO
ev_current_docno(void);

extern void
ev_double_click(
    _OutRef_    P_EV_SLR p_ev_slr_out,
    _InRef_     PC_EV_SLR p_ev_slr_in);

_Check_return_
extern EV_DOCNO
ev_establish_docno_from_docu_name(
    _InoutRef_  P_DOCU_NAME p_docu_name,
    _InVal_     EV_DOCNO ev_docno_from);

_Check_return_
extern BOOL
ev_event_occurred(
    _InVal_     EV_DOCNO ev_docno,
    _InVal_     EVENT_TYPE event_type);

extern void
ev_exit(void);

extern void
ev_external_data(
    P_SS_DATA p_ss_data_out,
    _InRef_     PC_EV_SLR p_ev_slr);

extern void
ev_field_data_read(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InVal_     EV_HANDLE h_name,
    _InVal_     S32 iy);

_Check_return_
extern STATUS
ev_field_n_records(
    _InVal_     EV_HANDLE h_name,
    _OutRef_    P_S32 p_n_out);

extern void
ev_field_names_check(
    P_NAME_UREF p_name_uref);

_Check_return_
extern STATUS
ev_make_cell(
    _InRef_     PC_EV_SLR p_ev_slr,
    P_SS_DATA p_ss_data);

_Check_return_
extern EV_COL
ev_numcol(
    _InVal_     EV_DOCNO ev_docno);

_Check_return_
extern EV_COL
ev_numcol_phys(
    _InVal_     EV_DOCNO ev_docno);

_Check_return_
extern STATUS
ev_numform(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _In_z_      PC_USTR ustr_format,
    _InRef_     PC_SS_DATA p_ss_data);

_Check_return_
extern EV_ROW
ev_numrow(
    _InVal_     EV_DOCNO ev_docno);

/* get pointer to spreadsheet instance data */

_Check_return_
_Ret_valid_
static inline P_SS_INSTANCE_DATA
p_object_instance_data_SS(
    _InRef_     P_DOCU p_docu)
{
    P_SS_INSTANCE_DATA p_ss_instance_data = &p_docu->ss_instance_data;
    return(p_ss_instance_data);
}

_Check_return_
_Ret_maybenone_
static inline P_SS_DOC
ev_p_ss_doc_from_docno(
    _InVal_     EV_DOCNO ev_docno)
{
    const P_DOCU p_docu = p_docu_from_ev_docno(ev_docno); /* SKS 06jan95 this could well have been it !!! */

    if(IS_DOCU_NONE(p_docu))
        return(P_SS_DOC_NONE);

    return(&p_object_instance_data_SS(p_docu)->ss_doc);
}

_Check_return_
extern S32
ev_page_last(
    _InVal_     EV_DOCNO ev_docno,
    _InVal_     BOOL y_flag);

_Check_return_
extern STATUS
ev_page_slr(
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     BOOL y_flag);

extern void
ev_recalc_start(
    _InVal_     BOOL must);

extern void
ev_recalc_status(
    _InVal_     STATUS to_calc);

extern void
ev_recalc_stop(void);

extern BOOL
g_ev_recalc_started;

extern void
ev_redraw_slot_range(
    _InRef_     PC_EV_RANGE p_ev_range);

#define ev_range_from_two_slrs(p_docu, p_ev_range, pc_slr_1, pc_slr_2) ( \
    ev_range_init(p_ev_range), \
    (p_ev_range)->s.docno = \
    (p_ev_range)->e.docno = EV_DOCNO_PACK(docno_from_p_docu(p_docu)), \
    (p_ev_range)->s.col = EV_COL_PACK((pc_slr_1)->col), \
    (p_ev_range)->s.row = (pc_slr_1)->row, \
    (p_ev_range)->e.col = EV_COL_PACK((pc_slr_2)->col), \
    (p_ev_range)->e.row = (pc_slr_2)->row)

#define ev_slr_from_slr(p_docu, p_ev_slr, pc_slr) ( \
    ev_slr_flags_init(p_ev_slr), \
    (p_ev_slr)->docno = EV_DOCNO_PACK(docno_from_p_docu(p_docu)), \
    (p_ev_slr)->col = EV_COL_PACK((pc_slr)->col), \
    (p_ev_slr)->row = (pc_slr)->row )

_Check_return_
extern BOOL
ev_tell_name_clients(
    _InVal_     EV_HANDLE ev_handle,
    _InVal_     BOOL changed);

_Check_return_
extern S32
ev_travel(
    _OutRef_    P_P_EV_CELL p_p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr);

extern void
ev_uref_change_range(
    _InRef_     PC_EV_RANGE p_ev_range);

_Check_return_
extern UREF_COMMS
ev_uref_match_range(
    _InoutRef_  P_EV_RANGE p_ev_range,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    P_UREF_EVENT_BLOCK p_uref_event_block);

_Check_return_
extern UREF_COMMS
ev_uref_match_slr(
    _InoutRef_  P_EV_SLR p_ev_slr,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UREF_MESSAGE uref_message,
    P_UREF_EVENT_BLOCK p_uref_event_block);

/*ncr*/
extern U32 /* number of chars output */
ev_write_docname_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     EV_DOCNO ev_docno_to,
    _InVal_     EV_DOCNO ev_docno_from);

#define slr_from_ev_slr(p_slr, p_ev_slr) ( \
    (p_slr)->col = (p_ev_slr)->col, \
    (p_slr)->row = (p_ev_slr)->row  )

/*
ev_name.c external functions
*/

extern void
ev_name_del_hold(void);

extern void
ev_name_del_release(void);

_Check_return_
extern STATUS
ev_name_make(
    _In_z_      PC_USTR ustr_name_id,
    _InVal_     EV_DOCNO ev_docno,
    _In_opt_z_  PC_USTR ustr_name_def,
    _InVal_     S32 undefine,
    _In_opt_z_  PC_USTR ustr_description);

/*
ev_rpn.c
*/

_Check_return_
extern S32
ev_len(
    _InRef_     PC_EV_CELL p_ev_cell);

/*
ev_tabl.c
*/

extern void
ev_enum_resource_dispose(
    _InoutRef_  P_RESOURCE_SPEC p_resource_spec);

extern void
ev_enum_resource_init(
    _OutRef_    P_EV_RESOURCE p_ev_resource,
    _OutRef_    P_RESOURCE_SPEC p_resource_spec,
    _InVal_     S32 category,
    _InVal_     S32 item_no,
    _InVal_     EV_DOCNO ev_docno_to,
    _InVal_     EV_DOCNO ev_docno_from);

_Check_return_
extern STATUS /* rpn number out */
ev_enum_resource_get(
    _InoutRef_  P_RESOURCE_SPEC p_resource_spec,
    P_EV_RESOURCE p_ev_resource);

/*
ev_todo.c
*/

extern void
ev_todo_add_doc_dependents(
    _InVal_     EV_DOCNO ev_docno);

/*ncr*/
extern BOOL
ev_todo_add_custom_dependents(
    _InVal_     EV_HANDLE h_custom);

/*ncr*/
extern BOOL
ev_todo_add_name_dependents(
    _InVal_     EV_HANDLE h_name);

extern void
ev_todo_add_slr(
    _InRef_     PC_EV_SLR p_ev_slr);

/*
ev_tree.c external functions
*/

_Check_return_
extern STATUS
ev_add_compiler_output_to_tree(
    P_COMPILER_OUTPUT p_compiler_output,
    _InRef_     PC_EV_SLR p_ev_slr,
    _InVal_     BOOL add_todo);

_Check_return_
extern STATUS
ev_add_ev_cell_to_tree(
    P_EV_CELL p_ev_cell,
    _InRef_     PC_EV_SLR p_ev_slr);

/*
ev_uref.c external functions
*/

PROC_UREF_EVENT_PROTO(extern, ev_uref_uref_event);

#endif /* __ev_eval_h */

/* end of ev_eval.h */
