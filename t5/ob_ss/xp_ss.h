/* xp_ss.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spreadsheet object module header */

/* MRJC April 1992 */

#ifndef __xp_ss_h
#define __xp_ss_h

#ifndef          __ev_eval_h
#include "cmodules/ev_eval.h"
#endif

/*
structure
*/

/* JAD : table structure for function calling by ev_rpn... */
typedef struct SS_FUNC_TABLE
{
#if CHECKING /* SKS 04apr95 not used except for consistency check */
    UINT index;
#endif
    P_PROC_EXEC p_proc_exec;
}
SS_FUNC_TABLE, * P_SS_FUNC_TABLE;

#if CHECKING
#define SS_FUNC_TABLE_ENTRY(i, p) { i, p }
#else
#define SS_FUNC_TABLE_ENTRY(i, p) { p }
#endif

/* ob_ss function table indices */
enum __ss_function_indices
{
    SS_FUNC_NULL = 0,

    SS_FUNC_UOP_NOT,
    SS_FUNC_UOP_MINUS,
    SS_FUNC_UOP_PLUS,

    SS_FUNC_BOP_AND,
    SS_FUNC_BOP_CONCATENATE,
    SS_FUNC_BOP_DIV,
    SS_FUNC_BOP_SUB,
    SS_FUNC_BOP_OR,
    SS_FUNC_BOP_ADD,
    SS_FUNC_BOP_POWER,
    SS_FUNC_BOP_MUL,

    SS_FUNC_REL_EQ,
    SS_FUNC_REL_GT,
    SS_FUNC_REL_GTEQ,
    SS_FUNC_REL_LT,
    SS_FUNC_REL_LTEQ,
    SS_FUNC_REL_NEQ,

    SS_FUNC_IF,

    SS_SPLIT_ABS,
    SS_SPLIT_ACOS,
    SS_SPLIT_ACOSEC,
    SS_SPLIT_ACOSECH,
    SS_SPLIT_ACOSH,
    SS_SPLIT_ACOT,
    SS_SPLIT_ACOTH,
    SS_SPLIT_ADDRESS,
    SS_SPLIT_AGE,
    /* NO    AND */
    SS_SPLIT_ASEC,
    SS_SPLIT_ASECH,
    SS_SPLIT_ASIN,
    SS_SPLIT_ASINH,
    SS_SPLIT_ATAN,
    SS_SPLIT_ATAN_2,
    SS_SPLIT_ATANH,
    /* NO    AVEDEV */
    /* NO    AVG */

    SS_SPLIT_BASE,
    SS_SPLIT_BESSELI,
    SS_SPLIT_BESSELJ,
    SS_SPLIT_BESSELK,
    SS_SPLIT_BESSELY,
    SS_SPLIT_BETA,
    SS_SPLIT_BETA_DIST,
    SS_SPLIT_BETA_INV,
    SS_SPLIT_BIN,
    SS_SPLIT_BIN2DEC,
    SS_SPLIT_BIN2HEX,
    SS_SPLIT_BIN2OCT,
    SS_SPLIT_BINOM_DIST,
    SS_SPLIT_BINOM_DIST_RANGE,
    SS_SPLIT_BINOM_INV,
    SS_SPLIT_BITAND,
    SS_SPLIT_BITLSHIFT,
    SS_SPLIT_BITOR,
    SS_SPLIT_BITRSHIFT,
    SS_SPLIT_BITXOR,

    SS_SPLIT_C_ACOS,
    SS_SPLIT_C_ACOSEC,
    SS_SPLIT_C_ACOSECH,
    SS_SPLIT_C_ACOSH,
    SS_SPLIT_C_ACOT,
    SS_SPLIT_C_ACOTH,
    SS_SPLIT_C_ADD,
    SS_SPLIT_C_ASEC,
    SS_SPLIT_C_ASECH,
    SS_SPLIT_C_ASIN,
    SS_SPLIT_C_ASINH,
    SS_SPLIT_C_ATAN,
    SS_SPLIT_C_ATANH,
    SS_SPLIT_C_COMPLEX,
    SS_SPLIT_C_CONJUGATE,
    SS_SPLIT_C_COS,
    SS_SPLIT_C_COSEC,
    SS_SPLIT_C_COSECH,
    SS_SPLIT_C_COSH,
    SS_SPLIT_C_COT,
    SS_SPLIT_C_COTH,
    SS_SPLIT_C_DIV,
    SS_SPLIT_C_EXP,
    SS_SPLIT_C_IMAGINARY,
    SS_SPLIT_C_LN,
    SS_SPLIT_C_MUL,
    SS_SPLIT_C_POWER,
    SS_SPLIT_C_RADIUS,
    SS_SPLIT_C_REAL,
    SS_SPLIT_C_ROUND,
    SS_SPLIT_C_SEC,
    SS_SPLIT_C_SECH,
    SS_SPLIT_C_SIN,
    SS_SPLIT_C_SINH,
    SS_SPLIT_C_SQRT,
    SS_SPLIT_C_SUB,
    SS_SPLIT_C_TAN,
    SS_SPLIT_C_TANH,
    SS_SPLIT_C_THETA,

