/* fs_xls_saveb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2016 Stuart Swales */

/* Excel spreadsheet BIFF saver */

/* SKS April 2014 */

#include "common/gflags.h"

#include "fs_xls/fs_xls.h"

#include "ob_skel/ff_io.h"

#ifndef          __ev_eval_h
#include "cmodules/ev_eval.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

/******************************************************************************
*
* Save as Microsoft Office Excel BIFF Format
*
******************************************************************************/

#define XLS_REC_U16(n)    LOBYTE(n), HIBYTE(n)
#define XLS_REC_OPCODE(n) LOBYTE(n), HIBYTE(n)
#define XLS_REC_LENGTH(n) LOBYTE(n), HIBYTE(n)

static U32 biff_version = 0;

static COL g_s_col, g_e_col;
static ROW g_s_row, g_e_row;

static U32 boundsheet_record_abs_posn[1]; /* just cater for a single worksheet */

static U32 index_record_abs_posn;

_Check_return_
static STATUS
xls_write_get_current_offset(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _OutRef_    P_U32 p_current_offset)
{
    if(OP_OUTPUT_FILE == p_ff_op_format->of_op_format.output.state)
    {
        const FILE_HANDLE file_handle = p_ff_op_format->of_op_format.output.u.file.file_handle;
        filepos_t cur_pos;
        STATUS status;

        if(status_fail(status = file_getpos(file_handle, &cur_pos)))
        {
            *p_current_offset = 0;
            return(status);
        }

        assert(0 == cur_pos.u.words.hi);
        *p_current_offset = cur_pos.u.words.lo;
        return(STATUS_OK);
    }

    assert(OP_OUTPUT_MEM == p_ff_op_format->of_op_format.output.state);
    *p_current_offset = array_elements32(p_ff_op_format->of_op_format.output.u.mem.p_array_handle);
    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_write_patch_offset_U32(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U32 patch_offset,
    _InVal_     U32 patch_value)
{
    if(OP_OUTPUT_FILE ==  p_ff_op_format->of_op_format.output.state)
    {
        const FILE_HANDLE file_handle = p_ff_op_format->of_op_format.output.u.file.file_handle;
        filepos_t cur_pos, patch_pos;

        status_return(file_getpos(file_handle, &cur_pos));

        patch_pos.u.words.hi = 0;
        patch_pos.u.words.lo = patch_offset;
        status_assert(file_setpos(file_handle, &patch_pos));

        status_assert(file_write_bytes(&patch_value, sizeof32(patch_value), file_handle)); /* endian-ness */

        return(file_setpos(file_handle, &cur_pos));
    }

    assert(OP_OUTPUT_MEM == p_ff_op_format->of_op_format.output.state);
    {
    P_BYTE p_byte = array_ptr(p_ff_op_format->of_op_format.output.u.mem.p_array_handle, BYTE, patch_offset);
    writeval_U32_LE(p_byte, patch_value);
    return(STATUS_OK);
    } /*block*/
}

/*
XLS formula scanner
*/

typedef struct XLS_SCAN_DATE
{
    F64 days;
}
XLS_SCAN_DATE, * P_XLS_SCAN_DATE;

typedef struct XLS_SCAN_STRING
{
    PC_U8 p_string;
    U32 len;
}
XLS_SCAN_STRING, * P_XLS_SCAN_STRING;

/* special symbol constants */
#define XLS_SCAN_SYMBOL_BAD             -1
#define XLS_SCAN_SYMBOL_BLANK           -2
#define XLS_SCAN_SYMBOL_BRACKET_OPEN    -3
#define XLS_SCAN_SYMBOL_BRACKET_CLOSE   -4
#define XLS_SCAN_SYMBOL_COMMA           -5

#define XLS_SCAN_SYMBOL_END             -10

/* these Fireworkz operators map to Excel functions */
#define XLS_SCAN_SYMBOL_OPR_AND         -20
#define XLS_SCAN_SYMBOL_OPR_OR          -21
#define XLS_SCAN_SYMBOL_OPR_NOT         -22

typedef struct XLS_SCAN_SYMBOL
{
    S32 symno; /* Excel RPN token or one of our special symbol constants */

    union XLS_SCAN_SYMBOL_ARG
    {
        F64 f64;
        struct XLS_SCAN_SYMBOL_ARG_RANGE
        {
            SLR s;
            SLR e;
        } range;
        XLS_SCAN_STRING string;
        const struct XLS_COMPILER_FUNC_ENTRY * p_xls_compiler_func_entry;
        XLS_SCAN_DATE date;
    } arg;
}
XLS_SCAN_SYMBOL, * P_XLS_SCAN_SYMBOL;

static XLS_SCAN_SYMBOL xls_scan_symbol;

#define check_next_symbol() ( \
    (xls_scan_symbol.symno != XLS_SCAN_SYMBOL_BLANK) \
        ? xls_scan_symbol.symno \
        : scan_next_symbol() )

typedef struct XLS_COMPILER_FUNC_ENTRY
{
    PC_A7STR a7str_text_t5; /* Fireworkz' function name */

    U16 xlf_number;         /* Excel's function number (see xlcall.h) */
    S32 token;              /* Excel function token (depends on return type) */
    PC_BYTE p_args;
    U8 min_args;
    U8 max_args;
    U8 _padding[2];
}
XLS_COMPILER_FUNC_ENTRY; typedef const XLS_COMPILER_FUNC_ENTRY * PC_XLS_COMPILER_FUNC_ENTRY;

#define xls_func_entry(xlf_number, token, p_args, min_args, max_args, p_text_t5) \
    { p_text_t5, xlf_number, token, p_args, min_args, max_args, 0, 0 }

static const BYTE
arg_R[] =
{
    tRefR,
    0
};

static const BYTE
arg_R_V[] =
{
    tRefR,
    tRefV,
    0
};

static const BYTE
arg_V[] =
{
    tRefV,
    0
};

static const BYTE
arg_V_R[] =
{
    tRefV,
    tRefR,
    0
};

static const BYTE
arg_V_R_V[] =
{
    tRefV,
    tRefR,
    tRefV,
    0
};

/*
static const BYTE
arg_V_R_R_V[] =
{
    tRefV,
    tRefR,
    tRefR,
    tRefV,
    0
};
*/

static const BYTE
arg_A[] =
{
    tRefR,
    0
};

static const BYTE
arg_A_A_V[] =
{
    tRefR,
    tRefR,
    tRefV,
    0
};

static const XLS_COMPILER_FUNC_ENTRY
BIFF3_compiler_functions[] = /* ordered as Fireworkz */
{
    xls_func_entry(xlfAbs,              tFuncV,         arg_V,          1, 1,   "ABS"),
    xls_func_entry(xlfAddress,          tFuncVarV,      arg_V,          2, 5,   "ADDRESS"),
    xls_func_entry(xlfAnd,              tFuncVarV,      arg_R,          1, 30,  "AND"),
    xls_func_entry(xlfAcos,             tFuncV,         arg_V,          1, 1,   "ACOS"),
    xls_func_entry(xlfAcosh,            tFuncV,         arg_V,          1, 1,   "ACOSH"),
    xls_func_entry(xlfAsin,             tFuncV,         arg_V,          1, 1,   "ASIN"),
    xls_func_entry(xlfAsinh,            tFuncV,         arg_V,          1, 1,   "ASINH"),
    xls_func_entry(xlfAtan,             tFuncV,         arg_V,          1, 1,   "ATAN"),
    xls_func_entry(xlfAtan2,            tFuncV,         arg_V,          2, 2,   "ATAN_2"), /* XLS:ATAN2 */
    xls_func_entry(xlfAtanh,            tFuncV,         arg_V,          1, 1,   "ATANH"),
    xls_func_entry(xlfAverage,          tFuncVarV,      arg_R,          1, 30,  "AVG"), /* XLS:AVERAGE */

    xls_func_entry(xlfChar,             tFuncV,         arg_V,          1, 1,   "CHAR"),
    xls_func_entry(xlfChoose,           tFuncVarV,      arg_V_R,        2, 30,  "CHOOSE"),
    xls_func_entry(xlfClean,            tFuncV,         arg_V,          1, 1,   "CLEAN"),
    xls_func_entry(xlfCode,             tFuncV,         arg_V,          1, 1,   "CODE"),
    xls_func_entry(xlfColumn,           tFuncVarV,      arg_R,          0, 1,   "COL"), /* XLS:COLUMN */
    xls_func_entry(xlfColumns,          tFuncV,         arg_R,          1, 1,   "COLS"), /* XLS:COLUMNS */
    xls_func_entry(xlfCos,              tFuncV,         arg_V,          1, 1,   "COS"),
    xls_func_entry(xlfCosh,             tFuncV,         arg_V,          1, 1,   "COSH"),
    xls_func_entry(xlfCount,            tFuncVarV,      arg_R,          0, 30,  "COUNT"),
    xls_func_entry(xlfCounta,           tFuncVarV,      arg_R,          0, 30,  "COUNTA"),

    xls_func_entry(xlfDate,             tFuncV,         arg_V,          3, 3,   "DATE"),
    xls_func_entry(xlfDatevalue,        tFuncV,         arg_V,          1, 1,   "DATEVALUE"),
    xls_func_entry(xlfDaverage,         tFuncV,         arg_R,          3, 3,   "DAVG"), /* XLS:DAVERAGE */
    xls_func_entry(xlfDay,              tFuncV,         arg_V,          1, 1,   "DAY"),
    xls_func_entry(xlfDays360,          tFuncV,         arg_V,          2, 2,   "DAYS_360"), /* XLS:DAYS360 */
    xls_func_entry(xlfDdb,              tFuncVarV,      arg_V,          4, 5,   "DDB"),
    xls_func_entry(xlfDegrees,          tFuncV,         arg_V,          1, 1,   "DEG"), /* XLS:DEGREES (BIFF5) */

    xls_func_entry(xlfExact,            tFuncV,         arg_V,          2, 2,   "EXACT"),
    xls_func_entry(xlfExp,              tFuncV,         arg_V,          1, 1,   "EXP"),

    xls_func_entry(xlfFact,             tFuncV,         arg_V,          1, 1,   "FACT"),
    xls_func_entry(xlfFalse,            tFuncV,         NULL,           0, 0,   "FALSE"),
    xls_func_entry(xlfFind,             tFuncVarV,      arg_V,          2, 3,   "FIND"),
    xls_func_entry(xlfFixed,            tFuncVarV,      arg_V,          2, 3,   "FIXED"),

    xls_func_entry(xlfGrowth,           tFuncVarA,      arg_R,          1, 3,   "GROWTH"),

    xls_func_entry(xlfHlookup,          tFuncV,         arg_V_R,        3, 3,   "HLOOKUP"),
    xls_func_entry(xlfHour,             tFuncV,         arg_V,          1, 1,   "HOUR"),

    xls_func_entry(xlfIf,               tFuncVarR,      arg_V_R,        2, 3,   "IF"),
    xls_func_entry(xlfInt,              tFuncV,         arg_V,          1, 1,   "INT"), /* NB XLS:INT is different for negative numbers but this is better than failing */
    xls_func_entry(xlfIsblank,          tFuncV,         arg_V,          1, 1,   "ISBLANK"),
    xls_func_entry(xlfIserr,            tFuncV,         arg_V,          1, 1,   "ISERR"),
    xls_func_entry(xlfIserror,          tFuncV,         arg_V,          1, 1,   "ISERROR"),
    xls_func_entry(xlfIslogical,        tFuncV,         arg_V,          1, 1,   "ISLOGICAL"),
    xls_func_entry(xlfIsna,             tFuncV,         arg_V,          1, 1,   "ISNA"),
    xls_func_entry(xlfIsnontext,        tFuncV,         arg_V,          1, 1,   "ISNONTEXT"),
    xls_func_entry(xlfIsnumber,         tFuncV,         arg_V,          1, 1,   "ISNUMBER"),
    xls_func_entry(xlfIsref,            tFuncV,         arg_R,          1, 1,   "ISREF"),
    xls_func_entry(xlfIstext,           tFuncV,         arg_V,          1, 1,   "ISTEXT"),

    xls_func_entry(xlfConcatenate,      tFuncVarV,      arg_V,          0, 30,  "JOIN"), /* XLS:CONCATENATE */

    xls_func_entry(xlfLeft,             tFuncVarV,      arg_V,          1, 2,   "LEFT"),
    xls_func_entry(xlfLen,              tFuncV,         arg_V,          1, 1,   "LENGTH"), /* XLS:LEN */
    xls_func_entry(xlfLinest,           tFuncVarA,      arg_R,          1, 2,   "LINEST"),
    xls_func_entry(xlfLn,               tFuncV,         arg_V,          1, 1,   "LN"),
    xls_func_entry(xlfLog10,            tFuncV,         arg_V,          1, 1,   "ODF.LOG10"), /* XLS:LOG10 */
    xls_func_entry(xlfLog,              tFuncVarV,      arg_V,          1, 2,   "LOG"),
    xls_func_entry(xlfLogest,           tFuncVarA,      arg_R,          1, 2,   "LOGEST"),
    xls_func_entry(xlfLookup,           tFuncVarV,      arg_V_R,        2, 3,   "LOOKUP"),
    xls_func_entry(xlfLower,            tFuncV,         arg_V,          1, 1,   "LOWER"),

    xls_func_entry(xlfMdeterm,          tFuncV,         arg_A,          1, 1,   "M_DETERM"),
    xls_func_entry(xlfMinverse,         tFuncA,         arg_A,          1, 1,   "M_INVERSE"),
    xls_func_entry(xlfMmult,            tFuncA,         arg_A,          2, 2,   "M_MULT"),

    xls_func_entry(xlfMatch,            tFuncVarV,      arg_V_R,        2, 3,   "MATCH"),
    xls_func_entry(xlfMax,              tFuncVarV,      arg_R,          1, 30,  "MAX"),
    xls_func_entry(xlfMedian,           tFuncVarV,      arg_R,          1, 30,  "MEDIAN"),
    xls_func_entry(xlfMid,              tFuncV,         arg_V,          3, 3,   "MID"),
    xls_func_entry(xlfMin,              tFuncVarV,      arg_R,          1, 30,  "MIN"),
    xls_func_entry(xlfMinute,           tFuncV,         arg_V,          1, 1,   "MINUTE"),
    xls_func_entry(xlfMirr,             tFuncV,         arg_R_V,        3, 3,   "MIRR"),
    xls_func_entry(xlfMod,              tFuncV,         arg_V,          2, 2,   "MOD"),
    xls_func_entry(xlfMonth,            tFuncV,         arg_V,          1, 1,   "MONTH"),

    xls_func_entry(xlfN,                tFuncV,         arg_R,          1, 1,   "N"),
    xls_func_entry(xlfNa,               tFuncV,         NULL,           0, 0,   "NA"),
    xls_func_entry(xlfNot,              tFuncV,         arg_V,          1, 1,   "NOT"),
    xls_func_entry(xlfNow,              tFuncV,         NULL,           0, 0,   "NOW"),
    xls_func_entry(xlfNper,             tFuncVarV,      arg_V,          3, 5,   "NPER"),
    xls_func_entry(xlfNpv,              tFuncVarV,      arg_V_R,        2, 30,  "NPV"),

    xls_func_entry(xlfFv,               tFuncVarV,      arg_V,          3, 5,   "ODF.FV"), /* XLS:FV */
    xls_func_entry(xlfIndex,            tFuncVarR,      arg_R_V,        2, 4,   "ODF.INDEX"), /* XLS:INDEX */
    xls_func_entry(xlfInt,              tFuncV,         arg_V,          1, 1,   "ODF.INT"), /* XLS:INT */
    xls_func_entry(xlfIrr,              tFuncVarV,      arg_R_V,        1, 2,   "ODF.IRR"), /* XLS:IRR */
    xls_func_entry(xlfPmt,              tFuncVarV,      arg_V,          3, 5,   "ODF.PMT"), /* XLS:PMT */
    xls_func_entry(xlfType,             tFuncV,         arg_V,          1, 1,   "ODF.TYPE"), /* XLS:TYPE */

    xls_func_entry(xlfOr,               tFuncVarV,      arg_R,          1, 30,  "OR"),

    xls_func_entry(xlfPi,               tFuncV,         NULL,           0, 0,   "PI"),
    xls_func_entry(xlfProduct,          tFuncVarV,      arg_R,          1, 30,  "PRODUCT"),
    xls_func_entry(xlfProper,           tFuncV,         arg_V,          1, 1,   "PROPER"),
    xls_func_entry(xlfPv,               tFuncVarV,      arg_V,          3, 5,   "PV"),

    xls_func_entry(xlfRadians,          tFuncV,         arg_V,          1, 1,   "RAD"), /* XLS:RADIANS */
    xls_func_entry(xlfRand,             tFuncV,         NULL,           0, 0,   "RAND"),
    xls_func_entry(xlfRate,             tFuncVarV,      arg_V,          3, 6,   "RATE"),
    xls_func_entry(xlfReplace,          tFuncV,         arg_V,          4, 4,   "REPLACE"),
    xls_func_entry(xlfRept,             tFuncV,         arg_V,          2, 2,   "REPT"),
    xls_func_entry(xlfRight,            tFuncVarV,      arg_V,          1, 2,   "RIGHT"),
    xls_func_entry(xlfRound,            tFuncV,         arg_V,          2, 2,   "ROUND"),
    xls_func_entry(xlfRounddown,        tFuncV,         arg_V,          2, 2,   "ROUNDDOWN"),
    xls_func_entry(xlfRoundup,          tFuncV,         arg_V,          2, 2,   "ROUNDUP"),
    xls_func_entry(xlfRow,              tFuncVarV,      arg_R,          0, 1,   "ROW"),
    xls_func_entry(xlfRows,             tFuncV,         arg_R,          1, 1,   "ROWS"),

    xls_func_entry(xlfSecond,           tFuncV,         arg_V,          1, 1,   "SECOND"),
    xls_func_entry(xlfSign,             tFuncV,         arg_V,          1, 1,   "SGN"), /* XLS:SIGN */
    xls_func_entry(xlfSin,              tFuncV,         arg_V,          1, 1,   "SIN"),
    xls_func_entry(xlfSinh,             tFuncV,         arg_V,          1, 1,   "SINH"),
    xls_func_entry(xlfSln,              tFuncV,         arg_V,          3, 3,   "SLN"),
    xls_func_entry(xlfSqrt,             tFuncV,         arg_V,          1, 1,   "SQR"), /* XLS:SQRT */
    xls_func_entry(xlfStdev,            tFuncVarV,      arg_R,          1, 30,  "STD"), /* XLS:STDEV */
    xls_func_entry(xlfStdevp,           tFuncVarV,      arg_R,          1, 30,  "STDP"), /* XLS:STDEVP */
    xls_func_entry(xlfSum,              tFuncVarV,      arg_R,          1, 30,  "SUM"),
    xls_func_entry(xlfSumproduct,       tFuncVarV,      arg_A,          1, 30,  "SUMPRODUCT"),
    xls_func_entry(xlfSyd,              tFuncV,         arg_V,          4, 4,   "SYD"),

    xls_func_entry(xlfT,                tFuncV,         arg_R,          1, 1,   "T"),
    xls_func_entry(xlfTan,              tFuncV,         arg_V,          1, 1,   "TAN"),
    xls_func_entry(xlfTanh,             tFuncV,         arg_V,          1, 1,   "TANH"),
    xls_func_entry(xlfText,             tFuncV,         arg_V,          2, 2,   "TEXT"),
    xls_func_entry(xlfTime,             tFuncV,         arg_V,          3, 3,   "TIME"),
    xls_func_entry(xlfTimevalue,        tFuncV,         arg_V,          1, 1,   "TIMEVALUE"),
    xls_func_entry(xlfToday,            tFuncV,         NULL,           0, 0,   "TODAY"),
    xls_func_entry(xlfTranspose,        tFuncA,         arg_A,          1, 1,   "TRANSPOSE"),
    xls_func_entry(xlfTrend,            tFuncVarA,      arg_R,          1, 3,   "TREND"),
    xls_func_entry(xlfTrim,             tFuncV,         arg_V,          1, 1,   "TRIM"),
    xls_func_entry(xlfTrue,             tFuncV,         NULL,           0, 0,   "TRUE"),
    xls_func_entry(xlfTrunc,            tFuncVarV,      arg_V,          1, 2,   "TRUNC"),

    xls_func_entry(xlfUpper,            tFuncV,         arg_V,          1, 1,   "UPPER"),

    xls_func_entry(xlfValue,            tFuncV,         arg_V,          1, 1,   "VALUE"),
    xls_func_entry(xlfVar,              tFuncVarV,      arg_R,          1, 30 , "VAR"),
    xls_func_entry(xlfVarp,             tFuncVarV,      arg_R,          1, 30,  "VARP"),
    xls_func_entry(xlfVlookup,          tFuncV,         arg_V_R,        3, 3,   "VLOOKUP"),

    xls_func_entry(xlfWeekday,          tFuncV,         arg_V,          1, 1,   "WEEKDAY"),

    xls_func_entry(xlfYear,             tFuncV,         arg_V,          1, 1,   "YEAR")
};

/*
Later Excel versions add more functions, and the number of arguments can change (and so can the token type)
*/

