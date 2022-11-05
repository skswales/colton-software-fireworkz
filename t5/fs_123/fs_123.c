/* fs_123.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Lotus 1-2-3 spreadsheet save object module for Fireworkz */

/* MRJC original 123 converter February 1988 / for Fireworkz July 1993 */

#include "common/gflags.h"

#include "fs_123/fs_123.h"

#include "ob_skel/ff_io.h"

#ifndef          __ev_eval_h
#include "cmodules/ev_eval.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#include <ctype.h> /* for "C"isalpha and friends */

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_FS_LOTUS123)
extern PC_U8 rb_fs_123_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_FS_LOTUS123 &rb_fs_123_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FS_LOTUS123 DONT_LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FS_LOTUS123 DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FS_LOTUS123 DONT_LOAD_RESOURCES

/*
scanner communication
*/

typedef struct LOTUS123_DATE
{
    S32 day;
    S32 month;
    S32 year;
}
LOTUS123_DATE, * P_LOTUS123_DATE;

typedef struct LOTUS123_STRING
{
    PC_U8 p_string;
    U32 len;
}
LOTUS123_STRING, * P_LOTUS123_STRING;

typedef struct LOTUS123_SYMBOL
{
    S32 symno;
    union LOTUS123_SYMBOL_ARG
    {
        F64 f64;
        struct LOTUS123_SYMBOL_ARG_RANGE
        {
            SLR s;
            SLR e;
        } range;
        LOTUS123_STRING lotus123_string;
        S32 ix_function;
        LOTUS123_DATE lotus123_date;
    } arg;
}
LOTUS123_SYMBOL, * P_LOTUS123_SYMBOL;

/* special symbol constants */
#define LOTUS123_SYM_BAD      -1
#define LOTUS123_SYM_BLANK    -2
#define LOTUS123_SYM_OBRACKET -3
#define LOTUS123_SYM_CBRACKET -4
#define LOTUS123_SYM_COMMA    -5
#define LOTUS123_SYM_FUNC     -6

/* table of opcodes giving Lotus 1-2-3 file structure */

typedef const struct LOTUS123_INS * PC_LOTUS123_INS;

typedef STATUS (* P_PROC_WRITE) (
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_LOTUS123_INS p_lotus123_ins);

#define PROC_WRITE_PROTO(_proc_name) \
static STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format, \
    _InRef_     PC_LOTUS123_INS p_lotus123_ins)

typedef struct LOTUS123_INS
{
    U8 opcode;
    U32 len_ins;
    PC_BYTE p_data;
    S32 len_data;
    U8 pattern;
    P_PROC_WRITE p_proc_write;
}
LOTUS123_INS;

static COL g_s_col, g_e_col;
static ROW g_s_row, g_e_row;

/******************************************************************************
*
* recognise a constant and classify
*
******************************************************************************/

enum RECOG_CODES
{
    RECOG_NONE,
    RECOG_DATE,
    RECOG_INTEGER,
    RECOG_REAL
};

_Check_return_
static S32 /* recog code */
lotus123_recog_constant(
    _In_z_      PC_U8Z p_u8_in,
    _OutRef_    P_F64 p_f64,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_mon,
    _OutRef_    P_S32 p_yr,
    _OutRef_    P_S32 p_cs /* n scanned */)
{
    int n;

    /* check for date without brackets */
    n = sscanf_s(p_u8_in, "%d.%d.%d" "%n", p_day, p_mon, p_yr, p_cs);
    if(3 == n)
    {
        *p_f64 = 0.0;
        return(RECOG_DATE);
    }

    *p_day = *p_mon = *p_yr = 0;

    CODE_ANALYSIS_ONLY(*p_f64 = 0.0);
    n = sscanf_s(p_u8_in, "%lf" "%n", p_f64, p_cs);
    if(1 == n)
    {
        if((floor(*p_f64) == *p_f64) && (*p_f64 < 32767.0) && (*p_f64 > -32767.0))
            return(RECOG_INTEGER);

        return(RECOG_REAL);
    }

    *p_f64 = 0.0;

    /* ensure that cs is zero - some sscanfs don't set it to zero if the scan fails */
    *p_cs = 0;

    return(RECOG_NONE);
}

/******************************************************************************
*
* scan a cell reference
*
******************************************************************************/

_Check_return_ _Success_(0 != return)
static U32 /* chars scanned */
lotus123_recog_slr(
    _In_z_      PC_U8Z p_u8_in,
    _OutRef_    P_SLR p_slr)
{
    PC_U8Z p_u8 = p_u8_in;
    U32 tot_scanned;
    S32 abs_col = 0, abs_row = 0;

    if(*p_u8 == CH_DOLLAR_SIGN)
    {
        ++p_u8;
        abs_col = 0x8000;
    }

    {
    S32 col;
    if((tot_scanned = (U32) stox((PC_USTR) p_u8, &col)) == 0)
        return(0);
    p_slr->col = (COL) col & 0x00FF;
    } /*block*/

    p_u8 += tot_scanned;
    if(*p_u8 == CH_DOLLAR_SIGN)
    {
        ++p_u8;
        abs_row = 0x8000;
    }

    if(!/*"C"*/isdigit(*p_u8))
        return(0);

    {
    S32 row;
    int scanned;

    if(sscanf_s(p_u8, "%d%n", &row, &scanned) < 1 || !scanned)
        return(0);

    p_slr->row = (row & 0x1FFF) - 1;
    tot_scanned += scanned;
    } /*block*/

    p_slr->col |= abs_col;
    p_slr->row |= abs_row;

    return(tot_scanned);
}

/*
lotus123 compiler statics
*/

enum SCAN_DATE_CODES
{
    SCAN_DATE_CBRACKET = 1,
    SCAN_DATE_DAY,
    SCAN_DATE_COMMA_1,
    SCAN_DATE_MONTH,
    SCAN_DATE_COMMA_2,
    SCAN_DATE_YEAR,
    SCAN_DATE_OBRACKET,
    SCAN_DATE_START
};

#define check_next_symbol() ( \
    (lotus123_symbol.symno != LOTUS123_SYM_BLANK) \
        ? lotus123_symbol.symno \
        : scan_next_symbol() )

static LOTUS123_SYMBOL lotus123_symbol;

static QUICK_BLOCK quick_block_rpn;

static PC_U8Z p_compile_scan;

static S32 scan_date;

static SLR reference_slr;

_Check_return_
static STATUS
lotus123_expr(void);

/******************************************************************************
*
* table of Lotus operators and equivalents that we can handle for save
*
* this table is in opcode (fno) order
*
******************************************************************************/

typedef struct OPR_EQUIVALENT
{
    PC_A7STR p_text_t5;
    U8 ftype;
    U8 fno;
    U8 n_args;
#define OPR_ARGS_VAR '\xFF'
}
OPR_EQUIVALENT; typedef const OPR_EQUIVALENT * PC_OPR_EQUIVALENT; /* don't you dare remove that const! */