    SS_SPLIT_CEILING,
    SS_SPLIT_CHAR,
    SS_SPLIT_CHISQ_DIST,
    SS_SPLIT_CHISQ_DIST_RT,
    SS_SPLIT_CHISQ_INV,
    SS_SPLIT_CHISQ_INV_RT,
    SS_SPLIT_CHISQ_TEST,
    SS_SPLIT_CHOOSE,
    SS_SPLIT_CLEAN,
    SS_SPLIT_CODE,
    SS_SPLIT_COL,
    SS_SPLIT_COLS,
    SS_SPLIT_COMBIN,
    SS_SPLIT_COMBINA,
    SS_SPLIT_COMMAND,
    SS_SPLIT_CONFIDENCE_NORM,
    SS_SPLIT_CONFIDENCE_T,
    SS_SPLIT_CORREL,
    SS_SPLIT_COS,
    SS_SPLIT_COSEC,
    SS_SPLIT_COSECH,
    SS_SPLIT_COSH,
    SS_SPLIT_COT,
    SS_SPLIT_COTH,
    /* NO    COUNT */
    /* NO    COUNTA */
    /* NO    COUNTBLANK */
    SS_SPLIT_COVARIANCE_P,
    SS_SPLIT_COVARIANCE_S,
    SS_SPLIT_CTERM,
    SS_SPLIT_CURRENT_CELL,

    SS_SPLIT_DATE,
    SS_SPLIT_DATEVALUE,
    SS_SPLIT_DAY,
    SS_SPLIT_DAYNAME,
    SS_SPLIT_DAYS,
    SS_SPLIT_DAYS_360,
    SS_SPLIT_DB,
    /* NO    DCOUNT etc. */
    SS_SPLIT_DDB,
    SS_SPLIT_DEC2BIN,
    SS_SPLIT_DEC2HEX,
    SS_SPLIT_DEC2OCT,
    SS_SPLIT_DECIMAL,
    SS_SPLIT_DEG,
    SS_SPLIT_DELTA,
    SS_SPLIT_DEREF,
    SS_SPLIT_DOLLAR,
    SS_SPLIT_DOUBLECLICK,

    SS_SPLIT_EDATE,
    SS_SPLIT_EOMONTH,
    SS_SPLIT_ERF,
    SS_SPLIT_ERFC,
    SS_SPLIT_EVEN,
    SS_SPLIT_EXACT,
    SS_SPLIT_EXP,
    SS_SPLIT_EXPON_DIST,

    SS_SPLIT_F_DIST,
    SS_SPLIT_F_DIST_RT,
    SS_SPLIT_F_INV,
    SS_SPLIT_F_INV_RT,
    SS_SPLIT_F_TEST,
    SS_SPLIT_FACT,
    SS_SPLIT_FACTDOUBLE,
    SS_SPLIT_FALSE,
    SS_SPLIT_FIND,
    SS_SPLIT_FISHER,
    SS_SPLIT_FISHERINV,
    SS_SPLIT_FIXED,
    SS_SPLIT_FLIP,
    SS_SPLIT_FLOOR,
    SS_SPLIT_FORECAST,
    SS_SPLIT_FORMULA_TEXT,
    SS_SPLIT_FREQUENCY,
    SS_SPLIT_FV,
    SS_SPLIT_FVSCHEDULE,

    SS_SPLIT_GAMMA,
    SS_SPLIT_GAMMA_DIST,
    SS_SPLIT_GAMMA_INV,
    SS_SPLIT_GAMMALN,
    /* NO    GCD */
    /* NO    GEOMEAN */
    SS_SPLIT_GESTEP,
    SS_SPLIT_GRAND,
    SS_SPLIT_GROWTH,

    /* NO    HARMEAN */
    SS_SPLIT_HEX2BIN,
    SS_SPLIT_HEX2DEC,
    SS_SPLIT_HEX2OCT,
    SS_SPLIT_HOUR,
    SS_SPLIT_HYPGEOM_DIST,