static const XLS_COMPILER_FUNC_ENTRY
BIFF4_compiler_functions[] = /* ordered as Fireworkz */
{
    xls_func_entry(xlfAvedev,           tFuncVarV,      arg_R,          1, 30,  "AVEDEV"),
    xls_func_entry(xlfBinomdist,        tFuncV,         arg_V,          4, 4,   "BINOM.DIST"), /* XLS:BINOMDIST */
    xls_func_entry(xlfCritbinom,        tFuncV,         arg_V,          3, 3,   "BINOM.INV"), /* XLS:CRITBINOM */
    xls_func_entry(xlfCeiling,          tFuncV,         arg_V,          2, 2,   "CEILING"),
    xls_func_entry(xlfChidist,          tFuncV,         arg_V,          2, 2,   "CHISQ.DIST.RT"), /* XLS:CHIDIST */
    xls_func_entry(xlfChiinv,           tFuncV,         arg_V,          2, 2,   "CHISQ.INV.RT"), /* XLS:CHIINV */
    xls_func_entry(xlfChitest,          tFuncV,         arg_A,          2, 2,   "CHISQ.TEST"), /* XLS:CHITEST */
    xls_func_entry(xlfCombin,           tFuncV,         arg_V,          2, 2,   "COMBIN"),
    xls_func_entry(xlfConfidence,       tFuncV,         arg_V,          3, 3,   "CONFIDENCE.NORM"), /* XLS:CONFIDENCE */
    xls_func_entry(xlfCorrel,           tFuncV,         arg_A,          2, 2,   "CORREL"),
    xls_func_entry(xlfCovar,            tFuncV,         arg_A,          2, 2,   "COVARIANCE.P"), /* XLS:COVAR */
    xls_func_entry(xlfDb,               tFuncVarV,      arg_V,          4, 5,   "DB"),
    xls_func_entry(xlfDevsq,            tFuncVarV,      arg_R,          1, 30,  "DEVSQ"),
    xls_func_entry(xlfEven,             tFuncV,         arg_V,          1, 1,   "EVEN"),
    xls_func_entry(xlfExpondist,        tFuncV,         arg_V,          3, 3,   "EXPON.DIST"), /* XLS:EXPONDIST */ /* NB Excel requires third optional parameter */
    xls_func_entry(xlfFdist,            tFuncV,         arg_V,          3, 3,   "F.DIST.RT"), /* XLS:FDIST */
    xls_func_entry(xlfFinv,             tFuncV,         arg_V,          3, 3,   "F.INV.RT"), /* XLS:FINV */
    xls_func_entry(xlfFtest,            tFuncV,         arg_A,          2, 2,   "F.TEST"), /* XLS:FTEST */
    xls_func_entry(xlfFisher,           tFuncV,         arg_V,          1, 1,   "FISHER"),
    xls_func_entry(xlfFisherinv,        tFuncV,         arg_V,          1, 1,   "FISHERINV"),
    xls_func_entry(xlfFixed,            tFuncVarV,      arg_V,          2, 3,   "FIXED"),
    xls_func_entry(xlfFloor,            tFuncV,         arg_V,          2, 2,   "FLOOR"),
    xls_func_entry(xlfFrequency,        tFuncA,         arg_R,          2, 2,   "FREQUENCY"),
    xls_func_entry(xlfGammadist,        tFuncV,         arg_V,          4, 4,   "GAMMA.DIST"), /* XLS:GAMMADIST */ /* NB Excel requires fourth optional parameter */
    xls_func_entry(xlfGammainv,         tFuncV,         arg_V,          3, 3,   "GAMMA.INV"), /* XLS:GAMMAINV */
    xls_func_entry(xlfGammaln,          tFuncV,         arg_V,          1, 1,   "GAMMALN"),
    xls_func_entry(xlfGeomean,          tFuncVarV,      arg_R,          1, 30,  "GEOMEAN"),
    xls_func_entry(xlfHarmean,          tFuncVarV,      arg_R,          1, 30,  "HARMEAN"),
    xls_func_entry(xlfHypgeomdist,      tFuncV,         arg_V,          4, 4,   "HYPGEOM.DIST"), /* XLS:HYPGEOMDIST */ /* NB Excel doesn't cater for fifth optional parameter */
    xls_func_entry(xlfLognormdist,      tFuncV,         arg_V,          3, 3,   "LOGNORM.DIST"), /* XLS:LOGNORMDIST*/ /* NB Excel doesn't cater for fourth optional parameter */
    xls_func_entry(xlfLoginv,           tFuncV,         arg_V,          3, 3,   "LOGNORM.INV"), /* XLS:LOGINV */
    xls_func_entry(xlfKurt,             tFuncVarV,      arg_R,          1, 30,  "KURT"),
    xls_func_entry(xlfMode,             tFuncV,         arg_A,          1, 1,   "MODE"),
    xls_func_entry(xlfNegbinomdist,     tFuncV,         arg_V,          3, 3,   "NEGBINOM.DIST"), /* XLS:NEGBINOMDIST */ /* NB Excel doesn't cater for fourth optional parameter */
    xls_func_entry(xlfNormdist,         tFuncV,         arg_V,          4, 4,   "NORM.DIST"), /* XLS:NORMDIST */ /* NB Excel requires fourth optional parameter */
    xls_func_entry(xlfNorminv,          tFuncV,         arg_V,          3, 3,   "NORM.INV"), /* XLS:NORMINV */
    xls_func_entry(xlfNormsdist,        tFuncV,         arg_V,          1, 1,   "NORM.S.DIST"), /* XLS:NORMSDIST */ /* NB Excel doesn't cater for second optional parameter */
    xls_func_entry(xlfNormsinv,         tFuncV,         arg_V,          1, 1,   "NORM.S.INV"), /* XLS:NORMSINV */
    xls_func_entry(xlfOdd,              tFuncV,         arg_V,          1, 1,   "ODD"),

    xls_func_entry(xlfBetadist,         tFuncVarV,      arg_V,          3, 5,   "ODF.BETADIST"), /* NB Excel doesn't cater for sixth optional parameter */
    xls_func_entry(xlfTdist,            tFuncV,         arg_V,          3, 3,   "ODF.TDIST"), /* XLS:TDIST */

    xls_func_entry(xlfPearson,          tFuncV,         arg_A,          2, 2,   "PEARSON"),
    xls_func_entry(xlfPercentile,       tFuncV,         arg_R_V,        2, 2,   "PERCENTILE.INC"), /* XLS:PERCENTILE */
    xls_func_entry(xlfPercentrank,      tFuncVarV,      arg_R_V,        2, 3,   "PERCENTRANK.INC"), /* XLS:PERCENTRANK */
    xls_func_entry(xlfPermut,           tFuncV,         arg_V,          2, 2,   "PERMUT"),
    xls_func_entry(xlfPoisson,          tFuncV,         arg_V,          3, 3,   "POISSON.DIST"), /* XLS:POISSON */
    xls_func_entry(xlfProb,             tFuncVarV,      arg_A_A_V,      3, 4,   "PROB"),
    xls_func_entry(xlfQuartile,         tFuncV,         arg_R_V,        2, 2,   "QUARTILE.INC"), /* XLS:QUARTILE */
    xls_func_entry(xlfRank,             tFuncVarV,      arg_V_R_V,      2, 3,   "RANK.EQ"), /* XLS:RANK */
    xls_func_entry(xlfRsq,              tFuncV,         arg_A,          2, 2,   "RSQ"),
    xls_func_entry(xlfSkew,             tFuncVarV,      arg_R,          1, 30,  "SKEW"),
    xls_func_entry(xlfSlope,            tFuncV,         arg_A,          2, 2,   "SLOPE"),
    xls_func_entry(xlfStandardize,      tFuncV,         arg_V,          3, 3,   "STANDARDIZE"),
    xls_func_entry(xlfSumsq,            tFuncVarV,      arg_R,          0, 30,  "SUMSQ"),
    xls_func_entry(xlfSumx2my2,         tFuncV,         arg_A,          2, 2,   "SUM_X2MY2"),
    xls_func_entry(xlfSumx2py2,         tFuncV,         arg_A,          2, 2,   "SUM_X2PY2"),
    xls_func_entry(xlfSumxmy2,          tFuncV,         arg_A,          2, 2,   "SUM_XMY2"),
    xls_func_entry(xlfTinv,             tFuncV,         arg_V,          2, 2,   "T.INV.2T"), /* XLS:TINV */
    xls_func_entry(xlfTtest,            tFuncV,         arg_A_A_V,      4, 4,   "T.TEST"), /* XLS:TTEST */
    xls_func_entry(xlfTrimmean,         tFuncV,         arg_R_V,        2, 2,   "TRIMMEAN"),
    xls_func_entry(xlfWeibull,          tFuncV,         arg_V,          4, 4,   "WEIBULL.DIST"), /* XLS:WEIBULL */
    xls_func_entry(xlfZtest,            tFuncVarV,      arg_R_V,        2, 3,   "Z.TEST") /* XLS:ZTEST */
};

static const XLS_COMPILER_FUNC_ENTRY
BIFF5_compiler_functions[] = /* ordered as Fireworkz */
{
    xls_func_entry(xlfCountblank,       tFuncV,         arg_V,          1, 1,   "COUNTBLANK"),
    xls_func_entry(xlfDays360,          tFuncVarV,      arg_V,          2, 3,   "DAYS_360"), /* XLS:DAYS360 */
  /*xls_func_entry(xlfHlookup,          tFuncVarV,      arg_V_R_R_V,    3, 4,   "HLOOKUP"),*/ /* our 4th argument is not the same */
    xls_func_entry(xlfConcatenate,      tFuncVarV,      arg_V,          0, 30,  "JOIN"), /* XLS:CONCATENATE */
    xls_func_entry(xlfLarge,            tFuncV,         arg_R_V,        2, 2,   "LARGE"),
    xls_func_entry(xlfPower,            tFuncV,         arg_V,          2, 2,   "POWER"),
    xls_func_entry(xlfSmall,            tFuncV,         arg_R_V,        2, 2,   "SMALL"),
  /*xls_func_entry(xlfVlookup,          tFuncVarV,      arg_V_R_R_V,    3, 4,   "VLOOKUP"),*/ /* our 4th argument is not the same */
    xls_func_entry(xlfWeekday,          tFuncVarV,      arg_V,          1, 2,   "WEEKDAY")
};

static const XLS_COMPILER_FUNC_ENTRY
BIFF8_compiler_functions[] = /* ordered as Fireworkz */
{
    xls_func_entry(xlfAverageA,         tFuncVarV,      arg_R,          1, 30,  "AVERAGEA"),
    xls_func_entry(xlfMaxA,             tFuncVarV,      arg_R,          1, 30,  "MINA"),
    xls_func_entry(xlfMinA,             tFuncVarV,      arg_R,          1, 30,  "MAXA"),
    xls_func_entry(xlfStDevA,           tFuncVarV,      arg_R,          1, 30,  "STDEVA"),
    xls_func_entry(xlfStDevPA,          tFuncVarV,      arg_R,          1, 30,  "STDEVPA"),
    xls_func_entry(xlfVarA,             tFuncVarV,      arg_R,          1, 30,  "VARA"),
    xls_func_entry(xlfVarPA,            tFuncVarV,      arg_R,          1, 30,  "VARPA")
};

/* Fireworkz functions unsupported for Excel BIFF export */

/*acosec*/
/*acosech*/
/*acot*/
/*acoth*/
/*age*/
/*alert*/
/*asec*/
/*asech*/
/*beta*/
/*bin*/
/*break*/
/*c_acos*/
/*c_acosec*/
/*c_acosech*/
/*c_acosh*/
/*c_acot*/
/*c_acoth*/
/*c_add*/
/*c_asec*/
/*c_asech*/
/*c_asin*/
/*c_asinh*/
/*c_atan*/
/*c_atanh*/
/*c_cos*/
/*c_cosec*/
/*c_cosech*/
/*c_cosh*/
/*c_cot*/
/*c_coth*/
/*c_div*/
/*c_exp*/
/*c_ln*/
/*c_mul*/
/*c_power*/
/*c_radius*/
/*c_sec*/
/*c_sech*/
/*c_sin*/
/*c_sinh*/
/*c_sub*/
/*c_tan*/
/*c_tanh*/
/*c_theta*/
/*command*/
/*continue*/
/*cosec*/
/*cosech*/
/*cot*/
/*coth*/
/*cterm*/ /* XLS:NPV-ish */
/*current_cell*/
/*dayname*/
/*dcount*/
/*dcounta*/
/*deref*/
/*dmax*/
/*dmin*/
/*doubleclick*/
/*dproduct*/
/*dstd*/
/*dstdp*/
/*dsum*/
/*dvar*/
/*dvarp*/
/*else*/
/*elseif*/
/*endif*/
/*endwhile*/
/*flip*/
/*for*/
/*formula_text*/
/*function*/
/*fv*//* Fireworkz FV has different parameter order & result to XLS */
/*goto*/
/*grand*/
/*hlookup*/ /* our 4th argument is not the same */
/*index*/ /* Fireworkz INDEX has different parameter order to XLS */
/*input*/
/*irr*/ /* Fireworkz IRR has different parameter order & result to XLS */
/*listcount*/
/*monthdays*/
/*monthname*/
/*next*/
/*page*/
/*pages*/
/*phi*/
/*pmt*/ /* Fireworkz PMT has different parameter order & result to XLS */
/*repeat*/
/*result*/
/*reverse*/
/*sec*/
/*sech*/
/*set_name*/
/*set_value*/
/*sort*/
/*spearman*/
/*string*/
/*term*/
/*type*/ /* Fireworkz TYPE meaningless to XLS */
/*until*/
/*version*/
/*vlookup*/ /* our 4th argument is not the same */
/*weeknumber*/
/*while*/

typedef struct XLS_COMPILER_OPR_ENTRY
{
    A7CHARZ a7str_text_t5[4];   /* Fireworkz' operator text */

    S32 symno;                  /* Excel's token number (see xlcall.h) or our internal scan symbol */
}
XLS_COMPILER_OPR_ENTRY; typedef const XLS_COMPILER_OPR_ENTRY * PC_XLS_COMPILER_OPR_ENTRY;

#define xls_opr_entry(text_t5, symno) \
    { text_t5, symno }

static const XLS_COMPILER_OPR_ENTRY
xls_compiler_operators[] =
{
    /* longest ones first! */
    xls_opr_entry("&&",     tConcat), /* XLS:& */
    xls_opr_entry("<=",     tLE),
    xls_opr_entry(">=",     tGE),
    xls_opr_entry("<>",     tNE),

    xls_opr_entry("+",      tAdd),
    xls_opr_entry("-",      tSub),
    xls_opr_entry("*",      tMul),
    xls_opr_entry("/",      tDiv),
    xls_opr_entry("^",      tPower),
    xls_opr_entry("<",      tLT),
    xls_opr_entry(">",      tGT),
    xls_opr_entry("=",      tEQ),

    /* Fireworkz operators which mutate to functions in Excel */
    xls_opr_entry("&",      XLS_SCAN_SYMBOL_OPR_AND),
    xls_opr_entry("|",      XLS_SCAN_SYMBOL_OPR_OR),
    xls_opr_entry("!",      XLS_SCAN_SYMBOL_OPR_NOT),

    /* include some special scanner tokens here to simplify scanner */
    xls_opr_entry("(",      XLS_SCAN_SYMBOL_BRACKET_OPEN),
    xls_opr_entry(")",      XLS_SCAN_SYMBOL_BRACKET_CLOSE),
    xls_opr_entry(",",      XLS_SCAN_SYMBOL_COMMA)
};

static P_QUICK_BLOCK p_quick_block_rpn;

static PC_USTR p_compile_scan;

static SLR reference_slr;

static BYTE ref_or_value = tRefR;

_Check_return_
static STATUS
xls_expr(void);

#define xls_rpn_out_bytes(p_quick_block_rpn, p_data, n_bytes) \
    quick_block_bytes_add(p_quick_block_rpn, p_data, n_bytes)

/******************************************************************************
*
* output byte to rpn
*
******************************************************************************/

_Check_return_
static STATUS
xls_rpn_out_U8(
    _InVal_     U8 u8)
{
    return(xls_rpn_out_bytes(p_quick_block_rpn, &u8, 1));
}

/******************************************************************************
*
* output 16-bit value to rpn (little-endian)
*
******************************************************************************/

_Check_return_
static STATUS
xls_rpn_out_U16(
    _InVal_     U16 u16)
{
    /* OK 'cos we are little-endian, as is the file */
    return(xls_rpn_out_bytes(p_quick_block_rpn, (PC_U8) &u16, sizeof32(u16)));
}

/******************************************************************************
*
* output floating point value to rpn (8087-format)
*
******************************************************************************/

_Check_return_
static STATUS
xls_rpn_out_F64(
    _InVal_     F64 f64)
{
    BYTE fp_8087[8];

    writeval_F64_as_8087(fp_8087, f64);

    return(xls_rpn_out_bytes(p_quick_block_rpn, fp_8087, sizeof32(fp_8087)));
}

/******************************************************************************
*
* make an Excel form absolute/relative cell/range reference
*
******************************************************************************/

_Check_return_
static STATUS
xls_rpn_out_row(
    _InVal_     ROW row,
    _InVal_     COL col)
{
    S32 xls_row;

    if(0 == (row & 0x80000000))
    {
        xls_row = (S32) (row /* - reference_slr.row */); /* NB NOT method B */

        xls_row = (xls_row & 0x3FFF);

        xls_row |= 0x8000; /* flag as relative row */
    }
    else
    {
        xls_row = (S32) (row & 0x3FFF); /* absolute row */
    }

    if(0 == (col & 0x80000000))
    {
        xls_row |= 0x4000; /* flag as relative col */
    }

    return(xls_rpn_out_U16((U16) xls_row));
}

_Check_return_
static STATUS
xls_rpn_out_col(
    _InVal_     COL col)
{
    S32 xls_col;

    if(0 == (col & 0x80000000))
    {
        xls_col = (S32) (col /* - reference_slr.col */); /* NB NOT method B */

        xls_col = (xls_col & 0x00FF); /* relative col - NB corresponding flag is in the row! */
    }
    else
    {
        xls_col = (S32) (col & 0x00FF); /* absolute col */
    }

    return(xls_rpn_out_U8((U8) xls_col));
}

_Check_return_
static STATUS
xls_rpn_out_slr(
    _InRef_     PC_SLR p_slr)
{
    status_return(xls_rpn_out_row(p_slr->row, p_slr->col));

    return(xls_rpn_out_col(p_slr->col));
}

_Check_return_
static STATUS
xls_rpn_out_range(
    _InRef_     PC_SLR p_slr_s,
    _InRef_     PC_SLR p_slr_e)
{
    status_return(xls_rpn_out_row(p_slr_s->row, p_slr_s->col));

    status_return(xls_rpn_out_row(p_slr_e->row, p_slr_e->col));

    status_return(xls_rpn_out_col(p_slr_s->col));

    return(xls_rpn_out_col(p_slr_e->col));
}

_Check_return_
static STATUS
xls_rpn_out_tStr(
    _In_reads_(len) PC_U8 p_u8,
    _In_        U32 len)
{
    status_return(xls_rpn_out_U8(tStr));

    if(len > 255)
        len = 255; /* sorry, we have to truncate */

    status_return(xls_rpn_out_U8((U8) len));

    return(xls_rpn_out_bytes(p_quick_block_rpn, p_u8, len));
}

_Check_return_
static STATUS
xls_rpn_out_tBool(
    _InVal_     BOOL fBool)
{
    status_return(xls_rpn_out_U8(tBool));

    return(xls_rpn_out_U8((U8) (fBool != FALSE)));
}

_Check_return_
static STATUS
xls_rpn_out_tInt(
    _InVal_     U16 u16)
{
    status_return(xls_rpn_out_U8(tInt));

    return(xls_rpn_out_U16(u16));
}

_Check_return_
static STATUS
xls_rpn_out_tNum(
    _InVal_     F64 f64)
{
    status_return(xls_rpn_out_U8(tNum));

    return(xls_rpn_out_F64(f64));
}

/******************************************************************************
*
* recognise a constant and classify
*
******************************************************************************/

enum XLS_RECOG_CODES
{
    XLS_RECOG_NONE,
    XLS_RECOG_DATE,
    XLS_RECOG_INTEGER,
    XLS_RECOG_REAL
};

_Check_return_
static S32 /* recog code */
xls_recog_constant(
    _In_z_      PC_USTR ustr_in,
    _OutRef_    P_F64 p_f64,
    _OutRef_    P_S32 p_cs /* n scanned */)
{
    PC_U8Z p_u8_in = (PC_U8Z) ustr_in;

    /* check for date */
    for(;;) /* loop for structure */
    {
        PC_U8Z p_u8;
        P_U8Z p_u8_tmp;
        unsigned long v;
        S32 day, month, year;

        p_u8 = p_u8_in;
        v = strtoul(p_u8, &p_u8_tmp, 10);
        if(p_u8 == p_u8_tmp)
            break;
        p_u8 = p_u8_tmp;
        day = (S32) v;

        if(*p_u8++ != CH_FULL_STOP)
            break;

        v = strtoul(p_u8, &p_u8_tmp, 10);
        if(p_u8 == p_u8_tmp)
            break;
        p_u8 = p_u8_tmp;
        month = (S32) v;

        if(*p_u8++ != CH_FULL_STOP)
            break;

        v = strtoul(p_u8, &p_u8_tmp, 10);
        if(p_u8 == p_u8_tmp)
            break;
        p_u8 = p_u8_tmp;
        year = (S32) v;

        *p_f64 = day; /*etc*/
        *p_cs = (S32) (p_u8 - p_u8_in);

        return(XLS_RECOG_DATE);
        /*NOTREACHED*/
    }

    {
    int n;

    {
    P_U8 p_u8;
    F64 f64 = strtod(p_u8_in, &p_u8);
    if(p_u8_in == p_u8)
        n = 0;
    else
    {
        n = 1;
        *p_f64 = f64;
        *p_cs = (S32) (p_u8 - p_u8_in);
    }
    } /*block*/

    if(n > 0)
    {
        if((floor(*p_f64) == *p_f64) && (*p_f64 <= 65535.0))
            return(XLS_RECOG_INTEGER);

        return(XLS_RECOG_REAL);
    }
    } /*block*/

    *p_f64 = 0.0;
    *p_cs = 0;

    return(XLS_RECOG_NONE);
}