#define opr_entry(fno, ftype, n_args, p_text_t5) \
    { (p_text_t5), (ftype), (fno), (n_args) }

static const OPR_EQUIVALENT
opr_equivalent[] =
{
    opr_entry(LF_CONST,      LO_CONST,              0,  ""),
    opr_entry(LF_SLR,        LO_CONST,              0,  ""),
    opr_entry(LF_RANGE,      LO_CONST,              0,  ""),
    opr_entry(LF_END,        LO_END,                0,  ""),
    opr_entry(LF_BRACKETS,   LO_BRACKETS,           0,  ""),
    opr_entry(LF_INTEGER,    LO_CONST,              0,  ""),
    opr_entry(LF_STRING,     LO_CONST,              0,  ""),

    opr_entry(LF_UMINUS,     LO_UNARY,              1,  "-"),
    opr_entry(LF_PLUS,       LO_BINARY,             2,  "+"),
    opr_entry(LF_MINUS,      LO_BINARY,             2,  "-"),
    opr_entry(LF_TIMES,      LO_BINARY,             2,  "*"),
    opr_entry(LF_DIVIDE,     LO_BINARY,             2,  "/"),
    opr_entry(LF_POWER,      LO_BINARY,             2,  "^"),
    opr_entry(LF_EQUALS,     LO_BINARY,             2,  "="),
    opr_entry(LF_NOTEQUAL,   LO_BINARY,             2,  "<>"),
    opr_entry(LF_LTEQUAL,    LO_BINARY,             2,  "<="),
    opr_entry(LF_GTEQUAL,    LO_BINARY,             2,  ">="),
    opr_entry(LF_LT,         LO_BINARY,             2,  "<"),
    opr_entry(LF_GT,         LO_BINARY,             2,  ">"),
    opr_entry(LF_AND,        LO_BINARY,             2,  "&"),
    opr_entry(LF_OR,         LO_BINARY,             2,  "|"),
    opr_entry(LF_NOT,        LO_UNARY,              1,  "!"),
    opr_entry(LF_UPLUS,      LO_UNARY,              1,  "+"),

    opr_entry(LF_NA,         LO_FUNC,               0,  "NA"),
  /*opr_entry(LF_ERR,        LO_FUNC,               0,  ".ERR"),*/
    opr_entry(LF_ABS,        LO_FUNC,               1,  "ABS"),
    opr_entry(LF_INT,        LO_FUNC,               1,  "INT"),
    opr_entry(LF_SQRT,       LO_FUNC,               1,  "SQR"), /* 123:SQRT */
    opr_entry(LF_LOG,        LO_FUNC,               1,  "LOG"), /* 123 won't support second optional parameter */
    opr_entry(LF_LN,         LO_FUNC,               1,  "LN"),
    opr_entry(LF_PI,         LO_FUNC,               0,  "PI"),
    opr_entry(LF_SIN,        LO_FUNC,               1,  "SIN"),
    opr_entry(LF_COS,        LO_FUNC,               1,  "COS"),
    opr_entry(LF_TAN,        LO_FUNC,               1,  "TAN"),
    opr_entry(LF_ATAN2,      LO_FUNC,               1,  "ATAN_2"), /* 123:ATAN2 */
    opr_entry(LF_ATAN,       LO_FUNC,               1,  "ATAN"),
    opr_entry(LF_ASIN,       LO_FUNC,               1,  "ASIN"),
    opr_entry(LF_ACOS,       LO_FUNC,               1,  "ACOS"),
    opr_entry(LF_EXP,        LO_FUNC,               1,  "EXP"),
    opr_entry(LF_MOD,        LO_FUNC,               2,  "MOD"),
    opr_entry(LF_CHOOSE,     LO_FUNC,    OPR_ARGS_VAR,  "CHOOSE"),
    opr_entry(LF_ISNA,       LO_FUNC,               1,  "ISNA"),
    opr_entry(LF_ISERR,      LO_FUNC,               1,  "ISERR"),
    opr_entry(LF_FALSE,      LO_FUNC,               0,  "FALSE"),
    opr_entry(LF_TRUE,       LO_FUNC,               1,  "TRUE"),
    opr_entry(LF_RAND,       LO_FUNC,               0,  "RAND"),
    opr_entry(LF_DATE,       LO_FUNC,               3,  "DATE"),
    opr_entry(LF_TODAY,      LO_FUNC,               0,  "NOW"),
    opr_entry(LF_PMT,        LO_FUNC,               3,  "PMT"),
    opr_entry(LF_PV,         LO_FUNC,               3,  "PV"),
    opr_entry(LF_FV,         LO_FUNC,               3,  "FV"),
    opr_entry(LF_IF,         LO_FUNC,               3,  "IF"),
    opr_entry(LF_DAY,        LO_FUNC,               1,  "DAY"),
    opr_entry(LF_MONTH,      LO_FUNC,               1,  "MONTH"),
    opr_entry(LF_YEAR,       LO_FUNC,               1,  "YEAR"),
    opr_entry(LF_ROUND,      LO_FUNC,               2,  "ROUND"),
    opr_entry(LF_TIME,       LO_FUNC,               3,  "TIME"),
    opr_entry(LF_HOUR,       LO_FUNC,               1,  "HOUR"),
    opr_entry(LF_MINUTE,     LO_FUNC,               1,  "MINUTE"),
    opr_entry(LF_SECOND,     LO_FUNC,               1,  "SECOND"),
    opr_entry(LF_ISN,        LO_FUNC,               1,  "ISNONTEXT"), /* 123:ISNUMBER */
    opr_entry(LF_ISS,        LO_FUNC,               1,  "ISTEXT"), /* 123:ISSTRING */
    opr_entry(LF_LENGTH,     LO_FUNC,               1,  "LENGTH"),
    opr_entry(LF_VALUE,      LO_FUNC,               1,  "VALUE"),
    opr_entry(LF_FIXED,      LO_FUNC,               2,  "STRING"),
    opr_entry(LF_MID,        LO_FUNC,               3,  "MID"),
    opr_entry(LF_CHR,        LO_FUNC,               1,  "CHAR"),
    opr_entry(LF_ASCII,      LO_FUNC,               1,  "CODE"),
    opr_entry(LF_FIND,       LO_FUNC,               3,  "FIND"),
    opr_entry(LF_DATEVALUE,  LO_FUNC,               1,  "DATEVALUE"),
    opr_entry(LF_TIMEVALUE,  LO_FUNC,               1,  "TIMEVALUE"),
  /*opr_entry(LF_CELLPOINTER,LO_FUNC,               1,  ".CELLPOINTER"),*/
    opr_entry(LF_SUM,        LO_FUNC,    OPR_ARGS_VAR,  "SUM"),
    opr_entry(LF_AVG,        LO_FUNC,    OPR_ARGS_VAR,  "AVG"),
    opr_entry(LF_CNT,        LO_FUNC,    OPR_ARGS_VAR,  "COUNTA"), /* 123:COUNT (maps to COUNTA according to Excel) */
    opr_entry(LF_MIN,        LO_FUNC,    OPR_ARGS_VAR,  "MIN"),
    opr_entry(LF_MAX,        LO_FUNC,    OPR_ARGS_VAR,  "MAX"),
    opr_entry(LF_VLOOKUP,    LO_FUNC,               3,  "VLOOKUP"),
    opr_entry(LF_NPV,        LO_FUNC,               2,  "NPV"),
    opr_entry(LF_VAR,        LO_FUNC,    OPR_ARGS_VAR,  "VARP"), /* 123:VAR (maps to VARP according to Excel) */
    opr_entry(LF_STD,        LO_FUNC,    OPR_ARGS_VAR,  "STDP"), /* 123:STD (maps to STDEVP according to Excel) */
    opr_entry(LF_IRR,        LO_FUNC,               2,  "IRR"),
    opr_entry(LF_HLOOKUP,    LO_FUNC,               3,  "HLOOKUP"),
  /*opr_entry(LF_DSUM,       LO_FUNC,               3,  ".DSUM_123"),*/ /* 123:DSUM */ /* Fireworkz is incompatible */
  /*opr_entry(LF_DAVG,       LO_FUNC,               3,  ".DAVG_123"),*/ /* 123:DAVG */ /* Fireworkz is incompatible */
  /*opr_entry(LF_DCNT,       LO_FUNC,               3,  ".DCOUNT_123"),*/ /* 123:DCOUNT (maps to DCOUNTA according to Excel) */ /* Fireworkz is incompatible */
  /*opr_entry(LF_DMIN,       LO_FUNC,               3,  ".DMIN_123"),*/ /* 123:DMIN */ /* Fireworkz is incompatible */
  /*opr_entry(LF_DMAX,       LO_FUNC,               3,  ".DMAX_123"),*/ /* 123:DMAX */ /* Fireworkz is incompatible */
  /*opr_entry(LF_DVAR,       LO_FUNC,               3,  ".DVAR_123"),*/ /* 123:DVAR (maps to DVARP according to Excel) */ /* Fireworkz is incompatible */
  /*opr_entry(LF_DSTD,       LO_FUNC,               3,  ".DSTD_123"),*/ /* 123:DSTD (maps to DSTDEVP according to Excel) */ /* Fireworkz is incompatible */
    opr_entry(LF_INDEX,      LO_FUNC,               3,  "INDEX"),
    opr_entry(LF_COLS,       LO_FUNC,               1,  "COLS"),
    opr_entry(LF_ROWS,       LO_FUNC,               1,  "ROWS"),
    opr_entry(LF_REPEAT,     LO_FUNC,               2,  "REPT"), /* 123:REPEAT */
    opr_entry(LF_UPPER,      LO_FUNC,               1,  "UPPER"),
    opr_entry(LF_LOWER,      LO_FUNC,               1,  "LOWER"),
    opr_entry(LF_LEFT,       LO_FUNC,               2,  "LEFT"),
    opr_entry(LF_RIGHT,      LO_FUNC,               2,  "RIGHT"),
    opr_entry(LF_REPLACE,    LO_FUNC,               4,  "REPLACE"),
    opr_entry(LF_PROPER,     LO_FUNC,               1,  "PROPER"),
  /*opr_entry(LF_CELL,       LO_FUNC,               2,  ".CELL"),*/
    opr_entry(LF_TRIM,       LO_FUNC,               1,  "TRIM"),
    opr_entry(LF_CLEAN,      LO_FUNC,               1,  "CLEAN"),
    opr_entry(LF_S,          LO_FUNC,               1,  "T"), /* 123:"S" */
    opr_entry(LF_V,          LO_FUNC,               1,  "N"), /* 123:"V" now "N"? */
    opr_entry(LF_STREQ,      LO_FUNC,               2,  "EXACT"),
  /*opr_entry(LF_CALL,       LO_FUNC,               1,  ".CALL"),*/
  /*opr_entry(LF_INDIRECT,   LO_FUNC,               1,  ".INDIRECT"),*/ /* end of 1985 documentation */

    opr_entry(LF_RATE,       LO_FUNC,               3,  "RATE"),
    opr_entry(LF_TERM,       LO_FUNC,               3,  "TERM"),
    opr_entry(LF_CTERM,      LO_FUNC,               3,  "CTERM"),
    opr_entry(LF_SLN,        LO_FUNC,               3,  "SLN"),
    opr_entry(LF_SOY,        LO_FUNC,               4,  "SYD"),
    opr_entry(LF_DDB,        LO_FUNC,               4,  "DDB")
};