    SS_SPLIT_INDEX,
    SS_SPLIT_INT,
    SS_SPLIT_INTERCEPT,
    /* NO    IRR */
    SS_SPLIT_ISBLANK,
    SS_SPLIT_ISERR,
    SS_SPLIT_ISERROR,
    SS_SPLIT_ISEVEN,
    SS_SPLIT_ISLOGICAL,
    SS_SPLIT_ISNA,
    SS_SPLIT_ISNONTEXT,
    SS_SPLIT_ISNUMBER,
    SS_SPLIT_ISODD,
    SS_SPLIT_ISOWEEKNUM,
    SS_SPLIT_ISREF,
    SS_SPLIT_ISTEXT,

    SS_SPLIT_JOIN,

    /* NO    KURT */

    SS_SPLIT_LARGE,
    /* NO    LCM */
    SS_SPLIT_LEFT,
    SS_SPLIT_LENGTH,
    SS_SPLIT_LINEST,
    SS_SPLIT_LISTCOUNT,
    SS_SPLIT_LN,
    SS_SPLIT_LOG,
    SS_SPLIT_LOGEST,
    SS_SPLIT_LOGNORM_DIST,
    SS_SPLIT_LOGNORM_INV,
    SS_SPLIT_LOWER,

    SS_SPLIT_M_DETERM,
    SS_SPLIT_M_INVERSE,
    SS_SPLIT_M_MULT,
    SS_SPLIT_M_UNIT,

    /* NO    MAX */
    /* NO    MEDIAN */
    SS_SPLIT_MID,
    /* NO    MIN */
    SS_SPLIT_MINUTE,
    /* NO    MIRR */
    SS_SPLIT_MOD,
    SS_SPLIT_MODE_SNGL,
    SS_SPLIT_MONTH,
    SS_SPLIT_MONTHDAYS,
    SS_SPLIT_MONTHNAME,
    SS_SPLIT_MROUND,

    SS_SPLIT_N,
    SS_SPLIT_NA,
    SS_SPLIT_NEGBINOM_DIST,
    SS_SPLIT_NORM_DIST,
    SS_SPLIT_NORM_S_DIST,
    SS_SPLIT_NORM_INV,
    SS_SPLIT_NORM_S_INV,
    SS_SPLIT_NOT,
    SS_SPLIT_NOW,
    SS_SPLIT_NPER,
    /* NO    NPV */

    SS_SPLIT_OCT2BIN,
    SS_SPLIT_OCT2DEC,
    SS_SPLIT_OCT2HEX,
    SS_SPLIT_ODD,

    SS_SPLIT_ODF_BETADIST,
    SS_SPLIT_ODF_COMPLEX,
    SS_SPLIT_ODF_FV,
    SS_SPLIT_ODF_IMABS,
    SS_SPLIT_ODF_IMAGINARY,
    SS_SPLIT_ODF_IMARGUMENT,
    SS_SPLIT_ODF_IMCONJUGATE,
    SS_SPLIT_ODF_IMCOS,
    SS_SPLIT_ODF_IMDIV,
    SS_SPLIT_ODF_IMEXP,
    SS_SPLIT_ODF_IMLN,
    SS_SPLIT_ODF_IMLOG10,
    SS_SPLIT_ODF_IMLOG2,
    SS_SPLIT_ODF_IMPOWER,
    SS_SPLIT_ODF_IMPRODUCT,
    SS_SPLIT_ODF_IMREAL,
    SS_SPLIT_ODF_IMSIN,
    SS_SPLIT_ODF_IMSQRT,
    SS_SPLIT_ODF_IMSUB,
    SS_SPLIT_ODF_IMSUM,
    SS_SPLIT_ODF_INDEX,
    SS_SPLIT_ODF_INT,
    SS_SPLIT_ODF_IRR,
    SS_SPLIT_ODF_LOG10,
    SS_SPLIT_ODF_MOD,
    SS_SPLIT_ODF_PMT,
    SS_SPLIT_ODF_TDIST,
    SS_SPLIT_ODF_TYPE,

    /* NO    OR */

    SS_SPLIT_PAGE,
    SS_SPLIT_PAGES,
    SS_SPLIT_PEARSON,
    SS_SPLIT_PERCENTILE_EXC,
    SS_SPLIT_PERCENTILE_INC,
    SS_SPLIT_PERCENTRANK_EXC,
    SS_SPLIT_PERCENTRANK_INC,
    SS_SPLIT_PERMUT,
    SS_SPLIT_PHI,
    SS_SPLIT_PI,
    SS_SPLIT_PMT,
    SS_SPLIT_POISSON_DIST,
    SS_SPLIT_POWER,
    SS_SPLIT_PROB,
    /* NO    PRODUCT */
    SS_SPLIT_PROPER,
    SS_SPLIT_PV,