/******************************************************************************
*
* scan a cell reference
*
******************************************************************************/

_Check_return_ _Success_(0 != return)
static U32 /* chars scanned */
xls_recog_slr(
    _In_z_      PC_USTR ustr_in,
    _OutRef_    P_SLR p_slr)
{
    PC_U8Z p_u8_in = (PC_U8Z) ustr_in;
    PC_U8Z p_u8 = p_u8_in;
    U32 scanned;
    BOOL abs_col = FALSE;
    BOOL abs_row = FALSE;

    if(*p_u8 == CH_DOLLAR_SIGN)
    {
        ++p_u8;
        abs_col = TRUE;
    }

    {
    S32 col;
    if(0 == (scanned = (U32) stox((PC_USTR) p_u8, &col)))
        return(0);
    if(col > XLS_MAXCOL_BIFF2)
        return(0);

    p_slr->col = (COL) col;
    p_u8 += scanned;
    } /*block*/

    if(*p_u8 == CH_DOLLAR_SIGN)
    {
        ++p_u8;
        abs_row = TRUE;
    }

    if(!/*"C"*/isdigit(*p_u8))
        return(0);

    {
    S32 row;

    {
    P_U8 p_u8_tmp;
    unsigned long v = strtoul(p_u8, &p_u8_tmp, 10);
    if(p_u8 == p_u8_tmp)
        return(0);
    if(v > (unsigned long) ((biff_version == 8) ? XLS_MAXROW_BIFF8 : XLS_MAXROW_BIFF2))
        return(0);
    row = (S32) v;
    scanned = (U32) (p_u8_tmp - p_u8);
    } /*block*/

    p_slr->row = row - 1;
    p_u8 += scanned;
    } /*block*/

    if(abs_col)
        p_slr->col |= 0x80000000;

    if(abs_row)
        p_slr->row |= 0x80000000;

    return(PtrDiffBytesU32(p_u8, ustr_in));
}

/******************************************************************************
*
* lookup function in master table
*
******************************************************************************/

_Check_return_
static S32
xls_func_lookup_in(
    _In_reads_(uchars_n) PC_USTR ustr_ftext,
    _InVal_     U32 uchars_n,
    _In_reads_(n_compiler_functions) PC_XLS_COMPILER_FUNC_ENTRY compiler_functions,
    _InVal_     U32 n_compiler_functions)
{
    PC_A7STR a7str_ftext = (PC_A7STR) ustr_ftext;
    S32 res = -1;
    U32 i;

    for(i = 0; i < n_compiler_functions; ++i)
    {
        PC_XLS_COMPILER_FUNC_ENTRY p_xls_compiler_func_entry = &compiler_functions[i];
        const U32 len_compare = strlen32(p_xls_compiler_func_entry->a7str_text_t5);

        if(uchars_n != len_compare)
            continue;

        /* no need for case-sensitive comparision - we have determined what the Fireworkz decompiler yields */
        if(0 != short_memcmp32(a7str_ftext, p_xls_compiler_func_entry->a7str_text_t5, len_compare))
            continue;

        xls_scan_symbol.symno = p_xls_compiler_func_entry->token;
        xls_scan_symbol.arg.p_xls_compiler_func_entry = p_xls_compiler_func_entry;
        res = len_compare;
        break;
    }

    return(res);
}

_Check_return_
static S32
xls_func_lookup(
    _In_reads_(uchars_n) PC_USTR ustr_ftext,
    _InVal_     U32 uchars_n)
{
    if(biff_version >= 8)
    {
        S32 res = xls_func_lookup_in(ustr_ftext, uchars_n, BIFF8_compiler_functions, elemof32(BIFF8_compiler_functions));

        if(res >= 0)
            return(res);
    }

    if(biff_version >= 5)
    {
        S32 res = xls_func_lookup_in(ustr_ftext, uchars_n, BIFF5_compiler_functions, elemof32(BIFF5_compiler_functions));

        if(res >= 0)
            return(res);
    }

    if(biff_version >= 4)
    {
        S32 res = xls_func_lookup_in(ustr_ftext, uchars_n, BIFF4_compiler_functions, elemof32(BIFF4_compiler_functions));

        if(res >= 0)
            return(res);
    }

    if(biff_version >= 3)
    {
        S32 res = xls_func_lookup_in(ustr_ftext, uchars_n, BIFF3_compiler_functions, elemof32(BIFF3_compiler_functions));

        if(res >= 0)
            return(res);
    }

    /* untranslated function */
    return(-1);
}

/******************************************************************************
*
* lookup operator in master table
*
******************************************************************************/

_Check_return_
static S32
xls_opr_lookup(
    _In_    PC_USTR ustr_text_t5)
{
    PC_A7STR a7str_text_t5 = (PC_A7STR) ustr_text_t5; 
    S32 res = -1;
    U32 i;

    for(i = 0; i < elemof32(xls_compiler_operators); ++i)
    {
        PC_XLS_COMPILER_OPR_ENTRY p_xls_compiler_opr_entry = &xls_compiler_operators[i];

        if(*a7str_text_t5 != *(p_xls_compiler_opr_entry->a7str_text_t5))
            continue;

        if(CH_NULL == p_xls_compiler_opr_entry->a7str_text_t5[1])
        {   /* most Fireworkz operators are single character */
            res = 1;
        }
        else
        {
            const U32 len_compare = strlen32(p_xls_compiler_opr_entry->a7str_text_t5);

            if(0 != /*"C"*/strncmp(a7str_text_t5 + 1, p_xls_compiler_opr_entry->a7str_text_t5, len_compare - 1))
                continue;

            res = len_compare;
        }

        xls_scan_symbol.symno = p_xls_compiler_opr_entry->symno;
        break;
    }

    return(res);
}

static void
xls_scan_symbol_set_bad(void)
{
    xls_scan_symbol.symno = XLS_SCAN_SYMBOL_BAD;
}

static inline void
xls_scan_symbol_set_blank(void)
{
    xls_scan_symbol.symno = XLS_SCAN_SYMBOL_BLANK;
}

/******************************************************************************
*
* scan a symbol from the expression
*
******************************************************************************/

_Check_return_
static S32
scan_next_symbol(void)
{
    PtrSkipSpaces(PC_USTR, p_compile_scan);

    assert(PtrGetByte(p_compile_scan) != LF); /* we asked for these to be filtered out */
    assert(PtrGetByte(p_compile_scan) != CR);

    /* check for end of expression */
    if(CH_NULL == PtrGetByte(p_compile_scan))
    {
        xls_scan_symbol.symno = XLS_SCAN_SYMBOL_END;
        return(xls_scan_symbol.symno);
    }

    /* check for constant */
    if(/*"C"*/isdigit(PtrGetByte(p_compile_scan)) /*|| (PtrGetByte(p_compile_scan) == CH_FULL_STOP)*/)
    {
        S32 scan_code, scanned;

        if((scan_code = xls_recog_constant(p_compile_scan,
                                           &xls_scan_symbol.arg.f64,
                                           &scanned)) != 0)
        {
            ustr_IncBytes(p_compile_scan, scanned);

            switch(scan_code)
            {
            default: default_unhandled();
#if CHECKING
            case XLS_RECOG_DATE:
            case XLS_RECOG_REAL:
#endif
                return(xls_scan_symbol.symno = tNum);

            case XLS_RECOG_INTEGER:
                return(xls_scan_symbol.symno = tInt);
            }
        }
    }

    /* check for cell reference/range */
    if(sbchar_isalpha(PtrGetByte(p_compile_scan)) || (PtrGetByte(p_compile_scan) == CH_DOLLAR_SIGN))
    {
        S32 scanned;

        if((scanned = xls_recog_slr(p_compile_scan, &xls_scan_symbol.arg.range.s)) != 0)
        {
            ustr_IncBytes(p_compile_scan, scanned);

            /* check for another SLR (always preceded by colon by decompiler for Excel) to make range */
            if( (PtrGetByte(p_compile_scan) == CH_COLON) &&
                ((scanned = xls_recog_slr(ustr_AddBytes(p_compile_scan, 1), &xls_scan_symbol.arg.range.e)) != 0) )
            {
                ustr_IncBytes(p_compile_scan, (1 /*:*/ + scanned));

                return(xls_scan_symbol.symno = tAreaR);
            }

            return(xls_scan_symbol.symno = ref_or_value); /* may be tRefR or tRefV depending on context */
        }
    }

    /* check for function (Fireworkz rules) */
    if(sbchar_isalpha(PtrGetByte(p_compile_scan)))
    {
        int len = 0;

        if( sbchar_isalpha(PtrGetByteOff(p_compile_scan, len)) ||
            (CH_UNDERSCORE == PtrGetByteOff(p_compile_scan, len)) )
        {   /* "XID_Start" */
            ++len;

            while( sbchar_isalnum(PtrGetByteOff(p_compile_scan, len)) ||
                   (CH_UNDERSCORE == PtrGetByteOff(p_compile_scan, len)) ||
                   (CH_FULL_STOP == PtrGetByteOff(p_compile_scan, len)) )
            {   /* "XID_Continue" */
                ++len;
            }
        }

        if(status_ok(xls_func_lookup(p_compile_scan, len)))
        {
            ustr_IncBytes(p_compile_scan, len);

            return(xls_scan_symbol.symno);
        }

        xls_scan_symbol_set_bad();
        return(xls_scan_symbol.symno);
    }

    /* check for string */
    if(PtrGetByte(p_compile_scan) == CH_QUOTATION_MARK)
    {
        PC_U8Z p_u8 = PtrAddBytes(PC_U8Z, p_compile_scan, 1);
        while(*p_u8 && *p_u8 != PtrGetByte(p_compile_scan))
            ++p_u8;
        if(*p_u8 != PtrGetByte(p_compile_scan))
        {
            xls_scan_symbol_set_bad();
            return(xls_scan_symbol.symno);
        }

        xls_scan_symbol.arg.string.p_string = PtrAddBytes(PC_U8Z, p_compile_scan, 1);
        xls_scan_symbol.arg.string.len = PtrDiffBytesU32(p_u8, p_compile_scan);
        p_compile_scan = ustr_AddBytes(p_u8, 1);

        return(xls_scan_symbol.symno = tStr);
    }

    { /* check for operator (or other special internal scanner symbols) */
    S32 len;

    if(status_ok(len = xls_opr_lookup(p_compile_scan)))
    {
        ustr_IncBytes(p_compile_scan, len);

        return(xls_scan_symbol.symno);
    }
    } /*block*/

    xls_scan_symbol_set_bad();
    return(xls_scan_symbol.symno);
}

/******************************************************************************
*
* recognise an element of a list of function arguments
*
******************************************************************************/

_Check_return_
static STATUS
element(
    _InVal_     BOOL skip_output)
{
    STATUS status;

    if(check_next_symbol() == tAreaR)
    {
        xls_scan_symbol_set_blank();

        if(skip_output)
            status = STATUS_OK;
        else
        if(status_ok(status = xls_rpn_out_U8(tAreaR)))
        {
            status = xls_rpn_out_range(&xls_scan_symbol.arg.range.s, &xls_scan_symbol.arg.range.e);
        }

        return(status);
    }

    return(xls_expr());
}

/******************************************************************************
*
* process a function with arguments
*
******************************************************************************/

_Check_return_
static STATUS
process_extra_function_arguments(
    _InRef_     PC_XLS_COMPILER_FUNC_ENTRY p_xls_compiler_func_entry,
    _In_        S32 n_args);

_Check_return_
static STATUS
process_function_arguments(
    _InRef_     PC_XLS_COMPILER_FUNC_ENTRY p_xls_compiler_func_entry)
{
    S32 n_args = 0;
    S32 args_idx = 0;

    if(0 == p_xls_compiler_func_entry->max_args)
    {   /* this function can't have any arguments */
        assert(NULL == p_xls_compiler_func_entry->p_args);
        return(0);
    }

    PTR_ASSERT(p_xls_compiler_func_entry->p_args);

    if(check_next_symbol() != XLS_SCAN_SYMBOL_BRACKET_OPEN)
    {
        if(0 != p_xls_compiler_func_entry->min_args)
            xls_scan_symbol_set_bad();
        return(0);
    }

    do  {
        BYTE cur_ref_or_value = ref_or_value;
        if(0 != p_xls_compiler_func_entry->p_args[args_idx])
            ref_or_value = p_xls_compiler_func_entry->p_args[args_idx++]; /* set to what we expect here */

        xls_scan_symbol_set_blank();

        /* don't emit any extra parameters that Fireworkz functions have that Excel can't take */
        status_return(element(n_args >= p_xls_compiler_func_entry->max_args));

        ++n_args;

        ref_or_value = cur_ref_or_value;
    }
    while(check_next_symbol() == XLS_SCAN_SYMBOL_COMMA);

    if(check_next_symbol() != XLS_SCAN_SYMBOL_BRACKET_CLOSE)
    {
        xls_scan_symbol_set_bad();
        return(n_args);
    }

    if( (n_args < p_xls_compiler_func_entry->min_args) )
    {
        if(status_fail(process_extra_function_arguments(p_xls_compiler_func_entry, n_args)))
        {
            xls_scan_symbol_set_bad();
            return(STATUS_FAIL);
        }
    }

    xls_scan_symbol_set_blank();

    return(n_args);
}

/******************************************************************************
*
* process any extra function arguments that are
* required by Excel but are optional in Fireworkz
*
******************************************************************************/

_Check_return_
static inline STATUS
xls_rpn_out_xlfFalse(void)
{
    status_return(tFuncV);

    return(xls_rpn_out_U16(xlfFalse));
}

_Check_return_
static inline STATUS
xls_rpn_out_xlfTrue(void)
{
    status_return(tFuncV);

    return(xls_rpn_out_U16(xlfTrue));
}

_Check_return_
static STATUS
process_extra_function_arguments(
    _InRef_     PC_XLS_COMPILER_FUNC_ENTRY p_xls_compiler_func_entry,
    _In_        S32 n_args)
{
    switch(p_xls_compiler_func_entry->xlf_number)
    {
    default:
        break;

    case xlfCeiling:
    case xlfFloor:
        if(n_args < 2)
        {   /* Excel requires second parameter - Fireworkz significance is one if omitted */
            status_return(xls_rpn_out_tNum(1.0));
            ++n_args;
        }
        break;

    case xlfExpondist:
    case xlfPoisson:
        if(n_args < 3)
        {   /* Excel requires third parameter - Fireworkz default is TRUE (CDF) */
            status_return(xls_rpn_out_tBool(TRUE)); /*xls_rpn_out_xlfTrue();*/
            ++n_args;
        }
        break;

    case xlfGammadist:
    case xlfNormdist:
        if(n_args < 4)
        {   /* Excel requires fourth parameter - Fireworkz default is TRUE (CDF) */
            status_return(xls_rpn_out_tBool(TRUE)); /*xls_rpn_out_xlfTrue();*/
            ++n_args;
        }
        break;
    }

    if(n_args >= p_xls_compiler_func_entry->min_args)
        return(STATUS_OK); /* now satisfied */

    return(STATUS_FAIL);
}

/******************************************************************************
*
* recognise constants and functions
*
******************************************************************************/

_Check_return_
static STATUS
xls_lterm(void)
{
    STATUS status = STATUS_OK;
    const S32 symno = check_next_symbol();

    switch(symno)
    {
    case tStr:
        xls_scan_symbol_set_blank();

        status = xls_rpn_out_tStr(xls_scan_symbol.arg.string.p_string, xls_scan_symbol.arg.string.len);
        break;

    case tInt:
        xls_scan_symbol_set_blank();

        status = xls_rpn_out_tInt((U16) xls_scan_symbol.arg.f64);
        break;

    case tNum:
        xls_scan_symbol_set_blank();

        status = xls_rpn_out_tNum(xls_scan_symbol.arg.f64);
        break;

    case tRefR:
    case tRefV:
        xls_scan_symbol_set_blank();

        status = xls_rpn_out_U8((U8) symno);

        if(status_ok(status))
            status = xls_rpn_out_slr(&xls_scan_symbol.arg.range.s);
        break;

    case tFuncR:
    case tFuncV:
    case tFuncA:
    case tFuncVarR:
    case tFuncVarV:
    case tFuncVarA:
        {
        PC_XLS_COMPILER_FUNC_ENTRY p_xls_compiler_func_entry = xls_scan_symbol.arg.p_xls_compiler_func_entry;
        STATUS n_args;

        xls_scan_symbol_set_blank();

        if(status_fail(n_args = process_function_arguments(p_xls_compiler_func_entry)))
        {
            status = n_args;
        }
        else
        {
            status = xls_rpn_out_U8((U8) p_xls_compiler_func_entry->token);

            if(status_ok(status))
            {
                switch(p_xls_compiler_func_entry->token)
                {
                default: default_unhandled();
#if CHECKING
                case tFuncR:
                case tFuncV:
                case tFuncA:
#endif
                    /* ignore n_args - Excel knows what's expected */
                    break;

                case tFuncVarR:
                case tFuncVarV:
                case tFuncVarA:
                    status = xls_rpn_out_U8((U8) n_args);
                    break;
                }
            }

            if(status_ok(status))
                status = xls_rpn_out_U16(p_xls_compiler_func_entry->xlf_number);
        }

        break;
        }
    }

    return(status);
}

/******************************************************************************
*
* recognise xls_lterm or brackets
*
******************************************************************************/

_Check_return_
static STATUS
xls_gterm(void)
{
    STATUS status;

    if(check_next_symbol() == XLS_SCAN_SYMBOL_BRACKET_OPEN)
    {
        xls_scan_symbol_set_blank();

        status_return(status = xls_expr());

        if(check_next_symbol() != XLS_SCAN_SYMBOL_BRACKET_CLOSE)
        {
            xls_scan_symbol_set_bad();
            return(status);
        }

        xls_scan_symbol_set_blank();

        return(xls_rpn_out_U8(tParen));
    }

    return(xls_lterm());
}

/******************************************************************************
*
* recognise unary +, -, !
*
******************************************************************************/