#define n_opr_equivalent elemof32(opr_equivalent)

/******************************************************************************
*
* lookup function in master table
*
******************************************************************************/

/*ncr*/
static S32
lotus123_flookup(
    _In_reads_(sbchars_n) PC_U8 p_ftext,
    _InVal_     U32 sbchars_n,
    _InVal_     U8 ftypes)
{
    S32 res = -1;
    U32 i;
    PC_OPR_EQUIVALENT p_opr_equivalent;

    for(i = 0, p_opr_equivalent = opr_equivalent; i < n_opr_equivalent; ++i, ++p_opr_equivalent)
    {
        if(0 != (p_opr_equivalent->ftype & ftypes))
        {
            U32 len_compare = strlen32(p_opr_equivalent->p_text_t5);

            len_compare = MAX(len_compare, sbchars_n);

            if(0 == C_strnicmp(p_ftext, p_opr_equivalent->p_text_t5, len_compare))
            {
                switch(p_opr_equivalent->fno)
                {
                case LF_UMINUS:
                case LF_UPLUS:
                    continue;

                default:
                    break;
                }

                lotus123_symbol.arg.ix_function = i;
                lotus123_symbol.symno = p_opr_equivalent->fno;
                res = len_compare;
                break;
            }
        }
    }

    return(res);
}

/******************************************************************************
*
* scan a symbol from the expression
*
******************************************************************************/