    SS_SPLIT_QUARTILE_EXC,
    SS_SPLIT_QUARTILE_INC,
    SS_SPLIT_QUOTIENT,

    SS_SPLIT_RAD,
    SS_SPLIT_RAND,
    SS_SPLIT_RANDBETWEEN,
    SS_SPLIT_RATE,
    SS_SPLIT_RANK,
    SS_SPLIT_RANK_EQ,
    SS_SPLIT_REPLACE,
    SS_SPLIT_REPT,
    SS_SPLIT_REVERSE,
    SS_SPLIT_RIGHT,
    SS_SPLIT_ROUND,
    SS_SPLIT_ROUNDDOWN,
    SS_SPLIT_ROUNDUP,
    SS_SPLIT_ROW,
    SS_SPLIT_ROWS,
    SS_SPLIT_RSQ,

    SS_SPLIT_SERIESSUM,
    SS_SPLIT_SEC,
    SS_SPLIT_SECH,
    SS_SPLIT_SECOND,
    SS_SPLIT_SET_NAME,
    SS_SPLIT_SGN,
    SS_SPLIT_SIN,
    SS_SPLIT_SINH,
    /* NO    SKEW */
    SS_SPLIT_SLN,
    SS_SPLIT_SLOPE,
    SS_SPLIT_SMALL,
    SS_SPLIT_SORT,
    SS_SPLIT_SPEARMAN,
    SS_SPLIT_SQR,
    SS_SPLIT_STANDARDIZE,
    /* NO    STD */
    /* NO    STDP */
    SS_SPLIT_STEYX,
    SS_SPLIT_STRING,
    SS_SPLIT_SUBSTITUTE,
    /* NO    SUM */
    /* NO    SUMSQ */
    SS_SPLIT_SUMPRODUCT,
    SS_SPLIT_SUM_X2MY2,
    SS_SPLIT_SUM_X2PY2,
    SS_SPLIT_SUM_XMY2,
    SS_SPLIT_SYD,

    SS_SPLIT_T,
    SS_SPLIT_T_DIST,
    SS_SPLIT_T_DIST_2T,
    SS_SPLIT_T_DIST_RT,
    SS_SPLIT_T_INV,
    SS_SPLIT_T_INV_2T,
    SS_SPLIT_T_TEST,
    SS_SPLIT_TAN,
    SS_SPLIT_TANH,
    SS_SPLIT_TERM,
    SS_SPLIT_TEXT,
    SS_SPLIT_TIME,
    SS_SPLIT_TIMEVALUE,
    SS_SPLIT_TODAY,
    SS_SPLIT_TRANSPOSE,
    SS_SPLIT_TREND,
    SS_SPLIT_TRIM,
    SS_SPLIT_TRIMMEAN,
    SS_SPLIT_TRUE,
    SS_SPLIT_TRUNC,
    SS_SPLIT_TYPE,

    SS_SPLIT_UPPER,

    SS_SPLIT_VALUE,
    /* NO    VAR */
    /* NO    VARP */
    SS_SPLIT_VERSION,

    SS_SPLIT_WEEKDAY,
    SS_SPLIT_WEEKNUMBER,
    SS_SPLIT_WEIBULL_DIST,

    SS_SPLIT_YEAR,

    SS_SPLIT_Z_TEST,

    SS_FUNC__MAX
};

enum ALERT_RESULT_CODES
{
    ALERT_RESULT_NONE = 0,
    ALERT_RESULT_CLOSE,
    ALERT_RESULT_BUTTON_1,
    ALERT_RESULT_BUTTON_2
};

typedef struct SS_INPUT_EXEC
{
    PC_SS_STRINGC p_ss_string_message;
    PC_SS_STRINGC p_ss_string_button_1;
    PC_SS_STRINGC p_ss_string_button_2;
    EV_DOCNO ev_docno;
}
SS_INPUT_EXEC, * P_SS_INPUT_EXEC;

extern H_DIALOG h_alert_dialog;
extern S32 alert_result;

extern H_DIALOG h_input_dialog;
extern S32 input_result;
extern UI_TEXT ui_text_input;

#endif /* __xp_ss_h */

/* end of xp_ss.h */