_Check_return_
static STATUS
xls_fterm(void)
{
    STATUS status;

    switch(check_next_symbol())
    {
    case tAdd:
        xls_scan_symbol_set_blank();

        status = xls_fterm();

        if(status_ok(status))
            status = xls_rpn_out_U8(tUplus); /* mutate */
        break;

    case tSub:
        xls_scan_symbol_set_blank();

        status = xls_fterm();

        if(status_ok(status))
            status = xls_rpn_out_U8(tUminus); /* mutate */
        break;

    case XLS_SCAN_SYMBOL_OPR_NOT:
        xls_scan_symbol_set_blank();

        status = xls_fterm();

        /* mutate Fireworkz operator into Excel function */
        if(status_ok(status))
            status = xls_rpn_out_U8(tFuncV);

        if(status_ok(status))
            status = xls_rpn_out_U16(xlfNot);
        break;

    default:
        status = xls_gterm();
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
xls_eterm(void)
{
    STATUS status;

    status_return(status = xls_fterm());

    while(check_next_symbol() == tPower)
    {
        xls_scan_symbol_set_blank();

        status_break(status = xls_fterm());

        status_break(status = xls_rpn_out_U8(tPower));
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
xls_dterm(void)
{
    STATUS status;
    S32 nxsym;

    status_return(status = xls_eterm());

    for(;;)
    {
        switch(nxsym = check_next_symbol())
        {
        case tMul:
        case tDiv:
            xls_scan_symbol_set_blank();
            break;

        default:
            return(status);
        }

        status_break(status = xls_eterm());

        status_break(status = xls_rpn_out_U8((U8) nxsym));
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
xls_cterm(void)
{
    STATUS status;
    S32 nxsym;

    status_return(status = xls_dterm());

    for(;;)
    {
        switch(nxsym = check_next_symbol())
        {
        case tAdd:
        case tSub:
            xls_scan_symbol_set_blank();
            break;

        default:
            return(status);
        }

        status_break(status = xls_dterm());

        status_break(status = xls_rpn_out_U8((U8) nxsym));
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
xls_bterm(void)
{
    STATUS status;
    S32 nxsym;

    status_return(status = xls_cterm());

    for(;;)
    {
        switch(nxsym = check_next_symbol())
        {
        case tEQ:
        case tNE:
        case tLT:
        case tGT:
        case tLE:
        case tGE:
            xls_scan_symbol_set_blank();
            break;

        default:
            return(status);
        }

        status_break(status = xls_cterm());

        status_break(status = xls_rpn_out_U8((U8) nxsym));
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
xls_aterm(void)
{
    STATUS status;
    U32 n_args = 1;

    status_return(status = xls_bterm());

    while(check_next_symbol() == XLS_SCAN_SYMBOL_OPR_AND)
    {
        xls_scan_symbol_set_blank();

        status_break(status = xls_bterm());

        ++n_args;
    }

    if((n_args > 1) && status_ok(status))
    {   /* mutate Fireworkz operator into Excel function */
        status = xls_rpn_out_U8(tFuncVarV);

        if(status_ok(status))
            status = xls_rpn_out_U8((U8) n_args);

        if(status_ok(status))
            status = xls_rpn_out_U16(xlfAnd);
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
xls_expr(void)
{
    STATUS status;
    U32 n_args = 1;

    status_return(status = xls_aterm());

    while(check_next_symbol() == XLS_SCAN_SYMBOL_OPR_OR)
    {
        xls_scan_symbol_set_blank();

        status_break(status = xls_aterm());

        ++n_args;
    }

    if((n_args > 1) && status_ok(status))
    {   /* mutate Fireworkz operator into Excel function */
        status = xls_rpn_out_U8(tFuncVarV);

        if(status_ok(status))
            status = xls_rpn_out_U8((U8) n_args);

        if(status_ok(status))
            status = xls_rpn_out_U16(xlfOr);
    }

    return(status);
}

/******************************************************************************
*
* compile expression into RPN for Excel BIFF
*
******************************************************************************/

_Check_return_
static STATUS
xls_compile(
    _InoutRef_  P_QUICK_BLOCK p_quick_block_rpn_in,
    _In_z_      PC_USTR p_text,
    _InRef_     PC_SLR p_slr)
{
    STATUS status;

    p_quick_block_rpn = p_quick_block_rpn_in; /* sad globals */

    p_compile_scan = p_text;
    reference_slr = *p_slr;

    ref_or_value = tRefV; /* =A1 should return value else Excel goes #VALUE! */

    xls_scan_symbol_set_blank();

    /* allow leading equals sign - decompiler should have put it there for Excel! */
    assert(PtrGetByte(p_compile_scan) == CH_EQUALS_SIGN);
    if(PtrGetByte(p_compile_scan) == CH_EQUALS_SIGN)
        ustr_IncByte(p_compile_scan);

    status = xls_expr();

    if(status_ok(status))
    {   /* failed to completly compile? Excel can't accept half-baked stuff */
        assert(xls_scan_symbol.symno == XLS_SCAN_SYMBOL_END);
        if(xls_scan_symbol.symno != XLS_SCAN_SYMBOL_END)
            status = STATUS_FAIL;
    }

    return(status);
}

#define xls_write_bytes(p_ff_op_format, p_data, n_bytes) \
    binary_write_bytes(&(p_ff_op_format)->of_op_format.output, p_data, n_bytes)

#define xls_write_U8(p_ff_op_format, u8) \
    binary_write_byte(&(p_ff_op_format)->of_op_format.output, u8)

/******************************************************************************
*
* write an unsigned 16-bit value out to the Excel BIFF stream
*
******************************************************************************/

_Check_return_
static inline STATUS
xls_write_U16(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U16 u16)
{
    /* OK 'cos we are little-endian, as is the file */
    return(xls_write_bytes(p_ff_op_format, &u16, sizeof32(u16)));
}

/******************************************************************************
*
* write an Excel record header (16-bit opcode, 16-bit length)
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_record_header(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     XLS_OPCODE opcode,
    _InVal_     U16 record_length)
{
    status_return(xls_write_U16(p_ff_op_format, opcode));

    return(xls_write_U16(p_ff_op_format, record_length));
}

/******************************************************************************
*
* write an Excel record (16-bit opcode, 16-bit length, record contents)
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     XLS_OPCODE opcode,
    _InVal_     U16 record_length,
    _In_reads_(record_length) PC_BYTE p_x)
{
    status_return(xls_write_record_header(p_ff_op_format, opcode, record_length));

    if(0 == record_length)
        return(STATUS_OK);

    return(xls_write_bytes(p_ff_op_format, p_x, record_length));
}

_Check_return_
static STATUS
xls_write_BOF_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     BOOL worksheet)
{
    XLS_OPCODE opcode;
    static BYTE x[] =   /* BIFF8 BOF record */
    {
        XLS_REC_U16(0), /* version LSB, MSB */

        XLS_REC_U16(0), /* type LSB, MSB */

        /* unused in BIFF3/4 but present,
         * BIFF5/7/8: build identifier
         */
        XLS_REC_U16(0),

        /* BIFF5/7/8: build year */
        XLS_REC_U16(0),

        /* BIFF8: file history flags */
        0x00, 0x00, 0x00, 0x00,

        /* BIFF8: lowest Excel version that can read all the records (see [MS-XLS]) */
        0x06, 0x00, 0x00, 0x00
    };
    U16 sizeof_x;

    if(worksheet)
        x[2] = 0x10; /* Worksheet */
    else
        x[2] = 0x05; /* Workbook Globals (BIFF 5/7/8) */

    assert(biff_version >= 3);

    if(biff_version == 3)
    {
        opcode = X_BOF_B3;
        sizeof_x = 6;
        x[1] = 3;
    }
    else if(biff_version == 4)
    {
        opcode = X_BOF_B4;
        sizeof_x = 8;
        x[1] = 4;
    }
    else /* BIFF5/7/8 */
    {
        opcode = X_BOF_B5_B8;
        if(biff_version == 5)
        {
            sizeof_x = 8;
            x[1] = worksheet ? 6 /* as per Excel 2000 files */ : 5;
            writeval_U16_LE(&x[4], 0x2CA3); /* build identifier (11427) */
        }
        else
        {
            sizeof_x = 16;
            x[1] = 6;
            writeval_U32_LE(&x[8],
                (1U <<  0) | /* file last edited on Windows platform (I take that to mean little-endian) */
                (1U <<  0) | /* file ever edited on Windows platform */
                (6U << 14)); /* highest version of application that has ever saved this file */
            writeval_U32_LE(&x[12],
                (6U <<  0) | /* see [MS-XLS] */
                (6U <<  8)); /* last version of application that has ever saved this file */
        }
        writeval_U16_LE(&x[6], 1997); /* see [MS-XLS] */
    }

    return(xls_write_record(p_ff_op_format, opcode, sizeof_x, x));
}

_Check_return_
static STATUS
xls_write_EOF_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    const XLS_OPCODE opcode = X_EOF; /* NB all have the same EOF record */

    return(xls_write_record_header(p_ff_op_format, opcode, 0)); /* no record body */
}

/******************************************************************************
*
* write out a BLANK record
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_BLANK_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_SLR p_slr)
{
    const XLS_OPCODE opcode = X_BLANK_B3_B8;
    static BYTE x[14] =             /* BIFF3-8 BLANK record */
    {
        XLS_REC_U16(0),             /* zero-based index of this row */
        XLS_REC_U16(0),             /* zero-based index of this column */
        XLS_REC_U16(15 /*0x0F*/)    /* zero-based index of the XF record */
    };
    const U16 sizeof_x = sizeof32(x);

    assert(biff_version >= 3);

    writeval_U16_LE(&x[0], (U16) p_slr->row);
    writeval_U16_LE(&x[2], (U16) p_slr->col);

    /* 4,5 are XF index (already set) */

    return(xls_write_record(p_ff_op_format, opcode, sizeof_x, x));
}

/******************************************************************************
*
* write out a BOOLERR record
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_BOOLERR_record_boolean(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     U8 boolean_value,
    _InRef_     PC_SLR p_slr)
{
    const XLS_OPCODE opcode = X_BOOLERR_B3_B8;
    static BYTE x[8] =              /* BIFF3-8 BOOLERR record */
    {
        XLS_REC_U16(0),             /* zero-based index of this row */
        XLS_REC_U16(0),             /* zero-based index of this column */
        XLS_REC_U16(15 /*0x0F*/),   /* zero-based index of the XF record */
        0,                          /* Boolean value */
        0                           /* flag as Boolean(0)/Error(1) value */
    };
    const U16 sizeof_x = sizeof32(x);

    assert(biff_version >= 3);

    writeval_U16_LE(&x[0], (U16) p_slr->row);
    writeval_U16_LE(&x[2], (U16) p_slr->col);

    /* 4,5 are XF index (already set) */

    x[6] = boolean_value;
    /* 7 is flag as Boolean/Error value (already set) */

    return(xls_write_record(p_ff_op_format, opcode, sizeof_x, x));
}

/******************************************************************************
*
* write out a number
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_NUMBER_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_F64 p_f64,
    _InRef_     PC_SLR p_slr)
{
    const XLS_OPCODE opcode = X_NUMBER_B3_B8;
    static BYTE x[14] =             /* BIFF3-8 NUMBER record */
    {
        XLS_REC_U16(0),             /* zero-based index of this row */
        XLS_REC_U16(0),             /* zero-based index of this column */
        XLS_REC_U16(15 /*0x0F*/),   /* zero-based index of the XF record */
        0, 0, 0, 0, 0, 0, 0, 0      /* number */
    };
    const U16 sizeof_x = sizeof32(x);

    assert(biff_version >= 3);

    writeval_U16_LE(&x[0], (U16) p_slr->row);
    writeval_U16_LE(&x[2], (U16) p_slr->col);

    /* 4,5 are XF index (already set) */

    writeval_F64_as_8087(&x[6], *p_f64);

    return(xls_write_record(p_ff_op_format, opcode, sizeof_x, x));
}

/******************************************************************************
*
* write out a date
*
******************************************************************************/

_Check_return_
static F64
xls_get_f64_from_date(
    _InRef_     PC_EV_DATE p_ev_date)
{
    F64 f64 = 0.0;

    if(EV_DATE_NULL != p_ev_date->date)
    {   /* Convert the date component to a largely Excel-compatible serial number */
        S32 serial_number = ss_dateval_to_serial_number(&p_ev_date->date);

        /* Excel can't represent dates earlier than 1900 at all (and doesn't correctly handle 1900 as non-leap year) */
        if( (serial_number <      61) /* 1st March 1900 is first common OK date */ ||
            (serial_number > 2958656) /* 31st December 9999 is last valid date in Excel */)
        {
            return(0.0);
        }

        f64 = (F64) serial_number;
    }

    if(EV_TIME_NULL != p_ev_date->time)
    {
        f64 += ss_timeval_to_serial_fraction(&p_ev_date->time);
    }

    return(f64);
}

_Check_return_
static STATUS
xls_write_date(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_EV_DATE p_ev_date,
    _InRef_     PC_SLR p_slr)
{
    const XLS_OPCODE opcode = X_NUMBER_B3_B8;
    static BYTE x[14] =         /* BIFF3-8 NUMBER record */
    {
        XLS_REC_U16(0),             /* zero-based index of this row */
        XLS_REC_U16(0),             /* zero-based index of this column */
        XLS_REC_U16(21 /*0x15*/),   /* zero-based index of the XF record */ /* NB format number as date */
        0, 0, 0, 0, 0, 0, 0, 0      /* date */
    };
    const U16 sizeof_x = sizeof32(x);

    writeval_U16_LE(&x[0], (U16) p_slr->row);
    writeval_U16_LE(&x[2], (U16) p_slr->col);

    /* 4,5 are XF index (already set) */

    writeval_F64_as_8087(&x[6], xls_get_f64_from_date(p_ev_date));

    return(xls_write_record(p_ff_op_format, opcode, sizeof_x, x));
}

/******************************************************************************
*
* write out a cell as a STRING record
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_STRING_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(uchars_n_in) PC_UCHARS uchars,
    _InVal_     U32 uchars_n_in)
{
    const XLS_OPCODE opcode = X_STRING_B3_B8;
    static BYTE x[] =               /* BIFF3-8 STRING record */
    {
        XLS_REC_U16(0)              /* BIFF3-5 Byte string, 16-bit string length */
    };
    const U16 sizeof_x = sizeof32(x);
    const U16 n_bytes = (uchars_n_in <= 0xFFFFU) ? (U16) uchars_n_in : 0xFFFFU; /* paranoid truncation */
    const U16 contents_length = n_bytes;
    const U16 record_length = sizeof_x + contents_length;

    writeval_U16_LE(&x[0], n_bytes);

    status_return(xls_write_record_header(p_ff_op_format, opcode, record_length));

    status_return(xls_write_bytes(p_ff_op_format, x, sizeof_x));

    status_return(xls_write_bytes(p_ff_op_format, uchars, n_bytes));

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_write_string(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(uchars_n_in) PC_UCHARS uchars,
    _InVal_     U32 uchars_n_in,
    _InRef_     PC_SLR p_slr)
{
    const XLS_OPCODE opcode = X_FORMULA_B2_B5_B8;
    static BYTE x[] =               /* BIFF5-8 FORMULA record */
    {
        XLS_REC_U16(0),             /* zero-based index of this row */
        XLS_REC_U16(0),             /* zero-based index of this column */
        XLS_REC_U16(15 /*0x0F*/),   /* zero-based index of the XF record */
        0x00, 0, 0, 0, 0, 0, 0xFF, 0xFF, /* denote STRING result of the formula */
        XLS_REC_U16(0),             /* option flags */
        0, 0, 0, 0,                 /* not used */
        XLS_REC_U16(0)              /* 16-bit length of RPN that follows */
    };
    const U16 sizeof_x = sizeof32(x);
    static BYTE rpn_header[] =  /* RPN header */
    {
        tStr,
        0                           /* 8-bit length of string that follows */
    };
    const U16 sizeof_rpn_header = sizeof32(rpn_header);
    const U16 n_bytes = (uchars_n_in <= 0xFFU) ? (U16) uchars_n_in : 0xFFU; /* sorry, we must truncate */
    const U16 contents_length = sizeof_rpn_header + n_bytes;
    const U16 record_length = sizeof_x + contents_length;

    writeval_U16_LE(&x[0], (U16) p_slr->row);
    writeval_U16_LE(&x[2], (U16) p_slr->col);

    /* 4,5 are XF index (already set) */

    /* 6..13 denote STRING result (already set) */

    writeval_U16_LE(&x[20], contents_length);

    rpn_header[1] = (U8) n_bytes;

    status_return(xls_write_record_header(p_ff_op_format, opcode, record_length));

    status_return(xls_write_bytes(p_ff_op_format, x, sizeof_x));

    status_return(xls_write_bytes(p_ff_op_format, rpn_header, sizeof_rpn_header));

    status_return(xls_write_bytes(p_ff_op_format, uchars, n_bytes));

    /* need a STRING record to follow as FORMULA record had a string result */
    status_return(xls_write_STRING_record(p_ff_op_format, uchars, n_bytes));

    return(STATUS_OK);
}

/******************************************************************************
*
* write out a cell as a LABEL record
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_LABEL_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_SLR p_slr)
{
    const XLS_OPCODE opcode = X_LABEL_B3_B8;
    static BYTE x[] =       /* BIFF3/4/5/7 LABEL record */
    {
        XLS_REC_U16(0),     /* zero-based index of this row */
        XLS_REC_U16(0),     /* zero-based index of this column */
        XLS_REC_U16(0),     /* zero-based index of the XF record */
        XLS_REC_U16(0)      /* 16-bit length of byte string (BIFF8 Unicode string) */
    };
    const U16 sizeof_x = sizeof32(x);
    U32 contents_length;
    U16 record_length;

    contents_length = uchars_n; /* !!! WCHAR??? !!! */

    assert(biff_version >= 3);

    /* write out label */

    writeval_U16_LE(&x[0], (U16) p_slr->row);
    writeval_U16_LE(&x[2], (U16) p_slr->col);

    /* 4,5 are XF index */

    if(contents_length > 4095-sizeof32(x))
        contents_length = 4095-sizeof32(x); /* limit to what is valid for this format */

    writeval_U16_LE(&x[6], (U16) contents_length);

    record_length = sizeof_x + (U16) contents_length;

    status_return(xls_write_record_header(p_ff_op_format, opcode, record_length));

    status_return(xls_write_bytes(p_ff_op_format, x, sizeof_x));

    /* byte string, 16-bit length */
    status_return(xls_write_bytes(p_ff_op_format, uchars, contents_length));

    return(STATUS_OK);
}

/******************************************************************************
*
* write out a cell to a Excel file
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_cell_ss_constant(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_SLR p_slr,
    P_EV_DATA p_ev_data)
{
    switch(p_ev_data->did_num)
    {
    case RPN_DAT_REAL:
        {
        const F64 f64 = p_ev_data->arg.fp;
        return(xls_write_NUMBER_record(p_ff_op_format, &f64, p_slr));
        }

    case RPN_DAT_BOOL8:
        return(xls_write_BOOLERR_record_boolean(p_ff_op_format, (U8) p_ev_data->arg.integer, p_slr));

    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        {
        const F64 f64 = (F64) p_ev_data->arg.integer;
        return(xls_write_NUMBER_record(p_ff_op_format, &f64, p_slr));
        }

    case RPN_DAT_STRING:
        if(p_ev_data->arg.string.size > 255)
            return(xls_write_LABEL_record(p_ff_op_format, p_ev_data->arg.string.uchars, p_ev_data->arg.string.size, p_slr));

        return(xls_write_string(p_ff_op_format, p_ev_data->arg.string.uchars, p_ev_data->arg.string.size, p_slr));

    case RPN_DAT_DATE:
        return(xls_write_date(p_ff_op_format, &p_ev_data->arg.ev_date, p_slr));

    case RPN_DAT_BLANK:
        return(xls_write_BLANK_record(p_ff_op_format, p_slr));

    default: default_unhandled();
#if CHECKING
    case RPN_DAT_ARRAY:
    case RPN_DAT_ERROR:
#endif
        break;
    }

    return(STATUS_OK);
}

static void
encode_result_STRING(
    /*_Out_writes_(8)*/ P_BYTE result)
{
    result[0] = 0x00; /* denote STRING result */
    result[6] = 0xFF;
    result[7] = 0xFF;
}

static void
encode_result_BOOLEAN(
    /*_Out_writes_(8)*/ P_BYTE result,
    _InVal_     BOOL fBool)
{
    result[0] = 0x01; /* denote Boolean value result */
    result[2] = (BYTE) (fBool != FALSE);
    result[6] = 0xFF;
    result[7] = 0xFF;
}

static void
encode_result_ERROR_NUM(
    /*_Out_writes_(8)*/ P_BYTE result)
{
    result[0] = 0x02; /* denote error value result */
    result[2] = 0x24;
    result[6] = 0xFF;
    result[7] = 0xFF;
}

static void
encode_result_EMPTY(
    /*_Out_writes_(8)*/ P_BYTE result)
{
    result[0] = 0x03; /* denote EMPTY result */
    result[6] = 0xFF;
    result[7] = 0xFF;
}

_Check_return_
static STATUS
xls_write_cell_ss_normal(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_SLR p_slr,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /* NOT CH_NULL-terminated */,
    P_EV_DATA p_ev_data)
{
    STATUS status = STATUS_OK;
    BYTE quick_block_rpn_buffer[200];
    QUICK_BLOCK quick_block_rpn;
    quick_block_with_buffer_setup(quick_block_rpn);

    if( status_ok(status = quick_ublock_nullch_add(p_quick_ublock)) && /* SKS - compiler needs CH_NULL-terminated string */
        status_ok(status = xls_compile(&quick_block_rpn, quick_ublock_ustr(p_quick_ublock), p_slr)) )
    {
        const XLS_OPCODE opcode = X_FORMULA_B2_B5_B8;
        static BYTE x[] =               /* BIFF5-8 FORMULA record */
        {
            XLS_REC_U16(0),             /* zero-based index of this row */
            XLS_REC_U16(0),             /* zero-based index of this column */
            XLS_REC_U16(15 /*0x0F*/),   /* zero-based index of the XF record */
            0, 0, 0, 0, 0, 0, 0, 0,     /* result of the formula */
            XLS_REC_U16(1),             /* option flags: [1]='Calculate on open' as Excel will often differ */
            0, 0, 0, 0,                 /* not used */
            XLS_REC_U16(0)              /* 16-bit length of RPN that follows */
        };
        const U16 sizeof_x = sizeof32(x);
        const U16 contents_length = (U16) quick_block_bytes(&quick_block_rpn);
        const U16 record_length = sizeof_x + contents_length;
        PC_UCHARS string_uchars = NULL;
        U32 string_size = 0;

        switch(p_ev_data->did_num)
        {
        default: default_unhandled();
#if CHECKING
        case RPN_DAT_ARRAY:
        case RPN_DAT_ERROR:
#endif
            break;

        case RPN_DAT_REAL:
            {
            const F64 f64_result = p_ev_data->arg.fp;
            if(isfinite((double) f64_result))
                writeval_F64_as_8087(&x[6], f64_result);
            else
                encode_result_ERROR_NUM(&x[6]);
            break;
            }

        case RPN_DAT_BOOL8:
            encode_result_BOOLEAN(&x[6], p_ev_data->arg.boolean);
            break;

        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
            {
            const F64 f64_result = (F64) p_ev_data->arg.integer;
            writeval_F64_as_8087(&x[6], f64_result);
            break;
            }

        case RPN_DAT_STRING:
            if(biff_version == 8)
            {
                if(0 == p_ev_data->arg.string.size)
                {
                    encode_result_EMPTY(&x[6]);
                    break;
                }
            }

            encode_result_STRING(&x[6]);
            string_uchars = p_ev_data->arg.string.uchars;
            string_size = p_ev_data->arg.string.size;
            break;

        case RPN_DAT_DATE:
            {
            const F64 f64_result = xls_get_f64_from_date(&p_ev_data->arg.ev_date);
            if(isfinite((double) f64_result))
                writeval_F64_as_8087(&x[6], f64_result);
            else
                encode_result_ERROR_NUM(&x[6]);
            break;
            }

        case RPN_DAT_BLANK:
            if(biff_version == 8)
            {
                encode_result_EMPTY(&x[6]);
                break;
            }
            string_uchars = uchars_empty_string;
            string_size = 0;
            break;
        }

        writeval_U16_LE(&x[0], (U16) p_slr->row);
        writeval_U16_LE(&x[2], (U16) p_slr->col);

        /* 4,5 are XF index (already set) */

        writeval_U16_LE(&x[20], contents_length);

        status = xls_write_record_header(p_ff_op_format, opcode, record_length);

        if(status_ok(status))
            status = xls_write_bytes(p_ff_op_format, x, sizeof_x);

        if(status_ok(status))
            status = xls_write_bytes(p_ff_op_format, quick_block_ptr(&quick_block_rpn), contents_length);

        /* may need a STRING record to follow if FORMULA record had a string result */
        if(status_ok(status) && (NULL != string_uchars)) /* NB length may be zero */
            status = xls_write_STRING_record(p_ff_op_format, string_uchars, string_size);
    }
    else if(STATUS_FAIL == status) /* just failed to compile; return all other errors */
    {
        quick_ublock_nullch_strip(p_quick_ublock); /* SKS - but xls_write_LABEL_record does not want CH_NULL-terminated string */

        status = xls_write_LABEL_record(p_ff_op_format, quick_ublock_uchars(p_quick_ublock), quick_ublock_bytes(p_quick_ublock), p_slr);
    }

    quick_block_dispose(&quick_block_rpn);

    return(status);
}

_Check_return_
static STATUS
xls_write_cell_ss(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_OBJECT_DATA p_object_data)
{
    STATUS status;
    OBJECT_DATA_READ object_data_read;

    object_data_read.object_data = *p_object_data;
    status = object_call_id(object_data_read.object_data.object_id,
                            p_docu, T5_MSG_OBJECT_DATA_READ, &object_data_read);
    status_return(status);

    if(object_data_read.constant)
    {
        status = xls_write_cell_ss_constant(p_ff_op_format, &object_data_read.object_data.data_ref.arg.slr, &object_data_read.ev_data);
    }
    else
    {
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
        quick_ublock_with_buffer_setup(quick_ublock);

        { /* Excel-ise the decompiler output - formulae that don't compile will appear better in Excel */
        SS_DECOMPILER_OPTIONS ss_decompiler_options = g_ss_decompiler_options;
        g_ss_decompiler_options.lf = 0/*1*/; /* prune these out for now */
        g_ss_decompiler_options.cr = 0;
        g_ss_decompiler_options.initial_formula_equals = 1;
        g_ss_decompiler_options.range_colon_separator = 1;
        g_ss_decompiler_options.upper_case_function = 1;
        g_ss_decompiler_options.upper_case_slr = 1;
        g_ss_decompiler_options.zero_args_function_parentheses = 0/*1*/; /* prune these out for now */

        {
        OBJECT_READ_TEXT object_read_text;
        object_read_text.object_data = *p_object_data;
        object_read_text.p_quick_ublock = &quick_ublock;
        object_read_text.type = OBJECT_READ_TEXT_PLAIN;
        status = object_call_id(object_read_text.object_data.object_id,
                                p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text);
        } /*block*/

        g_ss_decompiler_options = ss_decompiler_options;
        } /*block*/

        if(status_ok(status))
            status = xls_write_cell_ss_normal(p_ff_op_format, &object_data_read.object_data.data_ref.arg.slr, &quick_ublock, &object_data_read.ev_data);

        quick_ublock_dispose(&quick_ublock);
    }

    ss_data_free_resources(&object_data_read.ev_data);

    return(status);
}

_Check_return_
static STATUS
xls_write_cell_text(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_OBJECT_DATA p_object_data)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    {
    OBJECT_READ_TEXT object_read_text;
    object_read_text.object_data = *p_object_data;
    object_read_text.p_quick_ublock = &quick_ublock;
    object_read_text.type = OBJECT_READ_TEXT_PLAIN;
    status = object_call_id(object_read_text.object_data.object_id,
                            p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text);
    } /*block*/

    if(status_ok(status))
        status = xls_write_LABEL_record(p_ff_op_format, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock), &p_object_data->data_ref.arg.slr);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

/******************************************************************************
*
* write out CODEPAGE record
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_CODEPAGE_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff_CODEPAGE_record[] =
{
/* [CODEPAGE] (42h) */
    XLS_REC_OPCODE(X_CODEPAGE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1252) /* worksheet/workbook code page is 1252 */
};

    return(xls_write_bytes(p_ff_op_format, biff_CODEPAGE_record, sizeof32(biff_CODEPAGE_record)));
}

/******************************************************************************
*
* write out file range
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_DIMENSIONS_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    const XLS_OPCODE opcode = X_DIMENSIONS_B3_B8;
    static BYTE x[] =                                   /* BIFF8 DIMENSIONS record */
    {                       /* BIFF3/4/5/7 */           /* BIFF8 */
        XLS_REC_U16(0),     /* start row */             /* LOWORD(start row) */
        XLS_REC_U16(0),     /* end row (exclusive) */   /* HIWORD(start row) */
        XLS_REC_U16(0),     /* start col */             /* LOWORD(end row) (exclusive) */
        XLS_REC_U16(0),     /* end col (exclusive) */   /* HIWORD(end row) (exclusive) */
        XLS_REC_U16(0),     /* reserved (zero) */       /* start col */
        XLS_REC_U16(0),                                 /* end col (exclusive) */
        XLS_REC_U16(0)                                  /* reserved (zero) */
    };
    U16 sizeof_x;

    if(biff_version < 8)
    {   /* BIFF3/4/5/7 */
        assert(biff_version >= 3);

        writeval_U16_LE(&x[0], (U16) g_s_row);
        writeval_U16_LE(&x[2], (U16) g_e_row);
        writeval_U16_LE(&x[4], (U16) g_s_col);
        writeval_U16_LE(&x[6], (U16) g_e_col);

        /* 8,9 present but unused */
        writeval_U16_LE(&x[8], 0);

        sizeof_x = 10;
    }
    else
    {   /* BIFF8 */
        writeval_U32_LE(&x[0],  (U32) g_s_row);
        writeval_U32_LE(&x[4],  (U32) g_e_row);
        writeval_U16_LE(&x[8],  (U16) g_s_col);
        writeval_U16_LE(&x[10], (U16) g_e_col);

        /* 12,13 present but unused */
        writeval_U16_LE(&x[12], 0);

        sizeof_x = 14;
    }

    return(xls_write_record(p_ff_op_format, opcode, sizeof_x, x));
}

_Check_return_
static STATUS
xls_write_ROW_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InVal_     ROW row,
    _InVal_     COL first_col,
    _InVal_     COL last_col)
{
    const XLS_OPCODE opcode = X_ROW_B3_B8;
    static BYTE x[] =       /* BIFF3-8 ROW record */
    {
        XLS_REC_U16(0),     /* zero-based index of this row */
        XLS_REC_U16(0),     /* zero-based index of the column of the first cell which is described by a cell record */
        XLS_REC_U16(2),     /* zero-based index of the column after the last column in this row that contains a used cell */
        XLS_REC_U16(255),   /* row height in twips */
        XLS_REC_U16(0),     /* reserved1 */
        XLS_REC_U16(0),     /* unused1 */
        XLS_REC_U16(0x0100), /* option flags. [8] always set */
        XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the default XF record if options[7] */
    };
    const U16 sizeof_x = sizeof32(x);

    writeval_U16_LE(&x[0], (U16) row);
    writeval_U16_LE(&x[2], (U16) first_col);
    writeval_U16_LE(&x[4], (U16) last_col + 1U);

    return(xls_write_record(p_ff_op_format, opcode, sizeof_x, x));
}