static S32
scan_next_symbol(void)
{
    if(scan_date)
        scan_date -= 1;

    /* special date scanning */
    switch(scan_date)
    {
    case SCAN_DATE_OBRACKET:
        return(lotus123_symbol.symno = LOTUS123_SYM_OBRACKET);

    case SCAN_DATE_YEAR:
        lotus123_symbol.arg.f64 = (F64) lotus123_symbol.arg.lotus123_date.year;
        return(lotus123_symbol.symno = LF_INTEGER);

    case SCAN_DATE_COMMA_2:
        return(lotus123_symbol.symno = LOTUS123_SYM_COMMA);

    case SCAN_DATE_MONTH:
        lotus123_symbol.arg.f64 = (F64) lotus123_symbol.arg.lotus123_date.month;
        return(lotus123_symbol.symno = LF_INTEGER);

    case SCAN_DATE_COMMA_1:
        return(lotus123_symbol.symno = LOTUS123_SYM_COMMA);

    case SCAN_DATE_DAY:
        lotus123_symbol.arg.f64 = (F64) lotus123_symbol.arg.lotus123_date.day;
        return(lotus123_symbol.symno = LF_INTEGER);

    case SCAN_DATE_CBRACKET:
        return(lotus123_symbol.symno = LOTUS123_SYM_CBRACKET);

    default:
        break;
    }

    StrSkipSpaces(p_compile_scan);

    /* check for end of expression */
    if(CH_NULL == PtrGetByte(p_compile_scan))
        return(lotus123_symbol.symno = LF_END);

    /* check for constant */
    if(/*"C"*/isdigit(PtrGetByte(p_compile_scan)) || (PtrGetByte(p_compile_scan) == CH_FULL_STOP))
    {
        S32 scan_code, scanned;

        if((scan_code = lotus123_recog_constant(p_compile_scan,
                                                &lotus123_symbol.arg.f64,
                                                &lotus123_symbol.arg.lotus123_date.day,
                                                &lotus123_symbol.arg.lotus123_date.month,
                                                &lotus123_symbol.arg.lotus123_date.year,
                                                &scanned)) != 0)
        {
            p_compile_scan += scanned;
            switch(scan_code)
            {
            case RECOG_DATE:
                scan_date = SCAN_DATE_START;
                (void) lotus123_flookup("datef", elemof32("datef")-1, LO_FUNC);
                return(lotus123_symbol.symno = LOTUS123_SYM_FUNC);

            case RECOG_INTEGER:
                return(lotus123_symbol.symno = LF_INTEGER);

            case RECOG_REAL:
                return(lotus123_symbol.symno = LF_CONST);

            default: default_unhandled();
                break;
            }
        }
    }

    /* check for cell reference/range */
    if(/*"C"*/isalpha(p_compile_scan[0]) || p_compile_scan[0] == CH_DOLLAR_SIGN)
    {
        S32 scanned;

        if((scanned = lotus123_recog_slr(p_compile_scan, &lotus123_symbol.arg.range.s)) != 0)
        {
            p_compile_scan += scanned;
            /* check for another SLR to make range */
            if((scanned = lotus123_recog_slr(p_compile_scan, &lotus123_symbol.arg.range.e)) != 0)
            {
                p_compile_scan += scanned;
                return(lotus123_symbol.symno = LF_RANGE);
            }

            return(lotus123_symbol.symno = LF_SLR);
        }
    }

    /* check for function */
    if(/*"C"*/isalpha(p_compile_scan[0]))
    {
        int len = 0;
        while(/*"C"*/isalpha(p_compile_scan[len]) || /*"C"*/isdigit(p_compile_scan[len]))
            ++len;

        if(status_ok(lotus123_flookup(p_compile_scan, len, LO_FUNC)))
        {
            p_compile_scan += len;
            return(lotus123_symbol.symno = LOTUS123_SYM_FUNC);
        }

        return(lotus123_symbol.symno = LOTUS123_SYM_BAD);
    }

    /* check for string */
    if(p_compile_scan[0] == CH_QUOTATION_MARK)
    {
        PC_U8Z p_u8 = p_compile_scan + 1;
        while(*p_u8 && *p_u8 != p_compile_scan[0])
            ++p_u8;
        if(*p_u8 != p_compile_scan[0])
            return(lotus123_symbol.symno = LOTUS123_SYM_BAD);

        lotus123_symbol.arg.lotus123_string.p_string = p_compile_scan + 1;
        lotus123_symbol.arg.lotus123_string.len = PtrDiffBytesU32(p_u8, p_compile_scan);
        p_compile_scan = p_u8 + 1;
        return(lotus123_symbol.symno = LF_STRING);
    }

    /* check for special operators */
    switch(p_compile_scan[0])
    {
    case CH_LEFT_PARENTHESIS:
        ++p_compile_scan;
        return(lotus123_symbol.symno = LOTUS123_SYM_OBRACKET);

    case CH_RIGHT_PARENTHESIS:
        ++p_compile_scan;
        return(lotus123_symbol.symno = LOTUS123_SYM_CBRACKET);

    case CH_COMMA:
        ++p_compile_scan;
        return(lotus123_symbol.symno = LOTUS123_SYM_COMMA);

    default:
        break;
    }

    { /* check for operator */
    STATUS len;

    if(status_ok(len = lotus123_flookup(p_compile_scan, 0, (U8) (LO_BINARY | LO_UNARY))))
        p_compile_scan += len;
    else
        lotus123_symbol.symno = LOTUS123_SYM_BAD;

    return(lotus123_symbol.symno);
    } /*block*/
}

#define lotus123_rpn_out_bytes(p_quick_block_rpn, p_data, n_bytes) \
    quick_block_bytes_add(p_quick_block_rpn, p_data, n_bytes)

/******************************************************************************
*
* output byte to rpn
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_rpn_out_U8(
    _InVal_     U8 u8)
{
    return(lotus123_rpn_out_bytes(&quick_block_rpn, &u8, 1));
}

/******************************************************************************
*
* output 16-bit value to rpn (little-endian)
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_rpn_out_S16(
    _InVal_     S16 s16)
{
    return(lotus123_rpn_out_bytes(&quick_block_rpn, (P_U8) &s16, sizeof32(s16)));
}

/******************************************************************************
*
* output floating point value to rpn (8087-format double)
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_rpn_out_F64(
    _InVal_     F64 f64)
{
    BYTE fp_8087[8];

    writeval_F64_as_8087(fp_8087, f64);

    return(lotus123_rpn_out_bytes(&quick_block_rpn, fp_8087, sizeof32(fp_8087)));
}

/******************************************************************************
*
* make a Lotus form absolute/relative cell reference
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_rpn_out_slr(
    _InRef_     PC_SLR p_slr)
{
    {
    S16 col = (S16) (p_slr->col & 0x00FF);
    if((p_slr->col & 0x8000) == 0)
        col = (S16) (((col - (S16) reference_slr.col) & 0x00FF) | 0x8000);
    status_return(lotus123_rpn_out_S16(col));
    } /*block*/

    {
    S16 row = (S16) (p_slr->row & 0x1FFF);
    if((p_slr->row & 0x8000) == 0)
        row = (S16) (((row - (S16) reference_slr.row) & 0x1FFF) | 0x8000);
    return(lotus123_rpn_out_S16(row));
    } /*block*/
}

/******************************************************************************
*
* recognise an element of a list of function arguments
*
******************************************************************************/

_Check_return_
static STATUS
element(void)
{
    STATUS status;

    if(check_next_symbol() == LF_RANGE)
    {
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        if(status_ok(status = lotus123_rpn_out_U8(LF_RANGE)))
        if(status_ok(status = lotus123_rpn_out_slr(&lotus123_symbol.arg.range.s)))
                     status = lotus123_rpn_out_slr(&lotus123_symbol.arg.range.e);
        return(status);
    }

    return(lotus123_expr());
}

/******************************************************************************
*
* process a function with arguments
*
******************************************************************************/

_Check_return_
static STATUS
procfunc(
    _InRef_     PC_OPR_EQUIVALENT p_opr_equivalent)
{
    S32 narg = 0;

    if(check_next_symbol() != LOTUS123_SYM_OBRACKET)
    {
        lotus123_symbol.symno = LOTUS123_SYM_BAD;
        return(0);
    }

    do
    {
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;
        status_return(element());
        ++narg;
    }
    while(check_next_symbol() == LOTUS123_SYM_COMMA);

    if(check_next_symbol() != LOTUS123_SYM_CBRACKET)
    {
        lotus123_symbol.symno = LOTUS123_SYM_BAD;
        return(narg);
    }

    if((p_opr_equivalent->n_args != OPR_ARGS_VAR) && (p_opr_equivalent->n_args != narg))
        return(lotus123_symbol.symno = LOTUS123_SYM_BAD);

    lotus123_symbol.symno = LOTUS123_SYM_BLANK;
    return(narg);
}

/******************************************************************************
*
* recognise constants and functions
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_lterm(void)
{
    STATUS status = STATUS_OK;

    switch(check_next_symbol())
    {
    case LF_CONST:
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status = lotus123_rpn_out_U8(LF_CONST);
        if(status_ok(status))
             status = lotus123_rpn_out_F64(lotus123_symbol.arg.f64);
        break;

    case LF_SLR:
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status = lotus123_rpn_out_U8(LF_SLR);
        if(status_ok(status))
             status = lotus123_rpn_out_slr(&lotus123_symbol.arg.range.s);
        break;

    case LF_INTEGER:
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status = lotus123_rpn_out_U8(LF_INTEGER);
        if(status_ok(status))
             status = lotus123_rpn_out_S16((S16) lotus123_symbol.arg.f64);
        break;

    case LF_STRING:
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status = lotus123_rpn_out_U8(LF_STRING);
        if(status_ok(status))
            status = lotus123_rpn_out_bytes(&quick_block_rpn, lotus123_symbol.arg.lotus123_string.p_string, lotus123_symbol.arg.lotus123_string.len);
        if(status_ok(status))
            status = lotus123_rpn_out_U8(CH_NULL);
        break;

    case LOTUS123_SYM_FUNC:
        {
        PC_OPR_EQUIVALENT p_opr_equivalent = &opr_equivalent[lotus123_symbol.arg.ix_function];
        STATUS narg;

        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        switch(p_opr_equivalent->n_args)
        {
        /* zero argument functions */
        case 0:
            status = lotus123_rpn_out_U8(p_opr_equivalent->fno);
            break;

        /* fixed argument functions */
        default:
            if(status_fail(narg = procfunc(p_opr_equivalent)))
                status = narg;
            else
                status = lotus123_rpn_out_U8(p_opr_equivalent->fno);
            break;

        /* variable argument functions */
        case OPR_ARGS_VAR:
            if(status_fail(narg = procfunc(p_opr_equivalent)))
                status = narg;
            else
                if(status_ok(status = lotus123_rpn_out_U8(p_opr_equivalent->fno)))
                             status = lotus123_rpn_out_U8((U8) narg);
            break;
        }

        break;
        }
    }

    return(status);
}