_Check_return_
static STATUS
xls_write_ROW_records(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    SCAN_BLOCK scan_block;
    OBJECT_DATA object_data;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        ROW current_row = BAD_ROW;
        COL first_col = BAD_COL;
        COL last_col = BAD_COL;

        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            if(current_row != scan_block.slr.row)
            {
                if(BAD_COL != first_col) /* dump info for current_row? */
                    status = xls_write_ROW_record(p_ff_op_format, current_row, first_col, last_col);

                first_col = BAD_COL;
                last_col = BAD_COL;

                current_row = scan_block.slr.row;
            }

            switch(object_data.object_id)
            {
            case OBJECT_ID_TEXT:
            case OBJECT_ID_SS:
                if(BAD_COL == first_col)
                    first_col = scan_block.slr.col;

                last_col = scan_block.slr.col;
                break;

            default:
                break;
            }

            status_break(status);
        }

        if(status_ok(status))
            if(BAD_COL != first_col) /* dump info for current_row? */
                status = xls_write_ROW_record(p_ff_op_format, current_row, first_col, last_col);
    }

    return(status);
}

/******************************************************************************
*
* BIFF3 - write out all the column information
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_biff3_core(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    SCAN_BLOCK scan_block;
    OBJECT_DATA object_data;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            save_reflect_status((P_OF_OP_FORMAT) p_ff_op_format, cells_scan_percent(&scan_block));

            switch(object_data.object_id)
            {
            case OBJECT_ID_TEXT:
                status = xls_write_cell_text(p_ff_op_format, p_docu, &object_data);
                break;

            case OBJECT_ID_SS:
                status = xls_write_cell_ss(p_ff_op_format, p_docu, &object_data);
                break;

            default:
                break;
            }

            status_break(status);
        }
    }

    return(status);
}

/*
BIFF3 worksheet stream
*/

_Check_return_
static STATUS
xls_write_biff3_worksheet_FileProtectionBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_FileProtectionBlock[] =
{
/* [WRITEACCESS] (5Ch) */
    XLS_REC_OPCODE(X_WRITEACCESS_B3_B8),
    XLS_REC_LENGTH(0x70),
    0x0d, 0x53, 0x74, 0x75, 0x61, 0x72, 0x74, 0x20, 0x53, 0x77, 0x61, 0x6c, 0x65, 0x73, 0x20, 0x20,  /* (13)Stuart Swales then space padded */
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_FileProtectionBlock, sizeof32(biff3_worksheet_FileProtectionBlock)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_INDEX_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_INDEX_record[] =
{
/* [INDEX] (Bh) */
    XLS_REC_OPCODE(X_INDEX_B3_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0x0000), XLS_REC_U16(0x0000), /* absolute stream position of the first DEFINEDNAME record */
    XLS_REC_U16(0), /* zero-based index of first used row (rf) */
    XLS_REC_U16(0 /*4*/), /* zero-based index of first row of unused tail of worksheet (rl, last used row + 1) */
    XLS_REC_U16(0x0000 /*0x0641*/), XLS_REC_U16(0x0000), /* absolute stream position of the first XF record */
    /* array of nm (=rl-rf) absolute stream positions to the DBCELL record of each Row Block */
    XLS_REC_U16(0x0000 /*0x0741*/), XLS_REC_U16(0x0000),
};

    /* note the absolute position of the INDEX record - we will need to patch it later */
    status_return(xls_write_get_current_offset(p_ff_op_format, &index_record_abs_posn));

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_INDEX_record, sizeof32(biff3_worksheet_INDEX_record)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_CalculationSettingsBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_CalculationSettingsBlock[] =
{
/* [CALCMODE] (Dh) */
    XLS_REC_OPCODE(X_CALCMODE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = automatic */

/* [CALCCOUNT] (Ch) */
    XLS_REC_OPCODE(X_CALCCOUNT),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(100), /* iteration count for a calculation in iterative calculation mode */

/* [REFMODE] (Fh) */
    XLS_REC_OPCODE(X_REFMODE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = A1 mode */

/* [ITERATION] (11h) */
    XLS_REC_OPCODE(X_ITERATION),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = iterations off */

/* [DELTA] (10h) */
    XLS_REC_OPCODE(X_DELTA),
    XLS_REC_LENGTH(8),
    0xfc, 0xa9, 0xf1, 0xd2, 0x4d, 0x62, 0x50, 0x3f, /* XLS_F64 minimum value change required for iterative calculation to continue */

/* [SAVERECALC] (5Fh) */
    XLS_REC_OPCODE(X_SAVERECALC_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = recalculate before saving the document */

/* [1904] (22h) */
    XLS_REC_OPCODE(X_DATEMODE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = workbook uses the 1900 date system */

    /* [PRECISION] (Eh) */
    XLS_REC_OPCODE(X_PRECISION),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1) /* 1 = precision as displayed mode is not selected */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_CalculationSettingsBlock, sizeof32(biff3_worksheet_CalculationSettingsBlock)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_stuff(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_stuff[] =
{
/* [PRINTHEADERS] (2Ah) */
    XLS_REC_OPCODE(X_PRINTHEADERS),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = do not print row/column headers */

/* [PRINTGRIDLINES] (2Bh) */
    XLS_REC_OPCODE(X_PRINTGRIDLINES),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = do not print worksheet grid lines */

/* [GRIDSET] (82h) */
    XLS_REC_OPCODE(X_GRIDSET_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = print grid lines option changed at some point in the past */

/* [GUTS] (80h) */
    XLS_REC_OPCODE(X_GUTS_B3_B8),
    XLS_REC_LENGTH(8),
    XLS_REC_U16(0), /* width of the area to display row outlines (left of the worksheet), in pixels */
    XLS_REC_U16(0), /* height of the area to display column outlines (above the worksheet), in pixels */
    XLS_REC_U16(0), /* number of visible row outline levels (used row levels + 1; or 0, if not used) */
    XLS_REC_U16(0), /* number of visible column outline levels (used column levels + 1; or 0, if not used) */

/* [DEFAULTROWHEIGHT] (25h) */
    XLS_REC_OPCODE(X_DEFAULTROWHEIGHT_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x0000), /* option flags */
    XLS_REC_U16(255), /* default height for unused rows, in twips */

/* [COUNTRY] (8Ch) */
    XLS_REC_OPCODE(X_COUNTRY_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(1), /* Windows country identifier of the user interface language of Excel. 1 = USA */
    XLS_REC_U16(44), /* Windows country identifier of the system regional settings. 44 = United Kingdom */

/* [HIDEOBJ] (8Dh) */
    XLS_REC_OPCODE(X_HIDEOBJ_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0),

/* [WSBOOL] (81h) */ /* aka SHEETPR */
    XLS_REC_OPCODE(X_WSBOOL_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0x04C1) /* [10] show row outline symbols; [7] outline buttons right of outline group; [6] outline buttons below outline group; [0] show automatic page breaks */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_stuff, sizeof32(biff3_worksheet_stuff)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_FONT_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_FONT_records[] =
{
  /* four FONT records (lookup by zero-based index) */

/* [FONT][0] (31h) */
    XLS_REC_OPCODE(X_FONT_B3_B4),
    XLS_REC_LENGTH(0x0C),
    XLS_REC_U16(0xC8), /* height of the font in twips. SHOULD be greater than or equal to 20 and less than or equal to 8191 */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value. MUST be greater than or equal to 0x0008 and less than or equal to 0x003F or 0x0051 or 0x7FFF */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c, /* (5)Arial */

/* [FONT][1] (31h) */
    XLS_REC_OPCODE(X_FONT_B3_B4),
    XLS_REC_LENGTH(0x0C),
    XLS_REC_U16(0xC8), /* height of the font in twips */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c, /* (5)Arial */

/* [FONT][2] (31h) */
    XLS_REC_OPCODE(X_FONT_B3_B4),
    XLS_REC_LENGTH(0x0C),
    XLS_REC_U16(0xC8), /* height of the font in twips */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c, /* (5)Arial */

/* [FONT][3] (31h) */
    XLS_REC_OPCODE(X_FONT_B3_B4),
    XLS_REC_LENGTH(0x0C),
    XLS_REC_U16(0xC8), /* height of the font in twips */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c /* (5)Arial */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_FONT_records, sizeof32(biff3_worksheet_FONT_records)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_PageSettingsBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_PageSettingsBlock[] =
{
/* [HEADER] (14h) */
    XLS_REC_OPCODE(X_HEADER),
    XLS_REC_LENGTH(0),
    /* worksheet does not contain a page header */

/* [FOOTER] (15h) */
    XLS_REC_OPCODE(X_FOOTER),
    XLS_REC_LENGTH(0),
    /* worksheet does not contain a page footer */

/* [HCENTER] (83h) */
    XLS_REC_OPCODE(X_HCENTER_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = print worksheet left aligned */

/* [VCENTER] (84h) */
    XLS_REC_OPCODE(X_VCENTER_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = print worksheet aligned at top page border */

/* [LEFTMARGIN] (26h) */
    XLS_REC_OPCODE(X_LEFTMARGIN),
    XLS_REC_LENGTH(8),
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x3F, /* XLS_F64 */

/* [RIGHTMARGIN] (27h) */
    XLS_REC_OPCODE(X_RIGHTMARGIN),
    XLS_REC_LENGTH(8),
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x3F, /* XLS_F64 */

/* [TOPMARGIN] (28h) */
    XLS_REC_OPCODE(X_TOPMARGIN),
    XLS_REC_LENGTH(8),
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, /* XLS_F64 */

/* [BOTTOMMARGIN] (29h) */
    XLS_REC_OPCODE(X_BOTTOMMARGIN),
    XLS_REC_LENGTH(8),
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F /* XLS_F64 */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_PageSettingsBlock, sizeof32(biff3_worksheet_PageSettingsBlock)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_BACKUP_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_BACKUP_record[] =
{
/* [BACKUP] (40h) */
    XLS_REC_OPCODE(X_BACKUP),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = do not save a backup file when workbook is saved */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_BACKUP_record, sizeof32(biff3_worksheet_BACKUP_record)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_FORMAT_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_FORMAT_records[] =
{
/* [BUILTINFMTCOUNT] (56h) */
    XLS_REC_OPCODE(X_BUILTINFMTCOUNT_B3_B4),
    XLS_REC_LENGTH(0x02),
    XLS_REC_U16(0x17),

    /* thirty-three FORMAT records, directly indexed */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x08),
    0x07, 0x47, 0x65, 0x6e, 0x65, 0x72, 0x61, 0x6c,
    /* General */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x02),
    0x01, 0x30,
    /* 0 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x05),
    0x04, 0x30, 0x2e, 0x30, 0x30,
    /* 0.00 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x06),
    0x05, 0x23, 0x2c, 0x23, 0x23, 0x30,
    /* #,##0 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x09),
    0x08, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    /* #,##0.00 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x14),
    0x13, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x3b, 0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x23,
    0x2c, 0x23, 0x23, 0x30,
    /* "£"#,##0;\-"£"#,##0 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x19),
    0x18, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x3b, 0x5b, 0x52, 0x65, 0x64, 0x5d, 0x5c,
    0x2d, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30,
    /* "£"#,##0;[Red]\-"£"#,##0 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x1A),
    0x19, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x3b, 0x5c, 0x2d, 0x22,
    0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    /* "£"#,##0.00;\-"£"#,##0.00 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x1F),
    0x1e, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x3b, 0x5b, 0x52, 0x65,
    0x64, 0x5d, 0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    /* "£"#,##0.00;[Red]\-"£"#,##0.00 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x03),
    0x02, 0x30, 0x25,
    /* 0% */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x06),
    0x05, 0x30, 0x2e, 0x30, 0x30, 0x25,
    /* 0.00% */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x09),
    0x08, 0x30, 0x2e, 0x30, 0x30, 0x45, 0x2b, 0x30, 0x30,
    /* 0.00E+00 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x07),
    0x06, 0x23, 0x5c, 0x20, 0x3f, 0x2f, 0x3f,
    /* #\ ?/? */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x09),
    0x08, 0x23, 0x5c, 0x20, 0x3f, 0x3f, 0x2f, 0x3f, 0x3f,
    /* #\ ??/?? */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x0B),
    0x0a, 0x64, 0x64, 0x2f, 0x6d, 0x6d, 0x2f, 0x79, 0x79, 0x79, 0x79,
    /* dd/mm/yyyy */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x0C),
    0x0b, 0x64, 0x64, 0x5c, 0x2d, 0x6d, 0x6d, 0x6d, 0x5c, 0x2d, 0x79, 0x79,
    /* dd\-mmm\-yy */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x08),
    0x07, 0x64, 0x64, 0x5c, 0x2d, 0x6d, 0x6d, 0x6d,
    /* dd\-mmm */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x08),
    0x07, 0x6d, 0x6d, 0x6d, 0x5c, 0x2d, 0x79, 0x79,
    /* mmm\-yy */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x0C),
    0x0b, 0x68, 0x3a, 0x6d, 0x6d, 0x5c, 0x20, 0x41, 0x4d, 0x2f, 0x50, 0x4d,
    /* h:mm\ AM/PM */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x0F),
    0x0e, 0x68, 0x3a, 0x6d, 0x6d, 0x3a, 0x73, 0x73, 0x5c, 0x20, 0x41, 0x4d, 0x2f, 0x50, 0x4d,
    /* h:mm:ss\ AM/PM */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x06),
    0x05, 0x68, 0x68, 0x3a, 0x6d, 0x6d,
    /* hh:mm */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x09),
    0x08, 0x68, 0x68, 0x3a, 0x6d, 0x6d, 0x3a, 0x73, 0x73,
    /* hh:mm:ss */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x12),
    0x11, 0x64, 0x64, 0x2f, 0x6d, 0x6d, 0x2f, 0x79, 0x79, 0x79, 0x79, 0x5c, 0x20, 0x68, 0x68, 0x3a,
    0x6d, 0x6d,
    /* dd/mm/yyyy\ hh:mm */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x0E),
    0x0d, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x3b, 0x5c, 0x2d, 0x23, 0x2c, 0x23, 0x23, 0x30,
    /* #,##0;\-#,##0 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x13),
    0x0d, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x3b, 0x5b, 0x52, 0x65, 0x64, 0x5d, 0x5c, 0x2d, 0x23, 0x2c,
    0x23, 0x23, 0x30,
    /* #,##0;[Red]\-#,##0 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x14),
    0x13, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x3b, 0x5c, 0x2d, 0x23, 0x2c, 0x23, 0x23,
    0x30, 0x2e, 0x30, 0x30,
    /* #,##0.00;\-#,##0.00 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x19),
    0x18, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x3b, 0x5b, 0x52, 0x65, 0x64, 0x5d, 0x5c,
    0x2d, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    /* #,##0.00;[Red]\-#,##0.00 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x09),
    0x08, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x45, 0x2b, 0x30,
    /* ##0.0E+0 */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x06),
    0x05, 0x6d, 0x6d, 0x3a, 0x73, 0x73,
    /* mm:ss */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x02),
    0x01, 0x40,
    /* @ */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x31),
    0x30, 0x5f, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b,
    0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b, 0x5f,
    0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x22, 0x2d, 0x22, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f,
    0x2d,
    /* _-"£"* #,##0_-;\-"£"* #,##0_-;_-"£"* "-"_-;_-@_- */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x28),
    0x27, 0x5f, 0x2d, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b, 0x5c, 0x2d, 0x2a,
    0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x2a, 0x20, 0x22, 0x2d, 0x22,
    0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f, 0x2d,
    /* _-* #,##0_-;\-* #,##0_-;_-* "-"_-;_-@_- */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x39),
    0x38, 0x5f, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    0x5f, 0x2d, 0x3b, 0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e,
    0x30, 0x30, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x22, 0x2d, 0x22, 0x3f,
    0x3f, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f, 0x2d,
    /* _-"#"* #,##0.00_-;\-"#"* #,##0.00_-;_-"#"* "-"??_-;_-@_- */

/* [FORMAT] (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B2_B3),
    XLS_REC_LENGTH(0x30),
    0x2f, 0x5f, 0x2d, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x5f, 0x2d, 0x3b,
    0x5c, 0x2d, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x5f, 0x2d, 0x3b, 0x5f,
    0x2d, 0x2a, 0x20, 0x22, 0x2d, 0x22, 0x3f, 0x3f, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f, 0x2d
    /* _-* #,##0.00_-;\-* #,##0.00_-;_-* "-"??_-;_-@_- */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_FORMAT_records, sizeof32(biff3_worksheet_FORMAT_records)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_WorksheetProtectionBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_WorksheetProtectionBlock[] =
{
/* [WINDOWPROTECT] (19h) */
    XLS_REC_OPCODE(X_WINDOWPROTECT),
    XLS_REC_LENGTH(2),
    0x00, 0x00
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_WorksheetProtectionBlock, sizeof32(biff3_worksheet_WorksheetProtectionBlock)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_XF_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_XF_records[] =
{
  /* twenty-one XF records (of which, two cell XF) (lookup by zero-based index) */

    /* 1b. FONT record index */
    /* 1b. FORMAT record index */
    /* 1b. XF_TYPE_PROT */
    /* 1b. [7:2] XF_USED_ATTRIB */
    /* 2b. [15:4] parent XF index; [3] wrapped; [2:0] XF_HOR_ALIGN */
    /* 2b. XF_AREA_34: [15:11] pattern background; [10:6] pattern colour; [5:0] fill pattern */
    /* 4b. XF_BORDER_34: [31:27] right line colour; [26:24] right line style; [23:19] & [18:16] bottom ; [15:11] & [10:8] left ; [7:3] & [2:0] top*/

/* [XF][0] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0x03,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][1] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x01, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][2] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x01, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][3] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x02, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][4] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x02, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][5] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][6] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][7] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][8] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][9] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][10] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][11] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][12] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][13] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][14] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0xf5, 0xf7,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][15] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x00, /* FONT record index */
    0x00, /* FORMAT record index */
    0x01, 0x00,
    0x00, 0x00, /* this one is quite different*/
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][16] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x01, /* FONT record index */
    0x21, /* FORMAT record index */
    0xf5, 0xfb,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][17] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x01, /* FONT record index */
    0x1F, /* FORMAT record index */
    0xf5, 0xfb,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][18] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x01, /* FONT record index */
    0x20, /* FORMAT record index */
    0xf5, 0xfb,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][19] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x01, /* FONT record index */
    0x1E, /* FORMAT record index */
    0xf5, 0xfb,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00,

/* [XF][20] (E0h) */
    XLS_REC_OPCODE(X_XF_B3),
    XLS_REC_LENGTH(12),
    0x01, /* FONT record index */
    0x09, /* FORMAT record index */
    0xf5, 0xfb,
    0xf0, 0xff,
    0x00, 0xCE,
    0x00, 0x00, 0x00, 0x00
};

    { /* patch the INDEX record that we output earlier */
    U32 xf_record_abs_posn;
    const U32 patch_abs_posn = index_record_abs_posn + (2 + 2 + 8);
    status_return(xls_write_get_current_offset(p_ff_op_format, &xf_record_abs_posn));
    status_return(xls_write_patch_offset_U32(p_ff_op_format, patch_abs_posn, xf_record_abs_posn));
    } /*block*/

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_XF_records, sizeof32(biff3_worksheet_XF_records)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_STYLE_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_STYLE_records[] =
{
  /* six STYLE records */

/* [STYLE][0] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8010), /* [11:0] zero-based index of the cell style XF; [14:12] undefined; [15] fBuiltIn */
    /* two byte built-in cell style properties. MUST exist if and only if fBuiltIn is 1 */
    0x03, /* 'Comma' style */
    0xff, /* outline level. only valid for types 1 & 2 */

/* [STYLE][1] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(12),
    XLS_REC_U16(0x0011),
    0x09, 0x43, 0x6f, 0x6d, 0x6d, 0x61, 0x20, 0x5b, 0x30, 0x5d, /* 'Comma [0]' style */

/* [STYLE][2] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8012),
    0x04, /* 'Currency' style */
    0xff,

/* [STYLE][3] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(15),
    XLS_REC_U16(0x0013),
    0x0c, 0x43, 0x75, 0x72, 0x72, 0x65, 0x6e, 0x63, 0x79, 0x20, 0x5b, 0x30, 0x5d, /* 'Currency [0]' style */

/* [STYLE][4] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8000), /* references first XF record -> 'Normal' style */
    0x00, /* 'Normal' style */
    0xff,

/* [STYLE][5] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8014),
    0x05, /* 'Percent' style */
    0xff
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_STYLE_records, sizeof32(biff3_worksheet_STYLE_records)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_DEFCOLWIDTH_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_DEFCOLWIDTH_record[] =
{
/* [DEFCOLWIDTH] (55h) */
    XLS_REC_OPCODE(X_DEFCOLWIDTH),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(8), /* column width in characters, using the width of the zero character from default font (first FONT record in the file) */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_DEFCOLWIDTH_record, sizeof32(biff3_worksheet_DEFCOLWIDTH_record)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_WINDOW1_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_WINDOW1_record[] =
{
/* [WINDOW1] (3Dh) */
    XLS_REC_OPCODE(X_WINDOW1),
    XLS_REC_LENGTH(10),
    XLS_REC_U16(0x01E0), /* horizontal position, in twips, of the window */
    XLS_REC_U16(0x005A), /* vertical position, in twips, of the window */
    XLS_REC_U16(0x6C0C), /* width, in twips, of the window */
    XLS_REC_U16(0x2F76), /* height, in twips, of the window */
    XLS_REC_U16(0) /* window is visible */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_WINDOW1_record, sizeof32(biff3_worksheet_WINDOW1_record)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet_SheetViewSettingsBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff3_worksheet_SheetViewSettingsBlock[] =
{
/* [WINDOW2] (3Eh) */
    XLS_REC_OPCODE(X_WINDOW2_B3_B8),
    XLS_REC_LENGTH(0xA),
    XLS_REC_U16(0x00B6), /* option flags */
    /* [7] Show outline symbols; [5] Automatic grid line colour; [4] Show zero values; [2] Show worksheet headers; [1] Show grid lines */
    XLS_REC_U16(0), /* zero-based index of first visible row */
    XLS_REC_U16(0), /* zero-based index of first visible column */
    0x00, 0x00, 0x00, 0x00, /* grid line RGBx colour */

/* [SELECTION] (1Dh) */
    XLS_REC_OPCODE(X_SELECTION),
    XLS_REC_LENGTH(0x0F),
    3, /* pane identifier. 3 = TL (main) */
    XLS_REC_U16(0), /* zero-based index of row of the active cell */
    XLS_REC_U16(0), /* zero-based index of column of the active cell */
    XLS_REC_U16(0), /* zero-based index into the following cell range list to the entry that contains the active cell */
    /* followed by cell range address list containing all selected cell ranges */
    XLS_REC_U16(1), /* number of following cell range addresses */
    XLS_REC_U16(0), /* zero-based index of first row */
    XLS_REC_U16(0), /* zero-based index of last row */
    0, /* zero-based index of first column. NB byte, even for BIFF8 */
    0 /* zero-based index of last column */
};

    return(xls_write_bytes(p_ff_op_format, biff3_worksheet_SheetViewSettingsBlock, sizeof32(biff3_worksheet_SheetViewSettingsBlock)));
}

_Check_return_
static STATUS
xls_write_biff3_worksheet(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    status_return(xls_write_BOF_record(p_ff_op_format, TRUE));

    status_return(xls_write_biff3_worksheet_FileProtectionBlock(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_INDEX_record(p_ff_op_format));

    status_return(xls_write_CODEPAGE_record(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_CalculationSettingsBlock(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_stuff(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_FONT_records(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_PageSettingsBlock(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_BACKUP_record(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_FORMAT_records(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_WorksheetProtectionBlock(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_XF_records(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_STYLE_records(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_DEFCOLWIDTH_record(p_ff_op_format));

    status_return(xls_write_DIMENSIONS_record(p_ff_op_format));

    status_return(xls_write_ROW_records(p_docu, p_ff_op_format));

    status_return(xls_write_biff3_core(p_docu, p_ff_op_format));

    status_return(xls_write_biff3_worksheet_WINDOW1_record(p_ff_op_format));

    status_return(xls_write_biff3_worksheet_SheetViewSettingsBlock(p_ff_op_format));

    status_return(xls_write_EOF_record(p_ff_op_format));

    return(STATUS_OK);
}

/******************************************************************************
*
* BIFF5 - write out all the column information
*
******************************************************************************/

_Check_return_
static STATUS
xls_write_biff5_core(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    SCAN_BLOCK scan_block;
    OBJECT_DATA object_data;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            save_reflect_status((P_OF_OP_FORMAT) p_ff_op_format, cells_scan_percent(&scan_block));

            switch(object_data.object_id)
            {
            case OBJECT_ID_TEXT:
                status = xls_write_cell_text(p_ff_op_format, p_docu, &object_data);
                break;

            case OBJECT_ID_SS:
                status = xls_write_cell_ss(p_ff_op_format, p_docu, &object_data);
                break;

            default:
                break;
            }

            status_break(status);
        }
    }

    return(status);
}

/*
The records listed below must be included in order for
Microsoft Excel to recognize the file as a valid BIFF5 file.
Because BIFF5 files are OLE compound document files,
these records must be written using OLE library functions.
For information on how to output an OLE docfile,
please see the OLE 2 Programmer's Reference Volume One.

Required Records

BOF - Set the 6 byte offset to 0x0005 (workbook globals)
Window1
FONT - At least five of these records must be included
XF - At least 15 Style XF records and 1 Cell XF record must be included
STYLE
BOUNDSHEET - Include one BOUNDSHEET record per worksheet
EOF

BOF - Set the 6 byte offset to 0x0010 (worksheet)
INDEX
DIMENSIONS
WINDOW2
EOF
*/

/*
BIFF5 workbook globals substream
*/

#if 0
static const BYTE
biff5_workbook_globals_BOF[] =
{
/* 00000: [BOF] (9h) */
    XLS_REC_OPCODE(X_BOF_B5_B8),
    XLS_REC_LENGTH(8),
    0x00, 0x05, /* version */
    XLS_REC_U16(0x05), /* type (0x05 = Workbook Globals) */
    XLS_REC_U16(0x2CA3), /* build identifier (11427) */
    XLS_REC_U16(1997) /* build year */
};
#endif

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_InterfaceBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_InterfaceBlock[] =
{
/* 0000C: [INTERFACEHDR] (E1h) */
    XLS_REC_OPCODE(X_INTERFACEHDR),
    XLS_REC_LENGTH(0),

/* 00010: [MMS] (C1h) */
    XLS_REC_OPCODE(X_MMS),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0),

/* 00016: [TOOLBARHDR] (BFh) */
    XLS_REC_OPCODE(X_TOOLBARHDR),
    XLS_REC_LENGTH(0),

/* 0001A: [TOOLBAREND] (C0h) */
    XLS_REC_OPCODE(X_TOOLBAREND),
    XLS_REC_LENGTH(0),

/* 0001E: [INTERFACEEND] (E2h) */
    XLS_REC_OPCODE(X_INTERFACEEND),
    XLS_REC_LENGTH(0)
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_InterfaceBlock, sizeof32(biff5_workbook_globals_InterfaceBlock)));
}

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_FileProtectionBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_FileProtectionBlock[] =
{
/* 00022: [WRITEACCESS] (5Ch) */
    XLS_REC_OPCODE(X_WRITEACCESS_B3_B8),
    XLS_REC_LENGTH(0x70),
    0x0d, 0x53, 0x74, 0x75, 0x61, 0x72, 0x74, 0x20, 0x53, 0x77, 0x61, 0x6c, 0x65, 0x73, 0x20, 0x20,  /* (13)Stuart Swales then space padded */
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_FileProtectionBlock, sizeof32(biff5_workbook_globals_FileProtectionBlock)));
}

#if 0
static const BYTE
biff5_workbook_globals_CODEPAGE[] =
{
/* 00096: [CODEPAGE] (42h) */
    XLS_REC_OPCODE(X_CODEPAGE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1252) /* workbook code page is 1252 */
};
#endif

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_stuff(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_stuff[] =
{
/* 0009C: [FNGROUPCOUNT] (9Ch) */
    XLS_REC_OPCODE(X_FNGROUPCOUNT),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0x0E), /* there are 14 built-in function categories in the workbook */

    /* Workbook Protection Block - start */

/* 000A2: [WINDOWPROTECT] (19h) */
    XLS_REC_OPCODE(X_WINDOWPROTECT),
    XLS_REC_LENGTH(2),
    0x00, 0x00,

/* 000A8: [PROTECT] (12h) */
    XLS_REC_OPCODE(X_PROTECT),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* workbook is not protected */

/* 000AE: [PASSWORD] (13h) */
    XLS_REC_OPCODE(X_PASSWORD),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* workbook has no password */

    /* Workbook Protection Block - end */

/* 000B4: [WINDOW1] (3Dh) */
    XLS_REC_OPCODE(X_WINDOW1),
    XLS_REC_LENGTH(0x12),
    XLS_REC_U16(0x01E0), /* horizontal position, in twips, of the window */
    XLS_REC_U16(0x005A), /* vertical position, in twips, of the window */
    XLS_REC_U16(0x6C0C), /* width, in twips, of the window */
    XLS_REC_U16(0x2F76), /* height, in twips, of the window */
    0x38, /* horizontal scroll bar is displayed, vertical scroll bar is displayed, worksheet tabs are displayed */
    0, /* reserved */
    XLS_REC_U16(0), /* zero-based index of selected worksheet tab */
    XLS_REC_U16(0), /* zero-based index of first displayed worksheet tab */
    XLS_REC_U16(1), /* number of selected worksheet tabs */
    XLS_REC_U16(0x0258), /* ratio of the width of the worksheet tabs to the width of the horizontal scroll bar, multiplied by 1000 */

/* 000CA: [BACKUP] (40h) */
    XLS_REC_OPCODE(X_BACKUP),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = do not save a backup file when workbook is saved */

/* 000D0: [HIDEOBJ] (8Dh) */
    XLS_REC_OPCODE(X_HIDEOBJ_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = SHOWALL (HideObjEnum value)*/

/* 000D6: [1904] (22h) */
    XLS_REC_OPCODE(X_DATEMODE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = workbook uses the 1900 date system */

/* 000DC: [PRECISION] (Eh) */
    XLS_REC_OPCODE(X_PRECISION),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = precision as displayed mode is not selected */

/* 000E2: [REFRESHALL] (1B7h) */
    XLS_REC_OPCODE(X_REFRESHALL),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = do not force refresh of external data ranges, PivotTables and XML maps on workbook load */

/* 000E8: [BOOKBOOL] (DAh) */
    XLS_REC_OPCODE(X_BOOKBOOL_B5_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0) /* external link values are saved, user prompted for update */
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_stuff, sizeof32(biff5_workbook_globals_stuff)));
}

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_FONT_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_FONT_records[] =
{
  /* four FONT records (lookup by zero-based index) */

/* 000EE: [FONT][0] (31h) */
    XLS_REC_OPCODE(X_FONT_B2_B5_B8),
    XLS_REC_LENGTH(0x14),
    XLS_REC_U16(0xC8), /* height of the font in twips. SHOULD be greater than or equal to 20 and less than or equal to 8191 */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value. MUST be greater than or equal to 0x0008 and less than or equal to 0x003F or 0x0051 or 0x7FFF */
    XLS_REC_U16(400), /* font weight. SHOULD be 400 (normal) or 700 (bold). MUST be 0, or greater than or equal to 100 and less than or equal to 1000 */
    XLS_REC_U16(0), /* 2 = subscript, 1 = superscript, or 0 = normal script */
    0x00, /* underline style */
    0x00, /* font family */
    0x00, /* 0 = ANSI_CHARSET */
    0, /* unused3 */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c, /* (5)Arial */

/* 00106: [FONT][1] (31h) */
    XLS_REC_OPCODE(X_FONT_B2_B5_B8),
    XLS_REC_LENGTH(0x14),
    XLS_REC_U16(0xC8), /* height of the font in twips */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value */
    XLS_REC_U16(400), /* normal weight */
    XLS_REC_U16(0), /* normal posn */
    0x00, /* underline style */
    0x00, /* font family */
    0x00, /* 0 = ANSI_CHARSET */
    0, /* unused3 */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c, /* (5)Arial */

/* 0011E: [FONT][2] (31h) */
    XLS_REC_OPCODE(X_FONT_B2_B5_B8),
    XLS_REC_LENGTH(0x14),
    XLS_REC_U16(0xC8), /* height of the font in twips */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value */
    XLS_REC_U16(400), /* normal weight */
    XLS_REC_U16(0), /* normal posn */
    0x00, /* underline style */
    0x00, /* font family */
    0x00, /* 0 = ANSI_CHARSET */
    0, /* unused3 */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c, /* (5)Arial */

/* 00136: [FONT][3] (31h) */
    XLS_REC_OPCODE(X_FONT_B2_B5_B8),
    XLS_REC_LENGTH(0x14),
    XLS_REC_U16(0xC8), /* height of the font in twips */
    0x00, /* fItalic etc. */
    0, /* reserved */
    XLS_REC_U16(0x7FFF), /* IcvFont value */
    XLS_REC_U16(400), /* normal weight */
    XLS_REC_U16(0), /* normal posn */
    0x00, /* underline style */
    0x00, /* font family */
    0x00, /* 0 = ANSI_CHARSET */
    0, /* unused3 */
    0x05, 0x41, 0x72, 0x69, 0x61, 0x6c /* (5)Arial */
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_FONT_records, sizeof32(biff5_workbook_globals_FONT_records)));
}

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_FORMAT_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_FORMAT_records[] =
{
  /* eight FORMAT records (lookup by ID) */

/* 0014E: [FORMAT]{5} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x16),
    XLS_REC_U16(5), /* identifier of the format string - see [MS-XML] restrictions */
    0x13, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x3b, 0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x23,
    0x2c, 0x23, 0x23, 0x30,
    /* "£"#,##0;\-"£"#,##0 */

/* 00168: [FORMAT]{6} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x1B),
    XLS_REC_U16(6),
    0x18, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x3b, 0x5b, 0x52, 0x65, 0x64, 0x5d, 0x5c,
    0x2d, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30,
    /* "£"#,##0;[Red]\-"£"#,##0 */

/* 00187: [FORMAT]{7} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x1C),
    XLS_REC_U16(7),
    0x19, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x3b, 0x5c, 0x2d, 0x22,
    0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    /* "£"#,##0.00;\-"£"#,##0.00 */

/* 001A7: [FORMAT]{8} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x21),
    XLS_REC_U16(8),
    0x1e, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x3b, 0x5b, 0x52, 0x65,
    0x64, 0x5d, 0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    /* "£"#,##0.00;[Red]\-"£"#,##0.00 */

/* 001CC: [FORMAT]{0x2A} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x33),
    XLS_REC_U16(0x2A),
    0x30, 0x5f, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b,
    0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b, 0x5f,
    0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x22, 0x2d, 0x22, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f,
    0x2d,
    /* _-"£"* #,##0_-;\-"£"* #,##0_-;_-"£"* "-"_-;_-@_- */

/* 00203: [FORMAT]{0x29} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x2A),
    XLS_REC_U16(0x29),
    0x27, 0x5f, 0x2d, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b, 0x5c, 0x2d, 0x2a,
    0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x2a, 0x20, 0x22, 0x2d, 0x22,
    0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f, 0x2d,
    /* _-* #,##0_-;\-* #,##0_-;_-* "-"_-;_-@_- */

/* 00231: [FORMAT]{0x2C} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x3B),
    XLS_REC_U16(0x2C),
    0x38, 0x5f, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30,
    0x5f, 0x2d, 0x3b, 0x5c, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e,
    0x30, 0x30, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x22, 0xa3, 0x22, 0x2a, 0x20, 0x22, 0x2d, 0x22, 0x3f,
    0x3f, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f, 0x2d,
    /* _-"#"* #,##0.00_-;\-"#"* #,##0.00_-;_-"#"* "-"??_-;_-@_- */

/* 00270: [FORMAT]{0x2B} (1Eh) */
    XLS_REC_OPCODE(X_FORMAT_B4_B8),
    XLS_REC_LENGTH(0x32),
    XLS_REC_U16(0x2B),
    0x2f, 0x5f, 0x2d, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x5f, 0x2d, 0x3b,
    0x5c, 0x2d, 0x2a, 0x20, 0x23, 0x2c, 0x23, 0x23, 0x30, 0x2e, 0x30, 0x30, 0x5f, 0x2d, 0x3b, 0x5f,
    0x2d, 0x2a, 0x20, 0x22, 0x2d, 0x22, 0x3f, 0x3f, 0x5f, 0x2d, 0x3b, 0x5f, 0x2d, 0x40, 0x5f, 0x2d
    /* _-* #,##0.00_-;\-* #,##0.00_-;_-* "-"??_-;_-@_- */
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_FORMAT_records, sizeof32(biff5_workbook_globals_FORMAT_records)));
}

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_XF_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_XF_records[] =
{
  /* twenty-two XF records (of which, two cell XF) (lookup by zero-based index) */

/* 002A6: [XF][0] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF; locked */
    0x20, /* General (H) & Bottom (V) alignment */
    0x00, /* not rotated; XF_USED_ATTRIB (clear -> ALL style attributes valid) */
    0xc0, 0x20, 0x00, 0x00, /* FG=0x40; BG=0x01; B line style & colour zero */
    0x00, 0x00, 0x00, 0x00, /* T,L,R line style & colour(s) zero */

/* 002BA: [XF][1] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(1), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* Not rotated; XF_USED_ATTRIB (set -> ignore style attributes, so just FONT valid) */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 002CE: [XF][2] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(1), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 002E2: [XF][3] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(2), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 002F6: [XF][4] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(2), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 0030A: [XF][5] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 0031E: [XF][6] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 00332: [XF][7] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 00346: [XF][8] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 0035A: [XF][9] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 0036E: [XF][10] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 00382: [XF][11] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 00396: [XF][12] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 003AA: [XF][13] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 003BE: [XF][14] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf4, /* just FONT valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 003D2: [XF][15] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0), /* number format identifier */
    XLS_REC_U16(1), /* cell XF; parent style = 0 */
    0x20, /* General (H) & Bottom (V) alignment */
    0x00, /* nothing valid here - always use parent */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 003E6: [XF][16] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(1), /* FONT record index */
    XLS_REC_U16(0x2b), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf8, /* just number format valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 003FA: [XF][17] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(1), /* FONT record index */
    XLS_REC_U16(0x29), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf8, /* just number format valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 0040E: [XF][18] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(1), /* FONT record index */
    XLS_REC_U16(0x2C), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf8, /* just number format valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 00422: [XF][19] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(1), /* FONT record index */
    XLS_REC_U16(0x2A), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf8, /* just number format valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 00436: [XF][20] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(1), /* FONT record index */
    XLS_REC_U16(0x09), /* number format identifier */
    XLS_REC_U16(0xFFF5), /* style XF */
    0x20,
    0xf8, /* just number format valid */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

/* 0044A: [XF][21] (E0h) */
    XLS_REC_OPCODE(X_XF_B5_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* FONT record index */
    XLS_REC_U16(0x0E), /* number format identifier */
    XLS_REC_U16(1), /* cell XF; parent style = 0 */
    0x20, /* General (H) & Bottom (V) alignment */
    0x04, /* just FONT valid here - otherwise use parent */
    0xc0, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_XF_records, sizeof32(biff5_workbook_globals_XF_records)));
}

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_STYLE_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_STYLE_records[] =
{
  /* six STYLE records */

/* 0045E: [STYLE][0] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8010), /* [11:0] zero-based index of the cell style XF; [14:12] undefined; [15] fBuiltIn */
    /* two byte built-in cell style properties. MUST exist if and only if fBuiltIn is 1 */
    0x03, /* 'Comma' style */
    0xff, /* outline level. only valid for types 1 & 2 */

/* 00466: [STYLE][1] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8011),
    0x06, /* 'Comma [0]' style */
    0xff,

/* 0046E: [STYLE][2] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8012),
    0x04, /* 'Currency' style */
    0xff,

/* 00476: [STYLE][3] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8013),
    0x07, /* 'Currency [0]' style */
    0xff,

/* 0047E: [STYLE][4] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8000), /* references first XF record -> 'Normal' style */
    0x00, /* 'Normal' style */
    0xff,

/* 00486: [STYLE][5] (93h) */
    XLS_REC_OPCODE(X_STYLE_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x8014),
    0x05, /* 'Percent' style */
    0xff
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_STYLE_records, sizeof32(biff5_workbook_globals_STYLE_records)));
}