/******************************************************************************
*
* recognise lotus123_lterm or brackets
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_gterm(void)
{
    STATUS status;

    if(check_next_symbol() == LOTUS123_SYM_OBRACKET)
    {
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status_return(status = lotus123_expr());

        if(check_next_symbol() != LOTUS123_SYM_CBRACKET)
        {
            lotus123_symbol.symno = LOTUS123_SYM_BAD;
            return(status);
        }

        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        return(lotus123_rpn_out_U8(LF_BRACKETS));
    }

    return(lotus123_lterm());
}

/******************************************************************************
*
* recognise unary +, -, !
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_fterm(void)
{
    STATUS status;

    switch(check_next_symbol())
    {
    case LF_PLUS:
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status = lotus123_fterm();
        if(status_ok(status))
             status = lotus123_rpn_out_U8(LF_UPLUS);
        break;

    case LF_MINUS:
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status = lotus123_fterm();
        if(status_ok(status))
             status = lotus123_rpn_out_U8(LF_UMINUS);
        break;

    case LF_NOT:
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status = lotus123_fterm();
        if(status_ok(status))
             status = lotus123_rpn_out_U8(LF_NOT);
        break;

    default:
        status = lotus123_gterm();
        break;
    }

    return(status);
}

/******************************************************************************
*
* recognise ^
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_eterm(void)
{
    STATUS status;

    status_return(status = lotus123_fterm());

    while(check_next_symbol() == LF_POWER)
    {
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status_break(status = lotus123_fterm());

        status_break(status = lotus123_rpn_out_U8(LF_POWER));
    }

    return(status);
}

/******************************************************************************
*
* recognise *, /
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_dterm(void)
{
    STATUS status;
    S32 nxsym;

    status_return(status = lotus123_eterm());

    for(;;)
    {
        switch(nxsym = check_next_symbol())
        {
        case LF_TIMES:
        case LF_DIVIDE:
            lotus123_symbol.symno = LOTUS123_SYM_BLANK;
            break;

        default:
            return(status);
        }

        status_break(status = lotus123_eterm());

        status_break(status = lotus123_rpn_out_U8((U8) nxsym));
    }

    return(status);
}

/******************************************************************************
*
* recognise +, -
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_cterm(void)
{
    STATUS status;
    S32 nxsym;

    status_return(status = lotus123_dterm());

    for(;;)
    {
        switch(nxsym = check_next_symbol())
        {
        case LF_PLUS:
        case LF_MINUS:
            lotus123_symbol.symno = LOTUS123_SYM_BLANK;
            break;

        default:
            return(status);
        }

        status_break(status = lotus123_dterm());

        status_break(status = lotus123_rpn_out_U8((U8) nxsym));
    }

    return(status);
}

/******************************************************************************
*
* recognise =, <>, <, >, <=, >=
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_bterm(void)
{
    STATUS status;
    S32 nxsym;

    status_return(status = lotus123_cterm());

    for(;;)
    {
        switch(nxsym = check_next_symbol())
        {
        case LF_EQUALS:
        case LF_NOTEQUAL:
        case LF_LT:
        case LF_GT:
        case LF_LTEQUAL:
        case LF_GTEQUAL:
            lotus123_symbol.symno = LOTUS123_SYM_BLANK;
            break;

        default:
            return(status);
        }

        status_break(status = lotus123_cterm());

        status_break(status = lotus123_rpn_out_U8((U8) nxsym));
    }

    return(status);
}

/******************************************************************************
*
* recognise &
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_aterm(void)
{
    STATUS status;

    status_return(status = lotus123_bterm());

    while(check_next_symbol() == LF_AND)
    {
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status_break(status = lotus123_bterm());

        status_break(status = lotus123_rpn_out_U8(LF_AND));
    }

    return(status);
}

/******************************************************************************
*
* expression recogniser works by recursive descent
* recognise |
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_expr(void)
{
    STATUS status;

    status_return(status = lotus123_aterm());

    while(check_next_symbol() == LF_OR)
    {
        lotus123_symbol.symno = LOTUS123_SYM_BLANK;

        status_break(status = lotus123_aterm());

        status_break(status = lotus123_rpn_out_U8(LF_OR));
    }

    return(status);
}

/******************************************************************************
*
* compile expression into RPN for LOTUS
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_compile(
    _In_z_      PC_U8Z p_text,
    _InRef_     PC_SLR p_slr)
{
    STATUS status = STATUS_OK;

    lotus123_symbol.symno = LOTUS123_SYM_BLANK;
    p_compile_scan = p_text;
    scan_date = 0;
    reference_slr = *p_slr;

    status_return(status = lotus123_expr());

    if(lotus123_symbol.symno != LF_END)
        status = STATUS_FAIL;
    else
        status = lotus123_rpn_out_U8(LF_END);

    return(status);
}

#define lotus123_write_bytes(p_ff_op_format, p_data, n_bytes) \
    binary_write_bytes(&(p_ff_op_format)->of_op_format.output, p_data, n_bytes)

#define lotus123_write_U8(p_ff_op_format, u8) \
    binary_write_byte(&(p_ff_op_format)->of_op_format.output, u8)

/******************************************************************************
*
* write a 8087-format double to the Lotus file
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_F64(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     F64 f64)
{
    BYTE fp_8087[sizeof32(F64)];

    writeval_F64_as_8087(fp_8087, f64);

    return(lotus123_write_bytes(p_ff_op_format, fp_8087, sizeof32(fp_8087)));
}

/******************************************************************************
*
* write an unsigned 16-bit value out to the Lotus file
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_U16(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U16 u16)
{
    return(lotus123_write_bytes(p_ff_op_format, &u16, sizeof32(u16)));
}

/******************************************************************************
*
* write a signed 16-bit value out to the Lotus file
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_S16(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     S16 s16)
{
    return(lotus123_write_bytes(p_ff_op_format, &s16, sizeof32(s16)));
}

/******************************************************************************
*
* write out a Lotus structure instruction
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_ins(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_LOTUS123_INS p_lotus123_ins)
{
    status_return(lotus123_write_U16(p_ff_op_format, p_lotus123_ins->opcode));
           return(lotus123_write_U16(p_ff_op_format, (U16) p_lotus123_ins->len_ins));
}

/******************************************************************************
*
* write out start of cell
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_cell_start(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U16 opcode,
    _InVal_     U16 len,
    _InRef_     PC_SLR p_slr)
{
    status_return(lotus123_write_U16(p_ff_op_format, opcode));
    status_return(lotus123_write_U16(p_ff_op_format, len));
    status_return(lotus123_write_U8(p_ff_op_format, L_PROT | L_SPECL | L_GENFMT));
    status_return(lotus123_write_U16(p_ff_op_format, (U16) p_slr->col));
           return(lotus123_write_U16(p_ff_op_format, (U16) p_slr->row));
}

/******************************************************************************
*
* write out a date
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_date(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     S32 day,
    _InVal_     S32 mon,
    _InVal_     S32 year,
    _InRef_     PC_SLR p_slr)
{
    /* write out length and date format */
    status_return(lotus123_write_U16(p_ff_op_format, L_FORMULA));
    status_return(lotus123_write_U16(p_ff_op_format, 26));
    status_return(lotus123_write_U8(p_ff_op_format, L_PROT | L_SPECL | L_DDMMYY));
    status_return(lotus123_write_U16(p_ff_op_format, (U16) p_slr->col));
    status_return(lotus123_write_U16(p_ff_op_format, (U16) p_slr->row));

    { /* write out value */
    int i;
    for(i = 0; i < 4; ++i)
        status_return(lotus123_write_U16(p_ff_op_format, 0));
    } /*block*/

    /* write out @DATE(year, mon, day) expression */
    status_return(lotus123_write_U16(p_ff_op_format, 3 * sizeof32(U16) + 5 * sizeof32(U8)));
    status_return(lotus123_write_U8(p_ff_op_format, LF_INTEGER));
    status_return(lotus123_write_U16(p_ff_op_format, (U16) year));
    status_return(lotus123_write_U8(p_ff_op_format, LF_INTEGER));
    status_return(lotus123_write_U16(p_ff_op_format, (U16) mon));
    status_return(lotus123_write_U8(p_ff_op_format, LF_INTEGER));
    status_return(lotus123_write_U16(p_ff_op_format, (U16) day));

    status_return(lotus123_write_U8(p_ff_op_format, LF_DATE));
           return(lotus123_write_U8(p_ff_op_format, LF_END));
}

/******************************************************************************
*
* write out a cell as a label
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_label(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_SBSTR sbstr,
    _InRef_     PC_SLR p_slr)
{
    S32 contents_len = strlen32(sbstr);
    STATUS status;

    for(;;)
    {
        /* write out label */
        status_break(status = lotus123_write_U16(p_ff_op_format, L_LABEL));

        /* length: overhead + align + contents + null */
        status_break(status = lotus123_write_U16(p_ff_op_format, (U16) (5 + 1 + contents_len + 1)));
        status_break(status = lotus123_write_U8( p_ff_op_format, 0xFF));
        status_break(status = lotus123_write_U16(p_ff_op_format, (U16) p_slr->col));
        status_break(status = lotus123_write_U16(p_ff_op_format, (U16) p_slr->row));

        { /* default left alignment */
        U8 align;
        align = CH_APOSTROPHE;
#if 0
        if(mask & BIT_RYT)
            align = CH_QUOTATION_MARK;
        if(mask & BIT_CEN)
            align = CH_CIRCUMFLEX_ACCENT;
#endif
        status_break(status = lotus123_write_U8(p_ff_op_format, align));
        }

        {
        S32 len = contents_len;
        PC_SBSTR p_u8 = sbstr;
        while(len--)
            status_break(status = lotus123_write_U8(p_ff_op_format, *p_u8++));
        status_break(status);
        } /*block*/

        status_break(status = lotus123_write_U8(p_ff_op_format, 0));

        break;
        /*NOTREACHED*/
    }

    return(status);
}