_Check_return_
static STATUS
xls_write_biff5_workbook_globals_PALETTE_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_workbook_globals_PALETTE_record[] =
{
/* 0048E: [PALETTE] (92h) */
    XLS_REC_OPCODE(X_PALETTE_B3_B8),
    XLS_REC_LENGTH(0xE2),
    0x38, /* number of colors following. MUST be 56 */
    0x00, 0x00, 0x00, /* BIFF5 is RGB. BIFF8 is LONGRGB */
    0x00, 0x00, 0xff,
    0xff, 0xff, 0x00,
    0xff, 0x00, 0x00,
    0x00, 0x00, 0xff,
    0x00, 0x00, 0x00,
    0x00, 0xff, 0x00,
    0xff, 0xff, 0x00,
    0x00, 0xff, 0x00,
    0xff, 0x00, 0x00,
    0xff, 0xff, 0x00,
    0x80, 0x00, 0x00,
    0x00, 0x00, 0x80,
    0x00, 0x00, 0x00,
    0x00, 0x80, 0x00,
    0x80, 0x80, 0x00,
    0x00, 0x80, 0x00,
    0x80, 0x00, 0x00,
    0x80, 0x80, 0x00,
    0xc0, 0xc0, 0xc0,
    0x00, 0x80, 0x80,
    0x80, 0x00, 0x99,
    0x99, 0xff, 0x00,
    0x99, 0x33, 0x66,
    0x00, 0xff, 0xff,
    0xcc, 0x00, 0xcc,
    0xff, 0xff, 0x00,
    0x66, 0x00, 0x66,
    0x00, 0xff, 0x80,
    0x80, 0x00, 0x00,
    0x66, 0xcc, 0x00,
    0xcc, 0xcc, 0xff,
    0x00, 0x00, 0x00,
    0x80, 0x00, 0xff,
    0x00, 0xff, 0x00,
    0xff, 0xff, 0x00,
    0x00, 0x00, 0xff,
    0xff, 0x00, 0x80,
    0x00, 0x80, 0x00,
    0x80, 0x00, 0x00,
    0x00, 0x00, 0x80,
    0x80, 0x00, 0x00,
    0x00, 0xff, 0x00,
    0x00, 0xcc, 0xff,
    0x00, 0xcc, 0xff,
    0xff, 0x00, 0xcc,
    0xff, 0xcc, 0x00,
    0xff, 0xff, 0x99,
    0x00, 0x99, 0xcc,
    0xff, 0x00, 0xff,
    0x99, 0xcc, 0x00,
    0xcc, 0x99, 0xff,
    0x00, 0xe3, 0xe3,
    0xe3, 0x00, 0x33,
    0x66, 0xff, 0x00,
    0x33, 0xcc, 0xcc,
    0x00, 0x99, 0xcc,
    0x00, 0x00, 0xff,
    0xcc, 0x00, 0x00,
    0xff, 0x99, 0x00,
    0x00, 0xff, 0x66,
    0x00, 0x00, 0x66,
    0x66, 0x99, 0x00,
    0x96, 0x96, 0x96,
    0x00, 0x00, 0x33,
    0x66, 0x00, 0x33,
    0x99, 0x66, 0x00,
    0x00, 0x33, 0x00,
    0x00, 0x33, 0x33,
    0x00, 0x00, 0x99,
    0x33, 0x00, 0x00,
    0x99, 0x33, 0x66,
    0x00, 0x33, 0x33,
    0x99, 0x00, 0x33,
    0x33, 0x33, 0x00
};

    return(xls_write_bytes(p_ff_op_format, biff5_workbook_globals_PALETTE_record, sizeof32(biff5_workbook_globals_PALETTE_record)));
}

#if 0
static const BYTE
biff5_workbook_globals_BUNDLESHEET[] =
{
/* 00574: [BUNDLESHEET] (85h) */
    XLS_REC_OPCODE(X_BOUNDSHEET_B5_B8),
    XLS_REC_LENGTH(0xD),
    XLS_REC_U16(0x0589), XLS_REC_U16(0x0000), /* absolute stream offset of corresponding worksheet BOF */
    0x00, 0x00,
    0x06, 0x53, 0x68, 0x65, 0x65, 0x74, 0x31  /* (6)Sheet1 */
};
#endif

#if 0
static const BYTE
biff5_workbook_globals_EOF[] =
{
/* 00585: [EOF] (Ah) */
    XLS_REC_OPCODE(X_EOF),
    XLS_REC_LENGTH(0)
};
#endif

_Check_return_
static STATUS
xls_write_biff5_workbook_globals(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    status_return(xls_write_biff5_workbook_globals_InterfaceBlock(p_ff_op_format));

    status_return(xls_write_biff5_workbook_globals_FileProtectionBlock(p_ff_op_format));

    status_return(xls_write_CODEPAGE_record(p_ff_op_format));

    status_return(xls_write_biff5_workbook_globals_stuff(p_ff_op_format));

    status_return(xls_write_biff5_workbook_globals_FONT_records(p_ff_op_format));

    status_return(xls_write_biff5_workbook_globals_FORMAT_records(p_ff_op_format));

    status_return(xls_write_biff5_workbook_globals_XF_records(p_ff_op_format));

    status_return(xls_write_biff5_workbook_globals_STYLE_records(p_ff_op_format));

    status_return(xls_write_biff5_workbook_globals_PALETTE_record(p_ff_op_format));

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_write_BOUNDSHEET_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_U8Z name)
{
    const XLS_OPCODE opcode = X_BOUNDSHEET_B5_B8;
    static BYTE x[] =   /* BIFF5-8 BOUNDSHEET record (start) */
    {
        0, 0, 0, 0,     /* absolute stream position of corresponding worksheet BOF */

        XLS_REC_U16(0)  /* option flags */
    };
    const U16 sizeof_x = sizeof32(x);
    const U32 name_length = strlen32(name);
    const U32 contents_length = (1 + ((biff_version >= 8) ? 1 : 0) + name_length);
    const U16 record_length = sizeof_x + (U16) contents_length;

    /* note the absolute position of the BOUNDSHEET record - we will need to patch it later */
    status_return(xls_write_get_current_offset(p_ff_op_format, &boundsheet_record_abs_posn[0]));

    status_return(xls_write_record_header(p_ff_op_format, opcode, record_length));

    status_return(xls_write_bytes(p_ff_op_format, x, sizeof_x));

    /* worksheet name follows:
     * byte string, 8-bit length in BIFF5/7
     * ShortXLUnicodeString in BIFF8
     */
    status_return(xls_write_U8(p_ff_op_format, (U8) name_length));
    
    if(biff_version >= 8)
        status_return(xls_write_U8(p_ff_op_format, 0)); /* flag byte */

    return(xls_write_bytes(p_ff_op_format, name, name_length));
}

_Check_return_
static STATUS
xls_write_workbook_globals_substream(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    assert(biff_version == 5);

    status_return(xls_write_BOF_record(p_ff_op_format, FALSE));

    status_return(xls_write_biff5_workbook_globals(p_ff_op_format));

    status_return(xls_write_BOUNDSHEET_record(p_ff_op_format, "Sheet1"));

    status_return(xls_write_EOF_record(p_ff_op_format));

    return(STATUS_OK);
}

/*
BIFF5 workbook worksheet substream
*/

#if 0
_Check_return_
static STATUS
xls_write_biff5_worksheet_BOF_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_BOF_record[] =
{
/* 00589: [BOF] (9h) */
    XLS_REC_OPCODE(X_BOF_B5_B8),
    XLS_REC_LENGTH(8),
    0x00, 0x06, /* version */
    XLS_REC_U16(0x10), /* type (0x10 = Worksheet) */
    XLS_REC_U16(0x2CA3), /* build identifier (11427) */
    XLS_REC_U16(1997) /* build year (1997) */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_BOF_record, sizeof32(biff5_worksheet_BOF_record)));
}
#endif

_Check_return_
static STATUS
xls_write_biff5_worksheet_INDEX_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_INDEX_record[] =
{
/* 00595: [INDEX] (Bh) */
    XLS_REC_OPCODE(X_INDEX_B3_B8),
    XLS_REC_LENGTH(0x10),
    XLS_REC_U16(0), /* not used */
    XLS_REC_U16(0), /* not used */
    XLS_REC_U16(0), /* zero-based index of first used row (rf) */
    XLS_REC_U16(0 /*4*/), /* zero-based index of first row of unused tail of worksheet (rl, last used row + 1) */
    XLS_REC_U16(0 /*0x0641*/), XLS_REC_U16(0x0000), /* absolute stream position of the DEFCOLWIDTH record of the current worksheet */
    /* array of nm (=rl-rf) absolute stream positions to the DBCELL record of each Row Block */
    XLS_REC_U16(0 /*0x0741*/), XLS_REC_U16(0x0000),
};

    /* note the absolute position of the INDEX record - we will need to patch it later */
    status_return(xls_write_get_current_offset(p_ff_op_format, &index_record_abs_posn));

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_INDEX_record, sizeof32(biff5_worksheet_INDEX_record)));
}

_Check_return_
static STATUS
xls_write_biff5_worksheet_CalculationSettingsBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_CalculationSettingsBlock[] =
{
/* 005A9: [CALCMODE] (Dh) */
    XLS_REC_OPCODE(X_CALCMODE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = automatic */

/* 005AF: [CALCCOUNT] (Ch) */
    XLS_REC_OPCODE(X_CALCCOUNT),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(100), /* iteration count for a calculation in iterative calculation mode */

/* 005B5: [REFMODE] (Fh) */
    XLS_REC_OPCODE(X_REFMODE),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = A1 mode */

/* 005BB: [ITERATION] (11h) */
    XLS_REC_OPCODE(X_ITERATION),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = iterations off */

/* 005C1: [DELTA] (10h) */
    XLS_REC_OPCODE(X_DELTA),
    XLS_REC_LENGTH(8),
    0xfc, 0xa9, 0xf1, 0xd2, 0x4d, 0x62, 0x50, 0x3f, /* XLS_F64 minimum value change required for iterative calculation to continue */

/* 005CD: [SAVERECALC] (5Fh) */
    XLS_REC_OPCODE(X_SAVERECALC_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1) /* 1 = recalculate before saving the document */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_CalculationSettingsBlock, sizeof32(biff5_worksheet_CalculationSettingsBlock)));
}

_Check_return_
static STATUS
xls_write_biff5_worksheet_stuff(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_stuff[] =
{
/* 005D3: [PRINTHEADERS] (2Ah) */
    XLS_REC_OPCODE(X_PRINTHEADERS),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = do not print row/column headers */

/* 005D9: [PRINTGRIDLINES] (2Bh) */
    XLS_REC_OPCODE(X_PRINTGRIDLINES),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = do not print worksheet grid lines */

/* 005DF: [GRIDSET] (82h) */
    XLS_REC_OPCODE(X_GRIDSET_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(1), /* 1 = print grid lines option changed at some point in the past */

/* 005E5: [GUTS] (80h) */
    XLS_REC_OPCODE(X_GUTS_B3_B8),
    XLS_REC_LENGTH(8),
    XLS_REC_U16(0), /* width of the area to display row outlines (left of the worksheet), in pixels */
    XLS_REC_U16(0), /* height of the area to display column outlines (above the worksheet), in pixels */
    XLS_REC_U16(0), /* number of visible row outline levels (used row levels + 1; or 0, if not used) */
    XLS_REC_U16(0), /* number of visible column outline levels (used column levels + 1; or 0, if not used) */

/* 005F1: [DEFAULTROWHEIGHT] (25h) */
    XLS_REC_OPCODE(X_DEFAULTROWHEIGHT_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(0x0000), /* option flags */
    XLS_REC_U16(255), /* default height for unused rows, in twips */

/* 005F9: [COUNTRY] (8Ch) */
    XLS_REC_OPCODE(X_COUNTRY_B3_B8),
    XLS_REC_LENGTH(4),
    XLS_REC_U16(1), /* Windows country identifier of the user interface language of Excel. 1 = USA */
    XLS_REC_U16(44), /* Windows country identifier of the system regional settings. 44 = United Kingdom */

/* 00601: [WSBOOL] (81h) */ /* aka SHEETPR */
    XLS_REC_OPCODE(X_WSBOOL_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0x04C1) /* [10] show row outline symbols; [7] outline buttons right of outline group; [6] outline buttons below outline group; [0] show automatic page breaks */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_stuff, sizeof32(biff5_worksheet_stuff)));
}

_Check_return_
static STATUS
xls_write_biff5_worksheet_PageSettingsBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_PageSettingsBlock[] =
{
/* 00607: [HEADER] (14h) */
    XLS_REC_OPCODE(X_HEADER),
    XLS_REC_LENGTH(0),
    /* worksheet does not contain a page header */

/* 0060B: [FOOTER] (15h) */
    XLS_REC_OPCODE(X_FOOTER),
    XLS_REC_LENGTH(0),
    /* worksheet does not contain a page footer */

/* 0060F: [HCENTER] (83h) */
    XLS_REC_OPCODE(X_HCENTER_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = print worksheet left aligned */

/* 00615: [VCENTER] (84h) */
    XLS_REC_OPCODE(X_VCENTER_B3_B8),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(0), /* 0 = print worksheet aligned at top page border */

/* 0061B: [PAGESETUP] (A1h) */
    XLS_REC_OPCODE(X_PAGESETUP_B4_B8),
    XLS_REC_LENGTH(0x22),
    XLS_REC_U16(0), /* paper size. 0 = custom printer paper size */
    XLS_REC_U16(0x2c), /* scaling factor for printing as a percentage */
    XLS_REC_U16(1), /* starting page number */
    XLS_REC_U16(1), /* number of pages the worksheet width is fit to. 0 = auto */
    XLS_REC_U16(1), /* number of pages the worksheet height is fit to. 0 = auto */
    XLS_REC_U16(0x44), /* [6] use default paper orientation (portrait); [3] paper size, scaling factor, paper orientation, resolution and copies are not initialised */
    XLS_REC_U16(0x2F), /* print resolution in dpi */
    XLS_REC_U16(0), /* vertical print resolution in dpi */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x3f, /* XLS_F64 header margin in inches */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x3f, /* XLS_F64 footer margin in inches */
    XLS_REC_U16(0x04E4) /* number of copies to print */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_PageSettingsBlock, sizeof32(biff5_worksheet_PageSettingsBlock)));
}

_Check_return_
static STATUS
xls_write_biff5_worksheet_DEFCOLWIDTH_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_DEFCOLWIDTH_record[] =
{
/* 00641: [DEFCOLWIDTH] (55h) */
    XLS_REC_OPCODE(X_DEFCOLWIDTH),
    XLS_REC_LENGTH(2),
    XLS_REC_U16(8), /* column width in characters, using the width of the zero character from default font (first FONT record in the file) */
};

    { /* patch the INDEX record that we output earlier */
    U32 defcolwidth_record_abs_posn;
    const U32 patch_abs_posn = index_record_abs_posn + (2 + 2 + 8);
    status_return(xls_write_get_current_offset(p_ff_op_format, &defcolwidth_record_abs_posn));
    status_return(xls_write_patch_offset_U32(p_ff_op_format, patch_abs_posn, defcolwidth_record_abs_posn));
    } /*block*/

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_DEFCOLWIDTH_record, sizeof32(biff5_worksheet_DEFCOLWIDTH_record)));
}

_Check_return_
static STATUS
xls_write_biff5_worksheet_COLINFO_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_COLINFO_record[] =
{
/* 00647: [COLINFO] (7Dh) */
    XLS_REC_OPCODE(X_COLINFO_B3_B8),
    XLS_REC_LENGTH(0x0C),
    XLS_REC_U16(0), /* zero-based index of the first column in the range */
    XLS_REC_U16(0), /* zero-based index of the last column in the range */
    XLS_REC_U16(0x0A24), /* width of the columns in 1/256 of the width of the zero character, using default font (first FONT record in the file) */
    XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record for default column formatting */
    XLS_REC_U16(0x0000), /* option flags */
    XLS_REC_U16(0x0044) /* not used */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_COLINFO_record, sizeof32(biff5_worksheet_COLINFO_record)));
}