/******************************************************************************
*
* write out a cell to a Lotus file
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_write_cell(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_SLR p_slr,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock  /*appended,NOT terminated*/,
    P_SS_DATA p_ss_data,
    _InVal_     S32 data_constant,
    _InVal_     OBJECT_ID object_id)
{
    STATUS status = STATUS_OK;

    switch(object_id)
    {
    default: default_unhandled();
#if CHECKING
    case OBJECT_ID_TEXT:
#endif
        if(status_ok(status = quick_ublock_nullch_add(p_quick_ublock)))
            status = lotus123_write_label(p_ff_op_format, _sbstr_from_ustr(quick_ublock_ustr(p_quick_ublock)), p_slr);
        break;

    case OBJECT_ID_SS:
        {
        if(data_constant)
        {
            switch(ss_data_get_data_id(p_ss_data))
            {
            case DATA_ID_REAL:
                {
                F64 f64;
                ss_data_copy_real(&f64, p_ss_data);
                status_break(status = lotus123_write_cell_start(p_ff_op_format, L_NUMBER, 13, p_slr));
                status_break(status = lotus123_write_F64(p_ff_op_format, f64));
                break;
                }

            case DATA_ID_LOGICAL:
            case DATA_ID_WORD16:
                status_break(status = lotus123_write_cell_start(p_ff_op_format, L_INTEGER, 7, p_slr));
                status_break(status = lotus123_write_S16(p_ff_op_format, (S16) ss_data_get_integer(p_ss_data)));
                break;

            case DATA_ID_WORD32:
                {
                const F64 f64 = (F64) ss_data_get_integer(p_ss_data);
                status_break(status = lotus123_write_cell_start(p_ff_op_format, L_NUMBER, 13, p_slr));
                status_break(status = lotus123_write_F64(p_ff_op_format, f64));
                break;
                }

            case DATA_ID_DATE:
                {
                S32 year, month, day;
                if(status_ok(ss_dateval_to_ymd(ss_data_get_date(p_ss_data)->date, &year, &month, &day)))
                    status = lotus123_write_date(p_ff_op_format, day, month, year, p_slr);
                break;
                }

            case DATA_ID_STRING:
            case DATA_ID_BLANK:
            case DATA_ID_ERROR:
            case DATA_ID_ARRAY:
                break;
            }
        }
        else
        {
            BYTE quick_block_rpn_buffer[200];
            quick_block_with_buffer_setup(quick_block_rpn);

            status_assert(quick_ublock_nullch_add(p_quick_ublock)); /* SKS 07apr95 compiler needs CH_NULL-terminated string */

            if(status_ok(lotus123_compile(_sbstr_from_ustr(quick_ublock_ustr(p_quick_ublock)), p_slr)))
            {
                if(status_ok(status))
                    status = lotus123_write_cell_start(p_ff_op_format, L_FORMULA, (U16) (quick_block_bytes(&quick_block_rpn) + 15), p_slr);

                if(status_ok(status))
                {
                    if(ss_data_is_number(p_ss_data))
                    {
                        const F64 f64 = ss_data_get_number(p_ss_data);
                        status = lotus123_write_F64(p_ff_op_format, f64);
                    }
                    else
                    {
                        const F64 f64 = 0.0;
                        status = lotus123_write_F64(p_ff_op_format, f64);
                    }
                }

                if(status_ok(status))
                {
                    PC_BYTE p_byte = quick_block_ptr(&quick_block_rpn);
                    S32 len = quick_block_bytes(&quick_block_rpn);
                    assert(0 != len);
                    status = lotus123_write_U16(p_ff_op_format, (U16) len);
                    if(status_ok(status))
                        status = lotus123_write_bytes(p_ff_op_format, p_byte, len);
                }
            }
            else
            {
                status = lotus123_write_label(p_ff_op_format, _sbstr_from_ustr(quick_ublock_ustr(p_quick_ublock)), p_slr);
            }

            quick_block_dispose(&quick_block_rpn);
        }

        break;
        }
    }

    return(status);
}

/******************************************************************************
*
* write out all the column information
*
******************************************************************************/

PROC_WRITE_PROTO(proc_write_cols)
{
    STATUS status = STATUS_OK;
    SCAN_BLOCK scan_block;
    OBJECT_DATA object_data;

    UNREFERENCED_PARAMETER_InRef_(p_lotus123_ins);

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            switch(object_data.object_id)
            {
            case OBJECT_ID_TEXT:
            case OBJECT_ID_SS:
                {
                OBJECT_READ_TEXT object_read_text;
                OBJECT_DATA_READ object_data_read;
                QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
                quick_ublock_with_buffer_setup(quick_ublock);

                object_data_read.object_data = object_data;
                status = object_call_id(object_data_read.object_data.object_id,
                                        p_docu,
                                        T5_MSG_OBJECT_DATA_READ,
                                        &object_data_read);

                if(status_ok(status))
                {
                    object_read_text.p_quick_ublock = &quick_ublock;
                    object_read_text.object_data = object_data;
                    object_read_text.type = OBJECT_READ_TEXT_PLAIN;
                    status = object_call_id(object_read_text.object_data.object_id,
                                            p_docu,
                                            T5_MSG_OBJECT_READ_TEXT,
                                            &object_read_text);
                }

                if(status_ok(status))
                    status = lotus123_write_cell(p_ff_op_format,
                                                 &object_data.data_ref.arg.slr,
                                                 &quick_ublock,
                                                 &object_data_read.ss_data,
                                                 object_data_read.constant,
                                                 object_data_read.object_data.object_id);

                quick_ublock_dispose(&quick_ublock);

                ss_data_free_resources(&object_data_read.ss_data);

                break;
                }

            default:
                break;
            }

            status_break(status);
        }
    }

    return(status);
}

/******************************************************************************
*
* write out Lotus graph record
*
******************************************************************************/

PROC_WRITE_PROTO(proc_write_graph)
{
    static const BYTE part1[] = { '\xFF', '\xFF', '\x0', '\x0' };
    static const BYTE part2[] = { '\x04', '\x00', '\x0', '\x3', '\x3', '\x3', '\x3', '\x3', '\x3' };
    static const BYTE part3[] = { '\x71', '\x71', '\x1', '\x0', '\x0', '\x0' };

    STATUS status = lotus123_write_ins(p_ff_op_format, p_lotus123_ins);
    S32 i;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    for(i = 0; i < 26 && status_ok(status); ++i)
        status = lotus123_write_bytes(p_ff_op_format, part1, sizeof32(part1));

    if(status_ok(status))
        status = lotus123_write_bytes(p_ff_op_format, part2, sizeof32(part2));

    for(i = 0; i < 320 && status_ok(status); ++i)
        status = lotus123_write_U8(p_ff_op_format, 0);

    if(status_ok(status))
        status = lotus123_write_bytes(p_ff_op_format, part3, sizeof32(part3));

    return(status);
}

/******************************************************************************
*
* write out file range
*
******************************************************************************/

PROC_WRITE_PROTO(proc_write_range)
{
    static const BYTE def_start[] = "\x0\x0\x0\x0";

    STATUS status = lotus123_write_ins(p_ff_op_format, p_lotus123_ins);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(status_ok(status))
        status = lotus123_write_bytes(p_ff_op_format, def_start, sizeof32(def_start) - 1);

    if(status_ok(status))
    {
        U16 ecol = (U16) (g_e_col - 1);
        status = lotus123_write_U16(p_ff_op_format, ecol);
    }

    if(status_ok(status))
    {
        U16 erow = (U16) (g_e_row - 1);
        status = lotus123_write_U16(p_ff_op_format, erow);
    }

    return(status);
}

/******************************************************************************
*
* Lotus file structure
*
******************************************************************************/

#define NO_PATTERN 0xFF

static const BYTE l_bof[] = "\x06\x04";

static const BYTE def_range[] = "\xFF\xFF\x00\x00";

static const BYTE def_win1[] = "\x00\x00\x00\x00" "\xF1\x00" "\x09\x00" "\x08\x00\x14\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x04\x00\x04\x00" "\x4c\x00";

static const BYTE def_margin[] = "\x00\x00 \x4C\x00 \x42\x00 \x00\x00 \x00\x00";

#define _BA(s) ((PC_BYTE) (s))

static const LOTUS123_INS
lotus123_ins[] =
{
    { L_BOF,           2,      l_bof,     sizeof32(l_bof) - 1,  NO_PATTERN,             NULL },
    { L_RANGE,         8,       NULL,                       0,  NO_PATTERN, proc_write_range },
    { L_CPI,           6,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_CALCCOUNT,     1, _BA("\x01"),                      1,  NO_PATTERN,             NULL },
    { L_CALCMODE,      1, _BA("\xFF"),                      1,  NO_PATTERN,             NULL },
    { L_CALCORDER,     1, _BA("\x00"),                      1,  NO_PATTERN,             NULL },
    { L_SPLIT,         1,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_SYNC,          1,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_WINDOW1,      32,   def_win1,                      30,  NO_PATTERN,             NULL },
    { L_HIDVEC1,      32,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_CURSORW12,     1, _BA("\x01"),                      1,  NO_PATTERN,             NULL },
    { L_TABLE,        25,  def_range, sizeof32(def_range) - 1,           1,             NULL },
    { L_QRANGE,       25,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_PRANGE,        8,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_UNFORMATTED,   1,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_FRANGE,        8,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_SRANGE,        8,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_KRANGE,        9,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_KRANGE2,       9,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_RRANGES,      25,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_MATRIXRANGES, 40,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_HRANGE,       16,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_PARSERANGES,  16,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_PROTEC,        1,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_FOOTER,      242,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_HEADER,      242,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_SETUP,        40,       NULL,                       0,  NO_PATTERN,             NULL },
    { L_MARGINS,      10, def_margin,                       0,  NO_PATTERN,             NULL },
    { L_LABELFMT,      1, _BA("\x27"),                      1,  NO_PATTERN,             NULL },
    { L_TITLES,       16,  def_range, sizeof32(def_range) - 1,           0,             NULL },
    { L_GRAPH,       439,       NULL,                       0,           0, proc_write_graph },
    { L_FORMULA,       0,       NULL,                       0,  NO_PATTERN,  proc_write_cols },
    { L_EOF,           0,       NULL,                       0,  NO_PATTERN,             NULL }
};

/******************************************************************************
*
* use the master Lotus structure table to write out the Lotus 1-2-3 file
*
******************************************************************************/

T5_MSG_PROTO(static, lotus123_msg_save_foreign, _InoutRef_ P_MSG_SAVE_FOREIGN p_msg_save_foreign)
{
    P_FF_OP_FORMAT p_ff_op_format = p_msg_save_foreign->p_ff_op_format;
    STATUS status = STATUS_OK;
    U32 i;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    limits_from_docu_area(p_docu, &g_s_col, &g_e_col, &g_s_row, &g_e_row, &p_ff_op_format->of_op_format.save_docu_area);

    if((g_e_col /*- g_s_col*/) > LOTUS123_MAXCOL)
        return(create_error(ERR_TOO_MANY_COLUMNS_FOR_EXPORT));

    if((g_e_row /*- g_s_row*/) > LOTUS123_MAXROW)
        return(create_error(ERR_TOO_MANY_ROWS_FOR_EXPORT));

    for(i = 0; i < elemof32(lotus123_ins); ++i)
    {
        PC_LOTUS123_INS p_lotus123_ins = &lotus123_ins[i];

        if(p_lotus123_ins->p_proc_write)
        {   /* call special function for this instruction */
            status_break(status = (*p_lotus123_ins->p_proc_write)(p_docu, p_ff_op_format, p_lotus123_ins));
            continue;
        }

        /* use table to write out default data */
        status_break(status = lotus123_write_ins(p_ff_op_format, p_lotus123_ins));

        if(0 != p_lotus123_ins->len_ins)
        {
            S32 len = p_lotus123_ins->len_ins;

            /* check for a pattern to be output */
            if(p_lotus123_ins->pattern == NO_PATTERN)
            {
                status = lotus123_write_bytes(p_ff_op_format, p_lotus123_ins->p_data, p_lotus123_ins->len_data);
                len -= p_lotus123_ins->len_data;
            }
            else
            {
                S32 len_t = p_lotus123_ins->pattern;

                /* output leading nulls before pattern */
                while(status_ok(status) && len_t--)
                {
                    status = lotus123_write_U8(p_ff_op_format, CH_NULL);
                    --len;
                }

                /* output as many patterns as possible */
                while(status_ok(status) && p_lotus123_ins->len_data <= len)
                {
                    status = lotus123_write_bytes(p_ff_op_format, p_lotus123_ins->p_data, p_lotus123_ins->len_data);
                    len -= p_lotus123_ins->len_data;
                }
            }

            /* pad with trailing nulls */
            while(status_ok(status) && (len-- > 0))
                status = lotus123_write_U8(p_ff_op_format, CH_NULL);

            status_break(status);
        }
    }

    return(status);
}

/******************************************************************************
*
* Lotus 1-2-3 file converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, lotus123_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FS_LOTUS123, P_BOUND_MESSAGES_OBJECT_ID_FS_LOTUS123, P_BOUND_RESOURCES_OBJECT_ID_FS_LOTUS123));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FS_LOTUS123));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fs_lotus123);
OBJECT_PROTO(extern, object_fs_lotus123)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(lotus123_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_SAVE_FOREIGN:
        return(lotus123_msg_save_foreign(p_docu, t5_message, (P_MSG_SAVE_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fs_123.c */