#if 0
_Check_return_
static STATUS
xls_write_biff5_worksheet_DIMENSIONS_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_DIMENSIONS_record[] =
{
/* 00657: [DIMENSIONS] (0h) */
    XLS_REC_OPCODE(X_DIMENSIONS_B3_B8),
    XLS_REC_LENGTH(0x0A),
    XLS_REC_U16(0), /* zero-based index of the first used row */
    XLS_REC_U16(4), /* zero-based index of the row after the last row in the worksheet that contains a used cell */
    XLS_REC_U16(0), /* zero-based index of the first used column */
    XLS_REC_U16(2), /* zero-based index of the column after the last column in the worksheet that contains a used cell */
    XLS_REC_U16(0) /* reserved */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_DIMENSIONS_record, sizeof32(biff5_worksheet_DIMENSIONS_record)));
}
#endif

#if 0
_Check_return_
static STATUS
xls_write_biff5_worksheet_ROW_records(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_ROW_records[] =
{
/* 00665: [ROW] (8h) */
  XLS_REC_OPCODE(X_ROW_B3_B8),
  XLS_REC_LENGTH(0x10),
  XLS_REC_U16(0), /* zero-based index of this row */
  XLS_REC_U16(0), /* zero-based index of the column of the first cell which is described by a cell record */
  XLS_REC_U16(2), /* zero-based index of the column after the last column in this row that contains a used cell */
  XLS_REC_U16(255), /* row height in twips */
  XLS_REC_U16(0), /* reserved1 */
  XLS_REC_U16(0), /* unused1 */
  XLS_REC_U16(0x0100), /* option flags. [8] always set */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the default XF record if options[7] */

/* 00679: [ROW] (8h) */
  XLS_REC_OPCODE(X_ROW_B3_B8),
  XLS_REC_LENGTH(0x10),
  XLS_REC_U16(1), /* zero-based index of this row */
  XLS_REC_U16(0), /* zero-based index of the column of the first cell which is described by a cell record */
  XLS_REC_U16(2), /* zero-based index of the column after the last column in this row that contains a used cell */
  XLS_REC_U16(255), /* row height in twips */
  XLS_REC_U16(0), /* reserved1 */
  XLS_REC_U16(0), /* unused1 */
  XLS_REC_U16(0x0100), /* option flags. [8] always set */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the default XF record if options[7] */

/* 0068D: [ROW] (8h) */
  XLS_REC_OPCODE(X_ROW_B3_B8),
  XLS_REC_LENGTH(0x10),
  XLS_REC_U16(2), /* zero-based index of this row */
  XLS_REC_U16(0), /* zero-based index of the column of the first cell which is described by a cell record */
  XLS_REC_U16(2), /* zero-based index of the column after the last column in this row that contains a used cell */
  XLS_REC_U16(255), /* row height in twips */
  XLS_REC_U16(0), /* reserved1 */
  XLS_REC_U16(0), /* unused1 */
  XLS_REC_U16(0x0100), /* option flags. [8] always set */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the default XF record if options[7] */

/* 006A1: [ROW] (8h) */
  XLS_REC_OPCODE(X_ROW_B3_B8),
  XLS_REC_LENGTH(0x10),
  XLS_REC_U16(3), /* zero-based index of this row */
  XLS_REC_U16(0), /* zero-based index of the column of the first cell which is described by a cell record */
  XLS_REC_U16(2), /* zero-based index of the column after the last column in this row that contains a used cell */
  XLS_REC_U16(255), /* row height in twips */
  XLS_REC_U16(0), /* reserved1 */
  XLS_REC_U16(0), /* unused1 */
  XLS_REC_U16(0x0100), /* option flags. [8] always set */
  XLS_REC_U16(15 /*0x0F*/) /* zero-based index of the default XF record if options[7] */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_ROW_records, sizeof32(biff5_worksheet_ROW_records)));
}
#endif

#if 0
_Check_return_
static STATUS
xls_write_biff5_worksheet_data(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_data[] =
{
/* 006B5: [RK] (7Eh) */
  XLS_REC_OPCODE(X_RK_B3_B8),
  XLS_REC_LENGTH(0x0A),
  XLS_REC_U16(0), /* zero-based index of the row */
  XLS_REC_U16(0), /* zero-based index of the column */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record */
  0x00, 0x00, 0xf0, 0x3f, /* RK value */

/* 006C3: [LABEL] (4h) */
  XLS_REC_OPCODE(X_LABEL_B3_B8),
  XLS_REC_LENGTH(0x0F),
  XLS_REC_U16(0), /* zero-based index of the row */
  XLS_REC_U16(1), /* zero-based index of the column */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record */
  XLS_REC_U16(7),
  0x69, 0x6e, 0x74, 0x65, 0x67, 0x65, 0x72,
  /* (7)integer */

/* 006D6: [FORMULA] (6h) */
  XLS_REC_OPCODE(X_FORMULA_B2_B5_B8),
  XLS_REC_LENGTH(0x1A),
  XLS_REC_U16(1), /* zero-based index of the row */
  XLS_REC_U16(0), /* zero-based index of the column */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, /* fp result */
  XLS_REC_U16(0), /* option flags */
  0x00, 0x00, 0xe0, 0xfc, /* not used */
  XLS_REC_U16(4), /* RPN length */
  0x44, 0x00, 0xc0, 0x00, /* RPN data */

/* 006F4: [LABEL] (4h) */
  XLS_REC_OPCODE(X_LABEL_B3_B8),
  XLS_REC_LENGTH(0x0F),
  XLS_REC_U16(1), /* zero-based index of the row */
  XLS_REC_U16(1), /* zero-based index of the column */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record */
  XLS_REC_U16(7), /* byte string, 16-bit string length */
  0x66, 0x6f, 0x72, 0x6d, 0x75, 0x6c, 0x61,
  /* (7)formula */

/* 00707: [RK] (7Eh) */
  XLS_REC_OPCODE(X_RK_B3_B8),
  XLS_REC_LENGTH(0x0A),
  XLS_REC_U16(2), /* zero-based index of the row */
  XLS_REC_U16(0), /* zero-based index of the column */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record */
  0x01, 0x00, 0x5e, 0x40, /* RK value */

/* 00715: [LABEL] (4h) */
  XLS_REC_OPCODE(X_LABEL_B3_B8),
  XLS_REC_LENGTH(0x0A),
  XLS_REC_U16(2), /* zero-based index of the row */
  XLS_REC_U16(1), /* zero-based index of the column */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record */
  XLS_REC_U16(2), /* byte string, 16-bit string length */
  0x66, 0x70,
  /* (2)fp */

/* 00723: [RK] (7Eh) */
  XLS_REC_OPCODE(X_RK_B3_B8),
  XLS_REC_LENGTH(0x0A),
  XLS_REC_U16(3), /* zero-based index of the row */
  XLS_REC_U16(0), /* zero-based index of the column */
  XLS_REC_U16(21 /*0x15*/), /* zero-based index of the XF record */
  0xe0, 0x58, 0xe4, 0x40, /* RK value */

/* 00731: [LABEL] (4h) */
  XLS_REC_OPCODE(X_LABEL_B3_B8),
  XLS_REC_LENGTH(0x0C),
  XLS_REC_U16(3), /* zero-based index of the row */
  XLS_REC_U16(1), /* zero-based index of the column */
  XLS_REC_U16(15 /*0x0F*/), /* zero-based index of the XF record */
  XLS_REC_U16(4), /* byte string, 16-bit string length */
  0x64, 0x61, 0x74, 0x65
  /* (4)date */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_data, sizeof32(biff5_worksheet_data)));
}
#endif

#if 0
_Check_return_
static STATUS
xls_write_biff5_worksheet_DBCELL_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_DBCELL_record[] =
{
/* 00741: [DBCELL] (D7h) */
    XLS_REC_OPCODE(X_DBCELL_B5_B8),
    XLS_REC_LENGTH(0x0C),
    XLS_REC_U16(0x00DC), XLS_REC_U16(0x0000), /* (+ve) relative stream offset to first ROW record in the Row Block */
    /* array of relative stream offsets to calculate stream position of the first cell record for the respective row */
    XLS_REC_U16(0x003C),
    XLS_REC_U16(0x0021),
    XLS_REC_U16(0x0031),
    XLS_REC_U16(0x001C)
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_DBCELL_record, sizeof32(biff5_worksheet_DBCELL_record)));
}
#endif

_Check_return_
static STATUS
xls_write_biff5_worksheet_WINDOW1_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_WINDOW1_record[] =
{
/* 00751: [WINDOW1] (3Dh) */
    XLS_REC_OPCODE(X_WINDOW1),
    XLS_REC_LENGTH(0x12),
    XLS_REC_U16(0x01E0), /* horizontal position, in twips, of the window */
    XLS_REC_U16(0x005A), /* vertical position, in twips, of the window */
    XLS_REC_U16(0x6C0C), /* width, in twips, of the window */
    XLS_REC_U16(0x2F76), /* height, in twips, of the window */
    0x38, /* horizontal scroll bar is displayed, vertical scroll bar is displayed, worksheet tabs are displayed */
    0, /* reserved */
    XLS_REC_U16(0), /* zero-based index of selected worksheet tab */
    XLS_REC_U16(0), /* zero-based index of first displayed worksheet tab */
    XLS_REC_U16(1), /* number of selected worksheet tabs */
    XLS_REC_U16(0x0258) /* ratio of the width of the worksheet tabs to the width of the horizontal scroll bar, multiplied by 1000 */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_WINDOW1_record, sizeof32(biff5_worksheet_WINDOW1_record)));
}

_Check_return_
static STATUS
xls_write_biff5_worksheet_SheetViewSettingsBlock(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_SheetViewSettingsBlock[] =
{
/* 00767: [WINDOW2] (3Eh) */
    XLS_REC_OPCODE(X_WINDOW2_B3_B8),
    XLS_REC_LENGTH(0xA),
    XLS_REC_U16(0x06B6), /* option flags */
    /* [10] Sheet active; [9] Sheet selected; [7] Show outline symbols; [5] Automatic grid line colour; [4] Show zero values; [2] Show worksheet headers; [1] Show grid lines */
    XLS_REC_U16(0), /* zero-based index of first visible row */
    XLS_REC_U16(0), /* zero-based index of first visible column */
    0x00, 0x00, 0x00, 0x00, /* grid line RGBx colour */

/* 00775: [SELECTION] (1Dh) */
    XLS_REC_OPCODE(X_SELECTION),
    XLS_REC_LENGTH(0x0F),
    3, /* pane identifier. 3 = TL (main) */
    XLS_REC_U16(0), /* zero-based index of row of the active cell */
    XLS_REC_U16(0), /* zero-based index of column of the active cell */
    XLS_REC_U16(0), /* zero-based index into the following cell range list to the entry that contains the active cell */
    /* followed by cell range address list containing all selected cell ranges */
    XLS_REC_U16(1), /* number of following cell range addresses */
    XLS_REC_U16(0), /* zero-based index of first row */
    XLS_REC_U16(0), /* zero-based index of last row */
    0, /* zero-based index of first column. NB byte, even for BIFF8 */
    0 /* zero-based index of last column */
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_SheetViewSettingsBlock, sizeof32(biff5_worksheet_SheetViewSettingsBlock)));
}

_Check_return_
static STATUS
xls_write_biff5_worksheet_GCW_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_GCW_record[] =
{
/* 00788: [GCW] (ABh) */
    XLS_REC_OPCODE(X_GCW_B4_B7),
    XLS_REC_LENGTH(0x22),
    XLS_REC_LENGTH(0x20), /* size of the following bitfield in bytes */
    /* bitfield with one bit for every column in the worksheet:
    * if a bit is set, the corresponding column uses the width set in the STANDARDWIDTH record;
    * if a bit is cleared, the corresponding column uses the width set in the COLINFO record for this column;
    * fallback is DEFCOLWIDTH if no STANDARDWIDTH or corresponding COLINFO.
    */
    0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_GCW_record, sizeof32(biff5_worksheet_GCW_record)));
}

#if 0
_Check_return_
static STATUS
xls_write_biff5_worksheet_EOF_record(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
static const BYTE
biff5_worksheet_EOF_record[] =
{
/* 007AE: [EOF] (Ah) */
    XLS_REC_OPCODE(X_EOF),
    XLS_REC_LENGTH(0)
};

    return(xls_write_bytes(p_ff_op_format, biff5_worksheet_EOF_record, sizeof32(biff5_worksheet_EOF_record)));
}
#endif

_Check_return_
static STATUS
xls_write_workbook_sheet_substream(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    assert(biff_version == 5);

    { /* patch the corresponding BOUNDSHEET record that we output earlier */
    U32 worksheet_abs_posn;
    const U32 patch_abs_posn = boundsheet_record_abs_posn[0] + (2 + 2 + 0);
    status_return(xls_write_get_current_offset(p_ff_op_format, &worksheet_abs_posn));
    status_return(xls_write_patch_offset_U32(p_ff_op_format, patch_abs_posn, worksheet_abs_posn));
    } /*block*/

    status_return(xls_write_BOF_record(p_ff_op_format, TRUE));

    status_return(xls_write_biff5_worksheet_INDEX_record(p_ff_op_format));

    status_return(xls_write_biff5_worksheet_CalculationSettingsBlock(p_ff_op_format));

    status_return(xls_write_biff5_worksheet_stuff(p_ff_op_format));

    status_return(xls_write_biff5_worksheet_PageSettingsBlock(p_ff_op_format));

    status_return(xls_write_biff5_worksheet_DEFCOLWIDTH_record(p_ff_op_format));

    status_return(xls_write_biff5_worksheet_COLINFO_record(p_ff_op_format));

    status_return(xls_write_DIMENSIONS_record(p_ff_op_format));

    status_return(xls_write_ROW_records(p_docu, p_ff_op_format));

#if 0
    status_return(xls_write_biff5_worksheet_data(p_ff_op_format));

    /*status_return(xls_write_biff5_worksheet_DBCELL_record(p_ff_op_format));*/
#else
    status_return(xls_write_biff5_core(p_docu, p_ff_op_format));
#endif

    status_return(xls_write_biff5_worksheet_WINDOW1_record(p_ff_op_format));

    status_return(xls_write_biff5_worksheet_SheetViewSettingsBlock(p_ff_op_format));

    status_return(xls_write_biff5_worksheet_GCW_record(p_ff_op_format));

    status_return(xls_write_EOF_record(p_ff_op_format));

    return(STATUS_OK);
}

/******************************************************************************
*
* Excel file structure
*
******************************************************************************/

#include "cmodules/cfbf.h"

#if WINDOWS && CHECKING && 0

_Check_return_
static STATUS
stg_cfbf_write_empty_xls(
    _In_z_      PCTSTR stream_name)
{
    static const BYTE empty_xls[32] =
    {
        /* [BOF] (9h) */
        XLS_REC_OPCODE(X_BOF_B5_B8),
        XLS_REC_LENGTH(8),
        0x00, 0x05, /* version */
        XLS_REC_U16(0x05), /* type (0x05 = Workbook Globals) */
        XLS_REC_U16(0x2ca3), /* build identifier (11427) */
        XLS_REC_U16(1997), /* build year (1997) */

        /* [EOF] (Ah) */
        XLS_REC_OPCODE(X_EOF),
        XLS_REC_LENGTH(0)
    };
    static BYTE stream_data[4*1024 + 4];
    U32 i;

    for(i = 0; i < sizeof32(stream_data); ++i)
        stream_data[i] = (U8) (i + (i >> 8));

    memcpy32(stream_data, empty_xls, sizeof32(empty_xls));

    status_return(stg_cfbf_write_stream_in_storage(NULL, L"C:\\Temp\\empty4095.xls", stream_name, FILETYPE_DATA, stream_data, 4096-1));
    status_return(stg_cfbf_write_stream_in_storage(NULL, L"C:\\Temp\\empty4096.xls", stream_name, FILETYPE_DATA, stream_data, 4096));
    status_return(stg_cfbf_write_stream_in_storage(NULL, L"C:\\Temp\\empty4097.xls", stream_name, FILETYPE_DATA, stream_data, 4096+1));

    return(STATUS_OK);
}

#endif

_Check_return_
static STATUS
xls_save_biff_cfbf(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    PCTSTR storage_filename = NULL;
    P_ARRAY_HANDLE caller_p_array_handle = NULL;
    T5_FILETYPE storage_filetype = p_ff_op_format->of_op_format.output.u.file.t5_filetype;
    ARRAY_HANDLE array_handle = 0;
    STATUS status = STATUS_OK;

    assert(biff_version >= 5);

    /* always hack the output stream to output to (local) memory which we will copy out to CFBF on success */
    if(OP_OUTPUT_FILE == p_ff_op_format->of_op_format.output.state)
    {
        storage_filename = p_ff_op_format->of_op_format.output.u.file.filename;

        status = t5_file_close(&p_ff_op_format->of_op_format.output.u.file.file_handle);

        if(status_ok(status))
            p_ff_op_format->of_op_format.output.state = OP_OUTPUT_MEM;
    }
    else /* OP_OUTPUT_MEM */
    {
        caller_p_array_handle = p_ff_op_format->of_op_format.output.u.mem.p_array_handle;
    }

    /* allocate new local memory to output the workbook data to */
    if(status_ok(status))
    {
#define XLS_CFBF_SAVE_OUTPUT_BUFFER_INC 4096
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(XLS_CFBF_SAVE_OUTPUT_BUFFER_INC, sizeof32(U8), FALSE);

        p_ff_op_format->of_op_format.output.u.mem.p_array_handle = &array_handle;

        status = al_array_preallocate_zero(p_ff_op_format->of_op_format.output.u.mem.p_array_handle, &array_init_block);
    }

    if( status_ok(status) &&
        status_ok(status = xls_write_workbook_globals_substream(p_ff_op_format)) &&
        status_ok(status = xls_write_workbook_sheet_substream(p_docu, p_ff_op_format)) )
    { /*EMPTY*/ }

    /* now write the memory containing the workbook data to the desired file as a CFBF storage with 'Book' stream */
    if(status_ok(status))
    {
        const U32 n_stream_bytes = array_elements32(&array_handle);
        const PC_BYTE stream_data = array_rangec(&array_handle, BYTE, 0, n_stream_bytes);

        /* get that to return the CFBF storage contained in a memory object */
        /* pass that back to the caller as a substitute for what he had */
        /* but retaining the same allocation attributes (e.g. for writing to clipboard) */
        p_ff_op_format->of_op_format.output.u.mem.p_array_handle = caller_p_array_handle;
        if(NULL != caller_p_array_handle)
            al_array_empty(caller_p_array_handle);

        status = cfbf_write_stream_in_storage(caller_p_array_handle, storage_filename, storage_filetype, TEXT("Book"), stream_data, n_stream_bytes);

#if 0
        if(status_ok(status)
        { /* write a copy to a temp file for debug */
            FILE_HANDLE file_handle;
            if(status_ok(status = t5_file_open("C:\\Temp\\clipdump.xls", file_open_write, &file_handle, TRUE)))
            {
                const U32 n_storage_bytes = array_elements32(caller_p_array_handle);
                const PC_BYTE storage_data = array_rangec(caller_p_array_handle, BYTE, 0, n_storage_bytes);
                status = file_write_bytes(storage_data, n_storage_bytes, file_handle);
                status_accumulate(status, t5_file_close(&file_handle));
            }
        }
#endif
    }

    al_array_dispose(&array_handle);

    return(status);
}

_Check_return_
static STATUS
xls_save_biff_unwrapped(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status;

    /* won't support BIFF2 - too old and different */
    /* won't support BIFF4 - too wacky */
    
    if(biff_version == 5)
    {   /* Excel 2000 croaks when presented with an unwrapped BIFF5 even though we can import it as can ExcelFile viewer */
        if( status_ok(status = xls_write_workbook_globals_substream(p_ff_op_format)) &&
            status_ok(status = xls_write_workbook_sheet_substream(p_docu, p_ff_op_format)) )
        { /*EMPTY*/ }

        return(status);
    }

    assert(biff_version == 3);

    status = xls_write_biff3_worksheet(p_docu, p_ff_op_format);

    return(status);
}

_Check_return_
extern STATUS
xls_save_biff(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    biff_version = 5;

#if 0
    if(OP_OUTPUT_MEM == p_ff_op_format->of_op_format.output.state)
        biff_version = 3; /* try a lower version for clipboard */
#endif

    limits_from_docu_area(p_docu, &g_s_col, &g_e_col, &g_s_row, &g_e_row, &p_ff_op_format->of_op_format.save_docu_area);

    if((g_e_col /*- g_s_col*/) > XLS_MAXCOL_BIFF2)
        return(create_error(ERR_TOO_MANY_COLUMNS_FOR_EXPORT));

    if((g_e_row /*- g_s_row*/) > ((biff_version == 8) ? XLS_MAXROW_BIFF8 : XLS_MAXROW_BIFF2))
        return(create_error(ERR_TOO_MANY_ROWS_FOR_EXPORT));

    if(biff_version < 5)
        return(xls_save_biff_unwrapped(p_docu, p_ff_op_format));

#if WINDOWS && CHECKING && 0
    status_return(stg_cfbf_write_empty_xls(TEXT("Book")));
#endif

    return(xls_save_biff_cfbf(p_docu, p_ff_op_format));
}

/* end of fs_xls_saveb.c */
