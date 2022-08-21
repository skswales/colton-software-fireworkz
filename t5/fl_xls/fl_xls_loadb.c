/* fl_xls_loadb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Excel spreadsheet BIFF loader */

/* MRJC March 1994; SKS September 2006 */

#include "common/gflags.h"

#include "fl_xls/fl_xls.h"

#include "ob_skel/ff_io.h"

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#include "cmodules/cfbf.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/* comment out '| (TRACE_OUT | TRACE_ANY)' when not needed */
#define TRACE__XLS_LOADB    (0 | (TRACE_OUT | TRACE_ANY))

/*
internal types
*/

typedef U16 FONT_INDEX;
/*#define FONT_INDEX_INVALID 0xFFFFU*/

typedef U16 FORMAT_INDEX;
/*#define FORMAT_INDEX_INVALID 0xFFFFU*/

typedef U16 XF_INDEX;
#define XF_INDEX_INVALID 0xFFFFU

typedef struct XLS_LOAD_INFO
{
    P_DOCU p_docu; /* the document being inserted into */
    P_MSG_INSERT_FOREIGN p_msg_insert_foreign;

    PC_BYTE p_file_start;
    U32 file_end_offset;
    U32 worksheet_substream_offset;
    U32 shared_formula_opcode_offset;
    U16 shared_formula_record_length;
    U16 _spare_16;
    PROCESS_STATUS process_status;
    DOCU_AREA docu_area;
    DOCU_AREA docu_area_table;
    DOCU_AREA docu_area_interior;
    SLR current_slr;
    COL max_col; /* NB inclusive */
    SLR offset_slr;

    BOOL loi_as_table; /* loading, or inserting, as table */
    COL lhs_extra_cols; /* additional col / row margins for nice formatting as table */
    ROW top_extra_rows;
    ROW bot_extra_rows;

    /* worksheet */
    SBCHAR_CODEPAGE sbchar_codepage;
    BOOL datemode_1904;

    /* DIMENSIONS */
    XLS_COL s_col, e_col;
    XLS_ROW s_row, e_row;

    /* cache for speedier lookup */
    U32 offset_of_font_records;
    U32 offset_of_format_records;
    U32 offset_of_xf_records;

    /* globals */
    BOOL process_multiple_worksheets;
    U32 num_sst_strings;

    /* BIFF2 */
    XF_INDEX biff2_xf_index;
}
XLS_LOAD_INFO, * P_XLS_LOAD_INFO;

_Check_return_
static XF_INDEX inline
xls_obtain_xf_index_B2(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_BYTE p_cell_attributes)
{
    XF_INDEX xf_index = p_cell_attributes[0] & 0x3F;

    if(xf_index == 63)
    {
        xf_index = p_xls_load_info->biff2_xf_index;
        p_xls_load_info->biff2_xf_index = 0;
    }

    return(xf_index);
}

_Check_return_
static inline STATUS
xls_style_docu_area_add(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area,
    P_STYLE_DOCU_AREA_ADD_PARM p_style_docu_area_add_parm)
{
    return(style_docu_area_add(p_xls_load_info->p_docu, p_docu_area, p_style_docu_area_add_parm));
}

/*
xls_data_types
*/

#define XLS_DATA_TEXT   0
#define XLS_DATA_SS     1

static U32 biff_version = 0;
static XLS_OPCODE biff_version_opcode_XF = 0;

static BOOL g_data_allocated = FALSE;
static ARRAY_HANDLE g_h_data = 0;
static U32 g_file_end_offset = 0;
static ARRAY_HANDLE g_h_sst_index = 0; /* [] of ARRAY_HANDLE_USTR */

#define MAXSTACK 100

static P_P_USTR arg_stack; /* [MAXSTACK]*/

#define XLS_NO_RECORD_FOUND 0xFFFFFFFFU

_Check_return_
static STATUS
xls_style_from_xf_data(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_STYLE p_style,
    _In_reads_c_(20) P_BYTE p_xf_data /* BIFF8 format */);

/******************************************************************************
*
* lookup table of Excel function and command names
*
******************************************************************************/

/* token lookup table */

enum XLS_PTG_TYPES
{
    NOTUSED,
    UNARY,
    BINARY,
    FUNC,
    CONSTANT,
    OPERAND,
    OTHER
};

typedef struct PTG_ENTRY
{
    U8 type;        /* XLS_PTG_TYPES */
    U8 biff_bytes;  /* number of bytes in first BIFF version that this token is present in */
#if CHECKING
    U8 xls_token_number;
    PC_SBSTR p_ptg_name;
#endif
}
PTG_ENTRY; typedef const PTG_ENTRY * PC_PTG_ENTRY;

#if CHECKING
#define ptg_table_entry(xls_token_number, type, bytes) \
{ \
    type, bytes \
    ,  xls_token_number \
    , #xls_token_number \
}
#else
#define ptg_table_entry(xls_token_number, type, bytes) \
{ \
    type, bytes \
}
#endif

static const PTG_ENTRY
token_table[128] = /* MUST be in Excel token order, complete */
{
    /*00H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*01H*/ ptg_table_entry(tExp,           OTHER,      4),
    /*02H*/ ptg_table_entry(tTbl,           OTHER,      4),
    /*03H*/ ptg_table_entry(tAdd,           BINARY,     1),
    /*04H*/ ptg_table_entry(tSub,           BINARY,     1),
    /*05H*/ ptg_table_entry(tMul,           BINARY,     1),
    /*06H*/ ptg_table_entry(tDiv,           BINARY,     1),
    /*07H*/ ptg_table_entry(tPower,         BINARY,     1),
    /*08H*/ ptg_table_entry(tConcat,        BINARY,     1),
    /*09H*/ ptg_table_entry(tLT,            BINARY,     1),
    /*0AH*/ ptg_table_entry(tLE,            BINARY,     1),
    /*0BH*/ ptg_table_entry(tEQ,            BINARY,     1),
    /*0CH*/ ptg_table_entry(tGE,            BINARY,     1),
    /*0DH*/ ptg_table_entry(tGT,            BINARY,     1),
    /*0EH*/ ptg_table_entry(tNE,            BINARY,     1),
    /*0FH*/ ptg_table_entry(tIsect,         BINARY,     1),

    /*10H*/ ptg_table_entry(tList,          BINARY,     1),
    /*11H*/ ptg_table_entry(tRange,         BINARY,     1),
    /*12H*/ ptg_table_entry(tUplus,         UNARY,      1),
    /*13H*/ ptg_table_entry(tUminus,        UNARY,      1),
    /*14H*/ ptg_table_entry(tPercent,       UNARY,      1),
    /*15H*/ ptg_table_entry(tParen,         OTHER,      1),
    /*16H*/ ptg_table_entry(tMissArg,       CONSTANT,   1),
    /*17H*/ ptg_table_entry(tStr,           CONSTANT,   1), /* var */
    /*18H*/ ptg_table_entry(tNlr,           OTHER,      1), /* var */
    /*19H*/ ptg_table_entry(tAttr,          OTHER,      1), /* var */
    /*1AH*/ ptg_table_entry(tSheet,         OTHER,      8),
    /*1BH*/ ptg_table_entry(tEndSheet,      OTHER,      4),
    /*1CH*/ ptg_table_entry(tErr,           CONSTANT,   2),
    /*1DH*/ ptg_table_entry(tBool,          CONSTANT,   2),
    /*1EH*/ ptg_table_entry(tInt,           CONSTANT,   3),
    /*1FH*/ ptg_table_entry(tNum,           CONSTANT,   9),

    /*20H*/ ptg_table_entry(tArrayR,        CONSTANT,   7),
    /*21H*/ ptg_table_entry(tFuncR,         FUNC,       2),
    /*22H*/ ptg_table_entry(tFuncVarR,      FUNC,       3),
    /*23H*/ ptg_table_entry(tNameR,         OPERAND,    0),
    /*24H*/ ptg_table_entry(tRefR,          OPERAND,    4),
    /*25H*/ ptg_table_entry(tAreaR,         OPERAND,    7),
    /*26H*/ ptg_table_entry(tMemAreaR,      OPERAND,    5),
    /*27H*/ ptg_table_entry(tMemErrR,       OPERAND,    5),
    /*28H*/ ptg_table_entry(tMemNoMemR,     OPERAND,    5),
    /*29H*/ ptg_table_entry(tMemFuncR,      OPERAND,    2),
    /*2AH*/ ptg_table_entry(tRefErrR,       OPERAND,    4),
    /*2BH*/ ptg_table_entry(tAreaErrR,      OPERAND,    7),
    /*2CH*/ ptg_table_entry(tRefNR,         OPERAND,    4),
    /*2DH*/ ptg_table_entry(tAreaNR,        OPERAND,    7),
    /*2EH*/ ptg_table_entry(tMemAreaNR,     OPERAND,    2),
    /*2FH*/ ptg_table_entry(tMemNoMemNR,    OPERAND,    2),

    /*30H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*31H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*32H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*33H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*34H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*35H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*36H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*37H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*38H*/ ptg_table_entry(tFuncCER,       FUNC,       3),
    /*39H*/ ptg_table_entry(tNameXR,        OPERAND,    0),
    /*3AH*/ ptg_table_entry(tRef3dR,        OPERAND,    0),
    /*3BH*/ ptg_table_entry(tArea3dR,       OPERAND,    0),
    /*3CH*/ ptg_table_entry(tRefErr3dR,     OPERAND,    0),
    /*3DH*/ ptg_table_entry(tAreaErr3dR,    OPERAND,    0),
    /*3EH*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*3FH*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),

    /*40H*/ ptg_table_entry(tArrayV,        CONSTANT,   7),
    /*41H*/ ptg_table_entry(tFuncV,         FUNC,       2),
    /*42H*/ ptg_table_entry(tFuncVarV,      FUNC,       3),
    /*43H*/ ptg_table_entry(tNameV,         OPERAND,    0),
    /*44H*/ ptg_table_entry(tRefV,          OPERAND,    4),
    /*45H*/ ptg_table_entry(tAreaV,         OPERAND,    7),
    /*46H*/ ptg_table_entry(tMemAreaV,      OPERAND,    5),
    /*47H*/ ptg_table_entry(tMemErrV,       OPERAND,    5),
    /*48H*/ ptg_table_entry(tMemNoMemV,     OPERAND,    5),
    /*49H*/ ptg_table_entry(tMemFuncV,      OPERAND,    2),
    /*4AH*/ ptg_table_entry(tRefErrV,       OPERAND,    4),
    /*4BH*/ ptg_table_entry(tAreaErrV,      OPERAND,    7),
    /*4CH*/ ptg_table_entry(tRefNV,         OPERAND,    4),
    /*4DH*/ ptg_table_entry(tAreaNV,        OPERAND,    7),
    /*4EH*/ ptg_table_entry(tMemAreaNV,     OPERAND,    2),
    /*4FH*/ ptg_table_entry(tMemNoMemNV,    OPERAND,    2),

    /*50H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*51H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*52H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*53H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*54H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*55H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*56H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*57H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*58H*/ ptg_table_entry(tFuncCEV,       FUNC,       3),
    /*59H*/ ptg_table_entry(tNameXV,        OPERAND,    0),
    /*5AH*/ ptg_table_entry(tRef3dV,        OPERAND,    0),
    /*5BH*/ ptg_table_entry(tArea3dV,       OPERAND,    0),
    /*5CH*/ ptg_table_entry(tRefErr3dV,     OPERAND,    0),
    /*5DH*/ ptg_table_entry(tAreaErr3dV,    OPERAND,    0),
    /*5EH*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*5FH*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),

    /*60H*/ ptg_table_entry(tArrayA,        CONSTANT,   7),
    /*61H*/ ptg_table_entry(tFuncA,         FUNC,       2),
    /*62H*/ ptg_table_entry(tFuncVarA,      FUNC,       3),
    /*63H*/ ptg_table_entry(tNameA,         OPERAND,    0),
    /*64H*/ ptg_table_entry(tRefA,          OPERAND,    4),
    /*65H*/ ptg_table_entry(tAreaA,         OPERAND,    7),
    /*66H*/ ptg_table_entry(tMemAreaA,      OPERAND,    5),
    /*67H*/ ptg_table_entry(tMemErrA,       OPERAND,    5),
    /*68H*/ ptg_table_entry(tMemNoMemA,     OPERAND,    5),
    /*69H*/ ptg_table_entry(tMemFuncA,      OPERAND,    2),
    /*6AH*/ ptg_table_entry(tRefErrA,       OPERAND,    4),
    /*6BH*/ ptg_table_entry(tAreaErrA,      OPERAND,    7),
    /*6CH*/ ptg_table_entry(tRefNA,         OPERAND,    4),
    /*6DH*/ ptg_table_entry(tAreaNA,        OPERAND,    7),
    /*6EH*/ ptg_table_entry(tMemAreaNA,     OPERAND,    2),
    /*6FH*/ ptg_table_entry(tMemNoMemNA,    OPERAND,    2),

    /*70H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*71H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*72H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*73H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*74H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*75H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*76H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*77H*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*78H*/ ptg_table_entry(tFuncCEA,       FUNC,       3),
    /*79H*/ ptg_table_entry(tNameXA,        OPERAND,    0),
    /*7AH*/ ptg_table_entry(tRef3dA,        OPERAND,    0),
    /*7BH*/ ptg_table_entry(tArea3dA,       OPERAND,    0),
    /*7CH*/ ptg_table_entry(tRefErr3dA,     OPERAND,    0),
    /*7DH*/ ptg_table_entry(tAreaErr3dA,    OPERAND,    0),
    /*7EH*/ ptg_table_entry(tNotUsed,       NOTUSED,    0),
    /*7FH*/ ptg_table_entry(tNotUsed,       NOTUSED,    0)
};

typedef struct XLS_FUNC_ENTRY
{
    PC_A7STR p_text_t5;         /* Fireworkz' function name */

    U16 biff_function_number;   /* Excel BIFF function number */

    U8 n_args;                  /* Required for tFuncV but leave zero for tFuncVarV */
    U8 _padding[1];
}
XLS_FUNC_ENTRY; typedef const XLS_FUNC_ENTRY * PC_XLS_FUNC_ENTRY;

#define xls_func_entry(biff_function_number, n_args, p_text_xls) \
    { p_text_xls, biff_function_number, n_args, 0 }

#define BIFF_FN_ExternCall 255 /* needs special handling */

static const XLS_FUNC_ENTRY
BIFF2_functions[] =  /* ordered as Excel for completeness checking */
{
    xls_func_entry(BIFF_FN_Count,           /*Var*/ 0, "COUNT"),
    xls_func_entry(BIFF_FN_If,              /*Var*/ 0, "IF"),
    xls_func_entry(BIFF_FN_Isna,                    1, "ISNA"),
    xls_func_entry(BIFF_FN_Iserror,                 1, "ISERROR"),
    xls_func_entry(BIFF_FN_Sum,             /*Var*/ 0, "SUM"),
    xls_func_entry(BIFF_FN_Average,         /*Var*/ 0, "AVG"), /* XLS:AVERAGE */
    xls_func_entry(BIFF_FN_Min,             /*Var*/ 0, "MIN"),
    xls_func_entry(BIFF_FN_Max,             /*Var*/ 0, "MAX"),
    xls_func_entry(BIFF_FN_Row,             /*Var*/ 0, "ROW"),
    xls_func_entry(BIFF_FN_Column,          /*Var*/ 0, "COL"), /* XLS:COLUMN */
    xls_func_entry(BIFF_FN_Na,                      0, "NA"),
    xls_func_entry(BIFF_FN_Npv,             /*Var*/ 0, "NPV"),
    xls_func_entry(BIFF_FN_Stdev,           /*Var*/ 0, "STD"), /* XLS:STDEV */
    xls_func_entry(BIFF_FN_Dollar,          /*Var*/ 0, "DOLLAR"),
    xls_func_entry(BIFF_FN_Fixed,                   2, "FIXED"), /* BIFF2 - becomes Var(2,3) in BIFF4 */
    xls_func_entry(BIFF_FN_Sin,                     1, "SIN"),
    xls_func_entry(BIFF_FN_Cos,                     1, "COS"),
    xls_func_entry(BIFF_FN_Tan,                     1, "TAN"),
    xls_func_entry(BIFF_FN_Atan,                    1, "ATAN"),
    xls_func_entry(BIFF_FN_Pi,                      0, "PI"),
    xls_func_entry(BIFF_FN_Sqrt,                    1, "SQR"), /* XLS:SQRT */
    xls_func_entry(BIFF_FN_Exp,                     1, "EXP"),
    xls_func_entry(BIFF_FN_Ln,                      1, "LN"),
    xls_func_entry(BIFF_FN_Log10,                   1, "ODF.LOG10"), /* XLS:LOG10 */
    xls_func_entry(BIFF_FN_Abs,                     1, "ABS"),
    xls_func_entry(BIFF_FN_Int,                     1, "ODF.INT"), /*XLS:INT*/
    xls_func_entry(BIFF_FN_Sign,                    1, "SGN"), /* XLS:SIGN */
    xls_func_entry(BIFF_FN_Round,                   2, "ROUND"),
    xls_func_entry(BIFF_FN_Lookup,          /*Var*/ 0, "LOOKUP"),
    xls_func_entry(BIFF_FN_Index,           /*Var*/ 0, "ODF.INDEX"), /* XLS:INDEX */
    xls_func_entry(BIFF_FN_Rept,                    2, "REPT"),
    xls_func_entry(BIFF_FN_Mid,                     3, "MID"),
    xls_func_entry(BIFF_FN_Len,                     1, "LENGTH"), /* XLS:LEN */
    xls_func_entry(BIFF_FN_Value,                   1, "VALUE"),
    xls_func_entry(BIFF_FN_True,                    0, "TRUE"),
    xls_func_entry(BIFF_FN_False,                   0, "FALSE"),
    xls_func_entry(BIFF_FN_And,             /*Var*/ 0, "AND"),
    xls_func_entry(BIFF_FN_Or,              /*Var*/ 0, "OR"),
    xls_func_entry(BIFF_FN_Not,                     1, "NOT"),
    xls_func_entry(BIFF_FN_Mod,                     2, "ODF.MOD"), /* XLS:MOD */
    xls_func_entry(BIFF_FN_Dcount,                  3, ".DCOUNT.XLS"), /* XLS:DCOUNT */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Dsum,                    3, ".DSUM.XLS"), /* XLS:DSUM */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Daverage,                3, ".DAVG.XLS"), /* XLS:DAVERAGE */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Dmin,                    3, ".DMIN.XLS"), /* XLS:DMIN */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Dmax,                    3, ".DMAX.XLS"), /* XLS:DMAX */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Dstdev,                  3, ".DSTD.XLS"), /* XLS:DSTD */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Var,             /*Var*/ 0, "VAR"),
    xls_func_entry(BIFF_FN_Dvar,                    3, ".DVAR.XLS"), /* XLS:DVAR */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Text,                    2, "TEXT"),
    xls_func_entry(BIFF_FN_Linest,          /*Var*/ 0, "LINEST"),
    xls_func_entry(BIFF_FN_Trend,           /*Var*/ 0, "TREND"),
    xls_func_entry(BIFF_FN_Logest,          /*Var*/ 0, "LOGEST"),
    xls_func_entry(BIFF_FN_Growth,          /*Var*/ 0, "GROWTH"),
  /*xls_func_entry(BIFF_FN_Goto,                    0, ".Goto"),*/
  /*xls_func_entry(BIFF_FN_Halt,                    0, ".Halt"),*/
    xls_func_entry(BIFF_FN_Pv,              /*Var*/ 0, "PV"),
    xls_func_entry(BIFF_FN_Fv,              /*Var*/ 0, "ODF.FV"), /* XLS:FV */
    xls_func_entry(BIFF_FN_Nper,            /*Var*/ 0, "NPER"),
    xls_func_entry(BIFF_FN_Pmt,             /*Var*/ 0, "ODF.PMT"), /* XLS:PMT */
    xls_func_entry(BIFF_FN_Rate,            /*Var*/ 0, "RATE"),
    xls_func_entry(BIFF_FN_Mirr,                    3, "MIRR"),
    xls_func_entry(BIFF_FN_Irr,             /*Var*/ 0, "ODF.IRR"), /* XLS:IRR */
    xls_func_entry(BIFF_FN_Rand,                    0, "RAND"),
    xls_func_entry(BIFF_FN_Match,           /*Var*/ 0, "MATCH"),
    xls_func_entry(BIFF_FN_Date,                    3, "DATE"),
    xls_func_entry(BIFF_FN_Time,                    3, "TIME"),
    xls_func_entry(BIFF_FN_Day,                     1, "DAY"),
    xls_func_entry(BIFF_FN_Month,                   1, "MONTH"),
    xls_func_entry(BIFF_FN_Year,                    1, "YEAR"),
    xls_func_entry(BIFF_FN_Weekday,                 1, "WEEKDAY"), /* BIFF2 - becomes Var(1,2) in BIFF5 */
    xls_func_entry(BIFF_FN_Hour,                    1, "HOUR"),
    xls_func_entry(BIFF_FN_Minute,                  1, "MINUTE"),
    xls_func_entry(BIFF_FN_Second,                  1, "SECOND"),
    xls_func_entry(BIFF_FN_Now,                     0, "NOW"),
    xls_func_entry(BIFF_FN_Areas,                   1, ".AREAS"),
    xls_func_entry(BIFF_FN_Rows,                    1, "ROWS"),
    xls_func_entry(BIFF_FN_Columns,                 1, "COLS"), /* XLS:COLUMNS */
    xls_func_entry(BIFF_FN_Offset,          /*Var*/ 0, ".OFFSET"),
  /*xls_func_entry(BIFF_FN_Absref,                  0, ".Absref"),*/
  /*xls_func_entry(BIFF_FN_Relref,                  0, ".Relref"),*/
  /*xls_func_entry(BIFF_FN_Argument,                0, ".Argument"),*/
    xls_func_entry(BIFF_FN_Search,          /*Var*/ 0, "SEARCH"),
    xls_func_entry(BIFF_FN_Transpose,               1, "TRANSPOSE"),
  /*xls_func_entry(BIFF_FN_Error,                   0, ".Error"),*/
  /*xls_func_entry(BIFF_FN_Step,                    0, ".Step"),*/
    xls_func_entry(BIFF_FN_Type,                    1, "ODF.TYPE"), /* XLS:TYPE */
  /*xls_func_entry(BIFF_FN_Echo,                    0, ".Echo"),*/
  /*xls_func_entry(BIFF_FN_SetName,                 0, ".Set.Name"),*/
  /*xls_func_entry(BIFF_FN_Caller,                  0, ".Caller"),*/
  /*xls_func_entry(BIFF_FN_Deref,                   0, ".Deref"),*/
  /*xls_func_entry(BIFF_FN_Windows,                 0, ".Windows"),*/
  /*xls_func_entry(BIFF_FN_Series,                  0, ".Series"),*/
  /*xls_func_entry(BIFF_FN_Documents,               0, ".Documents"),*/
  /*xls_func_entry(BIFF_FN_ActiveCell,              0, ".Active.Cell"),*/
  /*xls_func_entry(BIFF_FN_Selection,               0, ".Selection"),*/
  /*xls_func_entry(BIFF_FN_Result,                  0, ".Result"),*/
    xls_func_entry(BIFF_FN_Atan2,                   2, "ATAN_2"), /* XLS:ATAN2 */
    xls_func_entry(BIFF_FN_Asin,                    1, "ASIN"),
    xls_func_entry(BIFF_FN_Acos,                    1, "ACOS"),
    xls_func_entry(BIFF_FN_Choose,          /*Var*/ 0, "CHOOSE"),
    xls_func_entry(BIFF_FN_Hlookup,                 3, "HLOOKUP"), /* BIFF2 - becomes Var(3,4) in BIFF5 */
    xls_func_entry(BIFF_FN_Vlookup,                 3, "VLOOKUP"), /* BIFF2 - becomes Var(3,4) in BIFF5 */
  /*xls_func_entry(BIFF_FN_Links,                   0, ".Links"),*/
  /*xls_func_entry(BIFF_FN_Input,                   0, ".Input"),*/
    xls_func_entry(BIFF_FN_Isref,                   1, "ISREF"),
  /*xls_func_entry(BIFF_FN_GetFormula,              0, ".Get.Formula"),*/
  /*xls_func_entry(BIFF_FN_GetName,                 0, ".Get.Name"),*/
  /*xls_func_entry(BIFF_FN_SetValue,                0, ".Set.Value"),*/
    xls_func_entry(BIFF_FN_Log,             /*Var*/ 0, "LOG"),
  /*xls_func_entry(BIFF_FN_Exec,                    0, ".Exec"),*/
    xls_func_entry(BIFF_FN_Char,                    1, "CHAR"),
    xls_func_entry(BIFF_FN_Lower,                   1, "LOWER"),
    xls_func_entry(BIFF_FN_Upper,                   1, "UPPER"),
    xls_func_entry(BIFF_FN_Proper,                  1, "PROPER"),
    xls_func_entry(BIFF_FN_Left,            /*Var*/ 0, "LEFT"),
    xls_func_entry(BIFF_FN_Right,           /*Var*/ 0, "RIGHT"),
    xls_func_entry(BIFF_FN_Exact,                   2, "EXACT"),
    xls_func_entry(BIFF_FN_Trim,                    1, "TRIM"),
    xls_func_entry(BIFF_FN_Replace,                 4, "REPLACE"),
    xls_func_entry(BIFF_FN_Substitute,      /*Var*/ 0, "SUBSTITUTE"),
    xls_func_entry(BIFF_FN_Code,                    1, "CODE"),
  /*xls_func_entry(BIFF_FN_Names,                   0, ".Names"),*/
  /*xls_func_entry(BIFF_FN_Directory,               0, ".Directory"),*/
    xls_func_entry(BIFF_FN_Find,            /*Var*/ 0, "FIND"),
    xls_func_entry(BIFF_FN_Cell,            /*Var*/ 0, ".CELL"),
    xls_func_entry(BIFF_FN_Iserr,                   1, "ISERR"),
    xls_func_entry(BIFF_FN_Istext,                  1, "ISTEXT"),
    xls_func_entry(BIFF_FN_Isnumber,                1, "ISNUMBER"),
    xls_func_entry(BIFF_FN_Isblank,                 1, "ISBLANK"),
    xls_func_entry(BIFF_FN_T,                       1, "T"),
    xls_func_entry(BIFF_FN_N,                       1, "N"),
  /*xls_func_entry(BIFF_FN_Fopen,                   0, ".Fopen"),*/
  /*xls_func_entry(BIFF_FN_Fclose,                  0, ".Fclose"),*/
  /*xls_func_entry(BIFF_FN_Fsize,                   0, ".Fsize"),*/
  /*xls_func_entry(BIFF_FN_Freadln,                 0, ".Freadln"),*/
  /*xls_func_entry(BIFF_FN_Fread,                   0, ".Fread"),*/
  /*xls_func_entry(BIFF_FN_Fwriteln,                0, ".Fwriteln"),*/
  /*xls_func_entry(BIFF_FN_Fwrite,                  0, ".Fwrite"),*/
  /*xls_func_entry(BIFF_FN_Fpos,                    0, ".Fpos"),*/
    xls_func_entry(BIFF_FN_Datevalue,               1, "DATEVALUE"),
    xls_func_entry(BIFF_FN_Timevalue,               1, "TIMEVALUE"),
    xls_func_entry(BIFF_FN_Sln,                     3, "SLN"),
    xls_func_entry(BIFF_FN_Syd,                     4, "SYD"),
    xls_func_entry(BIFF_FN_Ddb,             /*Var*/ 0, "DDB"),
  /*xls_func_entry(BIFF_FN_GetDef,                  0, ".Get.Def"),*/
  /*xls_func_entry(BIFF_FN_Reftext,                 0, ".Reftext"),*/
  /*xls_func_entry(BIFF_FN_Textref,                 0, ".Textref"),*/
  /*xls_func_entry(BIFF_FN_Indirect,                0, ".INDIRECT"),*/
  /*xls_func_entry(BIFF_FN_Register,                0, ".Register"),*/
  /*xls_func_entry(BIFF_FN_Call,                    0, ".Call"),*/
  /*xls_func_entry(BIFF_FN_AddBar,                  0, ".Add.Bar"),*/
  /*xls_func_entry(BIFF_FN_AddMenu,                 0, ".Add.Menu"),*/
  /*xls_func_entry(BIFF_FN_AddCommand,              0, ".Add.Command"),*/
  /*xls_func_entry(BIFF_FN_EnableCommand,           0, ".Enable.Command"),*/
  /*xls_func_entry(BIFF_FN_CheckCommand,            0, ".Check.Command"),*/
  /*xls_func_entry(BIFF_FN_RenameCommand,           0, ".Rename.Command"),*/
  /*xls_func_entry(BIFF_FN_ShowBar,                 0, ".Show.Bar"),*/
  /*xls_func_entry(BIFF_FN_DeleteMenu,              0, ".Delete.Menu"),*/
  /*xls_func_entry(BIFF_FN_DeleteCommand,           0, ".Delete.Command"),*/
  /*xls_func_entry(BIFF_FN_GetChartItem,            0, ".Get.Chart.Item"),*/
  /*xls_func_entry(BIFF_FN_DialogBox,               0, ".Dialog.Box"),*/
    xls_func_entry(BIFF_FN_Clean,                   1, "CLEAN"),
    xls_func_entry(BIFF_FN_Mdeterm,                 1, "M_DETERM"), /* XLS:MDETERM */
    xls_func_entry(BIFF_FN_Minverse,                1, "M_INVERSE"), /* XLS:MINVERSE */
    xls_func_entry(BIFF_FN_Mmult,                   2, "M_MULT"), /* XLS:MMULT */
  /*xls_func_entry(BIFF_FN_Files,                   0, ".Files"),*/
    xls_func_entry(BIFF_FN_Ipmt,            /*Var*/ 0, ".IPMT"),
    xls_func_entry(BIFF_FN_Ppmt,            /*Var*/ 0, ".PPMT"),
    xls_func_entry(BIFF_FN_Counta,          /*Var*/ 0, "COUNTA"),
  /*xls_func_entry(BIFF_FN_CancelKey,               0, ".Cancel.Key"),*/
  /*xls_func_entry(BIFF_FN_Initiate,                0, ".Initiate"),*/
  /*xls_func_entry(BIFF_FN_Request,                 0, ".Request"),*/
  /*xls_func_entry(BIFF_FN_Poke,                    0, ".Poke"),*/
  /*xls_func_entry(BIFF_FN_Execute,                 0, ".Execute"),*/
  /*xls_func_entry(BIFF_FN_Terminate,               0, ".Terminate"),*/
  /*xls_func_entry(BIFF_FN_Restart,                 0, ".Restart"),*/
  /*xls_func_entry(BIFF_FN_Help,                    0, ".Help"),*/
  /*xls_func_entry(BIFF_FN_GetBar,                  0, ".Get.Bar"),*/
    xls_func_entry(BIFF_FN_Product,         /*Var*/ 0, "PRODUCT"),
    xls_func_entry(BIFF_FN_Fact,                    1, "FACT"),
  /*xls_func_entry(BIFF_FN_GetCell,                 0, ".Get.Cell"),*/
  /*xls_func_entry(BIFF_FN_GetWorkspace,            0, ".Get.Workspace"),*/
  /*xls_func_entry(BIFF_FN_GetWindow,               0, ".Get.Window"),*/
  /*xls_func_entry(BIFF_FN_GetDocument,             0, ".Get.Document"),*/
    xls_func_entry(BIFF_FN_Dproduct,                3, ".DPRODUCT.XLS"), /* XLS:DPRODUCT */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Isnontext,               1, "ISNONTEXT"),
  /*xls_func_entry(BIFF_FN_GetNote,                 0, ".Get.Note"),*/
  /*xls_func_entry(BIFF_FN_Note,                    0, ".Note"),*/
    xls_func_entry(BIFF_FN_Stdevp,          /*Var*/ 0, "STDP"), /* XLS:STDEVP */
    xls_func_entry(BIFF_FN_Varp,            /*Var*/ 0, "VARP"),
    xls_func_entry(BIFF_FN_Dstdevp,                 3, ".DSTDEVP.XLS"), /* XLS:DSTDEVP */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Dvarp,                   3, ".DVARP.XLS"), /* XLS:DVARP */ /* Fireworkz is incompatible */
    xls_func_entry(BIFF_FN_Trunc,                   2, "TRUNC"), /* BIFF2 - becomes Var(1,2) in BIFF3 */
    xls_func_entry(BIFF_FN_Islogical,               1, "ISLOGICAL"),
    xls_func_entry(BIFF_FN_Dcounta,                 3, ".DCOUNTA.XLS") /* XLS:DCOUNTA */ /* Fireworkz is incompatible */
  /*xls_func_entry(BIFF_FN_DeleteBar,               0, ".Delete.Bar"),*/
  /*xls_func_entry(BIFF_FN_Unregister,              0, ".Unregister"),*/
};

static const XLS_FUNC_ENTRY
BIFF3_functions[] =  /* ordered as Excel for completeness checking */
{
    xls_func_entry(BIFF_FN_Linest,          /*Var*/ 0, "LINEST"),
    xls_func_entry(BIFF_FN_Trend,           /*Var*/ 0, "TREND"),
    xls_func_entry(BIFF_FN_Logest,          /*Var*/ 0, "LOGEST"),
    xls_func_entry(BIFF_FN_Growth,          /*Var*/ 0, "GROWTH"),
    xls_func_entry(BIFF_FN_Trunc,           /*Var*/ 0, "TRUNC"), /*Var(1,2) in BIFF3 */

    xls_func_entry(BIFF_FN_Usdollar,        /*Var*/ 0, ".YEN"), /* BIFF3 - becomes USDOLLAR in BIFF4 */
    xls_func_entry(BIFF_FN_Findb,           /*Var*/ 0, "FIND"), /* XLS:FINDB */ /* Fireworkz doesn't handle DBCS */
    xls_func_entry(BIFF_FN_Searchb,         /*Var*/ 0, "SEARCH"), /* XLS:SEARCHB */
    xls_func_entry(BIFF_FN_Replaceb,                4, "REPLACE"), /* XLS:REPLACEB */
    xls_func_entry(BIFF_FN_Leftb,           /*Var*/ 0, "LEFT"), /* XLS:LEFTB */
    xls_func_entry(BIFF_FN_Rightb,          /*Var*/ 0, "RIGHT"), /* XLS:RIGHTB */
    xls_func_entry(BIFF_FN_Midb,                    3, "MID"), /* XLS:MIDB */
    xls_func_entry(BIFF_FN_Lenb,                    1, "LEN"), /* XLS:LENB */
    xls_func_entry(BIFF_FN_Roundup,                 2, "ROUNDUP"),
    xls_func_entry(BIFF_FN_Rounddown,               2, "ROUNDDOWN"),
    xls_func_entry(BIFF_FN_Asc,                     1, ".ASC"),
    xls_func_entry(BIFF_FN_Dbcs,                    1, ".JIS"), /* BIFF3 - becomes DBCS in BIFF4 */
    xls_func_entry(BIFF_FN_Address,         /*Var*/ 0, "ADDRESS"),
    xls_func_entry(BIFF_FN_Days360,                 2, "DAYS_360"), /* XLS:DAYS360 */ /* BIFF3 - becomes Var(2,3) in BIFF5 */
    xls_func_entry(BIFF_FN_Today,                   0, "TODAY"),
    xls_func_entry(BIFF_FN_Vdb,             /*Var*/ 0, ".VDB"),
    xls_func_entry(BIFF_FN_Median,          /*Var*/ 0, "MEDIAN"),
    xls_func_entry(BIFF_FN_Sumproduct,      /*Var*/ 0, "SUMPRODUCT"),
    xls_func_entry(BIFF_FN_Sinh,                    1, "SINH"),
    xls_func_entry(BIFF_FN_Cosh,                    1, "COSH"),
    xls_func_entry(BIFF_FN_Tanh,                    1, "TANH"),
    xls_func_entry(BIFF_FN_Asinh,                   1, "ASINH"),
    xls_func_entry(BIFF_FN_Acosh,                   1, "ACOSH"),
    xls_func_entry(BIFF_FN_Atanh,                   1, "ATANH"),
    xls_func_entry(BIFF_FN_Dget,                    3, ".DGET"),
  /*xls_func_entry(BIFF_FN_CreateObject,            0, ".Create.Object"),*/
  /*xls_func_entry(BIFF_FN_Volatile,                0, ".Volatile"),*/
  /*xls_func_entry(BIFF_FN_LastError,               0, ".Last.Error"),*/
  /*xls_func_entry(BIFF_FN_CustomUndo,              0, ".Custom.Undo"),*/
  /*xls_func_entry(BIFF_FN_CustomRepeat,            0, ".Custom.Repeat"),*/
  /*xls_func_entry(BIFF_FN_FormulaConvert,          0, ".Formula.Convert"),*/
  /*xls_func_entry(BIFF_FN_GetLinkInfo,             0, ".Get.Link.Info"),*/
  /*xls_func_entry(BIFF_FN_TextBox,                 0, ".Text.Box"),*/
    xls_func_entry(BIFF_FN_Info,                    1, ".INFO")
};

static const XLS_FUNC_ENTRY
BIFF4_functions[] =  /* ordered as Excel for completeness checking */
{
    xls_func_entry(BIFF_FN_Fixed,           /*Var*/ 0, "FIXED"), /* Var(2,3) in BIFF4 */
    xls_func_entry(BIFF_FN_Usdollar,        /*Var*/ 0, ".USDOLLAR"), /* BIFF4 - was YEN in BIFF3 */
    xls_func_entry(BIFF_FN_Dbcs,                    1, ".DBCS"), /* BIFF4 - was JIS in BIFF3 */

    xls_func_entry(BIFF_FN_Rank,            /*Var*/ 0, "RANK.EQ"), /* XLS:RANK */

  /*xls_func_entry(BIFF_FN_Group,                   0, ".Group"),*/
  /*xls_func_entry(BIFF_FN_GetObject,               0, ".Get.Object"),*/
    xls_func_entry(BIFF_FN_Db,              /*Var*/ 0, "DB"),
  /*xls_func_entry(BIFF_FN_Pause,                   0, ".Pause"),*/
  /*xls_func_entry(BIFF_FN_Resume,                  0, ".Resume"),*/
    xls_func_entry(BIFF_FN_Frequency,               2, "FREQUENCY"),
  /*xls_func_entry(BIFF_FN_AddToolbar,              0, ".Add.Toolbar"),*/
  /*xls_func_entry(BIFF_FN_DeleteToolbar,           0, ".Delete.Toolbar"),*/
    xls_func_entry(BIFF_FN_ExternCall,      /*Var*/ 0, "EXTERN.CALL"), /* needs special handling */
  /*xls_func_entry(BIFF_FN_ResetToolbar,            0, ".Reset.Toolbar"),*/
  /*xls_func_entry(BIFF_FN_Evaluate,                0, ".Evaluate"),*/
  /*xls_func_entry(BIFF_FN_GetToolbar,              0, ".Get.Toolbar"),*/
  /*xls_func_entry(BIFF_FN_GetTool,                 0, ".Get.Tool"),*/
  /*xls_func_entry(BIFF_FN_SpellingCheck,           0, ".Spelling.Check"),*/
    xls_func_entry(BIFF_FN_ErrorType,               1, ".ERROR.TYPE"),
  /*xls_func_entry(BIFF_FN_AppTitle,                0, ".App.Title"),*/
  /*xls_func_entry(BIFF_FN_WindowTitle,             0, ".Window.Title"),*/
  /*xls_func_entry(BIFF_FN_SaveToolbar,             0, ".Save.Toolbar"),*/
  /*xls_func_entry(BIFF_FN_EnableTool,              0, ".Enable.Tool"),*/
  /*xls_func_entry(BIFF_FN_PressTool,               0, ".Press.Tool"),*/
  /*xls_func_entry(BIFF_FN_RegisterId,              0, ".Register.Id"),*/
  /*xls_func_entry(BIFF_FN_GetWorkbook,             0, ".Get.Workbook"),*/
    xls_func_entry(BIFF_FN_Avedev,          /*Var*/ 0, "AVEDEV"),
    xls_func_entry(BIFF_FN_Betadist,        /*Var*/ 0, "ODF.BETADIST"), /* XLS:BETADIST */
    xls_func_entry(BIFF_FN_Gammaln,                 1, "GAMMALN"),
    xls_func_entry(BIFF_FN_Betainv,         /*Var*/ 0, "BETA.INV"), /* XLS:BETAINV */
    xls_func_entry(BIFF_FN_Binomdist,               4, "BINOM.DIST"), /* XLS:BINOMDIST */
    xls_func_entry(BIFF_FN_Chidist,                 2, "CHISQ.DIST.RT"), /* XLS:CHIDIST */
    xls_func_entry(BIFF_FN_Chiinv,                  2, "CHISQ.INV.RT"), /* XLS:CHIINV */
    xls_func_entry(BIFF_FN_Combin,                  2, "COMBIN"),
    xls_func_entry(BIFF_FN_Confidence,              3, "CONFIDENCE"),
    xls_func_entry(BIFF_FN_Critbinom,               3, "BINOM.INV"), /* XLS:CRITBINOM */
    xls_func_entry(BIFF_FN_Even,                    1, "EVEN"),
    xls_func_entry(BIFF_FN_Expondist,               3, "EXPON.DIST"), /* XLS:EXPONDIST */
    xls_func_entry(BIFF_FN_Fdist,                   3, "F.DIST.RT"), /* XLS:FDIST */
    xls_func_entry(BIFF_FN_Finv,                    3, "F.INV.RT"), /* XLS:FINV */
    xls_func_entry(BIFF_FN_Fisher,                  1, "FISHER"),
    xls_func_entry(BIFF_FN_Fisherinv,               1, "FISHERINV"),
    xls_func_entry(BIFF_FN_Floor,                   2, "FLOOR"),
    xls_func_entry(BIFF_FN_Gammadist,               4, "GAMMA.DIST"), /* XLS:GAMMADIST */
    xls_func_entry(BIFF_FN_Gammainv,                3, "GAMMA.INV"), /* XLS:GAMMAINV */
    xls_func_entry(BIFF_FN_Ceiling,                 2, "CEILING"),
    xls_func_entry(BIFF_FN_Hypgeomdist,             4, "HYPGEOM.DIST"), /* XLS:HYPGEOMDIST */
    xls_func_entry(BIFF_FN_Lognormdist,             3, "LOGNORM.DIST"), /* XLS:LOGNORMDIST */
    xls_func_entry(BIFF_FN_Loginv,                  3, "LOGNORM.INV"), /* XLS:LOGINV */
    xls_func_entry(BIFF_FN_Negbinomdist,            3, "NEGBINOM.DIST"), /* XLS:NEGBINOMDIST */
    xls_func_entry(BIFF_FN_Normdist,                4, "NORM.DIST"), /* XLS:NORMDIST */
    xls_func_entry(BIFF_FN_Normsdist,               1, "NORM.S.DIST"), /* XLS:NORMSDIST */
    xls_func_entry(BIFF_FN_Norminv,                 3, "NORM.INV"), /* XLS:NORMINV */
    xls_func_entry(BIFF_FN_Normsinv,                1, "NORM.S.INV"), /* XLS:NORMSINV */
    xls_func_entry(BIFF_FN_Standardize,             3, "STANDARDIZE"),
    xls_func_entry(BIFF_FN_Odd,                     1, "ODD"),
    xls_func_entry(BIFF_FN_Permut,                  2, "PERMUT"),
    xls_func_entry(BIFF_FN_Poisson,                 3, "POISSON.DIST"), /* XLS:POISSON */
    xls_func_entry(BIFF_FN_Tdist,                   3, "ODF.TDIST"), /* XLS:TDIST */
    xls_func_entry(BIFF_FN_Weibull,                 4, "WEIBULL.DIST"), /*XLS:WEIBULL */
    xls_func_entry(BIFF_FN_Sumxmy2,                 2, "SUM_XMY2"), /* XLS:SUMXMY2*/
    xls_func_entry(BIFF_FN_Sumx2my2,                2, "SUM_X2MY2"), /* XLS:SUMX2MY2 */
    xls_func_entry(BIFF_FN_Sumx2py2,                2, "SUM_X2PY2"), /* XLS:SUMX2PY2 */
    xls_func_entry(BIFF_FN_Chitest,                 2, "CHISQ.TEST"), /* XLS:CHITEST */
    xls_func_entry(BIFF_FN_Correl,                  2, "CORREL"),
    xls_func_entry(BIFF_FN_Covar,                   2, "COVARIANCE.P"), /* XLS:COVAR */
    xls_func_entry(BIFF_FN_Forecast,                3, "FORECAST"),
    xls_func_entry(BIFF_FN_Ftest,                   2, "F.TEST"), /* XLS:FTEST */
    xls_func_entry(BIFF_FN_Intercept,               2, "INTERCEPT"),
    xls_func_entry(BIFF_FN_Pearson,                 2, "PEARSON"),
    xls_func_entry(BIFF_FN_Rsq,                     2, "RSQ"),
    xls_func_entry(BIFF_FN_Steyx,                   2, "STEYX"),
    xls_func_entry(BIFF_FN_Slope,                   2, "SLOPE"),
    xls_func_entry(BIFF_FN_Ttest,                   4, "T.TEST"), /* XLS:TTEST */
    xls_func_entry(BIFF_FN_Prob,            /*Var*/ 0, "PROB"),
    xls_func_entry(BIFF_FN_Devsq,           /*Var*/ 0, "DEVSQ"),
    xls_func_entry(BIFF_FN_Geomean,         /*Var*/ 0, "GEOMEAN"),
    xls_func_entry(BIFF_FN_Harmean,         /*Var*/ 0, "HARMEAN"),
    xls_func_entry(BIFF_FN_Sumsq,           /*Var*/ 0, "SUMSQ"),
    xls_func_entry(BIFF_FN_Kurt,            /*Var*/ 0, "KURT"),
    xls_func_entry(BIFF_FN_Skew,            /*Var*/ 0, "SKEW"),
    xls_func_entry(BIFF_FN_Ztest,           /*Var*/ 0, "Z.TEST"), /* XLS:ZTEST */
    xls_func_entry(BIFF_FN_Large,                   2, "LARGE"),
    xls_func_entry(BIFF_FN_Small,                   2, "SMALL"),
    xls_func_entry(BIFF_FN_Quartile,                2, "QUARTILE.INC"), /* XLS:QUARTILE */
    xls_func_entry(BIFF_FN_Percentile,              2, "PERCENTILE.INC"), /* XLS:PERCENTILE */
    xls_func_entry(BIFF_FN_Percentrank,     /*Var*/ 0, "PERCENTRANK.INC"), /* XLS:PERCENTRANK */
    xls_func_entry(BIFF_FN_Mode,            /*Var*/ 0, "MODE"),
    xls_func_entry(BIFF_FN_Trimmean,                2, "TRIMMEAN"),
    xls_func_entry(BIFF_FN_Tinv,                    2, "T.INV.2T") /* XLS:TINV */
};

static const XLS_FUNC_ENTRY
BIFF5_functions[] =  /* ordered as Excel for completeness checking */
{
    xls_func_entry(BIFF_FN_Weekday,         /*Var*/ 0, "WEEKDAY"), /* Var(1,2) in BIFF5 */
    xls_func_entry(BIFF_FN_Hlookup,         /*Var*/ 0, "HLOOKUP"), /* Var(3,4) in BIFF5 */
    xls_func_entry(BIFF_FN_Vlookup,         /*Var*/ 0, "VLOOKUP"), /* Var(3,4) in BIFF5 */
    xls_func_entry(BIFF_FN_Days360,         /*Var*/ 0, "DAYS_360"), /* XLS:DAYS360 */ /* Var(2,3) in BIFF5 */

    xls_func_entry(BIFF_FN_Concatenate,     /*Var*/ 0, "JOIN"), /* XLS:CONCATENATE */
    xls_func_entry(BIFF_FN_Power,                   2, "POWER"),
    xls_func_entry(BIFF_FN_Radians,                 1, "RAD"), /* XLS:RADIANS */
    xls_func_entry(BIFF_FN_Degrees,                 1, "DEG"), /* XLS:DEGREES */
    xls_func_entry(BIFF_FN_Subtotal,        /*Var*/ 0, ".SUBTOTAL"),
    xls_func_entry(BIFF_FN_Sumif,           /*Var*/ 0, ".SUMIF"),
    xls_func_entry(BIFF_FN_Countif,                 2, ".COUNTIF"),
    xls_func_entry(BIFF_FN_Countblank,              1, "COUNTBLANK"),
    xls_func_entry(BIFF_FN_Ispmt,                   1, ".ISPMT"),
    xls_func_entry(BIFF_FN_Datedif,                 3, ".DATEDIF"),
    xls_func_entry(BIFF_FN_Datestring,              1, ".DATESTRING"),
    xls_func_entry(BIFF_FN_Numberstring,            2, ".NUMBERSTRING"),
    xls_func_entry(BIFF_FN_Roman,           /*Var*/ 0, ".ROMAN")
};

static const XLS_FUNC_ENTRY
BIFF8_functions[] =  /* ordered as Excel for completeness checking */
{
    xls_func_entry(BIFF_FN_GetPivotData,    /*Var*/ 0, ".GETPIVOTDATA"),
    xls_func_entry(BIFF_FN_Hyperlink,       /*Var*/ 0, ".HYPERLINK"),
    xls_func_entry(BIFF_FN_Phonetic,                1, ".PHONETIC"),
    xls_func_entry(BIFF_FN_AverageA,        /*Var*/ 0, "AVERAGEA"),
    xls_func_entry(BIFF_FN_MaxA,            /*Var*/ 0, "MAXA"),
    xls_func_entry(BIFF_FN_MinA,            /*Var*/ 0, "MINA"),
    xls_func_entry(BIFF_FN_StDevPA,         /*Var*/ 0, "STDEVPA"),
    xls_func_entry(BIFF_FN_VarPA,           /*Var*/ 0, "VARPA"),
    xls_func_entry(BIFF_FN_StDevA,          /*Var*/ 0, "STDEVA"),
    xls_func_entry(BIFF_FN_VarA,            /*Var*/ 0, "VARA")
};

_Check_return_
static STATUS
try_grokking_compound_file_for_xls(
    _InoutRef_ P_COMPOUND_FILE p_compound_file,
    _OutRef_  P_ARRAY_HANDLE p_h_data)
{
    STATUS status = STATUS_OK;
    U32 directory_index;
    COMPOUND_FILE_DECODED_DIRECTORY * decoded_directory;
    U32 size = 0;
    int sector_id;
    P_BYTE p_byte;

    *p_h_data = 0;

    for(directory_index = 0, decoded_directory = p_compound_file->decoded_directory_list;
        directory_index < p_compound_file->decoded_directory_count;
        directory_index++, decoded_directory++)
    {
        /* Do wide string match (can't use L"" as wchar_t is int on Norcroft) */
        static const WCHAR workbook_name[] = { 'W', 'o', 'r', 'k', 'b', 'o', 'o', 'k', CH_NULL }; /*L"Workbook"*/
        static const WCHAR book_name[]     = { 'B', 'o', 'o', 'k', CH_NULL }; /*L"Book"*/

        if(0 == C_wcsicmp(decoded_directory->name.wchar, workbook_name))
        {
            size = decoded_directory->_ulSize;
            trace_1(TRACE__XLS_LOADB, TEXT("*** Excel 'Workbook' stream located, size=") U32_XTFMT, size);
            break;
        }

        if(0 == C_wcsicmp(decoded_directory->name.wchar, book_name))
        {
            size = decoded_directory->_ulSize;
            trace_1(TRACE__XLS_LOADB, TEXT("*** Excel 'Book' stream located, size=") U32_XTFMT, size);
            break;
        }
    }

    if(0 == size)
    {
        status = STATUS_OK; /* no Excel stream found in this compound file */
    }
    else
    {
        U32 rounded_up_size;
        U32 total_bytes_read = 0;

        if(size < p_compound_file->hdr._ulMiniSectorCutoff)
            rounded_up_size = round_up(size, p_compound_file->ministream_sector_size);
        else
            rounded_up_size = round_up(size, p_compound_file->standard_sector_size);

        trace_2(TRACE_MODULE_CFBF, TEXT("*** Excel allocating rounded_up_size=") U32_XTFMT TEXT(" for size=") U32_XTFMT, rounded_up_size, size);

        if(NULL != (p_byte = al_array_extend_by_BYTE(p_h_data, rounded_up_size, &array_init_block_u8, &status)))
        {
            if(size < p_compound_file->hdr._ulMiniSectorCutoff)
            {   /* short sectors in the Ministream */
                trace_1(TRACE_MODULE_CFBF, TEXT("*** Excel reading Ministream chain starting at SSAT_sector_id=%d"), (int) decoded_directory->_sectStart);

                for(sector_id = decoded_directory->_sectStart;
                    sector_id >= 0;
                    sector_id = compound_file_get_next_SSAT_sector_id(p_compound_file, sector_id))
                {
                    status_break(status = compound_file_read_SSAT_sector(p_compound_file, sector_id, p_byte));

                    p_byte += p_compound_file->ministream_sector_size;

                    total_bytes_read += p_compound_file->ministream_sector_size;
                }
            }
            else
            {   /* standard sectors */
                trace_1(TRACE_MODULE_CFBF, TEXT("*** Excel reading chain starting at sector_id=%d"), (int) decoded_directory->_sectStart);

                for(sector_id = decoded_directory->_sectStart;
                    sector_id >= 0;
                    sector_id = compound_file_get_next_sector_id(p_compound_file, sector_id))
                {
                    U32 bytes_read;
                    
                    status_break(status = compound_file_read_file_sector(p_compound_file, sector_id, p_byte, &bytes_read));

                    p_byte += bytes_read;

                    total_bytes_read += bytes_read;
                }
            }

            if(status_ok(status))
            {   /* limit end of file to the lower of that requested or that read (otherwise get crud off the end) */
                trace_2(TRACE_MODULE_CFBF, TEXT("*** Excel read bytes=") U32_XTFMT TEXT(" for size=") U32_XTFMT, total_bytes_read, size);
                if(total_bytes_read < size)
                {   /* hopefully this is because we read data from a clipboard handle with a partial end sector */
                    status = (STATUS) total_bytes_read;
                }
                else
                {   /* otherwise we often read more bytes as they are read from whole sectors */
                    status = (STATUS) size;
                }
            }
        }
    }

    return(status);
}

_Check_return_
static STATUS
try_grokking_potential_cf_file_for_xls(
    _In_z_      PCTSTR filename_in,
    _OutRef_    P_ARRAY_HANDLE p_h_data)
{
    STATUS status = STATUS_OK;
    PCTSTR filename = filename_in;
    FILE_HANDLE file_handle;

    *p_h_data = 0;

    if(status_ok(status = t5_file_open(filename, file_open_read, &file_handle, TRUE)))
    {
        U32 bytesread;
        BYTE buffer[CFBF_FILE_HEADER_ID_BYTES];

        if(status_ok(status = file_read_bytes(buffer, sizeof32(buffer), &bytesread, file_handle)))
        {
            if(compound_file_file_header_id_test(buffer, bytesread))
            {   /* file exists and is a compound file (document) */
                P_COMPOUND_FILE p_compound_file = compound_file_create_from_file_handle(file_handle, &status);

                if(NULL != p_compound_file)
                {
                    status_assert(status);

                    status = try_grokking_compound_file_for_xls(p_compound_file, p_h_data);

                    compound_file_dispose(&p_compound_file);
                }
            }
            else
            {   /* file exists, but is not a compound file (document) */
                status = STATUS_OK;
            }
        }

        if(!status_done(status))
            al_array_dispose(p_h_data);

        status_accumulate(status, t5_file_close(&file_handle));
    }

    return(status);
}

_Check_return_
static STATUS
try_grokking_potential_cf_array_for_xls(
    _InRef_     PC_ARRAY_HANDLE p_h_data_in,
    _OutRef_    P_ARRAY_HANDLE p_h_data)
{
    STATUS status = STATUS_OK;
    U32 data_size = array_elements32(p_h_data_in);

    *p_h_data = 0;

    if(data_size > CFBF_FILE_HEADER_ID_BYTES)
    {
        PC_BYTE p_data = array_rangec(p_h_data_in, BYTE, 0, data_size);

        if(compound_file_file_header_id_test(p_data, data_size))
        {   /* data is a compound file (document) */
            P_COMPOUND_FILE p_compound_file = compound_file_create_from_data(p_data, data_size, &status);

            if(NULL != p_compound_file)
            {
                status_assert(status);

                status = try_grokking_compound_file_for_xls(p_compound_file, p_h_data);

                compound_file_dispose(&p_compound_file);
            }
        }
        else
        {   /* data is not a compound file (document) */
            status = STATUS_OK;
        }

        if(!status_done(status))
            al_array_dispose(p_h_data);
    }

    return(status);
}

/******************************************************************************
*
* read a double from possibly unaligned memory
*
******************************************************************************/

_Check_return_
static F64
xls_read_F64(
    _In_reads_bytes_c_(sizeof(F64)) PC_BYTE p)
{
    union
    {
        F64 f64;
        BYTE bytes[8];
        S32 wordz[2];
    } u;

#if RISCOS

    /* this for the 8087 -> ARM */
    u.bytes[4] = *p++;
    u.bytes[5] = *p++;
    u.bytes[6] = *p++;
    u.bytes[7] = *p++;

    u.bytes[0] = *p++;
    u.bytes[1] = *p++;
    u.bytes[2] = *p++;
    u.bytes[3] = *p++;

    if(((u.wordz[0] & 0x7FFFFFFF) >> 20) == 0x7FF)
        u.f64 = 0.0;

#elif WINDOWS

    u.bytes[0] = *p++;
    u.bytes[1] = *p++;
    u.bytes[2] = *p++;
    u.bytes[3] = *p++;

    u.bytes[4] = *p++;
    u.bytes[5] = *p++;
    u.bytes[6] = *p++;
    u.bytes[7] = *p++;

#endif /* OS */

    return(u.f64);
}

/******************************************************************************
*
* read an unsigned 16-bit value from possibly unaligned memory
*
******************************************************************************/

#if 1
#define xls_read_U16_LE(p) readval_U16_LE(p)
#else
_Check_return_
static inline U16
xls_read_U16_LE(
    _In_reads_bytes_c_(sizeof(U16)) PC_BYTE p)
{
    union
    {
        U16 u16;
        BYTE bytes[2];
    } u;

    u.bytes[0] = *p++;
    u.bytes[1] = *p++;

    return(u.u16);
}
#endif

_Check_return_
static inline WCHAR
xls_read_WCHAR_off(
    _In_        PCWCH wchars,
    _InVal_     U32 offset)
{
    PC_BYTE p = PtrAddBytes(PC_BYTE, wchars, offset * sizeof32(WCHAR));
    union
    {
        WCHAR wchar;
        BYTE bytes[2];
    } u;

    u.bytes[0] = *p++;
    u.bytes[1] = *p++;

    return(u.wchar);
}

/******************************************************************************
*
* read an unsigned 32-bit value from possibly unaligned memory
*
******************************************************************************/

#if 1
#define xls_read_U32_LE(p) readval_U32_LE(p)
#else
_Check_return_
static inline U32
xls_read_U32_LE(
    _In_reads_bytes_c_(sizeof(U32)) PC_BYTE p)
{
    union
    {
        U32 u32;
        BYTE bytes[4];
    } u;

    u.bytes[0] = *p++;
    u.bytes[1] = *p++;
    u.bytes[2] = *p++;
    u.bytes[3] = *p++;

    return(u.u32);
}
#endif

static void
xls_read_cell_address_r2_c2(
    _In_reads_bytes_c_(4) PC_BYTE p_byte,
    _OutRef_    P_XLS_ROW p_row,
    _OutRef_    P_XLS_COL p_col)
{
    *p_row = (XLS_ROW) xls_read_U16_LE(p_byte);
    *p_col = (XLS_COL) xls_read_U16_LE(p_byte + 2);
}

static void
xls_read_cell_address_formula(
    _In_reads_bytes_c_(4) PC_BYTE p_byte,
    _OutRef_    P_U16 p_row,
    _OutRef_    P_U16 p_col)
{
    *p_row = xls_read_U16_LE(p_byte);

    if(biff_version >= 8)
    {
        *p_col = xls_read_U16_LE(p_byte + 2);
    }
    else /* (biff_version < 8) */
    {
        *p_col = p_byte[2];
    }
}

static void
xls_read_cell_range_formula(
    _In_reads_bytes_c_(8) PC_BYTE p_byte,
    _OutRef_    P_U16 p_row_s,
    _OutRef_    P_U16 p_row_e,
    _OutRef_    P_U16 p_col_s,
    _OutRef_    P_U16 p_col_e)
{
    *p_row_s = xls_read_U16_LE(p_byte);
    *p_row_e = xls_read_U16_LE(p_byte + 2);

    if(biff_version >= 8)
    {
        *p_col_s = xls_read_U16_LE(p_byte + 4);
        *p_col_e = xls_read_U16_LE(p_byte + 6);
    }
    else /* (biff_version < 8) */
    {
        *p_col_s = p_byte[4];
        *p_col_e = p_byte[5];
    }
}

/******************************************************************************
*
* get a pointer to the contents of a record
*
******************************************************************************/

_Check_return_
_Ret_writes_(record_length)
static inline PC_BYTE
p_xls_record(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _InVal_     U32 record_length)
{
    PC_BYTE p_x = p_xls_load_info->p_file_start + opcode_offset;
    assert(opcode_offset + 4 /*opcode,record_length*/ + record_length <= p_xls_load_info->file_end_offset);
    UNREFERENCED_PARAMETER_InVal_(record_length);
    p_x += 4 /*opcode,record_length*/;
    return(p_x);
}

_Check_return_
static inline XLS_OPCODE
xls_read_record_header(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _OutRef_    P_U16 p_record_length)
{
    XLS_OPCODE opcode;
    PC_BYTE p_data;
    assert(opcode_offset + 4 /*opcode,record_length*/ <= p_xls_load_info->file_end_offset);
    p_data = p_xls_load_info->p_file_start + opcode_offset;
    opcode = xls_read_U16_LE(p_data);
    *p_record_length = xls_read_U16_LE(p_data + 2);
    return(opcode);
}

_Check_return_
static inline U16
xls_read_record_length(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset)
{
    PC_BYTE p_data;
    assert(opcode_offset + 4 /*opcode,record_length*/ <= p_xls_load_info->file_end_offset);
    p_data = p_xls_load_info->p_file_start + opcode_offset;
    return(xls_read_U16_LE(p_data + 2));
}

/******************************************************************************
*
* locate the first record in the Excel file
*
******************************************************************************/

_Check_return_
static U32 /* offset of opcode of record */
xls_first_record(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _OutRef_    P_XLS_OPCODE p_opcode,
    _OutRef_    P_U16 p_record_length)
{
    if(opcode_offset + 4 /*opcode,record_length*/ > p_xls_load_info->file_end_offset)
    {
        assert(opcode_offset + 4 /*opcode,record_length*/ <= p_xls_load_info->file_end_offset);
        *p_opcode = X_EOF;
        *p_record_length = 0;
        return(XLS_NO_RECORD_FOUND);
    }

    *p_opcode = xls_read_record_header(p_xls_load_info, opcode_offset, p_record_length);

    if(opcode_offset + 4 /*opcode,record_length*/ + *p_record_length > p_xls_load_info->file_end_offset)
    {
        assert(opcode_offset + 4 /*opcode,record_length*/ + *p_record_length <= p_xls_load_info->file_end_offset);
        *p_opcode = X_EOF;
        *p_record_length = 0;
        return(XLS_NO_RECORD_FOUND);
    }

    if(X_EOF == *p_opcode)
        return(XLS_NO_RECORD_FOUND);

    return(opcode_offset);
}

/******************************************************************************
*
* locate the next record in the Excel file
*
******************************************************************************/

_Check_return_
static U32 /* offset of opcode of record */
xls_next_record(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 this_opcode_offset,
    _OutRef_    P_XLS_OPCODE p_opcode,
    _InoutRef_  P_U16 p_record_length)
{
    U32 opcode_offset = this_opcode_offset + 4 /*opcode,record_length*/ + (U32) *p_record_length;

    if(opcode_offset + 4 /*opcode,record_length*/ > p_xls_load_info->file_end_offset)
    {
        assert(opcode_offset + 4 /*opcode,record_length*/ <= p_xls_load_info->file_end_offset);
        *p_opcode = X_EOF;
        *p_record_length = 0;
        return(XLS_NO_RECORD_FOUND);
    }

    *p_opcode = xls_read_record_header(p_xls_load_info, opcode_offset, p_record_length);

    if(opcode_offset + 4 /*opcode,record_length*/ + *p_record_length > p_xls_load_info->file_end_offset)
    {
        assert(opcode_offset + 4 /*opcode,record_length*/ + *p_record_length <= p_xls_load_info->file_end_offset);
        *p_opcode = X_EOF;
        *p_record_length = 0;
        return(XLS_NO_RECORD_FOUND);
    }

    if(X_EOF == *p_opcode)
        return(XLS_NO_RECORD_FOUND);

    return(opcode_offset);
}

/******************************************************************************
*
* locate the first record of a given type in
* the Excel file starting at a given offset
*
******************************************************************************/

_Check_return_
static U32 /* offset of opcode of found record */
xls_find_record_first(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 start_offset,
    _InVal_     XLS_OPCODE opcode_find,
    _OutRef_    P_U16 p_record_length)
{
    XLS_OPCODE opcode;
    U32 opcode_offset = start_offset;

    /* search for required opcode */
    for(opcode_offset = xls_first_record(p_xls_load_info, opcode_offset, &opcode, p_record_length);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_next_record (p_xls_load_info, opcode_offset, &opcode, p_record_length))
    {
        if(opcode == opcode_find)
            return(opcode_offset);
    }

    return(XLS_NO_RECORD_FOUND);
}

/******************************************************************************
*
* locate the next record of a given type in the Excel
* file starting at a previously determined offset and record_length
*
******************************************************************************/

_Check_return_
static U32 /* offset of opcode of found record */
xls_find_record_next(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 start_offset,
    _InVal_     XLS_OPCODE opcode_find,
    _InoutRef_  P_U16 p_record_length)
{
    XLS_OPCODE opcode;
    U32 opcode_offset = start_offset;

    /* search for required opcode */
    for(;;)
    {
        opcode_offset = xls_next_record(p_xls_load_info, opcode_offset, &opcode, p_record_length);

        if(XLS_NO_RECORD_FOUND == opcode_offset)
            return(XLS_NO_RECORD_FOUND);

        if(opcode == opcode_find)
            return(opcode_offset);
    }
}

/******************************************************************************
*
* find a given indexed record
* used for:
*       names
*       external names
*       external sheets
*
******************************************************************************/

_Check_return_
static U32 /* offset of found record */
xls_find_record_index(
    P_XLS_LOAD_INFO p_xls_load_info,
    _OutRef_    P_U16 p_record_length,
    _InVal_     U16 index,
    _InVal_     XLS_OPCODE opcode_find,
    _InVal_     U16 based /* 1-based or 0-based ?! */)
{
    U32 opcode_offset = 0 /* scan from start of file, not start of worksheet */;
    U16 index_found = based;

    /* search for required opcode */
    for(opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, opcode_find, p_record_length);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next( p_xls_load_info, opcode_offset, opcode_find, p_record_length))
    {
        if(index == index_found)
            return(opcode_offset);

        ++index_found;
    }

    return(opcode_offset /*XLS_NO_RECORD_FOUND*/);
}

_Check_return_
static U32 /* offset of found record */
xls_find_record_XF_INDEX(
    P_XLS_LOAD_INFO p_xls_load_info,
    _OutRef_    P_U16 p_record_length,
    _InVal_     XF_INDEX xf_index,
    _InVal_     XLS_OPCODE opcode_find)
{
    U16 record_length = 0;
    U32 opcode_offset = (biff_version == 4) ? p_xls_load_info->worksheet_substream_offset : 0; /* for BIFF4W worksheets */
    XF_INDEX xf_index_found = 0; /*based*/

    /* NB all XF records occur in a block - use cache */
    if(0 == p_xls_load_info->offset_of_xf_records)
        p_xls_load_info->offset_of_xf_records = xls_find_record_first(p_xls_load_info, opcode_offset, opcode_find, &record_length);

    /* search for required opcode */
    for(opcode_offset = p_xls_load_info->offset_of_xf_records, record_length = xls_read_record_length(p_xls_load_info, opcode_offset);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next( p_xls_load_info, opcode_offset, opcode_find, &record_length))
    {
        if(xf_index_found != xf_index)
        {
            ++xf_index_found;
            continue;
        }

        *p_record_length = record_length;
        return(opcode_offset);
    }

    *p_record_length = 0;
    return(opcode_offset /*XLS_NO_RECORD_FOUND*/);
}

_Check_return_
static U32 /* offset of found record */
xls_find_record_FONT_INDEX(
    P_XLS_LOAD_INFO p_xls_load_info,
    _OutRef_    P_U16 p_record_length,
    _InVal_     FONT_INDEX font_index,
    _InVal_     XLS_OPCODE opcode_find)
{
    U16 record_length = 0;
    U32 opcode_offset = (biff_version == 4) ? p_xls_load_info->worksheet_substream_offset : 0; /* for BIFF4W worksheets */
    FONT_INDEX font_index_found = 0 /*based*/;

    /* NB all FONT records occur in a block - use cache */
    if(0 == p_xls_load_info->offset_of_font_records)
        p_xls_load_info->offset_of_font_records = xls_find_record_first(p_xls_load_info, opcode_offset, opcode_find, &record_length);

    /* search for required opcode */
    for(opcode_offset = p_xls_load_info->offset_of_font_records, record_length = xls_read_record_length(p_xls_load_info, opcode_offset);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next( p_xls_load_info, opcode_offset, opcode_find, &record_length))
    {
        if(font_index_found == 4)
            font_index_found = 5; /* unbelievable: changes from 0-based to 1-based here */

        if(font_index_found != font_index)
        {
            ++font_index_found;
            continue;
        }

        *p_record_length = record_length;
        return(opcode_offset);
    }

    *p_record_length = 0;
    return(opcode_offset /*XLS_NO_RECORD_FOUND*/);
}

_Check_return_
static U32 /* offset of found record */
xls_find_record_FORMAT_INDEX(
    P_XLS_LOAD_INFO p_xls_load_info,
    _OutRef_    P_U16 p_record_length,
    _InVal_     FORMAT_INDEX format_index,
    _InVal_     XLS_OPCODE opcode_find)
{
    U16 record_length = 0;
    U32 opcode_offset = (biff_version == 4) ? p_xls_load_info->worksheet_substream_offset : 0; /* for BIFF4W worksheets */
    FORMAT_INDEX format_index_found = 0; /*based*/

    /* NB all FORMAT records occur in a block - use cache */
    if(0 == p_xls_load_info->offset_of_format_records)
        p_xls_load_info->offset_of_format_records = xls_find_record_first(p_xls_load_info, opcode_offset, opcode_find, &record_length);

    /* search for required opcode */
    for(opcode_offset = p_xls_load_info->offset_of_format_records, record_length = xls_read_record_length(p_xls_load_info, opcode_offset);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next( p_xls_load_info, opcode_offset, opcode_find, &record_length))
    {
        if(biff_version >= 5)
        {   /* no longer stored as a sequentially indexed array - must search the FORMAT record itself for a match */
            PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
            FORMAT_INDEX record_format_index = xls_read_U16_LE(p_x);

            if(record_format_index != format_index)
            {
                continue;
            }

            *p_record_length = record_length;
            return(opcode_offset);
        }

        if(format_index_found != format_index)
        {
            ++format_index_found;
            continue;
        }

        *p_record_length = record_length;
        return(opcode_offset);
    }

    *p_record_length = 0;
    return(opcode_offset /*XLS_NO_RECORD_FOUND*/);
}

#define XF_USED_ATTRIB              0xFC /* always [7..2] */
#define XF_USED_ATTRIB_PROTECTION   0x80
#define XF_USED_ATTRIB_BACKGROUND   0x40
#define XF_USED_ATTRIB_BORDER       0x20
#define XF_USED_ATTRIB_ALIGNMENT    0x10
#define XF_USED_ATTRIB_FONT         0x08
#define XF_USED_ATTRIB_FORMAT       0x04

static void
xls_slurp_xf_data_from_XF_INDEX(
    P_XLS_LOAD_INFO p_xls_load_info,
    _Out_writes_c_(20) P_BYTE p_xf_data, /* always as BIFF8 format data */ /*writes[20]*/
    _InVal_     XF_INDEX xf_index_find,
    _InVal_     XLS_OPCODE xf_opcode)
{
    XF_INDEX xf_index = xf_index_find;

    memset32(p_xf_data, 0, 20);

    for(;;)
    {
        U16 record_length;
        U32 opcode_offset = xls_find_record_XF_INDEX(p_xls_load_info, &record_length, xf_index, xf_opcode);
        PC_BYTE p_x;
        BYTE xf_used_attrib;
        BYTE xf_type_protection;

        if(XLS_NO_RECORD_FOUND == opcode_offset)
            return;

        /* transfer data from this XF record to the output only if a) still required and b) valid in this record */
        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        /* which attributes are valid in this record? */
        if(biff_version >= 8)
            xf_used_attrib = p_x[9];
        else if(biff_version == 5)
            xf_used_attrib = p_x[7];
        else if(biff_version == 4)
            xf_used_attrib = p_x[5];
        else if(biff_version == 3)
            xf_used_attrib = p_x[3];
        else /* (biff_version == 2) */
            xf_used_attrib = XF_USED_ATTRIB_FORMAT | XF_USED_ATTRIB_FONT | XF_USED_ATTRIB_PROTECTION;

        xf_used_attrib &= XF_USED_ATTRIB;

        /* XF_TYPE_PROT */
        if(biff_version >= 5)
            xf_type_protection = p_x[4] & 0x07;
        else if(biff_version >= 3)
            xf_type_protection = p_x[2] & 0x07;
        else /* (biff_version == 2) */
            xf_type_protection = (p_x[2] >> 6) & 0x03;

        if(0 != (0x04 & xf_type_protection))
        {   /* Style XF: invert sense such that bit set -> valid like Cell XF */
            xf_used_attrib = xf_used_attrib ^ XF_USED_ATTRIB;
        }

        if( (0 == (XF_USED_ATTRIB_FORMAT & p_xf_data[9]  )) &&
            (0 != (XF_USED_ATTRIB_FORMAT & xf_used_attrib)) )
        {
            FORMAT_INDEX format_index;

            if(biff_version >= 5)
                format_index = xls_read_U16_LE(p_x + 2);
            else if(biff_version >= 3)
                format_index = (FORMAT_INDEX) p_x[1];
            else /* (biff_version == 2) */
                format_index = (FORMAT_INDEX) (p_x[2] & 0x3F);

            writeval_U16_LE(&p_xf_data[2], format_index);

            p_xf_data[9] |= XF_USED_ATTRIB_FORMAT;
        }

        if( (0 == (XF_USED_ATTRIB_FONT & p_xf_data[9]  )) &&
            (0 != (XF_USED_ATTRIB_FONT & xf_used_attrib)) )
        {
            FONT_INDEX font_index;

            if(biff_version >= 5)
                font_index = xls_read_U16_LE(p_x);
            else /* BIFF4,BIFF3,BIFF2 */
                font_index = (FONT_INDEX) p_x[0];

            writeval_U16_LE(&p_xf_data[0], font_index);

            p_xf_data[9] |= XF_USED_ATTRIB_FONT;
        }

        if( (0 == (XF_USED_ATTRIB_ALIGNMENT & p_xf_data[9]  )) &&
            (0 != (XF_USED_ATTRIB_ALIGNMENT & xf_used_attrib)) )
        {
            /* XF_HOR_ALIGN */
            BYTE xf_horizontal_alignment;

            if(biff_version >= 5)
                xf_horizontal_alignment = p_x[6] & 0x07;
            else if(biff_version >= 3)
                xf_horizontal_alignment = p_x[4] & 0x07;
            else /* (biff_version == 2) */
                xf_horizontal_alignment = p_x[3] & 0x07;

            p_xf_data[6] = (p_xf_data[6] & ~0x07) | (xf_horizontal_alignment);

            /* XF_VERT_ALIGN */
            if(biff_version >= 4)
            {
                BYTE xf_vertical_alignment;

                if(biff_version >= 5)
                    xf_vertical_alignment = (p_x[6] >> 4) & 0x07;
                else /* (biff_version == 4) */
                    xf_vertical_alignment = (p_x[4] >> 4) & 0x03;

                p_xf_data[6] = (p_xf_data[6] & ~0x70) | (xf_vertical_alignment << 4);
            }

            /* XF_ROTATION */
            if(biff_version >= 8)
                p_xf_data[7] = p_x[7];

            p_xf_data[9] |= XF_USED_ATTRIB_ALIGNMENT;
        }

        if( (0 == (XF_USED_ATTRIB_BORDER & p_xf_data[9]  )) &&
            (0 != (XF_USED_ATTRIB_BORDER & xf_used_attrib)) )
        {
            p_xf_data[9] |= XF_USED_ATTRIB_BORDER;
        }

        if( (0 == (XF_USED_ATTRIB_BACKGROUND & p_xf_data[9]  )) &&
            (0 != (XF_USED_ATTRIB_BACKGROUND & xf_used_attrib)) )
        {
            if(biff_version >= 8)
            {
                /* nothing in top bits */
                writeval_U16_LE(&p_xf_data[18], readval_U16_LE(&p_x[18]));

                p_xf_data[9] |= XF_USED_ATTRIB_BACKGROUND;
            }
            else if(biff_version >= 3)
            {
                BYTE pattern_colour;
                BYTE pattern_background_colour;

                if(biff_version == 5)
                {
                    pattern_colour            =        ((                p_x[8]       ) & 0x007F); /* [6..0] */
                    pattern_background_colour = (BYTE) ((readval_U16_LE(&p_x[8]) >>  7) & 0x007F); /* [13..7] */
                }
                else /* BIFF4,BIFF3 */
                {
                    pattern_colour            = (BYTE) ((readval_U16_LE(&p_x[6]) >>  6) & 0x001F); /* [10..6] */
                    pattern_background_colour = (BYTE) ((readval_U16_LE(&p_x[6]) >> 11) & 0x001F); /* [15..11] */
                }

                /* nothing in top bits */
                writeval_U16_LE(&p_xf_data[18], (((U16) pattern_background_colour << 7) | pattern_colour));

                p_xf_data[9] |= XF_USED_ATTRIB_BACKGROUND;
            }

            /* not present for BIFF2 */
        }

        if( (0 == (XF_USED_ATTRIB_PROTECTION & p_xf_data[9]  )) &&
            (0 != (XF_USED_ATTRIB_PROTECTION & xf_used_attrib)) )
        {
            p_xf_data[4] = (p_xf_data[4] & ~0x07) | (xf_type_protection);

            p_xf_data[9] |= XF_USED_ATTRIB_PROTECTION;
        }

        /* nothing left to enquire about? */
        if(XF_USED_ATTRIB == (p_xf_data[9] & XF_USED_ATTRIB))
            return;

        { /* get our parent style to fill in the blanks */
        XF_INDEX parent_xf_index;

        if(biff_version >= 5)
            parent_xf_index = xls_read_U16_LE(p_x + 4) >> 4;
        else if(biff_version == 4)
            parent_xf_index = xls_read_U16_LE(p_x + 2) >> 4;
        else if(biff_version == 3)
            parent_xf_index = xls_read_U16_LE(p_x + 4) >> 4;
        else /* (biff_version == 2) */
            return;

        if(0xFFF == parent_xf_index)
            return; /* end of chain */

        if(xf_index == parent_xf_index)
        {
            /* assert(xf_index != parent_xf_index); some Mac software triggers this */
            return; /* end of chain */
        }

        xf_index = parent_xf_index;
        } /*block*/
    }
}

_Check_return_
static STATUS
xls_quick_ublock_sbchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars,
    _InVal_     U32 sbchars_n,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _InVal_     BOOL allow_il_unicode)
{
    U32 offset = 0;

    for(;;)
    {
        UCS4 ucs4;
        SBCHAR sbchar;

        if(offset == sbchars_n) /* adding 0 SBCHARs is OK */
            break;

        sbchar = sbchars[offset++];

        if(CH_NULL == sbchar)
            break; /* unexpected end of string */

        if(ucs4_is_ascii7(sbchar))
            ucs4 = sbchar;
        else
            ucs4 = ucs4_from_sbchar_with_codepage(sbchar, sbchar_codepage);

#if USTR_IS_SBSTR
        if(!ucs4_is_sbchar(ucs4))
        {
            if(allow_il_unicode)
            {
                status_return(quick_ublock_ucs4_add_aiu(p_quick_ublock, ucs4));
                continue;
            }

            /* out-of-range -> force mapping to native */
            ucs4 = ucs4_to_sbchar_force_with_codepage(ucs4, get_system_codepage(), CH_QUESTION_MARK);
        }
#endif

        status_return(quick_ublock_ucs4_add(p_quick_ublock, ucs4));
    }

    return(STATUS_OK);
}

#if TSTR_IS_SBSTR

#ifndef          __utf16_h
#include "cmodules/utf16.h"
#endif

_Check_return_
static STATUS
xls_quick_ublock_wchars_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(wchars_n) PCWCH pwch,
    _InVal_     U32 wchars_n,
    _InVal_     BOOL allow_il_unicode)
{
    U32 offset = 0;

    /* now caters for unaligned pwch */ /*assert(0 == (1 & (uintptr_t) pwch));*/ /* OK on x86 but not on ARM */

    for(;;)
    {
        UCS4 ucs4;
        WCHAR ch;

        if(offset == wchars_n) /* adding 0 WCHARs is OK */
            break;

        ch = xls_read_WCHAR_off(pwch, offset); offset++; /* may be unaligned */

        if(CH_NULL == ch)
            break; /* unexpected end of string */

        /* consider surrogate pair in UTF-16 */
        if(WCHAR_is_utf16_high_surrogate(ch))
        {
            WCHAR high_surrogate = ch;
            WCHAR low_surrogate;

            if(offset == wchars_n)
            {   /* no low surrogate available */
                assert0();
                break;
            }

            low_surrogate = xls_read_WCHAR_off(pwch, offset); offset++; /* may be unaligned */
            assert(WCHAR_is_utf16_low_surrogate(low_surrogate));

            ucs4 = utf16_char_decode_surrogates(high_surrogate, low_surrogate);
        }
        else
        {
            assert(!WCHAR_is_utf16_low_surrogate(ch));
            ucs4 = ch;
        }

        if(status_fail(ucs4_validate(ucs4)))
        {
            assert(status_ok(ucs4_validate(ucs4)));
            ucs4 = CH_QUESTION_MARK;
        }

#if USTR_IS_SBSTR
        assert(!ucs4_is_C1(ucs4)); /* hopefully no C1 to complicate things */

        if(!ucs4_is_sbchar(ucs4))
        {
            if(allow_il_unicode)
            {
                status_return(quick_ublock_ucs4_add_aiu(p_quick_ublock, ucs4));
                continue;
            }

            /* out-of-range -> force mapping to native */
            ucs4 = ucs4_to_sbchar_force_with_codepage(ucs4, get_system_codepage(), CH_QUESTION_MARK);
        }
#endif

        status_return(quick_ublock_ucs4_add(p_quick_ublock, ucs4));
    }

    return(STATUS_OK);
}

#else /* TSTR_IS_SBSTR */

#define xls_quick_ublock_wchars_add(p_quick_ublock, pwch, wchars_n, allow_il_unicode) \
    quick_ublock_wchars_add(p_quick_ublock,pwch, wchars_n)

#endif /* TSTR_IS_SBSTR */

_Check_return_
static STATUS
xls_quick_ublock_xls_string_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    PC_BYTE xls_string,
    _In_        U32 n_chars,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _InVal_     BOOL allow_il_unicode)
{
    STATUS status;
    BYTE string_flags;

    if(biff_version < 8)
        return(xls_quick_ublock_sbchars_add(p_quick_ublock, (PC_SBCHARS) xls_string, n_chars, sbchar_codepage, allow_il_unicode));

    string_flags = *xls_string++;

    if(string_flags & 0x08) /* rich-text */
        xls_string += 2;

    if(string_flags & 0x04) /* phonetic */
        xls_string += 4;

    if(string_flags & 0x01)
    {   /* Unicode UTF-16LE string - NB may be unaligned */
        status = xls_quick_ublock_wchars_add(p_quick_ublock, (PCWCH) xls_string, n_chars, allow_il_unicode);
    }
    else
    {   /* 'Compressed' BYTE string (high bytes of WCHAR all zero) */
        status = xls_quick_ublock_sbchars_add(p_quick_ublock, (PC_SBCHARS) xls_string, n_chars, sbchar_codepage, allow_il_unicode);
    }

    UNREFERENCED_PARAMETER_InVal_(allow_il_unicode); /* on UNICODE builds this is ignored as no inlines are generated */

    return(status);
}

static void
xls_sst_index_dispose(void)
{
    ARRAY_INDEX i;

    for(i = 0; array_index_is_valid(&g_h_sst_index, i); i++)
    {
        P_ARRAY_HANDLE p_array_handle = array_ptr_no_checks(&g_h_sst_index, ARRAY_HANDLE, i);
        al_array_dispose(p_array_handle);
    }

    al_array_dispose(&g_h_sst_index);
}

/*
 * locate and load the contents of the Shared String Table (in Workbook Globals Substream)
 */

_Check_return_
static STATUS
xls_read_SST(
    P_XLS_LOAD_INFO p_xls_load_info)
{
    STATUS status = STATUS_OK;
    U16 record_length;
    U32 opcode_offset = xls_find_record_first(p_xls_load_info, 0, X_SST_B8, &record_length);

    xls_sst_index_dispose(); /* in case one got left over from a previous multi-worksheet load that errored ... */

    if(XLS_NO_RECORD_FOUND != opcode_offset)
    {
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
        U32 continue_opcode_offset = opcode_offset;
        PC_BYTE p_data = p_x + 8; /* skip SST header */
        U32 record_seg_bytes_remain = record_length - 8; /* remove SST header */
        PC_BYTE p_string_data = NULL;
        U32 string_chars_remain = 0;
        BYTE string_flags = 0;
        U32 rich_text_bytes = 0;
        U32 phonetic_settings_bytes = 0;
        BOOL seg_broken = FALSE;
        U32 si = 0;
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup_using_array(&quick_ublock, 0); /* force it to go to array handle */

        p_xls_load_info->num_sst_strings = xls_read_U32_LE(p_x + 4);

        /* loop, dealing with what we have accumulated to date */
        while(si < p_xls_load_info->num_sst_strings)
        {
            if(0 == record_seg_bytes_remain)
            {   /* the current record of the SST has been consumed - advance */
                XLS_OPCODE opcode;

                /* read a following CONTINUE record */
                if(XLS_NO_RECORD_FOUND == (continue_opcode_offset = xls_next_record(p_xls_load_info, continue_opcode_offset, &opcode, &record_length)))
                    break;

                if(opcode != X_CONTINUE)
                    break;

                p_x = p_xls_record(p_xls_load_info, continue_opcode_offset, record_length);

                p_data = p_x; /* nothing to skip */
                record_seg_bytes_remain = record_length;

                if(seg_broken)
                {
                    string_flags = *p_data++; /* load and skip grbit */
                    record_seg_bytes_remain -= 1;
                }

                p_string_data = p_data;
                continue;
            }

            if(NULL == p_string_data)
            {   /* load a new string */
                string_chars_remain = xls_read_U16_LE(p_data);
                string_flags = p_data[2];

                p_string_data = p_data + 3; /* len(2) + string_flags(1) */
                record_seg_bytes_remain -= 3;

                if(string_flags & 0x08)
                {
                    rich_text_bytes = (4 * xls_read_U16_LE(p_string_data)); /* data */
                    p_string_data += 2; /* skip rich text stuff in header */
                    record_seg_bytes_remain -= 2;
                }

                if(string_flags & 0x04)
                {
                    phonetic_settings_bytes =  xls_read_U32_LE(p_string_data); /* data */
                    p_string_data += 4; /* skip phonetic settings stuff in header */
                    record_seg_bytes_remain -= 4;
                }

                continue;
            }

            /* output as much of the string as we have so far */
            if(string_flags & 0x01)
            {   /* Unicode UTF-16LE string */
                PCWCH wchars = (PCWCH) (p_string_data);
                U32 string_chars_avail = MIN(string_chars_remain, record_seg_bytes_remain / sizeof32(WCHAR));

#if TRACE_ALLOWED && 0
#if WINDOWS
                WCHAR wbuf[8000];
                if(-1 == _snwprintf_s(wbuf, elemof32(wbuf), _TRUNCATE,
                                      L" SST %d(%d/%dc): Unicode %.*s" L"\n",
                                      si, string_chars_avail, string_chars_remain, string_chars_avail, wchars))
                {
                    wbuf[0] = CH_EXCLAMATION_MARK;
                    wbuf[elemof32(wbuf)-2] = CH_FULL_STOP;
                    wbuf[elemof32(wbuf)-3] = CH_FULL_STOP;
                    wbuf[elemof32(wbuf)-4] = CH_FULL_STOP;
                }
                OutputDebugStringW(wbuf);
#else
#endif
#endif /* TRACE_ALLOWED */

                /* add string_chars_avail */
                status = xls_quick_ublock_wchars_add(&quick_ublock, wchars, string_chars_avail, TRUE); /* NB may be unaligned */

                string_chars_remain -= string_chars_avail;
                p_string_data += string_chars_avail * sizeof32(WCHAR);
                record_seg_bytes_remain -= string_chars_avail * sizeof32(WCHAR);
            }
            else
            {   /* 'Compressed' BYTE (assume Latin-1) */
                PC_SBCHARS sbchars = (PC_SBCHARS) (p_string_data);
                U32 string_chars_avail = MIN(string_chars_remain, record_seg_bytes_remain / sizeof32(BYTE));

#if TRACE_ALLOWED && 0
#if WINDOWS
                CHAR cbuf[8000];
                if(-1 == _snprintf_s(cbuf, elemof32(cbuf), _TRUNCATE,
                                     " SST %d(%d/%db): %.*s" "\n",
                                     si, string_chars_avail, string_chars_remain, string_chars_avail, sbchars))
                {
                    cbuf[0] = CH_EXCLAMATION_MARK;
                    cbuf[elemof32(cbuf)-2] = CH_FULL_STOP;
                    cbuf[elemof32(cbuf)-3] = CH_FULL_STOP;
                    cbuf[elemof32(cbuf)-4] = CH_FULL_STOP;
                }
                OutputDebugStringA(cbuf);
#else
                tracef(TRACE__XLS_LOADB, " SST %d(%d/%db): %.*s",
                       si, string_chars_avail, string_chars_remain, string_chars_avail, sbchars);
#endif
#endif /* TRACE_ALLOWED */

                /* add string_chars_avail */
                status = xls_quick_ublock_sbchars_add(&quick_ublock, sbchars, string_chars_avail, p_xls_load_info->sbchar_codepage, FALSE);

                string_chars_remain -= string_chars_avail;
                p_string_data += string_chars_avail * sizeof32(BYTE);
                record_seg_bytes_remain -= string_chars_avail * sizeof32(BYTE);
            }

            status_break(status);

            if(0 != string_chars_remain)
            {
                seg_broken = TRUE;
                assert(0 == record_seg_bytes_remain);
                continue;
            }
            seg_broken = FALSE;

            p_data = p_string_data + (rich_text_bytes + phonetic_settings_bytes); /* skip trailing cruft */
            record_seg_bytes_remain -= (rich_text_bytes + phonetic_settings_bytes);

            status_break(status = quick_ublock_nullch_add(&quick_ublock));

            { /* this string has now ended - add it to our SST index and advance */
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(P_ARRAY_HANDLE_USTR), FALSE);
            ARRAY_HANDLE array_handle = quick_ublock_array_handle_ref(&quick_ublock);
            quick_ublock_array_handle_ref(&quick_ublock) = 0; /* steal the handle we just built */
            status_break(status = al_array_add(&g_h_sst_index, P_ARRAY_HANDLE_USTR, 1, &array_init_block, &array_handle));
            }

            p_string_data = NULL;
            string_chars_remain = 0;
            rich_text_bytes = 0;
            phonetic_settings_bytes = 0;
            si++;
        }
    }

    return(status);
}

/******************************************************************************
*
* check the start of file for a BOF record
*
******************************************************************************/

_Check_return_
static STATUS
xls_read_first_BOF(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    const U32 opcode_offset = 0;
    U16 record_length;
    const XLS_OPCODE opcode = xls_read_record_header(p_xls_load_info, opcode_offset, &record_length);
    PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    U16 following_data_type;

    switch(opcode)
    {
    default:
        trace_2(TRACE__XLS_LOADB, TEXT("*** Excel file BAD BOF=") U32_XTFMT TEXT(" @ ") U32_XTFMT TEXT(" ***"), (U32) opcode, opcode_offset);
        return(create_error(XLS_ERR_BADFILE_BAD_BOF));

    /* generate a BIFF version from the binary in the BOF record */
    case X_BOF_B2:    biff_version = 2; assert(record_length == 4); break;
    case X_BOF_B3:    biff_version = 3; assert(record_length == 6); break;

    case X_BOF_B4:
        {
        biff_version = 4;

        assert(record_length == 6);

        if(0x0100 == xls_read_U16_LE(p_x + 2))
            p_xls_load_info->process_multiple_worksheets = TRUE; /* BIFF4W */
        break;
        }

    case X_BOF_B5_B8:
        {
        U16 biff_version_code = xls_read_U16_LE(p_x);

        /*if(0 == opcode_offset)*/ /* see OpenOffice.org documentation regarding incorrect subsequent BOF records */
        {
            assert(record_length == (biff_version == 8) ? 16 : 8);

            switch(biff_version_code)
            {
                case 0x0000:
                case 0x0007:
                case 0x0200:    biff_version = 2; break; /* see OpenOffice Excel File Format doc for BOF */
                case 0x0300:    biff_version = 3; break;
                case 0x0400:    biff_version = 4; break;
                case 0x0500:    biff_version = 5; break; /* BIFF5/BIFF7 very similar */
                default:
                case 0x0600:    biff_version = 8; break;
            }
        }

        p_xls_load_info->process_multiple_worksheets = TRUE;
        break;
        }
    }

    following_data_type = xls_read_U16_LE(p_x + 2);

    /* set the XF opcode to look for unless we get a deviant record */
    if(biff_version >= 5)
        biff_version_opcode_XF = X_XF_B5_B8;
    else if(biff_version == 4)
        biff_version_opcode_XF = X_XF_B4;
    else if(biff_version == 3)
        biff_version_opcode_XF = X_XF_B3;
    else /* (biff_version == 2) */
        biff_version_opcode_XF = X_XF_B2;

    trace_4(TRACE__XLS_LOADB, TEXT("*** Excel file BOF=") U32_XTFMT TEXT(" @ ") U32_XTFMT TEXT(", biff_version=") U32_TFMT TEXT(", FDT=") U32_XTFMT TEXT(" ***"),
            (U32) opcode, opcode_offset, biff_version, (U32) following_data_type);
    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_check_BOF_is_worksheet(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 bof_offset)
{
    U16 record_length;
    const XLS_OPCODE opcode = xls_read_record_header(p_xls_load_info, bof_offset, &record_length);
    PC_BYTE p_x = p_xls_record(p_xls_load_info, bof_offset, record_length);
    U16 following_data_type;
    BOOL is_worksheet = FALSE;

    switch(opcode)
    {
    default:
        trace_2(TRACE__XLS_LOADB, TEXT("*** Excel file BAD BOF=") U32_XTFMT TEXT(" @ ") U32_XTFMT TEXT(" ***"), (U32) opcode, bof_offset);
        return(create_error(XLS_ERR_BADFILE_BAD_BOF));

    case X_BOF_B2:      assert(record_length == 4); break;
    case X_BOF_B3:      assert(record_length == 6); break;
    case X_BOF_B4:      assert(record_length == 6); break;
    case X_BOF_B5_B8:   assert(record_length >= 8); break;
    }

    following_data_type = xls_read_U16_LE(p_x + 2);
    is_worksheet = (following_data_type == 0x10);

    trace_3(TRACE__XLS_LOADB, TEXT("*** Excel file BOF=") U32_XTFMT TEXT(" @ ") U32_XTFMT TEXT(", FDT=") U32_XTFMT TEXT(" ***"), (U32) opcode, bof_offset, (U32) following_data_type);
    return(is_worksheet ? STATUS_DONE : STATUS_OK);
}

/* Dump all the BIFF records */

#if 0
#define BIFF_VERSION_ASSERT(rhs) assert(biff_version rhs)
#define BIFF_VERSION_ASSERT_BINOP(rhs1, op, rhs2) assert((biff_version rhs1) op (biff_version rhs2))
#else
#define BIFF_VERSION_ASSERT(rhs) /*EMPTY*/
#define BIFF_VERSION_ASSERT_BINOP(rhs1, op, rhs2) /*EMPTY*/
#endif

#if TRACE_ALLOWED && 1

static void
xls_dump_records(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    U16 previous_opcode = 0xFFFFU;
    U32 this_record_idx = 0;
    U32 overall_record_idx;
    U32 opcode_offset;
    U16 record_length = 0;

    for(opcode_offset = 0 /*start_offset*/, overall_record_idx = 0;
        opcode_offset + 4 /*opcode,record_length*/ <= p_xls_load_info->file_end_offset;
        opcode_offset += ((U32) record_length + 4 /*opcode,record_length*/), overall_record_idx++)
    {
        const XLS_OPCODE opcode = xls_read_record_header(p_xls_load_info, opcode_offset, &record_length);
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        PCTSTR opcode_name;
        U32 maxbytes = (biff_version >= 8) ? 8224U : 2080U; /* MS Excel doc */
        U32 minbytes = 0;
        U32 minver = biff_version;
        U32 maxver = biff_version;
        BOOL suppress = FALSE;
        BOOL unhandled = TRUE;
        PCTSTR extra_text = NULL;
        UCHARZ extra_data[64];

        if((0 == opcode) && (0 == record_length))
        {   /* some writers pad from EOF to end of stream with CH_NULL (including Excel 'Simple Save' which pads to 4k) */
            continue;
        }

        if(previous_opcode != opcode)
        {
            previous_opcode = opcode;
            this_record_idx = 0;
        }
        else
            ++this_record_idx;

        extra_data[0] = CH_NULL;

#if 0 /* limited reporting best for large sheets unless critical! */
        UNREFERENCED_PARAMETER(p_x);

        switch(opcode)
        {
        case X_BOF_B2:                  opcode_name = TEXT("BOF"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 4; break;
        case X_EOF:                     opcode_name = TEXT("EOF"); unhandled = FALSE; minbytes = maxbytes = 0; break;
        case X_BOF_B3:                  opcode_name = TEXT("BOF"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 3); minbytes = maxbytes = 6; break;
        case X_BOF_B4:                  opcode_name = TEXT("BOF"); unhandled = FALSE; minver = maxver = 4; BIFF_VERSION_ASSERT(== 4); minbytes = maxbytes = 6; break;
        case X_BOF_B5_B8:               opcode_name = TEXT("BOF"); unhandled = FALSE; minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); if(biff_version >= 8) minbytes = maxbytes = 16; else minbytes = maxbytes = 8; break;
        case X_SST_B8:                  opcode_name = TEXT("SST"); unhandled = FALSE; minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 8; break;
        default:                        opcode_name = TEXT("UNHANDLED"); unhandled = TRUE; suppress = TRUE; minbytes = 0; break;
        }
#else
        switch(opcode)
        {
        case X_DIMENSIONS_B2:           opcode_name = TEXT("DIMENSIONS"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 8; break;
        case X_BLANK_B2:                opcode_name = TEXT("BLANK"); suppress = TRUE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 7; break;
        case X_INTEGER_B2:              opcode_name = TEXT("INTEGER"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 9; break;
        case X_NUMBER_B2:               opcode_name = TEXT("NUMBER"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 15; break;
        case X_LABEL_B2:                opcode_name = TEXT("LABEL"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = 7+1+1; maxbytes = 7+1+255; break;
        case X_BOOLERR_B2:              opcode_name = TEXT("BOOLERR"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 9; break;

        case X_FORMULA_B2_B5_B8:
            unhandled = FALSE;
            if(biff_version >= 5)
            { opcode_name = TEXT("FORMULA"); minver = 5; maxver = 8; minbytes = 21; }
            else
            { opcode_name = TEXT("FORMULA"); minver = maxver = 2; minbytes = 17; }
            BIFF_VERSION_ASSERT_BINOP(== 2, ||, >= 5);
            break;

        case X_STRING_B2:               opcode_name = TEXT("STRING"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = 1+0; maxbytes = 1+255; break;
        case X_ROW_B2:                  opcode_name = TEXT("ROW"); suppress = TRUE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = 13; maxbytes = 18; break;
        case X_BOF_B2:                  opcode_name = TEXT("BOF"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 4; break;
        case X_EOF:                     opcode_name = TEXT("EOF"); unhandled = FALSE; minbytes = maxbytes = 0; break;
        case X_INDEX_B2:                opcode_name = TEXT("INDEX"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = 12; break;
        case X_CALCCOUNT:               opcode_name = TEXT("CALCCOUNT"); minbytes = maxbytes = 2; break;
        case X_CALCMODE:                opcode_name = TEXT("CALCMODE"); minbytes = maxbytes = 2; break;
        case X_PRECISION:               opcode_name = TEXT("PRECISION"); minbytes = maxbytes = 2; break;
        case X_REFMODE:                 opcode_name = TEXT("REFMODE"); minbytes = maxbytes = 2; break;

        case X_DELTA:                   opcode_name = TEXT("DELTA"); minbytes = maxbytes = 8; break;
        case X_ITERATION:               opcode_name = TEXT("ITERATION"); minbytes = maxbytes = 2; break;
        case X_PROTECT:                 opcode_name = TEXT("PROTECT"); minbytes = maxbytes = 2; break;
        case X_PASSWORD:                opcode_name = TEXT("PASSWORD"); minbytes = maxbytes = 2; break;
        case X_HEADER:                  opcode_name = TEXT("HEADER"); minbytes = 0; break;
        case X_FOOTER:                  opcode_name = TEXT("FOOTER"); minbytes = 0; break;
        case X_EXTERNCOUNT_B2_B7:       opcode_name = TEXT("EXTERNCOUNT"); minver = 2; maxver = 7; BIFF_VERSION_ASSERT(< 8); minbytes = maxbytes = 2; break;
        case X_EXTERNSHEET:             opcode_name = TEXT("EXTERNSHEET"); unhandled = FALSE; if(biff_version >= 8) minbytes = 8; else minbytes = 0+1+1; break;

        case X_DEFINEDNAME_B2_B5_B8:
            unhandled = FALSE;
            if(biff_version >= 5)
            { opcode_name = TEXT("DEFINEDNAME"); minver = 5; maxver = 8; if(biff_version >= 8) minbytes = 14+1+1+1+1; else minbytes = 14+1+1+1; }
            else
            { opcode_name = TEXT("DEFINEDNAME"); minver = maxver = 2; minbytes = 5+1+1+1; }
            BIFF_VERSION_ASSERT_BINOP(== 2, ||, >= 5);
            break;

        case X_WINDOWPROTECT:           opcode_name = TEXT("WINDOWPROTECT"); minbytes = maxbytes = 2;  break;
        case X_VERTICALPAGEBREAKS:      opcode_name = TEXT("VERTICALPAGEBREAKS"); minbytes = 2; break;
        case X_HORIZONTALPAGEBREAKS:    opcode_name = TEXT("HORIZONTALPAGEBREAKS"); minbytes = 2; break;
        case X_NOTE:                    opcode_name = TEXT("NOTE"); minbytes = 6+1; maxbytes = 6+2048; break;
        case X_SELECTION:               opcode_name = TEXT("SELECTION"); minbytes = 7; break;
        case X_FORMAT_B2_B3:            opcode_name = TEXT("FORMAT"); unhandled = FALSE; minver = 2; maxver = 3; BIFF_VERSION_ASSERT_BINOP(== 2, ||, == 3); minbytes = 0+1+1; break;
        case X_BUILTINFMTCOUNT_B2:      opcode_name = TEXT("BUILTINFMTCOUNT"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 2; break;

        case X_COLUMNDEFAULT_B2:        opcode_name = TEXT("COLUMNDEFAULT"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = 9; break;
        case X_ARRAY_B2:                opcode_name = TEXT("ARRAY"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = 8; break;
        case X_DATEMODE:                opcode_name = TEXT("DATEMODE"); minbytes = maxbytes = 2; break;

        case X_EXTERNNAME_B2_B5_B8:
            unhandled = FALSE;
            if(biff_version >= 5)
            { opcode_name = TEXT("EXTERNNAME"); minver = 5; maxver = 8; }
            else
            { opcode_name = TEXT("EXTERNNAME"); minver = maxver = 2; }
            BIFF_VERSION_ASSERT_BINOP(== 2, ||, >= 5);
            break;

        case X_COLWIDTH_B2:             opcode_name = TEXT("COLWIDTH"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 4; break;
        case X_DEFAULTROWHEIGHT_B2:     opcode_name = TEXT("DEFAULTROWHEIGHT"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 2; break;
        case X_LEFTMARGIN:              opcode_name = TEXT("LEFTMARGIN"); minbytes = maxbytes = 8; break;
        case X_RIGHTMARGIN:             opcode_name = TEXT("RIGHTMARGIN"); minbytes = maxbytes = 8; break;
        case X_TOPMARGIN:               opcode_name = TEXT("TOPMARGIN"); minbytes = maxbytes = 8; break;
        case X_BOTTOMMARGIN:            opcode_name = TEXT("BOTTOMMARGIN"); minbytes = maxbytes = 8; break;
        case X_PRINTHEADERS:            opcode_name = TEXT("PRINTHEADERS"); minbytes = maxbytes = 2; break;
        case X_PRINTGRIDLINES:          opcode_name = TEXT("PRINTGRIDLINES"); minbytes = maxbytes = 2; break;
        case X_FILEPASS:                opcode_name = TEXT("FILEPASS"); if(biff_version >= 8) minbytes = 6; else minbytes = 4; break;

        case X_FONT_B2_B5_B8:
            if(biff_version >= 5)
            { opcode_name = TEXT("FONT"); minver = 5; maxver = 8; if(biff_version >= 8) minbytes = 14+1+1+1; else minbytes = 14+1+1; }
            else
            { opcode_name = TEXT("FONT"); minver = maxver = 2; minbytes = 4+1+1; }
            BIFF_VERSION_ASSERT_BINOP(== 2, || , >= 5);
            break;

        case X_TABLEOP_B2:              opcode_name = TEXT("TABLEOP"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 12; break;
        case X_TABLEOP2_B2:             opcode_name = TEXT("TABLEOP2"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 16; break;
        case X_CONTINUE:                opcode_name = TEXT("CONTINUE"); minbytes = 0; if(biff_version >= 8) maxbytes = 8224; else maxbytes = 2080; break;
        case X_WINDOW1:                 opcode_name = TEXT("WINDOW1"); if(biff_version >= 5) minbytes = maxbytes = 18; else  minbytes = maxbytes = 10; break;
        case X_WINDOW2_B2:              opcode_name = TEXT("WINDOW2"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 14; break;

        case X_BACKUP:                  opcode_name = TEXT("BACKUP"); minbytes = maxbytes = 2; break;
        case X_PANE:                    opcode_name = TEXT("PANE"); minbytes = maxbytes = 10; break;
        case X_CODEPAGE:                opcode_name = TEXT("CODEPAGE"); minbytes = maxbytes = 2; break;
        case X_XF_B2:                   opcode_name = TEXT("XF"); unhandled = FALSE; minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 4; break;
        case X_IXFE_B2:                 opcode_name = TEXT("IXFE"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 2; break;
        case X_EFONT_B2:                opcode_name = TEXT("EFONT"); minver = maxver = 2; BIFF_VERSION_ASSERT(== 2); minbytes = maxbytes = 2; break;
        case X_PLS:                     opcode_name = TEXT("PLS"); suppress = TRUE; minbytes = 2; break;

        case X_DCONREF:                 opcode_name = TEXT("DCONREF"); minbytes = maxbytes = 8; break;
        case X_DEFCOLWIDTH:             opcode_name = TEXT("DEFCOLWIDTH"); minbytes = maxbytes = 2; break;
        case X_BUILTINFMTCOUNT_B3_B4:   opcode_name = TEXT("BUILTINFMTCOUNT"); minver = 3; maxver = 4; BIFF_VERSION_ASSERT_BINOP(>= 3, &&, <= 4); minbytes = maxbytes = 2; break;
        case X_XCT_B3_B8:               opcode_name = TEXT("XCT"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) minbytes = maxbytes = 4; else minbytes = maxbytes = 2; break;
        case X_CRN_B3_B8:               opcode_name = TEXT("CRN"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = 5; break;
        case X_FILESHARING_B3_B8:       opcode_name = TEXT("FILESHARING"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) minbytes = 4+2+1; else minbytes = 4+1; break;
        case X_WRITEACCESS_B3_B8:       opcode_name = TEXT("WRITEACCESS"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) { minbytes = 2+1+109; } else { if(biff_version >= 5) { minbytes = 32/*seen in files, 54 from OOo doc*/; } else { minbytes = 32; } }break;
        case X_UNCALCED_B3_B8:          opcode_name = TEXT("UNCALCED"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;
        case X_SAVERECALC_B3_B8:        opcode_name = TEXT("SAVERECALC"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;

        case X_OBJECTPROTECT_B3_B8:     opcode_name = TEXT("OBJECTPROTECT"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;

        case X_COLINFO_B3_B8:           opcode_name = TEXT("COLINFO"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 12; break;

        case X_GUTS_B3_B8:              opcode_name = TEXT("GUTS"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 8; break;
        case X_WSBOOL_B3_B8:            opcode_name = TEXT("WSBOOL"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;
        case X_GRIDSET_B3_B8:           opcode_name = TEXT("GRIDSET"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;
        case X_HCENTER_B3_B8:           opcode_name = TEXT("HCENTER"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;
        case X_VCENTER_B3_B8:           opcode_name = TEXT("VCENTER"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;
        case X_BOUNDSHEET_B5_B8:        opcode_name = TEXT("BOUNDSHEET"); unhandled = FALSE; minver = 4; maxver = 8; BIFF_VERSION_ASSERT(>= 4); if(biff_version >= 8) { minbytes = 6+1+1+1; } else { if(biff_version >= 5) minbytes = 6+1+1; else minbytes = 1; } break;
        case X_WRITEPROT_B3_B8:         opcode_name = TEXT("WRITEPROT"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 0; break;
        case X_COUNTRY_B3_B8:           opcode_name = TEXT("COUNTRY"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 4; break;
        case X_HIDEOBJ_B3_B8:           opcode_name = TEXT("HIDEOBJ"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 2; break;
        case X_SHEETSOFFSET_B4:         opcode_name = TEXT("SHEETSOFFSET"); minver = maxver = 4; BIFF_VERSION_ASSERT(== 4); minbytes = maxbytes = 4; break;
        case X_SHEETHDR_B4:             opcode_name = TEXT("SHEETHDR"); minver = maxver = 4; BIFF_VERSION_ASSERT(== 4); minbytes = 4+1+1; maxbytes = 4+1+255; break;

        case X_SORT_B5_B8:              opcode_name = TEXT("SORT"); minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); if(biff_version >= 8) { minbytes = 5+1+1+1; } else { minbytes = 5+1+1; maxbytes = 5+3*255+1; } break;
        case X_PALETTE_B3_B8:           opcode_name = TEXT("PALETTE"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = 2; break;
        case X_STANDARDWIDTH_B4_B8:     opcode_name = TEXT("STANDARDWIDTH"); minver = 4; maxver = 8; BIFF_VERSION_ASSERT(>= 4); minbytes = maxbytes = 2; break;
        case X_FNGROUPCOUNT:            opcode_name = TEXT("FNGROUPCOUNT"); minbytes = maxbytes = 2; break;

        case X_SCL_B4_B8:               opcode_name = TEXT("SCL"); minver = 4; maxver = 8; BIFF_VERSION_ASSERT(>= 4); minbytes = maxbytes = 4; break;
        case X_PAGESETUP_B4_B8:         opcode_name = TEXT("PAGESETUP"); minver = 4; maxver = 8; BIFF_VERSION_ASSERT(>= 4); if(biff_version >= 5) minbytes = maxbytes = 34; else minbytes = 12; break; /* Excel sometimes gives the 34 byte version for BIFF4 */
        case X_GCW_B4_B7:               opcode_name = TEXT("GCW"); minver = 4; maxver = 7; BIFF_VERSION_ASSERT_BINOP(>= 4, &&, <= 7); minbytes = maxbytes = 34; break;

        case X_MULRK_B5_B8:             opcode_name = TEXT("MULRK"); unhandled = FALSE; minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); minbytes = 6+1*6; maxbytes = 6+256*6; break;
        case X_MULBLANK_B5_B8:          opcode_name = TEXT("MULBLANK"); suppress = TRUE; minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); minbytes = 6+1*2; maxbytes = 6+256*2; break;

        case X_MMS:                     opcode_name = TEXT("MMS"); minbytes = maxbytes = 2; break;

        case X_RSTRING_B5_B7:           opcode_name = TEXT("RSTRING"); minver = 5; maxver = 8; BIFF_VERSION_ASSERT_BINOP(>= 5, &&, <= 8); if(biff_version >= 8) minbytes = 6+2+1+1+2; else minbytes = 6+2+1+1; break; /* NB 8 for clipboard */
        case X_DBCELL_B5_B8:            opcode_name = TEXT("DBCELL"); minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); minbytes = 6; break;
        case X_BOOKBOOL_B5_B8:          opcode_name = TEXT("BOOKBOOL"); minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); minbytes = maxbytes = 2; break;
        case X_SCENPROTECT_B5_B8:       opcode_name = TEXT("SCENPROTECT"); minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); minbytes = maxbytes = 2; break;break;

        case X_XF_B5_B8:                opcode_name = TEXT("XF"); unhandled = FALSE; minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); if(biff_version >= 8) minbytes = maxbytes = 20; else minbytes = maxbytes = 16; break;
        case X_INTERFACEHDR:            opcode_name = TEXT("INTERFACEHDR"); if(biff_version >= 8) minbytes = maxbytes = 2; else minbytes = maxbytes = 0; break;
        case X_INTERFACEEND:            opcode_name = TEXT("INTERFACEEND"); minbytes = maxbytes = 0; break;
        case X_MERGEDCELLS_B8:          opcode_name = TEXT("MERGEDCELLS"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 0; maxbytes = 8224; break;
        case X_BITMAP:                  opcode_name = TEXT("BITMAP"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 21; break;
        case X_MSODRAWINGGROUP:         opcode_name = TEXT("MSODRAWINGGROUP"); minbytes = 0; break;
        case X_MSODRAWING:              opcode_name = TEXT("MSODRAWING"); minbytes = 0; break;
        case X_MSODRAWINGSELECTION:     opcode_name = TEXT("MSODRAWINGSELECTION"); minbytes = 0; break;
        case X_PHONETIC_B8:             opcode_name = TEXT("PHONETIC"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 4; break;

        case X_SST_B8:                  opcode_name = TEXT("SST"); unhandled = FALSE; minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 8; break;
        case X_LABELSST_B8:             opcode_name = TEXT("LABELSST"); unhandled = FALSE; minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = maxbytes = 10; break;
        case X_EXTSST_B8:               opcode_name = TEXT("EXTSST"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 2; break;

        case X_TABID:                   opcode_name = TEXT("TABID"); minbytes = 2; break;

        case X_LABELRANGES:             opcode_name = TEXT("LABELRANGES"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 0; break;

        case X_USESELFS_B8:             opcode_name = TEXT("USESELFS"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = maxbytes = 2; break;
        case X_DSF:                     opcode_name = TEXT("DSF B8"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 2; break;

        case X_SUPBOOK_B8:              opcode_name = TEXT("SUPBOOK"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 4; break;
        case X_PROT4REV:                opcode_name = TEXT("PROT4REV"); minbytes = maxbytes = 2; break;

        case X_CONDFMT:                 opcode_name = TEXT("CONDFMT"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 10; break;
        case X_CF:                      opcode_name = TEXT("CF B8"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 12; break;
        case X_DVAL:                    opcode_name = TEXT("DVAL"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 18; break;
        case X_REFRESHALL:              opcode_name = TEXT("REFRESHALL"); minbytes = maxbytes = 2; break;
        case X_HLINK:                   opcode_name = TEXT("HLINK"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 32; break;
        case X_PROT4REVPASS:            opcode_name = TEXT("PROT4REVPASS"); minbytes = maxbytes = 2; break;
        case X_DV:                      opcode_name = TEXT("DV"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 12; break;

        case X_EXCEL9FILE:              opcode_name = TEXT("EXCEL9FILE"); minbytes = maxbytes = 0; break;
        case X_RECALCID:                opcode_name = TEXT("RECALCID"); minbytes = maxbytes = 8; break;

        case X_DIMENSIONS_B3_B8:        opcode_name = TEXT("DIMENSIONS"); unhandled = FALSE; minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) { minbytes = maxbytes = 14; } else { minbytes = maxbytes = 10; } break;
        case X_BLANK_B3_B8:             opcode_name = TEXT("BLANK"); suppress = TRUE; minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 6; break;
        case X_NUMBER_B3_B8:            opcode_name = TEXT("NUMBER"); unhandled = FALSE; minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 14; break;
        case X_LABEL_B3_B8:             opcode_name = TEXT("LABEL"); unhandled = FALSE; minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) minbytes = 6+2+1+1; else minbytes = 6+2+1; break;
        case X_BOOLERR_B3_B8:           opcode_name = TEXT("BOOLERR"); unhandled = FALSE; minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 8; break;
        case X_FORMULA_B3:              opcode_name = TEXT("FORMULA"); unhandled = FALSE; minver = 3; maxver = 3; BIFF_VERSION_ASSERT(>= 3); minbytes = 17; break;
        case X_STRING_B3_B8:            opcode_name = TEXT("STRING"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) minbytes = 2+1+1; else minbytes = 2+0; break;
        case X_ROW_B3_B8:               opcode_name = TEXT("ROW"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 16; break;
        case X_BOF_B3:                  opcode_name = TEXT("BOF"); unhandled = FALSE; minver = maxver = 3; BIFF_VERSION_ASSERT(== 3); minbytes = maxbytes = 6; break;
        case X_INDEX_B3_B8:             opcode_name = TEXT("INDEX"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) minbytes = 16; else minbytes = 12; break;
        case X_DEFINEDNAME_B3_B4:       opcode_name = TEXT("DEFINEDNAME"); unhandled = FALSE; minver = 3; maxver = 4; BIFF_VERSION_ASSERT_BINOP(>= 3, &&, <= 4); minbytes = 6+1+1; break;
        case X_ARRAY_B3_B8:             opcode_name = TEXT("ARRAY"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 5) minbytes = 13; else minbytes = 9; break;
        case X_EXTERNNAME_B3_B4:        opcode_name = TEXT("EXTERNNAME"); unhandled = FALSE; minver = 3; maxver = 4; BIFF_VERSION_ASSERT_BINOP(>= 3, &&, <= 4); break;
        case X_DEFAULTROWHEIGHT_B3_B8:  opcode_name = TEXT("DEFAULTROWHEIGHT"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 4; break;
        case X_FONT_B3_B4:              opcode_name = TEXT("FONT"); minver = 3; maxver = 4; BIFF_VERSION_ASSERT_BINOP(>= 3, &&, <= 4); minbytes = 6+1+1; break;
        case X_TABLEOP_B3_B8:           opcode_name = TEXT("TABLEOP"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 16; break;
        case X_WINDOW2_B3_B8:           opcode_name = TEXT("WINDOW2"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); if(biff_version >= 8) minbytes = maxbytes = 18; else minbytes = maxbytes = 10; break;
        case X_XF_B3:                   opcode_name = TEXT("XF"); unhandled = FALSE; minver = maxver = 3; BIFF_VERSION_ASSERT(== 3); minbytes = maxbytes = 12; break;
        case X_RK_B3_B8:                opcode_name = TEXT("RK"); unhandled = FALSE; minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = maxbytes = 10; break;
        case X_STYLE_B3_B8:             opcode_name = TEXT("STYLE"); minver = 3; maxver = 8; BIFF_VERSION_ASSERT(>= 3); minbytes = 2+1+1; if(biff_version < 8) maxbytes = 2+1+255; break;

        case X_FORMULA_B4:              opcode_name = TEXT("FORMULA"); unhandled = FALSE; minver = maxver = 4; BIFF_VERSION_ASSERT(== 4); minbytes = 17; break;
        case X_BOF_B4:                  opcode_name = TEXT("BOF"); unhandled = FALSE; minver = maxver = 4; BIFF_VERSION_ASSERT(== 4); minbytes = maxbytes = 6; break;
        case X_FORMAT_B4_B8:            opcode_name = TEXT("FORMAT"); unhandled = FALSE; minver = 4; maxver = 8; BIFF_VERSION_ASSERT(>= 4); if(biff_version >= 8) minbytes = 2+2+1+1; else minbytes = 2+1+1; break;
        case X_XF_B4:                   opcode_name = TEXT("XF"); unhandled = FALSE; minver = maxver = 4; BIFF_VERSION_ASSERT(== 4); minbytes = maxbytes = 12; break;
        case X_SHRFMLA_B5_B8:           opcode_name = TEXT("SHRFMLA"); unhandled = FALSE; minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); minbytes = 8+1; break;

        case X_QUICKTIP_B8:             opcode_name = TEXT("QUICKTIP"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = 10+2; break;
        case X_BOF_B5_B8:               opcode_name = TEXT("BOF"); unhandled = FALSE; minver = 5; maxver = 8; BIFF_VERSION_ASSERT(>= 5); if(biff_version >= 8) minbytes = maxbytes = 16; else minbytes = maxbytes = 8; break;
        case X_SHEETLAYOUT_B8:          opcode_name = TEXT("SHEETLAYOUT"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 5); minbytes = maxbytes = 20; break;
        case X_BOOKEXT:                 opcode_name = TEXT("BOOKEXT"); minbytes = 20; maxbytes = 22; break;
        case X_SHEETPROTECTION_B8:      opcode_name = TEXT("SHEETPROTECTION"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); minbytes = maxbytes = 23; break;
        case X_RANGEPROTECTION:         opcode_name = TEXT("RANGEPROTECTION"); minver = maxver = 8; BIFF_VERSION_ASSERT(>= 8); /*minbytes???*/ break;
        case X_XFCRC:                   opcode_name = TEXT("XFCRC"); minbytes = maxbytes = 20; break;
        case X_XFEXT:                   opcode_name = TEXT("XFEXT"); minbytes = 20; break;
        case X_PLV12:                   opcode_name = TEXT("PLV12"); minbytes = maxbytes = 16; break;
        case X_COMPAT12:                opcode_name = TEXT("COMPAT12"); minbytes = 16; break;
        case X_TABLESTYLES:             opcode_name = TEXT("TABLESTYLES"); minbytes = 20; break;
        case X_STYLEEXT:                opcode_name = TEXT("STYLEEXT"); minbytes = 18; break;
        case X_THEME:                   opcode_name = TEXT("THEME"); minbytes = 16; break;
        case X_MTRSETTINGS:             opcode_name = TEXT("MTRSETTINGS"); minbytes = maxbytes = 24; break;
        case X_COMPRESSPICTURES:        opcode_name = TEXT("COMPRESSPICTURES"); minbytes = maxbytes = 16; break;
        case X_HEADERFOOTER:            opcode_name = TEXT("HEADERFOOTER"); minbytes = 38; break;
        case X_FORCEFULLCALCULATION:    opcode_name = TEXT("FORCEFULLCALCULATION"); minbytes = maxbytes = 16; break;

        case 0xBF:                      opcode_name = TEXT("UNHANDLED 0xBF"); minbytes = 0; break; /* What are these two ??? */
        case 0xC0:                      opcode_name = TEXT("UNHANDLED 0xC0"); minbytes = 0; break;
        default:                        opcode_name = TEXT("UNHANDLED"); minbytes = 0; break;
        }

        switch(opcode)
        {
        case X_INTEGER_B2:
        case X_NUMBER_B2:
        case X_LABEL_B2:
        case X_BOOLERR_B2:
            {
            XLS_ROW row;
            XLS_COL col;
            XF_INDEX xf_index;
            UCHARZ col_ustr_buf[16];
            xls_read_cell_address_r2_c2(p_x, &row, &col);
            xf_index = xls_obtain_xf_index_B2(p_xls_load_info, p_x + 4); /* cell attributes */
            consume(U32, xtos_ustr_buf(ustr_bptr(col_ustr_buf), elemof32(col_ustr_buf), (COL) col, TRUE));
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                       USTR_TEXT("%s" ROW_FMT " xf=" U32_FMT),
                                       col_ustr_buf, (ROW) row + 1, (U32) xf_index & 0x0FFF));
            break;
            }

        case X_FORMULA_B2_B5_B8:
            {
            XLS_ROW row;
            XLS_COL col;
            XF_INDEX xf_index;
            UCHARZ col_ustr_buf[16];
            xls_read_cell_address_r2_c2(p_x, &row, &col);
            if(biff_version == 2)
                xf_index = xls_obtain_xf_index_B2(p_xls_load_info, p_x + 4); /* cell attributes */
            else
                xf_index = xls_read_U16_LE(p_x + 4); /* XF index */
            consume(U32, xtos_ustr_buf(ustr_bptr(col_ustr_buf), elemof32(col_ustr_buf), (COL) col, TRUE));
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                       USTR_TEXT("%s" ROW_FMT " xf=" U32_FMT),
                                       col_ustr_buf, (ROW) row + 1, (U32) xf_index & 0x0FFF));
            break;
            }

        case X_BOOLERR_B3_B8:
        case X_NUMBER_B3_B8:
        case X_LABEL_B3_B8:
        case X_FORMULA_B3:
        case X_RK_B3_B8:
        case X_FORMULA_B4:
        case X_MULRK_B5_B8:
            {
            XLS_ROW row;
            XLS_COL col;
            XF_INDEX xf_index;
            UCHARZ col_ustr_buf[16];
            xls_read_cell_address_r2_c2(p_x, &row, &col);
            xf_index = xls_read_U16_LE(p_x + 4); /* XF index */
            consume(U32, xtos_ustr_buf(ustr_bptr(col_ustr_buf), elemof32(col_ustr_buf), (COL) col, TRUE));
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                       USTR_TEXT("%s" ROW_FMT " xf=" U32_FMT),
                                       col_ustr_buf, (ROW) row + 1, (U32) xf_index & 0x0FFF));
            break;
            }

        case X_SHRFMLA_B5_B8:
            {
            UCHARZ tl_col_ustr_buf[16];
            UCHARZ br_col_ustr_buf[16];
            XLS_ROW tl_row = (XLS_ROW) xls_read_U16_LE(p_x);
            XLS_ROW br_row = (XLS_ROW) xls_read_U16_LE(p_x + 2);
            XLS_COL tl_col = (XLS_COL) p_x[4]; /* NB only a byte, even for BIFF8 */
            XLS_COL br_col = (XLS_COL) p_x[5];
            consume(U32, xtos_ustr_buf(ustr_bptr(tl_col_ustr_buf), elemof32(tl_col_ustr_buf), (COL) tl_col, TRUE));
            consume(U32, xtos_ustr_buf(ustr_bptr(br_col_ustr_buf), elemof32(br_col_ustr_buf), (COL) br_col, TRUE));
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                       USTR_TEXT("%s" ROW_FMT ":%s" ROW_FMT),
                                       tl_col_ustr_buf, (ROW) tl_row + 1, br_col_ustr_buf, (ROW) br_row + 1));
            break;
            }

        case X_LABELSST_B8:
            {
            XLS_ROW row;
            XLS_COL col;
            U32 sst_entry;
            UCHARZ col_ustr_buf[16];
            xls_read_cell_address_r2_c2(p_x, &row, &col);
            sst_entry = xls_read_U32_LE(p_x + 6);
            consume(U32, xtos_ustr_buf(ustr_bptr(col_ustr_buf), elemof32(col_ustr_buf), (COL) col, TRUE));
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                       USTR_TEXT("%s" ROW_FMT " SST:" U32_FMT),
                                       col_ustr_buf, (ROW) row + 1, (U32) sst_entry));
            break;
            }

        case X_EXTERNSHEET:
            {
            if(biff_version >= 8)
            {
                U32 nm = xls_read_U16_LE(p_x);
                PC_BYTE p_ref = p_x + 2;
                U32 i;
                for(i = 0; i < nm; ++i, p_ref += 6)
                {
                }
            }
            else
            {
                U32 name_len;
                PC_BYTE p_name;
                QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
                quick_ublock_with_buffer_setup(quick_ublock);

                name_len = PtrGetByteOff(p_x, 0); /* chars */
                p_name = p_x + 1;

                switch(PtrGetByte(p_name))
                {
                case 0x03:
                    p_name++;
                    status_assert(xls_quick_ublock_sbchars_add(&quick_ublock, (PC_SBCHARS) p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE));
                    break;

                default:
                    break;
                }

                ustr_xstrnkpy(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

                quick_ublock_dispose(&quick_ublock);
            }
            break;
            }

        case X_DEFINEDNAME_B2_B5_B8:
        case X_DEFINEDNAME_B3_B4:
            {
            U32 name_len;
            PC_BYTE p_name;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            name_len = p_x[3];

            if(X_DEFINEDNAME_B3_B4 == opcode)
            {
                p_name = p_x + 6;
            }
            else /* (X_DEFINEDNAME_B2_B5_B8 == opcode) */
            {
                if(biff_version >= 5)
                {
                    p_name = p_x + 14;
                }
                else /* (biff_version == 2) */
                {
                    p_name = p_x + 5;
                }
            }

            status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

            ustr_xstrnkpy(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case X_EXTERNNAME_B2_B5_B8:
            {
            U32 name_len;
            PC_BYTE p_name;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            if(biff_version >= 5)
            {
                name_len = PtrGetByteOff(p_x, 6); /* chars */
                p_name = p_x + 7;
            }
            else /* (biff_version == 2) */
            {
                name_len = PtrGetByteOff(p_x, 0);
                p_name = p_x + 1;
            }

            status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

            ustr_xstrnkpy(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case X_DIMENSIONS_B2:
        case X_DIMENSIONS_B3_B8:
            {
            XLS_COL s_col, e_col;
            XLS_ROW s_row, e_row;
            if(biff_version >= 8)
            {
                s_row = (XLS_ROW) xls_read_U32_LE(p_x);
                e_row = (XLS_ROW) xls_read_U32_LE(p_x + 4);
                s_col = (XLS_COL) xls_read_U16_LE(p_x + 8);
                e_col = (XLS_COL) xls_read_U16_LE(p_x + 10);
            }
            else /* (biff_version < 8) */
            {
                s_row = (XLS_ROW) xls_read_U16_LE(p_x);
                e_row = (XLS_ROW) xls_read_U16_LE(p_x + 2);
                s_col = (XLS_COL) xls_read_U16_LE(p_x + 4);
                e_col = (XLS_COL) xls_read_U16_LE(p_x + 6);
            }
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("rows=" U32_FMT ".." U32_FMT ", cols=" U32_FMT ".." U32_FMT), (U32) s_row, (U32) e_row, (U32) s_col, (U32) e_col));
            break;
            }

        case X_BOUNDSHEET_B5_B8:
            {
            U32 name_len;
            PC_BYTE p_name;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            if(biff_version >= 5)
            {
                name_len = p_x[6];
                p_name = p_x + 7;
            }
            else /* (biff_version == 4) */ /* Seen in BIFF4W created by Excel */
            {
                name_len = p_x[0];
                p_name = p_x + 1;
            }

            status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

            ustr_xstrnkpy(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case X_SHEETHDR_B4:
            {
            U32 name_len;
            PC_BYTE p_name;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                       USTR_TEXT("substream bytes=" U32_FMT " "),
                                       xls_read_U32_LE(p_x)));

            name_len = p_x[4];
            p_name = p_x + 5;

            status_assert(xls_quick_ublock_sbchars_add(&quick_ublock, (PC_SBCHARS) p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE));

            ustr_xstrnkat(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case X_FONT_B2_B5_B8:
        case X_FONT_B3_B4:
            {
            U16 height_twips = xls_read_U16_LE(p_x);
            U16 option_flags = xls_read_U16_LE(p_x + 2);
            U16 colour_index = xls_read_U16_LE(p_x + 4);
            U32 font_len;
            PC_BYTE p_font;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            if(biff_version >= 5)
            {
                font_len = p_x[14];
                p_font = p_x + 15;
            }
            else if(biff_version >= 3)
            {
                font_len = p_x[6];
                p_font = p_x + 7;
            }
            else /* (biff_version < 3) */
            {
                font_len = p_x[4];
                p_font = p_x + 5;
            }

            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                       USTR_TEXT("[" U32_FMT "] "),
                                       (U32) (this_record_idx >= 4) ? (this_record_idx + 1) : this_record_idx));

            status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_font, font_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

            status_assert(quick_ublock_printf(&quick_ublock,
                                              USTR_TEXT(", height=" F64_FMT ", option=" U32_XFMT ", colour=" U32_FMT),
                                              (F64) height_twips / 20.0, (U32) option_flags, (U32) colour_index));

            ustr_xstrnkat(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

            if(option_flags & 0x01)
                ustr_xstrkat(ustr_bptr(extra_data), elemof32(extra_data), USTR_TEXT(" Bold"));

            if(option_flags & 0x02)
                ustr_xstrkat(ustr_bptr(extra_data), elemof32(extra_data), USTR_TEXT(" Italic"));

            if(option_flags & 0xFC)
                ustr_xstrkat(ustr_bptr(extra_data), elemof32(extra_data), USTR_TEXT(" Unsupported options"));

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case X_FORMAT_B2_B3:
        case X_FORMAT_B4_B8:
            {
            U32 format_len;
            PC_BYTE p_format;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            if(biff_version >= 5)
            {
                consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                           USTR_TEXT("[" U32_FMT "] "),
                                           (U32) xls_read_U16_LE(p_x)));
            }
            else
            {
                consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                                           USTR_TEXT("[" U32_FMT "] "),
                                           (U32) this_record_idx));
            }

            if(biff_version >= 8)
            {
                format_len = xls_read_U16_LE(p_x + 2);
                p_format = p_x + 4;
            }
            else if(biff_version >= 4)
            {
                format_len = p_x[2];
                p_format = p_x + 3;
            }
            else /* (biff_version < 4) */
            {
                format_len = p_x[0];
                p_format = p_x + 1;
            }

            status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_format, format_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

            ustr_xstrnkat(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case X_IXFE_B2:
            {
            XF_INDEX real_xf_index = xls_read_U16_LE(p_x);
            p_xls_load_info->biff2_xf_index = real_xf_index;
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(U32_FMT), (U32) real_xf_index));
            break;
            }

        case X_XF_B2:
            {
            FONT_INDEX font_index;
            FORMAT_INDEX format_index;
            font_index   = (FONT_INDEX) p_x[0];
            format_index = (FORMAT_INDEX) (p_x[2] & 0x3F);
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("[" U32_FMT "] font=" U32_FMT ", format=" U32_FMT),
                        (U32) this_record_idx, (U32) font_index, (U32) format_index));
            break;
            }

        case X_XF_B3:
        case X_XF_B4:
        case X_XF_B5_B8:
            {
            FONT_INDEX font_index;
            FORMAT_INDEX format_index;
            XF_INDEX parent_xf_index;
            U8 xf_attrib; /* which attributes are valid in this record? */

            if(biff_version >= 5)
            {
                font_index      = xls_read_U16_LE(p_x);
                format_index    = xls_read_U16_LE(p_x + 2);
                parent_xf_index = xls_read_U16_LE(p_x + 4) >> 4;
                if(biff_version >= 8)
                    xf_attrib = p_x[9];
                else /*if(biff_version == 5)*/
                    xf_attrib = p_x[7];
            }
            else if(biff_version == 4)
            {
                font_index      = (FONT_INDEX) p_x[0];
                format_index    = (FORMAT_INDEX) p_x[1];
                parent_xf_index = xls_read_U16_LE(p_x + 2) >> 4;
                xf_attrib = p_x[5];
            }
            else /* (biff_version == 3) */
            {
                font_index      = (FONT_INDEX) p_x[0];
                format_index    = (FORMAT_INDEX) p_x[1];
                parent_xf_index = xls_read_U16_LE(p_x + 4) >> 4;
                xf_attrib = p_x[3];
            }
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("[" U32_FMT "] font=" U32_FMT ", format=" U32_FMT ", parent=" U32_FMT ", attr=0x%02X"),
                        (U32) this_record_idx, (U32) font_index, (U32) format_index, (U32) parent_xf_index, (U32) xf_attrib));
            break;
            }

        case X_STYLE_B3_B8:
            {
            XF_INDEX xf_index = xls_read_U16_LE(p_x);
            U32 name_len;
            PC_BYTE p_name;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("xf=" U32_FMT " "), (U32) xf_index & 0x0FFF));

            if(0 == (0x8000 & xf_index))
            {
                if(biff_version >= 8)
                {
                    name_len = xls_read_U16_LE(p_x + 2); /* chars */
                    p_name = p_x + 4;
                }
                else /* (biff_version >= 3) */
                {
                    name_len = PtrGetByteOff(p_x, 2);
                    p_name = p_x + 3;
                }

                status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

                ustr_xstrnkat(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));
            }
            else
            {
            }

            quick_ublock_dispose(&quick_ublock);
            break;
            }

        case X_DEFCOLWIDTH:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("chars=" U32_FMT), (U32) xls_read_U16_LE(p_x)));
            break;

        case X_STANDARDWIDTH_B4_B8:
            {
            const U16 width_in_chars_x_256 = xls_read_U16_LE(p_x + 4);
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("chars=" F64_FMT), (U32) width_in_chars_x_256 / 256.0));
            break;
            }

        case X_COLINFO_B3_B8:
            {
            const U16 first_column = xls_read_U16_LE(p_x);
            const U16 last_column = xls_read_U16_LE(p_x + 2);
            const U16 width_in_chars_x_256 = xls_read_U16_LE(p_x + 4);
            if(first_column == last_column)
                consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                            USTR_TEXT("col=" U32_FMT               ", chars=" F64_FMT), (U32) first_column,                    (F64) width_in_chars_x_256 / 256.0));
            else
                consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                            USTR_TEXT("cols=" U32_FMT ".." U32_FMT ", chars=" F64_FMT), (U32) first_column, (U32) last_column, (F64) width_in_chars_x_256 / 256.0));
            break;
            }

        case X_DEFAULTROWHEIGHT_B2:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("height=" U32_FMT), (U32) xls_read_U16_LE(p_x)));
            break;

        case X_DEFAULTROWHEIGHT_B3_B8:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT("height=" U32_FMT), (U32) xls_read_U16_LE(p_x + 2)));
            break;

        case X_ROW_B2:
        case X_ROW_B3_B8:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(U32_FMT ", cols=" U32_FMT ".." U32_FMT ", height=" U32_FMT), (U32) xls_read_U16_LE(p_x), (U32) xls_read_U16_LE(p_x + 2), (U32) xls_read_U16_LE(p_x + 4), (U32) xls_read_U16_LE(p_x + 6)));
            break;

        case X_CALCCOUNT:
        case X_CALCMODE:
        case X_PRECISION:
        case X_REFMODE:
        case X_ITERATION:
        case X_PROTECT:
        case X_EXTERNCOUNT_B2_B7:
        case X_WINDOWPROTECT:
        case X_VERTICALPAGEBREAKS:
        case X_HORIZONTALPAGEBREAKS:
        case X_BUILTINFMTCOUNT_B2:
        case X_BUILTINFMTCOUNT_B3_B4:
        case X_XCT_B3_B8:
        case X_PRINTHEADERS:
        case X_PRINTGRIDLINES:
        case X_BACKUP:
        case X_CODEPAGE:
        case X_UNCALCED_B3_B8:
        case X_SAVERECALC_B3_B8:
        case X_OBJECTPROTECT_B3_B8:
        case X_GRIDSET_B3_B8:
        case X_HCENTER_B3_B8:
        case X_VCENTER_B3_B8:
        case X_HIDEOBJ_B3_B8:
        case X_PALETTE_B3_B8:
        case X_BOOKBOOL_B5_B8:
        case X_SCENPROTECT_B5_B8:
        case X_USESELFS_B8:
        case X_DSF:
        case X_REFRESHALL:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(U32_FMT), (U32) xls_read_U16_LE(p_x)));
            break;

        case X_PASSWORD:
        case X_WSBOOL_B3_B8:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(U32_XFMT), (U32) xls_read_U16_LE(p_x)));
            break;

        case X_COUNTRY_B3_B8:
        case X_SCL_B4_B8:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(U32_FMT "," U32_FMT), (U32) xls_read_U16_LE(p_x), (U32) xls_read_U16_LE(p_x + 2)));
            break;

        case X_DATEMODE:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(U32_FMT), (1 == xls_read_U16_LE(p_x) ? 1904U : 1900U)));
            break;

        case X_DELTA:
        case X_LEFTMARGIN:
        case X_RIGHTMARGIN:
        case X_TOPMARGIN:
        case X_BOTTOMMARGIN:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(F64_FMT), xls_read_F64(p_x)));
            break;

        case X_BOF_B2:
        case X_BOF_B3:
        case X_BOF_B4:
        case X_BOF_B5_B8:
            consume_int(ustr_xsnprintf(ustr_bptr(extra_data), elemof32(extra_data),
                        USTR_TEXT(U32_XFMT), (U32) xls_read_U16_LE(p_x + 2)));
            break;

        case X_WRITEACCESS_B3_B8:
            {
            U32 name_len;
            PC_BYTE p_name;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            if(biff_version >= 8)
            {
                name_len = xls_read_U16_LE(p_x); /* chars */
                p_name = p_x + 2;
            }
            else /* (biff_version >= 3) */
            {
                name_len = PtrGetByteOff(p_x, 0);
                p_name = p_x + 1;
            }

            status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

            ustr_xstrnkat(ustr_bptr(extra_data), elemof32(extra_data), quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

            quick_ublock_dispose(&quick_ublock);
            break;
            }
        }

        if((0 != minbytes) && (record_length < minbytes))
        {
            extra_text = TEXT(" (*** too small ***)");
            suppress = FALSE;
        }
        else if(record_length > maxbytes)
        {
            extra_text = TEXT(" (*** too big ***)");
            suppress = FALSE;
        }
        /* else 'just right' */
#endif /* limited reporting */

        if(!suppress)
        {
            if((minver <= biff_version) && (biff_version <= maxver))
                tracef(TRACE__XLS_LOADB, TEXT("%6u@0x%.5X: op=0x%.3X len=%4u%s %s %s"),
                       overall_record_idx, opcode_offset, (U32) opcode, (U32) record_length,
                       extra_text ? report_tstr(extra_text) : tstr_empty_string,
                       report_tstr(opcode_name),
                       extra_data[0] ? report_ustr(ustr_bptr(extra_data)) : tstr_empty_string);
            else
                tracef(TRACE__XLS_LOADB, TEXT("%6u@0x%.5X: op=0x%.3X len=%4u%s %s B%u-B%u %s"),
                       overall_record_idx, opcode_offset, (U32) opcode, (U32) record_length,
                       extra_text ? report_tstr(extra_text) : tstr_empty_string,
                       report_tstr(opcode_name),
                       minver, maxver,
                       extra_data[0] ? report_ustr(ustr_bptr(extra_data)) : tstr_empty_string);
        }

        if(X_EOF == opcode)
        {
            tracef(TRACE__XLS_LOADB, TEXT("---"));
        }
    }
}

#else

#define xls_dump_records(a) /*nothing*/

#endif /* TRACE_ALLOWED */

/******************************************************************************
*
* if there is a FILEPASS record, decrypt the whole file
*
* See OpenOffice.org's Documentation of the Microsoft Excel File Format v1.42
*
******************************************************************************/

/* the get password box has OK, Cancel and Enter password boxes */

enum PSWD_CONTROL_IDS
{
    PSWD_ID_EDIT = 137,
    PSWD_ID_MESS
};

static const DIALOG_CONTROL
pswd_edit =
{
    PSWD_ID_EDIT, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_SYSCHARSL_H(40), DIALOG_STDEDIT_V },
    { DRT(LTLT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
pswd_edit_data= { { { FRAMED_BOX_EDIT } }, { UI_TEXT_TYPE_NONE } };

static const DIALOG_CTL_CREATE
pswd_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &pswd_edit },        &pswd_edit_data },

    { { &defbutton_ok },     &defbutton_ok_data },
    { { &stdbutton_cancel }, &stdbutton_cancel_data }
};

static UI_TEXT ui_text_edit;

_Check_return_
static STATUS
dialog_pswd_process_end(
    _InRef_     PC_DIALOG_MSG_PROCESS_END p_dialog_msg_process_end)
{
    if(status_ok(p_dialog_msg_process_end->completion_code))
    {
        ui_dlg_get_edit(p_dialog_msg_process_end->h_dialog, (DIALOG_CONTROL_ID) PSWD_ID_EDIT, &ui_text_edit);
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_pswd)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_END:
        return(dialog_pswd_process_end((PC_DIALOG_MSG_PROCESS_END) p_data));

    default:
        return(STATUS_OK);
    }
}

#if 1

_Check_return_
static STATUS
xls_get_password(
    _DocuRef_   P_DOCU p_docu,
    _Out_writes_z_(elemof_password) P_U8Z password,
    _InRef_     U32 elemof_password)
{
    STATUS status;

    *password = CH_NULL;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, pswd_ctl_create, elemof32(pswd_ctl_create), XLS_MSG_PASSWORD_CAPTION);
  /*dialog_cmd_process_dbox.help_topic_resource_id = 0;*/
    dialog_cmd_process_dbox.p_proc_client = dialog_event_pswd;
  /*dialog_cmd_process_dbox.client_handle = NULL;*/
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    if(status_ok(status))
    {
        PCTSTR tstr = ui_text_tstr(&ui_text_edit);
        U32 length = tstrlen32(tstr);

        if(length >= elemof_password)
        {
            myassert0(TEXT("Password is too long"));
            status = STATUS_FAIL;
        }
        else
            xstrkpy(password, elemof_password, _sbstr_from_tstr(tstr));

        SecureZeroMemory(de_const_cast(PTSTR, tstr), length * sizeof32(TCHAR));
    }

    return(status);
}

#else

static void
xls_get_password(
    _DocuRef_   P_DOCU p_docu,
    _Out_writes_z_(elemof_password) P_U8Z password,
    _InRef_     U32 elemof_password)
{
    xstrkpy(password, elemof_password, "abcdefghij");
}

#endif

_Check_return_
static U16
xls_get_password_hash(
    _In_z_      PC_U8Z password)
{
    U32 char_count = strlen32(password);
    U32 char_index = char_count;
    U32 hash = 0;

    if(char_count == 0)
        return(0);

    do  {
        U8 ch = password[--char_index];

        hash = hash ^ (U32) ch; /* hash with this char */

        hash <<= 1; /* rotate the bottom 15 bits left by one */
        if(hash & 0x8000U)
        {
            hash &= ~0x8000U;
            hash |= 1;
        }
    }
    while(char_index != 0);

    hash = hash ^ char_count ^ 0xCE4B;

    /* hash of "abcdefghij" is 0xFEF1 */
    return((U16) hash);
}

static void
xls_get_key_sequence_XOR(
    /*_Out_[16]*/ P_BYTE key_seq,
    _In_z_      PC_U8Z password,
    _InRef_     U16 key)
{
    static const BYTE tail_bytes[15] = { 0xBB, 0xFF, 0xFF, 0xBA, 0xFF, 0xFF, 0xB9, 0x80, 0x00, 0xBE, 0x0F, 0x00, 0xBF, 0x0F, 0x00 };

    U8 key_lower = LOBYTE(key);
    U8 key_upper = HIBYTE(key);
    U32 char_count = strlen32(password);
    U32 seq_index;

    assert(char_count != 0);
    memcpy32(key_seq, password, char_count); /* copy the first char_count bytes of password into key_seq */
    memcpy32(key_seq + char_count, tail_bytes, 16 - char_count); /* append (16 - char_count) bytes of tail_bytes */

    for(seq_index = 0; seq_index < 16; seq_index += 2)
    {
        key_seq[seq_index    ] = key_seq[seq_index    ] ^ key_lower;
        key_seq[seq_index + 1] = key_seq[seq_index + 1] ^ key_upper;
    }

    /* rotate all bytes of key_seq left by 2 bits */
    for(seq_index = 0; seq_index < 16; ++seq_index)
    {
        BYTE key_byte = key_seq[seq_index];
        BYTE key_byte_rotate_left_two = (BYTE) ((key_byte >> (8-2)) | (key_byte << 2));
        key_seq[seq_index] = key_byte_rotate_left_two;
    }
} 

static void
xls_decrypt_record_content(
    _In_/*[16]*/ PC_BYTE key_seq,
    _InRef_     U32 stream_offset,
    _Inout_updates_(record_length) P_BYTE p_x,
    _InRef_     U32 record_length,
    _InRef_     U32 decrypt_start)
{
    U32 key_index = (stream_offset + decrypt_start + record_length) & 0x0F;
    U32 record_index;

    for(record_index = decrypt_start; record_index < record_length; ++record_index)
    {
        BYTE data_byte = p_x[record_index];
        BYTE data_byte_rotate_left_three = (BYTE) ((data_byte >> (8-3)) | (data_byte << 3));
        BYTE new_byte = data_byte_rotate_left_three ^ key_seq[key_index];
        p_x[record_index] = new_byte;
        key_index = (key_index + 1) & 0x0F; /* rotate inside key_seq[] */
    }
}

_Check_return_
static STATUS
xls_decrypt_file(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    U32 decrypt_opcode_offset = XLS_NO_RECORD_FOUND;
    U16 key;
    U16 filepass_hash;
    BYTE key_seq[16];

    { /* is there a FILEPASS record? */
    U16 record_length;
    U32 opcode_offset = xls_find_record_first(p_xls_load_info, 0, X_FILEPASS, &record_length);
    PC_BYTE p_x;

    if(XLS_NO_RECORD_FOUND == opcode_offset)
        return(STATUS_OK);

    p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

    if(biff_version < 8)
    {
        key = xls_read_U16_LE(p_x + 0);
        filepass_hash = xls_read_U16_LE(p_x + 2);
    }
    else
    {
        if(0 == xls_read_U16_LE(p_x + 0))
        {
            key = xls_read_U16_LE(p_x + 2);
            filepass_hash = xls_read_U16_LE(p_x + 4);
        }
        else
        {
            return(XLS_ERR_BADFILE_ENCRYPTION);
        }
    }

    /* skip the FILEPASS record */
    decrypt_opcode_offset = opcode_offset +  ((U32) record_length + 4 /*opcode,record_length*/);
    } /*block*/

    { /* see if we can obtain a matching password from the user */
    U8 password[16];
    U16 hash;

    status_return(xls_get_password(p_xls_load_info->p_docu, password, elemof32(password)));

    xls_get_key_sequence_XOR(key_seq, password, key);

    hash = xls_get_password_hash(password);

    SecureZeroMemory(password, sizeof32(password));

    if(hash != filepass_hash)
        return(XLS_ERR_BADFILE_PASSWORD_MISMATCH);
    } /*block*/

    { /* process the whole file from that point on */
    U32 opcode_offset;
    U16 record_length = 0;

    for(opcode_offset = decrypt_opcode_offset /*start_offset*/;
        opcode_offset + 4 /*opcode,record_length*/ <= p_xls_load_info->file_end_offset;
        opcode_offset += ((U32) record_length + 4 /*opcode,record_length*/))
    {
        const XLS_OPCODE opcode = xls_read_record_header(p_xls_load_info, opcode_offset, &record_length);
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        if((0 == opcode) && (0 == record_length))
        {   /* some writers pad from EOF to end of stream with CH_NULL (including Excel 'Simple Save' which pads to 4k) */
            continue;
        }

        switch(opcode)
        {   /* There are a few records that are never encrypted */
        case X_BOF_B2:
        case X_BOF_B3:
        case X_BOF_B4:
        case X_BOF_B5_B8:
        case X_INTERFACEHDR:
            break;

            /* There are a few data fields that are never encrypted */
        case X_BOUNDSHEET_B5_B8:
            /* First 4 bytes are the absolute stream position of the BOF record: this field is never encrypted in protected files */
            assert(record_length > 4);
            xls_decrypt_record_content(key_seq, opcode_offset + 4 /*opcode,record_length*/, de_const_cast(P_BYTE, p_x), record_length, 4 /*field*/);
            break;

        default:
            if(0 != record_length)
                xls_decrypt_record_content(key_seq, opcode_offset + 4 /*opcode,record_length*/, de_const_cast(P_BYTE, p_x), record_length, 0);
            break;
        }
    }
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* find the PALETTE record and populate the colour table
*
******************************************************************************/

static RGB
xls_colour_table[0x40] =
{
    /*00H*/ RGB_INIT(0x00, 0x00, 0x00), /* EGA Black */
    /*01H*/ RGB_INIT(0xFF, 0xFF, 0xFF), /* EGA White */
    /*02H*/ RGB_INIT(0xFF, 0x00, 0x00), /* EGA Red */
    /*03H*/ RGB_INIT(0x00, 0xFF, 0x00), /* EGA Green */
    /*04H*/ RGB_INIT(0x00, 0x00, 0xFF), /* EGA Blue */
    /*05H*/ RGB_INIT(0xFF, 0xFF, 0x00), /* EGA Yellow */
    /*06H*/ RGB_INIT(0xFF, 0x00, 0xFF), /* EGA Magenta */
    /*07H*/ RGB_INIT(0x00, 0xFF, 0xFF)  /* EGA Cyan */
};

/*
18H (BIFF3-BIFF4)
40H (BIFF5-BIFF8)
System window text colour for border lines (used in records XF, CFRULE, and WINDOW2 (BIFF8 only))

19H (BIFF3-BIFF4)
41H (BIFF5-BIFF8)
System window background colour for pattern background (used in records XF, and CFRULE)

43H System face colour (dialogue background colour)
4DH System window text colour for chart border lines
4EH System window background colour for chart areas
4FH Automatic colour for chart border lines (seems to be always Black)
50H System tool tip background colour (used in note objects)
51H System tool tip text colour (used in note objects)
7FFFH System window text colour for fonts (used in records FONT, FONTCOLOR, and CFRULE)
*/

_Check_return_
static BOOL
rgb_from_colour_index(
    _OutRef_    P_RGB p_rgb,
    _InVal_     U16 colour_index)
{
    if(biff_version >= 5)
    {
        if(colour_index < 0x40)
        {
            *p_rgb = xls_colour_table[colour_index];
            return(TRUE);
        }
    }
    else /* BIFF3/4 */
    {
        if(colour_index < 0x18)
        {
            *p_rgb = xls_colour_table[colour_index];
            return(TRUE);
        }
    }

    rgb_set(p_rgb, 0, 0, 0);
    return(FALSE);
}

/* Default colour table for BIFF5 (colours 08H�17H are equal to the BIFF3/BIFF4 default colour table) */

static const RGB
xls_colour_table_BIFF5[0x40 - 0x08] =
{
    /*08H*/ RGB_INIT(0x00, 0x00, 0x00), /* EGA Black */
    /*09H*/ RGB_INIT(0xFF, 0xFF, 0xFF), /* EGA White */
    /*0AH*/ RGB_INIT(0xFF, 0x00, 0x00), /* EGA Red */
    /*0BH*/ RGB_INIT(0x00, 0xFF, 0x00), /* EGA Green */
    /*0CH*/ RGB_INIT(0x00, 0x00, 0xFF), /* EGA Blue */
    /*0DH*/ RGB_INIT(0xFF, 0xFF, 0x00), /* EGA Yellow */
    /*0EH*/ RGB_INIT(0xFF, 0x00, 0xFF), /* EGA Magenta */
    /*0FH*/ RGB_INIT(0x00, 0xFF, 0xFF), /* EGA Cyan */

    /*10H*/ RGB_INIT(0x80, 0x00, 0x00), /* EGA Dark Red */
    /*11H*/ RGB_INIT(0x00, 0x80, 0x00), /* EGA Dark Green */
    /*12H*/ RGB_INIT(0x00, 0x00, 0x80), /* EGA Dark Blue */
    /*13H*/ RGB_INIT(0x80, 0x80, 0x00), /* EGA Olive */
    /*14H*/ RGB_INIT(0x80, 0x00, 0x80), /* EGA Purple */
    /*15H*/ RGB_INIT(0x00, 0x80, 0x80), /* EGA Teal */
    /*16H*/ RGB_INIT(0xC0, 0xC0, 0xC0), /* EGA Silver */
    /*17H*/ RGB_INIT(0x80, 0x80, 0x80), /* EGA Grey */

    /*18H*/ RGB_INIT(0x80, 0x80, 0xFF), 
    /*19H*/ RGB_INIT(0x80, 0x20, 0x60), 
    /*1AH*/ RGB_INIT(0xFF, 0xFF, 0xC0), 
    /*1BH*/ RGB_INIT(0xA0, 0xE0, 0xF0), 
    /*1CH*/ RGB_INIT(0x60, 0x00, 0x80), 
    /*1DH*/ RGB_INIT(0xFF, 0x80, 0x80), 
    /*1EH*/ RGB_INIT(0x00, 0x80, 0xC0),  
    /*1FH*/ RGB_INIT(0xC0, 0xC0, 0xFF),  

    /*20H*/ RGB_INIT(0x00, 0x00, 0x80),
    /*21H*/ RGB_INIT(0xFF, 0x00, 0xFF),
    /*22H*/ RGB_INIT(0xFF, 0xFF, 0x00),
    /*23H*/ RGB_INIT(0x00, 0xFF, 0xFF),
    /*24H*/ RGB_INIT(0x80, 0x00, 0x80),
    /*25H*/ RGB_INIT(0x80, 0x00, 0x00),
    /*26H*/ RGB_INIT(0x00, 0x80, 0x80),
    /*27H*/ RGB_INIT(0x00, 0x00, 0xFF),

    /*28H*/ RGB_INIT(0x00, 0xCF, 0xFF),
    /*29H*/ RGB_INIT(0x69, 0xFF, 0xFF),
    /*2AH*/ RGB_INIT(0xE0, 0xFF, 0xE0),
    /*2BH*/ RGB_INIT(0xFF, 0xFF, 0x80),
    /*2CH*/ RGB_INIT(0xA6, 0xCA, 0xF0),
    /*2DH*/ RGB_INIT(0xDD, 0x9C, 0xB3),
    /*2EH*/ RGB_INIT(0xB3, 0x8F, 0xEE),
    /*2FH*/ RGB_INIT(0xE3, 0xE3, 0xE3),

    /*30H*/ RGB_INIT(0x2A, 0x6F, 0xF9),
    /*31H*/ RGB_INIT(0x3F, 0xB8, 0xCD),
    /*32H*/ RGB_INIT(0x48, 0x84, 0x36),
    /*33H*/ RGB_INIT(0x95, 0x8C, 0x41),
    /*34H*/ RGB_INIT(0x8E, 0x5E, 0x42),
    /*35H*/ RGB_INIT(0xA0, 0x62, 0x7A),
    /*36H*/ RGB_INIT(0x62, 0x4F, 0xAC),
    /*37H*/ RGB_INIT(0x96, 0x96, 0x96),

    /*38H*/ RGB_INIT(0x1D, 0x2F, 0xBE),
    /*39H*/ RGB_INIT(0x28, 0x66, 0x76),
    /*3AH*/ RGB_INIT(0x00, 0x45, 0x00),
    /*3BH*/ RGB_INIT(0x45, 0x3E, 0x01),
    /*3CH*/ RGB_INIT(0x6A, 0x28, 0x13),
    /*3DH*/ RGB_INIT(0x85, 0x39, 0x6A),
    /*3EH*/ RGB_INIT(0x4A, 0x32, 0x85),
    /*3FH*/ RGB_INIT(0x42, 0x42, 0x42)
};

/* Default colour table for BIFF8 (colours 08H�17H are equal to the BIFF3/BIFF4 default colour table) */

static const RGB
xls_colour_table_BIFF8[0x40 - 0x08] =
{
    /*08H*/ RGB_INIT(0x00, 0x00, 0x00), /* EGA Black */
    /*09H*/ RGB_INIT(0xFF, 0xFF, 0xFF), /* EGA White */
    /*0AH*/ RGB_INIT(0xFF, 0x00, 0x00), /* EGA Red */
    /*0BH*/ RGB_INIT(0x00, 0xFF, 0x00), /* EGA Green */
    /*0CH*/ RGB_INIT(0x00, 0x00, 0xFF), /* EGA Blue */
    /*0DH*/ RGB_INIT(0xFF, 0xFF, 0x00), /* EGA Yellow */
    /*0EH*/ RGB_INIT(0xFF, 0x00, 0xFF), /* EGA Magenta */
    /*0FH*/ RGB_INIT(0x00, 0xFF, 0xFF), /* EGA Cyan */

    /*10H*/ RGB_INIT(0x80, 0x00, 0x00), /* EGA Dark Red */
    /*11H*/ RGB_INIT(0x00, 0x80, 0x00), /* EGA Dark Green */
    /*12H*/ RGB_INIT(0x00, 0x00, 0x80), /* EGA Dark Blue */
    /*13H*/ RGB_INIT(0x80, 0x80, 0x00), /* EGA Olive */
    /*14H*/ RGB_INIT(0x80, 0x00, 0x80), /* EGA Purple */
    /*15H*/ RGB_INIT(0x00, 0x80, 0x80), /* EGA Teal */
    /*16H*/ RGB_INIT(0xC0, 0xC0, 0xC0), /* EGA Silver */
    /*17H*/ RGB_INIT(0x80, 0x80, 0x80), /* EGA Grey */

    /*18H*/ RGB_INIT(0x99, 0x99, 0xFF),
    /*19H*/ RGB_INIT(0x99, 0x33, 0x66),
    /*1AH*/ RGB_INIT(0xFF, 0xFF, 0xCC),
    /*1BH*/ RGB_INIT(0xCC, 0xFF, 0xFF),
    /*1CH*/ RGB_INIT(0x66, 0x00, 0x66),
    /*1DH*/ RGB_INIT(0xFF, 0x80, 0x80),
    /*1EH*/ RGB_INIT(0x00, 0x66, 0xCC),
    /*1FH*/ RGB_INIT(0xCC, 0xCC, 0xFF),

    /*20H*/ RGB_INIT(0x00, 0x00, 0x80),
    /*21H*/ RGB_INIT(0xFF, 0x00, 0xFF),
    /*22H*/ RGB_INIT(0xFF, 0xFF, 0x00),
    /*23H*/ RGB_INIT(0x00, 0xFF, 0xFF),
    /*24H*/ RGB_INIT(0x80, 0x00, 0x80),
    /*25H*/ RGB_INIT(0x80, 0x00, 0x00),
    /*26H*/ RGB_INIT(0x00, 0x80, 0x80),
    /*27H*/ RGB_INIT(0x00, 0x00, 0xFF),

    /*28H*/ RGB_INIT(0x00, 0xCC, 0xFF),
    /*29H*/ RGB_INIT(0xCC, 0xFF, 0xFF),
    /*2AH*/ RGB_INIT(0xCC, 0xFF, 0xCC),
    /*2BH*/ RGB_INIT(0xFF, 0xFF, 0x99),
    /*2CH*/ RGB_INIT(0x99, 0xCC, 0xFF),
    /*2DH*/ RGB_INIT(0xFF, 0x99, 0xCC),
    /*2EH*/ RGB_INIT(0xCC, 0x99, 0xFF),
    /*2FH*/ RGB_INIT(0xFF, 0xCC, 0x99),

    /*30H*/ RGB_INIT(0x33, 0x66, 0xFF),
    /*31H*/ RGB_INIT(0x33, 0xCC, 0xCC),
    /*32H*/ RGB_INIT(0x99, 0xCC, 0x00),
    /*33H*/ RGB_INIT(0xFF, 0xCC, 0x00),
    /*34H*/ RGB_INIT(0xFF, 0x99, 0x00),
    /*35H*/ RGB_INIT(0xFF, 0x66, 0x00),
    /*36H*/ RGB_INIT(0x66, 0x66, 0x99),
    /*37H*/ RGB_INIT(0x96, 0x96, 0x96),

    /*38H*/ RGB_INIT(0x00, 0x33, 0x66),
    /*39H*/ RGB_INIT(0x33, 0x99, 0x66),
    /*3AH*/ RGB_INIT(0x00, 0x33, 0x00),
    /*3BH*/ RGB_INIT(0x33, 0x33, 0x00),
    /*3CH*/ RGB_INIT(0x99, 0x33, 0x00),
    /*3DH*/ RGB_INIT(0x99, 0x33, 0x66),
    /*3EH*/ RGB_INIT(0x33, 0x33, 0x99),
    /*3FH*/ RGB_INIT(0x33, 0x33, 0x33)
};

static void
xls_read_PALETTE(
    P_XLS_LOAD_INFO p_xls_load_info)
{
    U16 record_length;
    U32 opcode_offset = (biff_version == 4) ? p_xls_load_info->worksheet_substream_offset : 0; /* for BIFF4W worksheets */
    PC_BYTE p_x;
    U16 nm;
    P_RGB p_rgb = &xls_colour_table[8];

    opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, X_PALETTE_B3_B8, &record_length);

    if(XLS_NO_RECORD_FOUND == opcode_offset)
    {
        /* set colour table to default set */
        memcpy32(p_rgb, (biff_version >= 8) ? xls_colour_table_BIFF8 : xls_colour_table_BIFF5, 4 * (0x40 - 0x08));
        return;
    }

    p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

    nm = xls_read_U16_LE(p_x);
    memcpy32(p_rgb, p_x + 2, 4 * nm); /* NB file data is little-endian */
}

/******************************************************************************
*
* read code page from CODEPAGE record in globals (or BIFF4W)
*
******************************************************************************/

static void
xls_read_CODEPAGE(
    P_XLS_LOAD_INFO p_xls_load_info)
{
    U16 record_length;
    U32 opcode_offset = (biff_version == 4) ? p_xls_load_info->worksheet_substream_offset : 0; /* for BIFF4W worksheets */
    PC_BYTE p_x;
    U16 codepage;

    opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, X_CODEPAGE, &record_length);

    if(XLS_NO_RECORD_FOUND == opcode_offset)
        return;

    p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

    codepage = xls_read_U16_LE(p_x);

    switch(codepage)
    {
    default: default_unhandled();
    case 367:   /* ASCII */ /* no translation needed */
    case 1200:  /* UTF-16 (BIFF8) */ /* no translation needed */
        break;

    case 1250:  /* Windows CP-1250 (Latin II) (Central European) */
    case 1252:  /* Windows CP-1252 (Latin I) (BIFF4-BIFF7) */
        p_xls_load_info->sbchar_codepage = (SBCHAR_CODEPAGE) codepage;
        break;

    case 32769: /* Windows CP-1252 (Latin I) (BIFF2-BIFF3) */
        p_xls_load_info->sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_1252;
        break;
    }
}

/******************************************************************************
*
* read date mode from DATEMODE record in globals (or BIFF4W)
*
******************************************************************************/

static void
xls_read_DATEMODE(
    P_XLS_LOAD_INFO p_xls_load_info)
{
    U16 record_length;
    U32 opcode_offset = (biff_version == 4) ? p_xls_load_info->worksheet_substream_offset : 0; /* for BIFF4W worksheets */
    PC_BYTE p_x;

    opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, X_DATEMODE, &record_length);

    if(XLS_NO_RECORD_FOUND == opcode_offset)
        return;

    p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

    if(1 == xls_read_U16_LE(p_x))
        p_xls_load_info->datemode_1904 = TRUE;
}

/******************************************************************************
*
* read worksheet limits from DIMENSIONS record in worksheet stream
*
******************************************************************************/

_Check_return_
static STATUS
xls_read_DIMENSIONS(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 worksheet_substream_offset,
    _OutRef_    P_XLS_COL p_s_col,
    _OutRef_    P_XLS_COL p_e_col,
    _OutRef_    P_XLS_ROW p_s_row,
    _OutRef_    P_XLS_ROW p_e_row)
{
    *p_s_col = 0;
    *p_e_col = 0;
    *p_s_row = 0;
    *p_e_row = 0;

    { /* Read the DIMENSIONS record from this worksheet */
    U16 record_length;
    U32 opcode_offset = xls_find_record_first(p_xls_load_info, worksheet_substream_offset, X_DIMENSIONS_B3_B8, &record_length);
    PC_BYTE p_x;

    if(XLS_NO_RECORD_FOUND == opcode_offset)
        opcode_offset = xls_find_record_first(p_xls_load_info, worksheet_substream_offset, X_DIMENSIONS_B2, &record_length);

    if(XLS_NO_RECORD_FOUND == opcode_offset)
        return(/*create_error*/(XLS_ERR_BADFILE_NO_DIMENSIONS));

    p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    if(biff_version >= 8) /* SKS 04.11.98 for Excel 97, fixed 08.09.02 */
    {
        *p_s_row = (XLS_ROW) xls_read_U32_LE(p_x);
        *p_e_row = (XLS_ROW) xls_read_U32_LE(p_x + 4);
        *p_s_col = (XLS_COL) xls_read_U16_LE(p_x + 8);
        *p_e_col = (XLS_COL) xls_read_U16_LE(p_x + 10);
    }
    else /* (biff_version < 8) */
    {
        *p_s_row = (XLS_ROW) xls_read_U16_LE(p_x);
        *p_e_row = (XLS_ROW) xls_read_U16_LE(p_x + 2);
        *p_s_col = (XLS_COL) xls_read_U16_LE(p_x + 4);
        *p_e_col = (XLS_COL) xls_read_U16_LE(p_x + 6);
    }
    } /*block*/

    if((*p_s_col != *p_e_col) && (*p_s_row != *p_e_row))
        return(STATUS_DONE);

    /* empty worksheet */
    return(STATUS_OK);
}

/* rename this Fireworkz document as per the Excel worksheet name (helper functions) */

_Check_return_
static STATUS
xls_rename_document_add_filename(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_z_      PCTSTR filename)
{
    U32 pathname_len = tstrlen32(filename);
    PCTSTR tstr_leafname = file_leafname(filename);
    PCTSTR tstr_extension = file_extension(filename);

    PTR_ASSERT(tstr_leafname);
    if(NULL != tstr_leafname)
        pathname_len = PtrDiffElemU32(tstr_leafname, filename); /* retain the dir sep ch */

    status_return(quick_tblock_tchars_add(p_quick_tblock, filename, pathname_len));

    if(NULL != tstr_leafname)
    {   /* append leafname, carefully */
        U32 leafname_len = tstrlen32(tstr_leafname);
        U32 i;

        if(NULL != tstr_extension)
            leafname_len = PtrDiffElemU32(tstr_extension - 1, tstr_leafname);

        for(i = 0; i < leafname_len; ++i)
        {
            TCHAR tchar = tstr_leafname[i];

            if(tchar == (TCHAR) CH_SPACE)
                tchar = (TCHAR) CH_UNDERSCORE; /* in case we are on, or wish to transfer files to, RISC OS */

            status_return(quick_tblock_tchar_add(p_quick_tblock, tchar));
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_rename_document_add_worksheet_tchar(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_        TCHAR tchar)
{
#if TSTR_IS_SBSTR
    if(!ucs4_is_ascii7(tchar))
    {
        UCS4 ucs4 = ucs4_from_sbchar_with_codepage((SBCHAR) tchar /*& x0xFF*/, p_xls_load_info->sbchar_codepage);

        if(ucs4_is_sbchar(ucs4))
            tchar = (TCHAR) ucs4;
        else
        {   /* out-of-range -> force mapping to native */
            tchar = (TCHAR) ucs4_to_sbchar_force_with_codepage(ucs4, get_system_codepage(), CH_UNDERSCORE /*CH_QUESTION_MARK*/);
        }
    }
#else
    UNREFERENCED_PARAMETER_InoutRef_(p_xls_load_info);
#endif

    if(tchar == (TCHAR) CH_SPACE)
        tchar = (TCHAR) CH_UNDERSCORE; /* in case we are on, or wish to transfer files to, RISC OS */

    return(quick_tblock_tchar_add(p_quick_tblock, tchar));
}

_Check_return_
static STATUS
xls_rename_document_add_worksheet_nameA(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_reads_(worksheet_name_len) PC_SBSTR p_name,
    _InVal_     U32 worksheet_name_len)
{
    U32 i;

    for(i = 0; i < worksheet_name_len; ++i)
    {
        SBCHAR sbchar = p_name[i];
        TCHAR tchar = sbchar;

        status_return(xls_rename_document_add_worksheet_tchar(p_xls_load_info, p_quick_tblock, tchar));
    }

    return(STATUS_OK);
}

/* Unicode UTF-16LE string */

_Check_return_
static STATUS
xls_rename_document_add_worksheet_nameW(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock,
    _In_reads_(worksheet_name_len) PCWCH p_name,
    _InVal_     U32 worksheet_name_len)
{
    U32 i;

    for(i = 0; i < worksheet_name_len; ++i)
    {
        WCHAR wchar = xls_read_WCHAR_off((PCWCH) p_name, i); /* may be unaligned */
        TCHAR tchar;
#if TSTR_IS_SBSTR
        if(ucs4_is_sbchar((UCS4) wchar))
            tchar = (TCHAR) (wchar & 0xFF);
        else
        {   /* out-of-range -> try mapping to native */
            tchar = (TCHAR) ucs4_to_sbchar_force_with_codepage((UCS4) wchar, get_system_codepage(), CH_UNDERSCORE /*CH_QUESTION_MARK*/);
        }
#else
        tchar = wchar;
#endif

        status_return(xls_rename_document_add_worksheet_tchar(p_xls_load_info, p_quick_tblock, tchar));
    }

    return(STATUS_OK);
}

/* foreign extension is useless here - replace if needed with native one, and terminate */

_Check_return_
static STATUS
xls_rename_document_add_extension(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
#if !RISCOS
    status_return(quick_tblock_tchar_add(p_quick_tblock, FILE_EXT_SEP_CH));
    status_return(quick_tblock_tstr_add(p_quick_tblock, extension_document_tstr));
#endif /* OS */
    return(quick_tblock_nullch_add(p_quick_tblock));
}

/* rename this Fireworkz document as per the Excel worksheet name in the BOUNDSHEET record */

_Check_return_
static STATUS
xls_rename_document_as_per_BOUNDSHEET_record(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _InVal_     U16 record_length)
{
    PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    U32 worksheet_name_len = p_x[6];
    PC_BYTE p_name = p_x + 7;
    BYTE string_flags = 0;
    STATUS status = STATUS_OK;
    PCTSTR filename = p_xls_load_info->p_msg_insert_foreign->filename;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 128);
    quick_tblock_with_buffer_setup(quick_tblock);

    status_return(xls_rename_document_add_filename(&quick_tblock, filename));

    /* append _ and worksheet name */
    if(status_ok(status))
        status = quick_tblock_tchar_add(&quick_tblock, (TCHAR) CH_UNDERSCORE);

    if(biff_version >= 8)
    {
        string_flags = *p_name++;

        if(string_flags & 0x08) /* rich-text */
            p_name += 2;
        if(string_flags & 0x04) /* phonetic */
            p_name += 4;
    }

#if 1
    if(string_flags & 0x01)
    {   /* Unicode UTF-16LE string */
        xls_rename_document_add_worksheet_nameW(p_xls_load_info, &quick_tblock, (PCWCH) p_name, worksheet_name_len);
    }
    else
    {
        xls_rename_document_add_worksheet_nameA(p_xls_load_info, &quick_tblock, p_name, worksheet_name_len);
    }
#else
    for(i = 0; (i < worksheet_name_len) && status_ok(status); ++i)
    {
        TCHAR tchar;

        if(string_flags & 0x01)
        {   /* Unicode UTF-16LE string */
            WCHAR wchar = xls_read_WCHAR_off((PCWCH) p_name, i); /* may be unaligned */
#if TSTR_IS_SBSTR
            if(ucs4_is_sbchar((UCS4) wchar))
                tchar = (TCHAR) (wchar & 0xFF);
            else
            {   /* out-of-range -> try mapping to native */
                tchar = (TCHAR) ucs4_to_sbchar_force_with_codepage((UCS4) wchar, get_system_codepage(), CH_UNDERSCORE /*CH_QUESTION_MARK*/);
            }
#else
            tchar = wchar;
#endif
        }
        else
        {
            SBCHAR sbchar = p_name[i];
            tchar = sbchar;
        }

#if TSTR_IS_SBSTR
        if(!ucs4_is_ascii7(tchar))
        {
            UCS4 ucs4 = ucs4_from_sbchar_with_codepage((SBCHAR) tchar /*& x0xFF*/, p_xls_load_info->sbchar_codepage);

            if(ucs4_is_sbchar(ucs4))
                tchar = (TCHAR) ucs4;
            else
            {   /* out-of-range -> force mapping to native */
                tchar = (TCHAR) ucs4_to_sbchar_force_with_codepage(ucs4, get_system_codepage(), CH_UNDERSCORE /*CH_QUESTION_MARK*/);
            }
        }
#endif

        if(tchar == (TCHAR) CH_SPACE)
            tchar = (TCHAR) CH_UNDERSCORE; /* in case we are on, or wish to transfer files to, RISC OS */

        status = quick_tblock_tchar_add(&quick_tblock, tchar);
    }
#endif

    if(status_ok(status))
        status = xls_rename_document_add_extension(&quick_tblock);

    /* suggest that this document be renamed */
    if(status_ok(status))
        status = rename_document_as_filename(p_xls_load_info->p_docu, quick_tblock_tstr(&quick_tblock));

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

/******************************************************************************
*
* Find offset of worksheet substream from the BOUNDSHEET record
* in the newer format Workbook Globals Substream
* or leave at zero for older format files
*
******************************************************************************/

_Check_return_
static STATUS
xls_find_BOUNDSHEET(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_U32 p_retry_arg)
{
    STATUS status = STATUS_OK;
    const BOOL is_first_worksheet = (0 == *p_retry_arg);
    U16 record_length;
    U32 opcode_offset = xls_find_record_first(p_xls_load_info, *p_retry_arg, X_BOUNDSHEET_B5_B8, &record_length);
    U16 this_record_length;
    U32 this_opcode_offset;
    U32 bof_offset;
    XLS_COL s_col, e_col;
    XLS_ROW s_row, e_row;

    *p_retry_arg = 0;

    p_xls_load_info->worksheet_substream_offset = 0;

    /* check that the given BOUNDSHEET entry refers to a non-empty worksheet */
    while(XLS_NO_RECORD_FOUND != opcode_offset)
    {
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        bof_offset = xls_read_U32_LE(p_x);
        if(bof_offset + 4 /*opcode,record_length*/ >= p_xls_load_info->file_end_offset)
            status_break(status = create_error(XLS_ERR_BADFILE_BAD_BOUNDSHEET));

        status_break(status = xls_check_BOF_is_worksheet(p_xls_load_info, bof_offset));

        if(status_done(status))
        {
            status = xls_read_DIMENSIONS(p_xls_load_info, bof_offset, &s_col, &e_col, &s_row, &e_row);

            if(status_done(status) || is_first_worksheet) /* allow us to open empty first worksheet */
            {
                p_xls_load_info->worksheet_substream_offset = bof_offset;
                break;
            }
        }

        /* hmm. could be a chart etc. */
        /*assert0();*/

        /* loop until we find a non-empty worksheet to load! */
        opcode_offset = xls_find_record_next(p_xls_load_info, opcode_offset, X_BOUNDSHEET_B5_B8, &record_length);
    }

    status_return(status);

    if(XLS_NO_RECORD_FOUND == opcode_offset)
        return(STATUS_OK);

    this_record_length = record_length;
    this_opcode_offset = opcode_offset;

    /* find the next non-empty worksheet in the BOUNDSHEET set so that the skel loader can retry with it */
    for(;;)
    {
        STATUS status_next;
        PC_BYTE p_x;

        opcode_offset = xls_find_record_next(p_xls_load_info, opcode_offset, X_BOUNDSHEET_B5_B8, &record_length);

        if(XLS_NO_RECORD_FOUND == opcode_offset)
            break; /* no further worksheets */

        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
        bof_offset = xls_read_U32_LE(p_x);
        if(bof_offset + 4 /*opcode,record_length*/ >= p_xls_load_info->file_end_offset)
            break; /* bad but ignore */

        status_break(status_next = xls_check_BOF_is_worksheet(p_xls_load_info, bof_offset)); /* may be bad but ignore */

        if(status_done(status_next))
        {
            status_next = xls_read_DIMENSIONS(p_xls_load_info, bof_offset, &s_col, &e_col, &s_row, &e_row);

            if(status_done(status_next))
            {
                *p_retry_arg = opcode_offset;
                break;
            }
        }
    }

    /* only rename this worksheet's document if really necessary */
    if(!is_first_worksheet || (0 != *p_retry_arg))
        status_return(xls_rename_document_as_per_BOUNDSHEET_record(p_xls_load_info, this_opcode_offset, this_record_length));

    return(STATUS_OK);
}

/* rename this Fireworkz document as per the Excel worksheet name in the SHEETHDR record */

_Check_return_
static STATUS
xls_rename_document_as_per_SHEETHDR_record(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _InVal_     U16 record_length)
{
    PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    U32 worksheet_name_len = p_x[4];
    PC_BYTE p_name = p_x + 5;
    STATUS status;
    PCTSTR filename = file_leafname(p_xls_load_info->p_msg_insert_foreign->filename);
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 128);
    quick_tblock_with_buffer_setup(quick_tblock);

    status = xls_rename_document_add_filename(&quick_tblock, filename);

    /* append _ and worksheet name */
    if(status_ok(status))
        status = quick_tblock_tchar_add(&quick_tblock, (TCHAR) CH_UNDERSCORE);

    if(status_ok(status))
        status = xls_rename_document_add_worksheet_nameA(p_xls_load_info, &quick_tblock, p_name, worksheet_name_len);

    if(status_ok(status))
        status = xls_rename_document_add_extension(&quick_tblock);

    /* suggest that this document be renamed */
    if(status_ok(status))
        status = rename_document_as_filename(p_xls_load_info->p_docu, quick_tblock_tstr(&quick_tblock));

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

/******************************************************************************
*
* Find offset of worksheet substream given the BIFF4W SHEETHDR record
*
******************************************************************************/

_Check_return_
static STATUS
xls_find_SHEETHDR(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_U32 p_retry_arg)
{
    STATUS status = STATUS_OK;
    const BOOL is_first_worksheet = (0 == *p_retry_arg);
    U16 record_length;
    U32 opcode_offset = xls_find_record_first(p_xls_load_info, *p_retry_arg, X_SHEETHDR_B4, &record_length);
    U16 this_record_length;
    U32 this_opcode_offset;
    U32 bof_offset;
    XLS_COL s_col, e_col;
    XLS_ROW s_row, e_row;
    U32 substream_bytes = 0;

    *p_retry_arg = 0;

    p_xls_load_info->worksheet_substream_offset = 0;

    /* check that the given SHEETHDR entry refers to a non-empty worksheet */
    while(XLS_NO_RECORD_FOUND != opcode_offset)
    {
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        substream_bytes = xls_read_U32_LE(p_x);

        bof_offset = opcode_offset + 4 /*opcode,record_length*/ + record_length;

        if(bof_offset + 4 /*opcode,record_length*/ >= p_xls_load_info->file_end_offset)
            status_break(status = create_error(XLS_ERR_BADFILE_BAD_BOUNDSHEET));

        status_break(status = xls_check_BOF_is_worksheet(p_xls_load_info, bof_offset));

        if(status_done(status))
        {
            status = xls_read_DIMENSIONS(p_xls_load_info, bof_offset, &s_col, &e_col, &s_row, &e_row);

            if(status_done(status) || is_first_worksheet) /* allow us to open empty first worksheet */
            {
                p_xls_load_info->worksheet_substream_offset = bof_offset;
                break;
            }
        }

        /* hmm. could be a chart etc. */
        /*assert0();*/

        /* skip this entire substream */
        opcode_offset = bof_offset + substream_bytes;

        /* loop until we find a non-empty worksheet to load! */
        if(X_SHEETHDR_B4 == xls_read_record_header(p_xls_load_info, opcode_offset, &record_length))
            continue;

        /* this EOF should be at the end of the BIFF4W workbook */
        assert(X_EOF == xls_read_record_header(p_xls_load_info, opcode_offset, &record_length));
        opcode_offset = XLS_NO_RECORD_FOUND;
        break;
    }

    status_return(status);

    if(XLS_NO_RECORD_FOUND == opcode_offset)
        return(STATUS_OK);

    this_record_length = record_length;
    this_opcode_offset = opcode_offset;

    /* find the next non-empty worksheet in the SHEETHDR set so that the skel loader can retry with it */
    opcode_offset = p_xls_load_info->worksheet_substream_offset /*bof_offset*/ + substream_bytes;

    for(;;)
    {
        STATUS status_next;
        PC_BYTE p_x;

        if(X_SHEETHDR_B4 != xls_read_record_header(p_xls_load_info, opcode_offset, &record_length))
        {
            /* this EOF should be at the end of the BIFF4W workbook */
            assert(X_EOF == xls_read_record_header(p_xls_load_info, opcode_offset, &record_length));
            opcode_offset = XLS_NO_RECORD_FOUND;
            break; /* no further worksheets */
        }

        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        substream_bytes = xls_read_U32_LE(p_x);

        bof_offset = opcode_offset + 4 /*opcode,record_length*/ + record_length;

        if(bof_offset + 4 /*opcode,record_length*/ >= p_xls_load_info->file_end_offset)
            break; /* bad but ignore */

        status_break(status_next = xls_check_BOF_is_worksheet(p_xls_load_info, bof_offset)); /* may be bad but ignore */

        if(status_done(status_next))
        {
            status_next = xls_read_DIMENSIONS(p_xls_load_info, bof_offset, &s_col, &e_col, &s_row, &e_row);

            if(status_done(status_next))
            {
                *p_retry_arg = opcode_offset;
                break;
            }
        }

        /* skip this entire substream */
        opcode_offset = bof_offset + substream_bytes;
    }

    /* only rename this worksheet's document if really necessary */
    if(!is_first_worksheet || (0 != *p_retry_arg))
        status_return(xls_rename_document_as_per_SHEETHDR_record(p_xls_load_info, this_opcode_offset, this_record_length));

    return(STATUS_OK);
}

/******************************************************************************
*
* Excel decompiler
*
******************************************************************************/

static S32 cursym;

static PC_BYTE p_scan;

static PC_PTG_ENTRY p_ptg_entry;

static S32 arg_stack_n;

static U32 extern_opcode_offset = 0;

PROC_BSEARCH_PROTO(static, xls_proc_func_compare, U16, XLS_FUNC_ENTRY)
{
    BSEARCH_KEY_VAR_DECL(PC_U16, key);
    const U16 s1 = *key;
    BSEARCH_DATUM_VAR_DECL(PC_XLS_FUNC_ENTRY, datum);
    const U16 s2 = datum->biff_function_number;

    if(s1 > s2)
        return(1);

    if(s1 < s2)
        return(-1);

    return(0);
}

_Check_return_
_Ret_maybenull_
static PC_XLS_FUNC_ENTRY
xls_func_lookup(
    _InVal_     U16 biff_function_number)
{
    PC_XLS_FUNC_ENTRY p_xls_func_entry = NULL;

    /* NB some parameter counts change between versions */
    if(biff_version >= 8)
    {
        if(NULL != (p_xls_func_entry = (PC_XLS_FUNC_ENTRY)
            bsearch(&biff_function_number, BIFF8_functions, elemof(BIFF8_functions), sizeof(BIFF8_functions[0]), xls_proc_func_compare)))
            return(p_xls_func_entry);
    }

    if(biff_version >= 5)
    {
        if(NULL != (p_xls_func_entry = (PC_XLS_FUNC_ENTRY)
            bsearch(&biff_function_number, BIFF5_functions, elemof(BIFF5_functions), sizeof(BIFF5_functions[0]), xls_proc_func_compare)))
            return(p_xls_func_entry);
    }

    if(biff_version >= 4)
    {
        if(NULL != (p_xls_func_entry = (PC_XLS_FUNC_ENTRY)
            bsearch(&biff_function_number, BIFF4_functions, elemof(BIFF4_functions), sizeof(BIFF4_functions[0]), xls_proc_func_compare)))
            return(p_xls_func_entry);
    }

    if(biff_version >= 3)
    {
        if(NULL != (p_xls_func_entry = (PC_XLS_FUNC_ENTRY)
            bsearch(&biff_function_number, BIFF3_functions, elemof(BIFF3_functions), sizeof(BIFF3_functions[0]), xls_proc_func_compare)))
            return(p_xls_func_entry);
    }

    p_xls_func_entry = (PC_XLS_FUNC_ENTRY)
        bsearch(&biff_function_number, BIFF2_functions, elemof(BIFF2_functions), sizeof(BIFF2_functions[0]), xls_proc_func_compare);

    return(p_xls_func_entry);
}

/******************************************************************************
*
* symbol lookahead
*
******************************************************************************/

static inline void
token_check(void)
{
    assert(0 == (0x80 & *p_scan)); /* top bit should be clear */
    cursym = *p_scan & 0x7F;

    p_ptg_entry = &token_table[cursym];
}

/******************************************************************************
*
* RPN scanner
*
******************************************************************************/

static void
token_skip(void)
{
    if(cursym != -1)
    {
        p_scan += p_ptg_entry->biff_bytes;

        /* deal with variable-size ones and those that change size between BIFF versions */
        if(cursym < 0x20)
        {   /* Base Tokens */
            switch(cursym)
            {
            default:
                assert(0 != p_ptg_entry->biff_bytes);
                break;

            case tExp:
            case tTbl:
                if(biff_version >= 3)
                    p_scan += (5 - 4);
                break;

            case tStr:
                {
                U32 n_chars;
                BYTE string_flags = 0;

                assert(1 == p_ptg_entry->biff_bytes);
                /* tStr token already skipped */

                n_chars = *p_scan++; /* NB 8-bit length byte - even in BIFF8 */

                if(biff_version >= 8) /* BIFF8 always has option flags byte */
                {
                    string_flags = *p_scan++;

                    if(string_flags & 0x08) /* rich-text */
                        p_scan += 2;
                    if(string_flags & 0x04) /* phonetic */
                        p_scan += 4;
                }

                if(string_flags & 0x01)
                    p_scan += n_chars * 2; /* Unicode UTF-16LE string */
                else
                    p_scan += n_chars; /* 'Compressed' BYTE string (high bytes of WCHAR all zero) */

                break;
                }

            case tAttr:
                {
                U8 grbit;

                assert(1 == p_ptg_entry->biff_bytes);
                /* tAttr token already skipped */

                grbit = p_scan[0];

                if(biff_version >= 3)
                {
                    if(grbit & 0x04)
                    {   /* optimised choose function */
                        U16 cases = xls_read_U16_LE(p_scan + 1);
                        p_scan += (cases + 1) * 2;
                    }
                    p_scan += 3;
                }
                else /* (biff_version == 2) */
                {
                    if(grbit & 0x04)
                    {   /* optimised choose function */
                        U8 cases = p_scan[1];
                        p_scan += (cases + 1);
                    }
                    p_scan += 2;
                }

                break;
                }

            case tSheet:
                BIFF_VERSION_ASSERT(<= 4);
                if(biff_version >= 3)
                    p_scan += (11 - 8);
                break;

            case tEndSheet:
                BIFF_VERSION_ASSERT(<= 4);
                if(biff_version >= 3)
                    p_scan += (5 - 4);
                break;
            }
        }
        else
        {   /* Classified Tokens */
            const S32 symR = (cursym & ~0x60) | 0x20; /* convert V and A to corresponding R token */

            switch(symR)
            {
            case tArrayR:
                if(biff_version >= 3)
                    p_scan += (8 - 7);
                break;

            case tFuncR:
                if(biff_version >= 4)
                    p_scan += (3 - 2);
                break;

            case tFuncVarR:
                if(biff_version >= 4)
                    p_scan += (4 - 3);
                break;

            case tNameR:
                assert(0 == p_ptg_entry->biff_bytes);
                if(biff_version >= 8)
                    p_scan += 5;
                else if(biff_version >= 5)
                    p_scan += 15;
                else if(biff_version >= 3)
                    p_scan += 11;
                else /* (biff_version == 2) */
                    p_scan += 8;
                break;

            case tRefR:
            case tRefErrR:
            case tRefNR:
                if(biff_version >= 8)
                    p_scan += (5 - 4);
                break;

            case tAreaR:
            case tAreaErrR:
            case tAreaNR:
                if(biff_version >= 8)
                    p_scan += (9 - 7);
                break;

            case tMemAreaR:
            case tMemErrR:
            case tMemNoMemR:
                if(biff_version >= 3)
                    p_scan += (7 - 5);
                break;

            case tMemFuncR:
            case tMemAreaNR:
            case tMemNoMemNR:
                if(biff_version >= 3)
                    p_scan += (3 - 2);
                break;

            case tNameXR:
                assert(0 == p_ptg_entry->biff_bytes);
                if(biff_version >= 8)
                    p_scan += 7;
                else /* (biff_version < 8) */
                    p_scan += 25;
                break;

            case tRef3dR:
            case tRefErr3dR:
                assert(0 == p_ptg_entry->biff_bytes);
                if(biff_version >= 8)
                    p_scan += 7;
                else /* (biff_version < 8) */
                    p_scan += 18;
                break;

            case tArea3dR:
            case tAreaErr3dR:
                assert(0 == p_ptg_entry->biff_bytes);
                if(biff_version >= 8)
                    p_scan += 11;
                else /* (biff_version < 8) */
                    p_scan += 21;
                break;
            }
        }
    }
}

/******************************************************************************
*
* free all elements on stack
*
******************************************************************************/

static void
stack_free(void)
{
    while(arg_stack_n)
        al_ptr_dispose(P_P_ANY_PEDANTIC(&arg_stack[--arg_stack_n]));
}

/******************************************************************************
*
* output a Fireworkz form external reference
*
******************************************************************************/

static void
extern_ref_sheet_name_out(
    PC_BYTE p_x,
    _Out_writes_(elemof_buffer) P_UCHARS uchars_buf /*filled at *p_len*/,
    _InVal_     U32 elemof_buffer,
    _InoutRef_  P_U32 p_len)
{
    U32 filename_len = p_x[1];
    PC_UCHARS filename = PtrAddBytes(PC_UCHARS, p_x, 2);
    assert(p_x[0] <= 0x04);
    assert(*p_len + filename_len + 1 + 1 < elemof_buffer);
    UNREFERENCED_PARAMETER_InVal_(elemof_buffer);
    PtrPutByteOff(uchars_buf, (*p_len)++, CH_LEFT_SQUARE_BRACKET);
    memcpy32(PtrAddBytes(P_UCHARS, uchars_buf, *p_len), filename, filename_len);
    *p_len += filename_len;
    PtrPutByteOff(uchars_buf, (*p_len)++, CH_RIGHT_SQUARE_BRACKET);
    /* no CH-NULL termination */
}

static void
extern_ref_out(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _Out_writes_(elemof_buffer) P_UCHARS uchars_buf /*filled at *p_len*/,
    _InVal_     U32 elemof_buffer,
    _InoutRef_  P_U32 p_len)
{
    if(0 != extern_opcode_offset)
    {
        PC_BYTE p_x = PtrAddBytes(PC_BYTE, p_xls_load_info->p_file_start, extern_opcode_offset);
        p_x += 4  /*opcode,record_length*/;

        assert(p_x[0] <= 0x04);

        if(p_x[0] == 0x00)
        {   /* reference is relative to the current worksheet (nothing will follow), used e.g. in defined names */
            assert(biff_version >= 5);
            return;
        }

        if(p_x[0] == 0x01)
        {
            if(p_x[1] == 0x3A)
                /* it refers to an add-in function - no actual worksheet name supplied */
                return;

            assert0(); /* Encoded URLs not handled */
            return;
        }

        if(p_x[0] == 0x02)
        {
            if(biff_version < 8)
                /* it is a self-reference to the current worksheet (nothing will follow) */
                return;

            /* reference to a worksheet in the own document (worksheet name follows) */
            extern_ref_sheet_name_out(p_x, uchars_buf /*filled at *p_len*/, elemof_buffer, p_len);
            return;
        }

        if(p_x[0] == 0x03)
        {   /* reference to a worksheet in the own document (worksheet name follows) */
            assert(biff_version == 5);
            extern_ref_sheet_name_out(p_x, uchars_buf /*filled at *p_len*/, elemof_buffer, p_len);
            return;
        }

        if(p_x[0] == 0x04)
        {   /* it is a self-reference to the current workbook (nothing will follow) */
            assert(biff_version == 5);
        }
    }
}

/******************************************************************************
*
* convert constant to a string
*
******************************************************************************/

_Ret_maybenull_z_
static P_USTR
constant_convert(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     S32 sym,
    _In_        PC_BYTE p_byte_in,
    _Inout_     PC_BYTE * const p_p_formula_end,
    _OutRef_    P_STATUS p_status)
{
    PC_BYTE p_byte = p_byte_in + 1; /* skip token */
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
    quick_ublock_with_buffer_setup(quick_ublock);

    *p_status = STATUS_OK;

    switch(sym)
    {
    case tStr:
        { /* NB 8-bit length byte - even in BIFF8 */
        U32 n_chars = *p_byte++;

        status_break(*p_status = quick_ublock_a7char_add(&quick_ublock, CH_QUOTATION_MARK));

        *p_status = xls_quick_ublock_xls_string_add(&quick_ublock, p_byte, n_chars, p_xls_load_info->sbchar_codepage, FALSE); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

        if(status_ok(*p_status))
            *p_status = quick_ublock_a7char_add(&quick_ublock, CH_QUOTATION_MARK);
        break;
        }

    case tErr:
        {
        PC_USTR p_error;

        switch(p_byte[0])
        {
        case 0:  p_error = USTR_TEXT("#NULL!"); break;
        case 7:  p_error = USTR_TEXT("#DIV/0!"); break;
        case 15: p_error = USTR_TEXT("#VALUE!"); break;
        case 23: p_error = USTR_TEXT("#REF!"); break;
        case 29: p_error = USTR_TEXT("#NAME!"); break;
        case 36: p_error = USTR_TEXT("#NUM!"); break;
        case 42: p_error = USTR_TEXT("#N/A!"); break;
        default: p_error = USTR_TEXT("#ERROR"); break;
        }

        *p_status = quick_ublock_ustr_add(&quick_ublock, p_error);
        break;
        }

    case tBool:
        *p_status = quick_ublock_a7char_add(&quick_ublock, (0 == p_byte[0]) ? CH_DIGIT_ZERO : '1');
        break;

    case tInt:
        {
        S32 s32 = (S32) xls_read_U16_LE(p_byte);
        *p_status = quick_ublock_printf(&quick_ublock, USTR_TEXT(S32_FMT), s32);
        break;
        }

    case tNum:
        {
        F64 f64 = xls_read_F64(p_byte);
        *p_status = quick_ublock_printf(&quick_ublock, USTR_TEXT("%." stringize(DBL_DECIMAL_DIG) "g"), f64);
        break;
        }

    case tArrayR: /* case tArrayV:,  case tArrayA: ??? */
        {
        U8 n_col, col;
        U16 n_row, row;

        p_byte = *p_p_formula_end;
        status_break(*p_status = quick_ublock_a7char_add(&quick_ublock, CH_LEFT_CURLY_BRACKET));
        n_col = p_byte[0];
        n_row = xls_read_U16_LE(p_byte + 1);
        p_byte += 3;

        for(row = 0; row < n_row; row += 1)
        {
            if(row != 0)
                status_break(*p_status = quick_ublock_a7char_add(&quick_ublock, CH_SEMICOLON));

            for(col = 0; col < n_col; col += 1)
            {
                if(col != 0)
                    status_break(*p_status = quick_ublock_a7char_add(&quick_ublock, CH_COMMA));

                switch(p_byte[0])
                {
                case 1: /* IEEE */
                    {
                    F64 f64 = xls_read_F64(&p_byte[1]);
                    *p_status = quick_ublock_printf(&quick_ublock, USTR_TEXT("%." stringize(DBL_DECIMAL_DIG) "g"), f64);
                    p_byte += 9;
                    break;
                    }

                case 2: /* string */
                    {
                    U8 sbchars_n = p_byte[1];
                    PC_SBCHARS sbchars = PtrAddBytes(PC_SBCHARS, p_byte, 2);
                    status_assert(*p_status = quick_ublock_a7char_add(&quick_ublock, CH_QUOTATION_MARK));
                    status_assert(*p_status = quick_ublock_sbchars_add(&quick_ublock, sbchars, sbchars_n));
                    status_assert(*p_status = quick_ublock_a7char_add(&quick_ublock, CH_QUOTATION_MARK));
                    break;
                    }

                default: default_unhandled();
                    break;
                }

                status_break(*p_status);
            }

            status_break(*p_status);
        }

        if(status_ok(*p_status))
            *p_status = quick_ublock_a7char_add(&quick_ublock, CH_RIGHT_CURLY_BRACKET);

        *p_p_formula_end = p_byte;
        break;
        }
    }

    /*if(status_ok(*p_status))*/
        /**p_status = quick_ublock_nullch_add(&quick_ublock); no longer CH_NULL terminate here - ustr_set_n will do that to the copy */

    if(status_ok(*p_status))
    {
        P_USTR ustr_res;

        *p_status = ustr_set_n(&ustr_res, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

        if(status_ok(*p_status))
        {
            quick_ublock_dispose(&quick_ublock);
            return(ustr_res);
        }
    }

    quick_ublock_dispose(&quick_ublock);
    return(P_USTR_NONE);
}

/******************************************************************************
*
* convert Excel SLR
*
******************************************************************************/

static void
slr_convert_method_A(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf /*filled at *p_len*/,
    _InVal_     U32 elemof_buffer,
    _InoutRef_  P_U32 p_len,
    _InVal_     U16 col_in,
    _InVal_     U16 row_in)
{
    U16 col, row;
    BOOL absolute_col, absolute_row;

    assert((elemof_buffer - *p_len) > 1);
    PtrPutByteOff(ustr_buf, *p_len, CH_NULL);

    if(biff_version >= 8)
    {   /* 16-bit row */
        absolute_col = (0 == (col_in & 0x4000));
        absolute_row = (0 == (col_in & 0x8000));
        col = col_in & 0x00FF;
        row = row_in;
    }
    else /* (biff_version < 8) */
    {   /* 14-bit row */
        col = col_in & 0x00FF; /* paranoia */
        absolute_col = (0 == (row_in & 0x4000));
        absolute_row = (0 == (row_in & 0x8000));
        row = row_in & 0x3FFF;
    }

    if((elemof_buffer - *p_len) > 2) /* room for at least one more char? */
    {
        if(absolute_col)
            PtrPutByteOff(ustr_buf, (*p_len)++, CH_DOLLAR_SIGN);
        *p_len += xtos_ustr_buf(ustr_AddBytes_wr(ustr_buf, *p_len), elemof_buffer - *p_len, (S32) col, FALSE);
    }

    if((elemof_buffer - *p_len) > 2) /* room for at least one more char? */
    {
        if(absolute_row)
            PtrPutByteOff(ustr_buf, (*p_len)++, CH_DOLLAR_SIGN);
        *p_len += ustr_xsnprintf(ustr_AddBytes_wr(ustr_buf, *p_len), elemof_buffer - *p_len, USTR_TEXT(ROW_FMT), (ROW) row + 1);
    }
}

static void
slr_convert_method_B(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf /*filled at *p_len*/,
    _InVal_     U32 elemof_buffer,
    _InoutRef_  P_U32 p_len,
    _InVal_     U16 col_in,
    _InVal_     U16 row_in,
    _InRef_     PC_SLR p_current_slr)
{
    U16 col, row;
    BOOL absolute_col, absolute_row;
    S16 new_col, new_row;

    assert((elemof_buffer - *p_len) > 1);
    PtrPutByteOff(ustr_buf, *p_len, CH_NULL);

    if(biff_version >= 8)
    {   /* 16-bit row */
        absolute_col = (0 == (col_in & 0x4000));
        absolute_row = (0 == (col_in & 0x8000));
        col = col_in & 0x00FF;
        row = row_in;
    }
    else /* (biff_version < 8) */
    {   /* 14-bit row */
        col = col_in & 0x00FF; /* paranoia */
        absolute_col = (0 == (row_in & 0x4000));
        absolute_row = (0 == (row_in & 0x8000));
        row = row_in & 0x3FFF;
        if(0x2000 & row) /* manually sign-extend the row offset from 14 to 16 bits */
            row |= 0xC000;
    }

    /* NB carefully sign-extend both col and row offsets */
    if(absolute_col)
        new_col = col;
    else
        new_col = (S16) (p_current_slr->col + (S8)  col);

    if(absolute_row)
        new_row = row;
    else
        new_row = (S16) (p_current_slr->row + (S16) row);

    if((elemof_buffer - *p_len) > 2) /* room for at least one more char? */
    {
        if(absolute_col)
            PtrPutByteOff(ustr_buf, (*p_len)++, CH_DOLLAR_SIGN);
        *p_len += xtos_ustr_buf(ustr_AddBytes_wr(ustr_buf, *p_len), elemof_buffer - *p_len, (S32) new_col, FALSE);
    }

    if((elemof_buffer - *p_len) > 2) /* room for at least one more char? */
    {
        if(absolute_row)
            PtrPutByteOff(ustr_buf, (*p_len)++, CH_DOLLAR_SIGN);
        *p_len += (U32) ustr_xsnprintf(ustr_AddBytes_wr(ustr_buf, *p_len), elemof_buffer - *p_len, USTR_TEXT(ROW_FMT), (ROW) new_row + 1);
    }
}

/******************************************************************************
*
* convert an Excel operand value to a string
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_USTR
operand_convert(
    P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     S32 sym,
    _In_        PC_BYTE p_byte_in,
    _Inout_     PC_BYTE * const p_p_formula_end,
    _OutRef_    P_STATUS p_status)
{
    const S32 symR = (sym & ~0x60) | 0x20; /* convert V and A to corresponding R token */
    PC_BYTE p_byte = p_byte_in + 1; /* skip token */
    P_USTR ustr_res;
    UCHARZ buffer[256];

    assert(0 != (sym & 0x60));

    switch(symR)
    {
    case tRefR:
        {
        U32 len = 0;
        U16 row, col;
        xls_read_cell_address_formula(p_byte, &row, &col);
        extern_ref_out(p_xls_load_info, uchars_bptr(buffer), elemof32(buffer), &len);
        slr_convert_method_A(ustr_bptr(buffer), elemof32(buffer), &len, col, row);
        break;
        }

    case tRefNR:
        {
        U32 len = 0;
        U16 row, col;
        xls_read_cell_address_formula(p_byte, &row, &col);
        extern_ref_out(p_xls_load_info, uchars_bptr(buffer), elemof32(buffer), &len);
        slr_convert_method_B(ustr_bptr(buffer), elemof32(buffer), &len, col, row, &p_xls_load_info->current_slr);
        break;
        }

    case tAreaR:
        {
        U32 len = 0;
        U16 row_s, row_e, col_s, col_e;
        xls_read_cell_range_formula(p_byte, &row_s, &row_e, &col_s, &col_e);
        extern_ref_out(p_xls_load_info, uchars_bptr(buffer), elemof32(buffer), &len);
        slr_convert_method_A(ustr_bptr(buffer), elemof32(buffer), &len, col_s, row_s);
        slr_convert_method_A(ustr_bptr(buffer), elemof32(buffer), &len, col_e, row_e);
        break;
        }

    case tAreaNR:
        {
        U32 len = 0;
        U16 row_s, row_e, col_s, col_e;
        xls_read_cell_range_formula(p_byte, &row_s, &row_e, &col_s, &col_e);
        extern_ref_out(p_xls_load_info, uchars_bptr(buffer), elemof32(buffer), &len);
        slr_convert_method_B(ustr_bptr(buffer), elemof32(buffer), &len, col_s, row_s, &p_xls_load_info->current_slr);
        slr_convert_method_B(ustr_bptr(buffer), elemof32(buffer), &len, col_e, row_e, &p_xls_load_info->current_slr);
        break;
        }

    case tArea3dR:
        {
        U32 len = 0;
        U16 row_s, row_e, col_s, col_e;
        xls_read_cell_range_formula(p_byte + 2, &row_s, &row_e, &col_s, &col_e);
        extern_ref_out(p_xls_load_info, uchars_bptr(buffer), elemof32(buffer), &len);
        slr_convert_method_A(ustr_bptr(buffer), elemof32(buffer), &len, col_s, row_s);
        slr_convert_method_A(ustr_bptr(buffer), elemof32(buffer), &len, col_e, row_e);
        break;
        }

    case tRefErrR:
    case tAreaErrR:
        ustr_xstrkpy(ustr_bptr(buffer), elemof32(buffer), USTR_TEXT("#REF!"));
        break;

    case tNameR:
        {
        U32 len = 0;
        U16 ilbl = xls_read_U16_LE(p_byte);

        buffer[0] = CH_NULL;

        if(0 != extern_opcode_offset)
        {
            U16 record_length;
            U32 opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, ilbl, X_EXTERNNAME_B2_B5_B8, 1);

            if(XLS_NO_RECORD_FOUND == opcode_offset)
                opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, ilbl, X_EXTERNNAME_B3_B4, 1);

            if(XLS_NO_RECORD_FOUND != opcode_offset)
            {
                PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
                U32 offset_len, offset_text;
                PC_U8 p_name;
                U8 name_len;

                if(biff_version >= 3)
                {
                    offset_len = 2; /* argue about which version <<< */
                }
                else /* (biff_version == 2) */
                {
                    offset_len = 0;
                }

                offset_text = offset_len + 1;

                name_len = p_x[offset_len];
                p_name = (PC_U8) p_x + offset_text;
                extern_ref_out(p_xls_load_info, uchars_bptr(buffer), elemof32(buffer), &len);
                buffer[len] = CH_NULL;
                xstrnkat(buffer, elemof32(buffer), p_name, name_len);
            }
        }
        else
        {
            XLS_OPCODE name_opcode = X_DEFINEDNAME_B2_B5_B8;
            U16 record_length;
            U32 opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, ilbl, name_opcode, 1);

            if(XLS_NO_RECORD_FOUND == opcode_offset)
            {
                name_opcode = X_DEFINEDNAME_B3_B4;
                opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, ilbl, name_opcode, 1);
            }

            if(XLS_NO_RECORD_FOUND != opcode_offset)
            {
                PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
                PC_BYTE p_name;
                U8 name_len;

                name_len = p_x[3];

                if(X_DEFINEDNAME_B3_B4 == name_opcode)
                {
                    p_name = p_x + 6;
                }
                else /* (X_DEFINEDNAME_B2_B5_B8 == name_opcode) */
                {
                    if(biff_version >= 5)
                    {
                        p_name = p_x + 14;
                        p_name += 1; /* skip grbit - assume non-Unicode string for now */
                    }
                    else /* (biff_version == 2) */
                    {
                        p_name = p_x + 5;
                    }
                }

                xstrnkpy(buffer, elemof32(buffer), (PC_U8) p_name, name_len);
            }
        }

        break;
        }

    case tNameXR:
        {
        U16 ref_idx = xls_read_U16_LE(p_byte);

        if(biff_version >= 8)
        {
            U16 other_idx = xls_read_U16_LE(p_byte + 2);
            U16 record_length;
            U32 opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, other_idx, X_EXTERNNAME_B2_B5_B8, 1);

            if(XLS_NO_RECORD_FOUND == opcode_offset)
                opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, other_idx, X_EXTERNNAME_B3_B4, 1);

            if(XLS_NO_RECORD_FOUND != opcode_offset)
            {
                PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
                U32 offset_len, offset_text;
                PC_BYTE p_name;
                U8 name_len;

                if(biff_version >= 5)
                {
                    offset_len  = 6;
                }
                else if(biff_version >= 3)
                {
                    offset_len  = 2; /* argue about which version <<< */
                }
                else /* (biff_version == 2) */
                {
                    offset_len  = 0;
                }

                offset_text = offset_len + 1;

                name_len = p_x[offset_len];
                p_name = p_x + offset_text;

                if(biff_version >= 8)
                {
                    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_name, 64);
                    quick_ublock_with_buffer_setup(quick_ublock_name);

                    status_assert(xls_quick_ublock_xls_string_add(&quick_ublock_name, p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE));

                    ustr_xstrnkpy(ustr_bptr(buffer), elemof32(buffer), quick_ublock_uchars(&quick_ublock_name), quick_ublock_bytes(&quick_ublock_name));

                    quick_ublock_dispose(&quick_ublock_name);
                }
                else
                    xstrnkpy(buffer, elemof32(buffer), (PC_SBCHARS) p_name, name_len);
            }
            else
            {
                consume_int(ustr_xsnprintf(ustr_bptr(buffer), elemof32(buffer),
                                           USTR_TEXT("OTHER_IDX " U32_XFMT),
                                           (U32) other_idx));
            }
        }
        else
        {
            consume_int(ustr_xsnprintf(ustr_bptr(buffer), elemof32(buffer),
                                       USTR_TEXT("REF_IDX " U32_XFMT),
                                       (U32) ref_idx));
        }
        break;
        }

    case tMemAreaR:
        {
        U16 n_rect = xls_read_U16_LE(*p_p_formula_end);
        *p_p_formula_end += n_rect * 6 + 2;
        break;
        }

    case tMemNoMemR:
    case tMemErrR:
    case tMemFuncR:
    case tMemAreaNR:
    case tMemNoMemNR:
    default:
        consume_int(ustr_xsnprintf(ustr_bptr(buffer), elemof32(buffer),
                                   USTR_TEXT("TOKEN " U32_XFMT),
                                   (U32) sym));
        break;
    }

    if(status_ok(*p_status = ustr_set(&ustr_res, ustr_bptr(buffer))))
        return(ustr_res);

    return(P_USTR_NONE);
}

_Check_return_
_Ret_z_ /* never NULL */
static inline P_USTR
stack_element_peek(
    _InVal_     U32 element_idx)
{
    P_USTR ret;
    PTR_ASSERT(arg_stack);
    assert(arg_stack_n >= (S32) (1 + element_idx));
    ret = arg_stack[arg_stack_n - (1 + element_idx)];
    PTR_ASSERT(ret);
    return(ret);
}

_Check_return_
_Ret_z_ /* never NULL */
static inline P_USTR
stack_element_pop(void)
{
    P_USTR ret;
    PTR_ASSERT(arg_stack);
    assert(arg_stack_n >= 1);
    ret = arg_stack[--arg_stack_n];
#if CHECKING
    arg_stack[arg_stack_n] = NULL;
#endif
    PTR_ASSERT(ret);
    return(ret);
}

_Check_return_
static inline STATUS
stack_element_push(
    _In_z_      P_USTR p_new_element)
{
    PTR_ASSERT(p_new_element);
    assert(arg_stack_n < MAXSTACK);
    if(arg_stack_n >= MAXSTACK)
        return(XLS_ERR_EXP);
    arg_stack[arg_stack_n++] = p_new_element;
    return(STATUS_OK);
}

/******************************************************************************
*
* decode function entry
*
******************************************************************************/

_Check_return_
static STATUS
func_decode(
    _InRef_     PC_XLS_FUNC_ENTRY p_xls_func_entry,
    _InVal_     U8 n_args_in)
{
    U8 n_args = n_args_in;
    STATUS status;
    PC_USTR ustr_t5 = (PC_USTR) p_xls_func_entry->p_text_t5; /* U is superset of A7 */
    P_USTR ustr_extern = NULL;
    U32 len_tot = 1; /*CH_NULL*/
    P_USTR p_ele_new;

    if( n_args > (U8) arg_stack_n)
        n_args = (U8) arg_stack_n; /* limit, and see what we can make of it */

    if(BIFF_FN_ExternCall == p_xls_func_entry->biff_function_number)
    {
        /* permute EXTERN.CALL args such that function name moves to TOS and all actual function args move down one */
        P_USTR function_arg = arg_stack[(arg_stack_n - n_args) + 0];
        memmove32(&arg_stack[(arg_stack_n - n_args) + 0],
                  &arg_stack[(arg_stack_n - n_args) + 1],
                  sizeof32(arg_stack[0]) * (n_args - 1));
        arg_stack[arg_stack_n - 1] = function_arg; /* TOS */
        ustr_extern = stack_element_pop();
        n_args -= 1;
        reportf(TEXT("EXTERN.CALL %s"), report_ustr(ustr_extern));
        ustr_t5 = ustr_extern;
    }

    len_tot += ustrlen32(ustr_t5);

#if 0
    if(BIFF_FN_Irr == p_xls_func_entry->biff_function_number)
    {
        /* SKS create guess arg for IRR() if needed */
        if(n_args == 1)
        {
            P_USTR guess_arg;

            status_return(ustr_set(&guess_arg, USTR_TEXT("0.1")));

            status_assert(stack_element_push(guess_arg));

            n_args++;
        }

        /* SKS swap arg order for IRR() */
        assert(n_args == 2);
        if(n_args == 2)
            memswap32(&arg_stack[(arg_stack_n - n_args) + 0],
                      &arg_stack[(arg_stack_n - n_args) + 1],
                      sizeof32(arg_stack[0]) * 1);
    }
#endif

    if(0 != n_args)
    {
        U32 arg_idx = n_args;

        /* add up the length of all the arguments and commas */
        do  {
            PC_USTR ustr_arg = stack_element_peek(--arg_idx);

            len_tot += ustrlen32(ustr_arg);

            if(0 != arg_idx)
                len_tot++;
        }
        while(0 != arg_idx);
    }

    /* add in space for function brackets */
    len_tot += 2;

    status_return(status = ustr_set_n(&p_ele_new, NULL /* allocate space */, len_tot));

    ustr_xstrkpy(p_ele_new, len_tot, ustr_t5);

    ustr_xstrkat(p_ele_new, len_tot, USTR_TEXT("("));

    if(0 != n_args)
    {
        U32 arg_idx = n_args;

        do  {
            P_USTR ustr_arg = stack_element_peek(--arg_idx);

            ustr_xstrkat(p_ele_new, len_tot, ustr_arg);

            if(0 != arg_idx)
                ustr_xstrkat(p_ele_new, len_tot, USTR_TEXT(","));

            ustr_clr(&ustr_arg);
        }
        while(0 != arg_idx);
    }

    ustr_xstrkat(p_ele_new, len_tot, USTR_TEXT(")"));

    arg_stack_n -= n_args;

    ustr_clr(&ustr_extern);

    return(stack_element_push(p_ele_new));
}

/******************************************************************************
*
* decompile an Excel formula
*
******************************************************************************/

static const struct OPERATOR_TEXT
{
    PC_A7STR p_text_t5;
}
operator_text[0x15] = /* MUST be in Excel token order, just spanning the base tokens that are unary/binary operators */
{
    /*00H*/ /*tNotUsed  */  NULL,
    /*01H*/ /*tExp      */  NULL,
    /*02H*/ /*tTbl      */  NULL,
    /*03H*/ /*tAdd      */  "+",
    /*04H*/ /*tSub      */  "-",
    /*05H*/ /*tMul      */  "*",
    /*06H*/ /*tDiv      */  "/",
    /*07H*/ /*tPower    */  "^",
    /*08H*/ /*tConcat   */  "&&", /* XLS:& */
    /*09H*/ /*tLT       */  "<",
    /*0AH*/ /*tLE       */  "<=",
    /*0BH*/ /*tEQ       */  "=",
    /*0CH*/ /*tGE       */  ">=",
    /*0DH*/ /*tGT       */  ">",
    /*0EH*/ /*tNE       */  "<>",
    /*0FH*/ /*tIsect    */  " ",

    /*10H*/ /*tList     */  ",",
    /*11H*/ /*tRange    */  ":",
    /*12H*/ /*tUplus    */  "+",
    /*13H*/ /*tUminus   */  "-",
    /*14H*/ /*tPercent  */  "%"
};

_Check_return_
static STATUS
xls_decode_formula_ptg_UNARY(void)
{
    const PC_USTR p_text_t5 = (PC_USTR) operator_text[cursym].p_text_t5;
    P_USTR p_ele_stack;
    U32 len_add, len_tot;
    P_USTR p_ele_new;

    if(arg_stack_n < 1)
        status_return(create_error(XLS_ERR_EXP));

    p_ele_stack = stack_element_pop();

    len_add = ustrlen32(p_text_t5);

    /* extra for new operator and terminator */
    len_tot = len_add + ustrlen32p1(p_ele_stack);

    status_return(ustr_set_n(&p_ele_new, NULL /* allocate space */, len_tot));

    ustr_xstrkpy(p_ele_new, len_tot, p_text_t5);

    ustr_xstrkat(p_ele_new, len_tot, p_ele_stack);

    ustr_clr(&p_ele_stack);

    return(stack_element_push(p_ele_new));
}

_Check_return_
static STATUS
xls_decode_formula_ptg_BINARY(void)
{
    const PC_USTR p_text_t5 = (PC_USTR) operator_text[cursym].p_text_t5;
    P_USTR p_ele_1, p_ele_2;
    U32 len_add, ele_1_len, ele_2_len, len_tot;
    P_USTR p_ele_new;

    if(arg_stack_n < 2)
        status_return(create_error(XLS_ERR_EXP));

    p_ele_2 = stack_element_pop();
    ele_2_len = ustrlen32(p_ele_2);

    p_ele_1 = stack_element_pop();
    ele_1_len = ustrlen32(p_ele_1);

    len_add = ustrlen32(p_text_t5);

    /* two arguments, operator and terminator */
    len_tot = ele_1_len + ele_2_len + len_add + 1;

    status_return(ustr_set_n(&p_ele_new, NULL /* allocate space */, len_tot));

    ustr_xstrkpy(p_ele_new, len_tot, p_ele_1);

    ustr_xstrkat(p_ele_new, len_tot, p_text_t5);

    ustr_xstrkat(p_ele_new, len_tot, p_ele_2);

    ustr_clr(&p_ele_1);
    ustr_clr(&p_ele_2);

    return(stack_element_push(p_ele_new));
}

_Check_return_
static STATUS
xls_decode_formula_ptg_FUNC(void)
{
    const S32 symR = (cursym & ~0x60) | 0x20; /* convert V and A to corresponding R token */
    U16 biff_function_number;
    U8 n_args = 0; /* keep dataflower happy */
    PC_XLS_FUNC_ENTRY p_xls_func_entry = NULL;

    switch(symR)
    {
    case tFuncR:
        if(biff_version >= 4)
            biff_function_number = xls_read_U16_LE(p_scan + 1);
        else /* (biff_version < 4) */
            biff_function_number = (U16) p_scan[1];

        if(NULL != (p_xls_func_entry = xls_func_lookup(biff_function_number)))
            n_args = p_xls_func_entry->n_args;
        break;

    case tFuncVarR:
        if(biff_version >= 4)
        {
            const U16 func_number_word = xls_read_U16_LE(p_scan + 2);
            n_args = (U8) (p_scan[1] & 0x7F);
            if(func_number_word & 0x8000U)
                break;
            biff_function_number = (U16) (func_number_word & 0x7FFFU);
        }
        else /* (biff_version < 4) */
        {
            n_args = (U8) p_scan[1];
            biff_function_number = (U16) p_scan[2];
        }

        p_xls_func_entry = xls_func_lookup(biff_function_number);
        break;

#if CHECKING
    case tFuncCER:
#endif
    default: default_unhandled();
        break;
    }

    if(NULL == p_xls_func_entry)
        return(create_error(XLS_ERR_EXP));

    return(func_decode(p_xls_func_entry, n_args));
}

_Check_return_
static STATUS
xls_decode_formula_ptg_CONSTANT(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _Inout_     PC_BYTE * const p_p_formula_end)
{
    STATUS status;
    P_USTR p_ele_new;

    if(NULL == (p_ele_new = constant_convert(p_xls_load_info, cursym, p_scan, p_p_formula_end, &status)))
        return(status);

    return(stack_element_push(p_ele_new));
}

_Check_return_
static STATUS
xls_decode_formula_ptg_OPERAND(
    P_XLS_LOAD_INFO p_xls_load_info,
    _Inout_     PC_BYTE * const p_p_formula_end)
{
    STATUS status;
    P_USTR p_ele_new;

    if(NULL == (p_ele_new = operand_convert(p_xls_load_info, cursym, p_scan, p_p_formula_end, &status)))
        return(status);

    return(stack_element_push(p_ele_new));
}

_Check_return_
static STATUS
xls_decode_formula_ptg_tParen(void)
{
    P_USTR p_ele_stack;
    U32 len_tot;
    P_USTR p_ele_new;

    if(arg_stack_n < 1)
        status_return(create_error(XLS_ERR_EXP));

    p_ele_stack = stack_element_pop();

    len_tot = ustrlen32p1(p_ele_stack) + 2 /* add two for brackets */;

    status_return(ustr_set_n(&p_ele_new, NULL /* allocate space */, len_tot));

    ustr_xstrkpy(p_ele_new, len_tot, USTR_TEXT("("));

    ustr_xstrkat(p_ele_new, len_tot, p_ele_stack);

    ustr_xstrkat(p_ele_new, len_tot, USTR_TEXT(")"));

    ustr_clr(&p_ele_stack);

    return(stack_element_push(p_ele_new));
}

_Check_return_
static STATUS
xls_decode_formula_ptg_OTHER(
    P_XLS_LOAD_INFO p_xls_load_info)
{
    STATUS status = STATUS_OK;

    switch(cursym)
    {
    default: default_unhandled();
        break;

    case tExp:
    case tTbl:
        assert0();
        break;

    case tParen:
        return(xls_decode_formula_ptg_tParen());

    case tNlr:
        assert0();
        break;

    case tAttr:
        {
        U8 grbit = p_scan[1];
        if(grbit & 0x10)
        {   /* optimised SUM function */
            PC_XLS_FUNC_ENTRY p_xls_func_entry = xls_func_lookup(BIFF_FN_Sum);
            PTR_ASSERT(p_xls_func_entry);
            status = func_decode(p_xls_func_entry, 1);
        }
        break;
        }

    case tSheet:
        {
        U16 ixals = xls_read_U16_LE(p_scan + 5);
        U16 record_length;
        U32 opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, ixals, X_EXTERNSHEET, 1);

        if(XLS_NO_RECORD_FOUND != opcode_offset)
            extern_opcode_offset = opcode_offset;

        break;
        }

    case tEndSheet:
        extern_opcode_offset = 0;
        break;
    }

    return(status);
}

_Check_return_
static STATUS
xls_decode_formula(
    P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(formula_len) PC_BYTE p_formula,
    _InVal_     U16 formula_len)
{
    STATUS status = STATUS_OK;
    PC_BYTE p_formula_end = p_formula + formula_len;

    arg_stack_n = 0;

    p_scan = p_formula;

    for(;;)
    {
        token_check();

        switch(p_ptg_entry->type)
        {
        default: default_unhandled();
        case NOTUSED:
            break;

        case UNARY:
            status = xls_decode_formula_ptg_UNARY();
            break;

        case BINARY:
            status = xls_decode_formula_ptg_BINARY();
            break;

        case FUNC:
            status = xls_decode_formula_ptg_FUNC();
            break;

        case CONSTANT:
            status = xls_decode_formula_ptg_CONSTANT(p_xls_load_info, &p_formula_end);
            break;

        case OPERAND:
            status = xls_decode_formula_ptg_OPERAND(p_xls_load_info, &p_formula_end);
            break;

        case OTHER:
            status = xls_decode_formula_ptg_OTHER(p_xls_load_info);
            break;
        }

        status_break(status);

        token_skip();

        if(PtrDiffBytesU32(p_scan, p_formula) >= (U32) formula_len)
        {   /* ended formula */
            assert(PtrDiffBytesU32(p_scan, p_formula) == (U32) formula_len);
            break;
        }
    }

    if(status_ok(status))
        if(arg_stack_n != 1)
            status = /*create_error*/(XLS_ERR_EXP);

    if(status_ok(status))
        status = quick_ublock_ustr_add(p_quick_ublock, stack_element_peek(0));

    stack_free();

    return(status);
}

_Check_return_
static STATUS
xls_decode_formula_result(
    P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_c_(sizeof32(F64)) PC_BYTE p_formula_result)
{
    UNREFERENCED_PARAMETER(p_xls_load_info);

    if( (0xFF == PtrGetByteOff(p_formula_result, 7)) &&
        (0xFF == PtrGetByteOff(p_formula_result, 6)) )
    {
        switch(PtrGetByte(p_formula_result))
        {
        case 0: /* STRING */
            return(STATUS_OK);

        case 1: /* Boolean */
            return(quick_ublock_ustr_add(p_quick_ublock, (0 != PtrGetByteOff(p_formula_result, 2)) ? USTR_TEXT("TRUE") : USTR_TEXT("FALSE")));

        case 2: /* Error */
            return(STATUS_OK);

        default:
            break;
        }
    }

    {
    F64 f64 = xls_read_F64(p_formula_result);
    STATUS status = quick_ublock_printf(p_quick_ublock, USTR_TEXT("%." stringize(DBL_DECIMAL_DIG) "g"), f64);
    return(status);
    } /*block*/
}

/* locate the shared formula that covers this cell */

_Check_return_
static STATUS
xls_try_this_shared_formula(
    P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _InVal_     U16 record_length,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     XLS_COL tl_col,
    _InVal_     XLS_ROW tl_row)
{
    PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    XLS_ROW this_tl_row = (XLS_ROW) xls_read_U16_LE(p_x);
    XLS_ROW this_br_row = (XLS_ROW) xls_read_U16_LE(p_x + 2);
    XLS_COL this_tl_col = (XLS_COL) p_x[4]; /* NB only a byte, even for BIFF8 */
    XLS_COL this_br_col = (XLS_COL) p_x[5];

    if( (tl_col >= this_tl_col) && (tl_row >= this_tl_row) &&
        (tl_col <= this_br_col) && (tl_row <= this_br_row) )
    {
        PC_BYTE p_shared_formula = p_x + 10;
        U16 shared_formula_len = xls_read_U16_LE(p_x + 8);
        status_return(xls_decode_formula(p_xls_load_info, p_quick_ublock, p_shared_formula, shared_formula_len));
        return(STATUS_DONE);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_get_shared_formula(
    P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(formula_len) PC_BYTE p_formula,
    _InVal_     U16 formula_len)
{
    STATUS status = STATUS_OK;
    U16 record_length;
    U32 opcode_offset;
    XLS_COL tl_col;
    XLS_ROW tl_row;

    tl_row = (XLS_ROW) xls_read_U16_LE(p_formula + 1);

    if(biff_version >= 3)
        tl_col = (XLS_COL) xls_read_U16_LE(p_formula + 3);
    else /* (biff_version == 2) */
        tl_col = (XLS_COL) p_formula[3];

    UNREFERENCED_PARAMETER_InVal_(formula_len);

    /* first, try cached offset as they do repeat a LOT */
    if(0 != p_xls_load_info->shared_formula_opcode_offset)
    {
        status_return(status = xls_try_this_shared_formula(p_xls_load_info, p_xls_load_info->shared_formula_opcode_offset, p_xls_load_info->shared_formula_record_length, p_quick_ublock, tl_col, tl_row));
        if(status == STATUS_DONE)
        {
            return(STATUS_OK);
        }
    }

    p_xls_load_info->shared_formula_opcode_offset = 0;
    p_xls_load_info->shared_formula_record_length = 0;

    opcode_offset = p_xls_load_info->worksheet_substream_offset;

    for(opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, X_SHRFMLA_B5_B8, &record_length);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next (p_xls_load_info, opcode_offset, X_SHRFMLA_B5_B8, &record_length))
    {
        status_return(status = xls_try_this_shared_formula(p_xls_load_info, opcode_offset, record_length, p_quick_ublock, tl_col, tl_row));
        if(status == STATUS_DONE)
        {
            p_xls_load_info->shared_formula_opcode_offset = opcode_offset;
            p_xls_load_info->shared_formula_record_length = record_length;
            return(STATUS_OK);
        }
    }

    return(status);
}

/******************************************************************************
*
* make names
*
******************************************************************************/

static const PC_USTR
built_in_name_table[] =
{
    USTR_TEXT("Consolidate_Area"),
    USTR_TEXT("Auto_Open"),
    USTR_TEXT("Auto_Close"),
    USTR_TEXT("Extract"),
    USTR_TEXT("Database"),
    USTR_TEXT("Criteria"),
    USTR_TEXT("Print_Area"),
    USTR_TEXT("Print_Titles"),
    USTR_TEXT("Recorder"),
    USTR_TEXT("Data_Form"),
    USTR_TEXT("Auto_Activate"),
    USTR_TEXT("Auto_Deactivate"),
    USTR_TEXT("Sheet_Title"),
    USTR_TEXT("_FilterDatabase")
};

_Check_return_
static STATUS
xls_names_make(
    P_XLS_LOAD_INFO p_xls_load_info,
    _DocuRef_   P_DOCU p_docu,
    const U16 name_opcode)
{
    STATUS status = STATUS_OK;
    S16 i;

    for(i = 0; status_ok(status); ++i)
    {
        U16 record_length;
        U32 opcode_offset = xls_find_record_index(p_xls_load_info, &record_length, i, name_opcode, 0);
        PC_BYTE p_x;
        U16 option_flags;
        BYTE string_flags = 0;
        U8 name_len;
        PC_BYTE p_name;
        U16 formula_len;
        PC_BYTE p_formula;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_name, 64);
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_formula, 128);
        quick_ublock_with_buffer_setup(quick_ublock_name);
        quick_ublock_with_buffer_setup(quick_ublock_formula);

        if(XLS_NO_RECORD_FOUND == opcode_offset)
            break;

        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        option_flags = xls_read_U16_LE(p_x);

        if(option_flags & ((1<<12/*binary*/)|(1<<3/*macro*/)|(1<<2/*vbasic*/)|(1<<1/*func*/)))
            continue;

        name_len = p_x[3];

        if(X_DEFINEDNAME_B3_B4 == name_opcode)
        {
            formula_len = xls_read_U16_LE(p_x + 4);
            p_name = p_x + 6;
        }
        else /* (X_DEFINEDNAME_B2_B5_B8 == name_opcode) */
        {
            if(biff_version >= 5)
            {
                formula_len = xls_read_U16_LE(p_x + 4);
                p_name = p_x + 14;
            }
            else /* (biff_version == 2) */
            {
                formula_len = p_x[4];
                p_name = p_x + 5;
            }
        }

        if(option_flags & 0x20)
        {   /* built-in name */
            U16 name_idx;
            
            BIFF_VERSION_ASSERT(>= 3);

            if(biff_version >= 8)
            {
                string_flags = *p_name++;

                if(string_flags & 0x01)
                {   /* Unicode UTF-16LE string */
                    name_idx = xls_read_U16_LE(p_name);
                }
                else
                    name_idx = *p_name;
            }
            else
                name_idx = *p_name;

            if(name_idx < elemof32(built_in_name_table))
            {
                PC_USTR built_in_name = built_in_name_table[name_idx];
                status = quick_ublock_ustr_add(&quick_ublock_name, built_in_name);
            }
            else
            {
                assert(name_idx < elemof32(built_in_name_table));
                status = status_check();
            }

            if(string_flags & 0x01)
                p_formula = PtrAddBytes(PC_BYTE, p_name, name_len * 2); /* Unicode UTF-16LE string */
            else
                p_formula = PtrAddBytes(PC_BYTE, p_name, name_len);
        }
        else
        {   /* full name */
            if(biff_version >= 8)
            {
                U16 rt = 0;
                U32 sz = 0;

                status = xls_quick_ublock_xls_string_add(&quick_ublock_name, p_name, name_len, p_xls_load_info->sbchar_codepage, FALSE); /* NB may be unaligned */

                string_flags = *p_name++;

                if(string_flags & 0x08) /* rich-text */
                {
                    rt = xls_read_U16_LE(p_name);
                    p_name += 2;
                }
                if(string_flags & 0x04) /* phonetic */
                {
                    sz = xls_read_U32_LE(p_name);
                    p_name += 4;
                }

                if(string_flags & 0x01)
                    p_name += name_len * 2; /* Unicode UTF-16LE string */
                else
                    p_name += name_len;

                p_name += 4 * rt;
                p_name += sz;

                p_formula = p_name;
            }
            else
            {
                status = quick_ublock_sbchars_add(&quick_ublock_name, (PC_SBCHARS) p_name, name_len);

                p_formula = PtrAddBytes(PC_BYTE, p_name, name_len);
            }
        }

        if(status_ok(status))
           status = quick_ublock_nullch_add(&quick_ublock_name);

        if(formula_len && status_ok(status) &&
           (0 != quick_ublock_bytes(&quick_ublock_name))
           &&
           status_ok(xls_decode_formula(p_xls_load_info, &quick_ublock_formula, p_formula, formula_len))
           &&
           (0 != quick_ublock_bytes(&quick_ublock_formula))
           &&
           status_ok(quick_ublock_nullch_add(&quick_ublock_formula)))
        {
            SS_NAME_MAKE ss_name_make;

            ss_name_make.ustr_name_id = quick_ublock_ustr(&quick_ublock_name);
            ss_name_make.ustr_name_def = quick_ublock_ustr(&quick_ublock_formula);
            ss_name_make.undefine = 0;
            ss_name_make.ustr_description = NULL;
            status_assert(status = object_call_id(OBJECT_ID_SS, p_docu, T5_MSG_SS_NAME_MAKE, &ss_name_make));
        }

        quick_ublock_dispose(&quick_ublock_name);
        quick_ublock_dispose(&quick_ublock_formula);
    }

    return(status);
}

/******************************************************************************
*
* check Excel cell format for a date
*
******************************************************************************/

_Check_return_
static BOOL
xls_may_be_date_format(
    PC_BYTE p_format,
    _InVal_     U32 format_len)
{
    BOOL seen_seconds = FALSE;
    BYTE string_flags = 0;
    U32 i;

    if(biff_version >= 8)
    {
        string_flags = *p_format++;

        if(string_flags & 0x08) /* rich-text */
            p_format += 2;
        if(string_flags & 0x04) /* phonetic */
            p_format += 4;
    }

    /* If all of the characters in the format string are in the set
     * SPACE, MINUS, COLON, SLASH, h, m, s, d, y, AM, PM
     * (and DOT and ZERO for fractional seconds)
     * then it may be a date format string
     */
    for(i = 0; i < format_len; ++i)
    {
        WCHAR format_ch;

        if(string_flags & 0x01)
        {   /* Unicode UTF-16LE string */
            format_ch = xls_read_WCHAR_off((PCWCH) p_format, i);
        }
        else
            format_ch = p_format[i];

        switch(format_ch)
        {
        default:
            return(FALSE);

        case CH_DIGIT_ZERO:
            if(!seen_seconds)
                return(FALSE);
            break;

        case CH_SPACE:
        case CH_HYPHEN_MINUS:
        case CH_COLON:
        case CH_FORWARDS_SLASH:
        case CH_BACKWARDS_SLASH: /* things can be escaped (seen 'D\ MMM\ YY' in files) */
        case CH_FULL_STOP:
        case 'A': case 'P': /* AM / PM */
            break;

        case 'd': case 'D':
        case 'h': case 'H':
        case 'm': case 'M':
        case 'y': case 'Y':
            break;

        case 's': case 'S':
            seen_seconds = TRUE;
            break;
        }
    }

    return(TRUE);
}

static const struct ExcelBuiltinFORMAT
{
    BOOL is_date_format;
    PC_SBSTR sbstr_format; /* Now Fireworkz spec - original Excel spec in adjacent comment */
}
ExcelBuiltinFORMAT[0x32] =
{
/*0x00*/ {  FALSE,  SBSTR_TEXT("General") }, /* NB this is treated specially */
/*0x01*/ {  FALSE,  SBSTR_TEXT("0") },
/*0x02*/ {  FALSE,  SBSTR_TEXT("0.00") },
/*0x03*/ {  FALSE,  SBSTR_TEXT("#,##0") },
/*0x04*/ {  FALSE,  SBSTR_TEXT("#,##0.00") },
/*0x05*/ {  FALSE,  SBSTR_TEXT(" \\" "\xA3" "#,##0 ;(\\" "\xA3" "#,##0)") }, /*XLS spec "_(�#,##0_);(�#,##0)"*/
/*0x06*/ {  FALSE,  SBSTR_TEXT(" \\" "\xA3" "#,##0 ;[Red](\\" "\xA3" "#,##0)") }, /*XLS spec "_(�#,##0_);[Red](�#,##0)"*/
/*0x07*/ {  FALSE,  SBSTR_TEXT(" \\" "\xA3" "#,##0.00 ;(\\" "\xA3" "#,##0.00)") }, /*XLS spec "_(�#,##0.00_);(�#,##0.00)"*/
/*0x08*/ {  FALSE,  SBSTR_TEXT(" \\" "\xA3" "#,##0.00 ;[Red](\\" "\xA3" "#,##0.00)") }, /*XLS spec "_(�#,##0.00_);[Red](�#,##0.00)"*/
/*0x09*/ {  FALSE,  SBSTR_TEXT("0%") },
/*0x0a*/ {  FALSE,  SBSTR_TEXT("0.00%") },
/*0x0b*/ {  FALSE,  SBSTR_TEXT("0.00E+00") },
/*0x0c*/ {  FALSE,  SBSTR_TEXT("0.0#####") }, /*XLS spec "# \?/\?"*/ /*Fraction*/
/*0x0d*/ {  FALSE,  SBSTR_TEXT("0.0#####") }, /*XLS spec "# \?\?/\?\?"*/ /*Fraction*/
/*0x0e*/ {  TRUE,   SBSTR_TEXT("m/d/yy") },
/*0x0f*/ {  TRUE,   SBSTR_TEXT("d\\-Mmm\\-yy") },
/*0x10*/ {  TRUE,   SBSTR_TEXT("d\\-Mmm") },
/*0x11*/ {  TRUE,   SBSTR_TEXT("Mmm\\-yy") },
/*0x12*/ {  TRUE,   SBSTR_TEXT("h:mm \"AM\";h:mm \"PM\"") }, /*XLS spec "h:mm AM/PM"*/
/*0x13*/ {  TRUE,   SBSTR_TEXT("h:mm:ss \"AM\";h:mm:ss \"PM\"") }, /*XLS spec "h:mm:ss AM/PM"*/
/*0x14*/ {  TRUE,   SBSTR_TEXT("h:mm") },
/*0x15*/ {  TRUE,   SBSTR_TEXT("h:mm:ss") },
/*0x16*/ {  TRUE,   SBSTR_TEXT("m/d/yy h:mm") },
/*0x17*/ {  FALSE,  SBSTR_TEXT("") },
/*0x18*/ {  FALSE,  SBSTR_TEXT("") },
/*0x19*/ {  FALSE,  SBSTR_TEXT("") },
/*0x1a*/ {  FALSE,  SBSTR_TEXT("") },
/*0x1b*/ {  FALSE,  SBSTR_TEXT("") },
/*0x1c*/ {  FALSE,  SBSTR_TEXT("") },
/*0x1d*/ {  FALSE,  SBSTR_TEXT("") },
/*0x1e*/ {  FALSE,  SBSTR_TEXT("") },
/*0x1f*/ {  FALSE,  SBSTR_TEXT("") },
/*0x20*/ {  FALSE,  SBSTR_TEXT("") },
/*0x21*/ {  FALSE,  SBSTR_TEXT("") },
/*0x22*/ {  FALSE,  SBSTR_TEXT("") },
/*0x23*/ {  FALSE,  SBSTR_TEXT("") },
/*0x24*/ {  FALSE,  SBSTR_TEXT("") },
/*0x25*/ {  FALSE,  SBSTR_TEXT(" #,##0 ;(#,##0)") }, /*XLS spec "_(#,##0_);(#,##0)"*/
/*0x26*/ {  FALSE,  SBSTR_TEXT(" #,##0 ;[Red](#,##0)") },/*XLS spec "_(#,##0_);[Red](#,##0)"*/
/*0x27*/ {  FALSE,  SBSTR_TEXT(" #,##0.00 ;(#,##0.00)") }, /*XLS spec "_(#,##0.00_);(#,##0.00)"*/
/*0x28*/ {  FALSE,  SBSTR_TEXT(" #,##0.00 ;[Red](#,##0.00)") }, /*XLS spec "_(#,##0.00_);[Red](#,##0.00)" */
/*0x29*/ {  FALSE,  SBSTR_TEXT("  #,##0 ; (#,##0)") }, /*XLS spec "_(* #,##0_);_(* (#,##0);_(* \"-\"_);_(@_)"*/
/*0x2a*/ {  FALSE,  SBSTR_TEXT("  \\" "\xA3" " #,##0 ; (\\" "\xA3" " #,##0)") }, /*XLS spec "_(�* #,##0_);_(�* (#,##0);_(�* \"-\"_);_(@_)"*/
/*0x2b*/ {  FALSE,  SBSTR_TEXT("  #,##0.00 ; (#,##0.00)") }, /*XLS spec "_(* #,##0.00_);_(* (#,##0.00);_(* \"-\"??_);_(@_)"*/
/*0x2c*/ {  FALSE,  SBSTR_TEXT("  \\" "\xA3" " #,##0.00 ; (\\" "\xA3" " #,##0.00)") }, /*XLS spec "_(�* #,##0.00_);_(�* (#,##0.00);_(�* \"-\"??_);_(@_)"*/
/*0x2d*/ {  TRUE,   SBSTR_TEXT("mm:ss") },
/*0x2e*/ {  TRUE,   SBSTR_TEXT("h:mm:ss") }, /*XLS spec "[h]:mm:ss"*/
/*0x2f*/ {  TRUE,   SBSTR_TEXT("mm:ss") }, /*XLS spec "mm:ss.0"*/
/*0x30*/ {  FALSE,  SBSTR_TEXT("##0.0E+0") },
/*0x31*/ {  FALSE,  SBSTR_TEXT("") } /*XLS spec "@"*/
};

_Check_return_
static STATUS
xls_check_format(
    P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     FORMAT_INDEX format_index_encoded,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_format,
    _OutRef_    P_BOOL p_is_date_format)
{
    STATUS status;
    U16 record_length;
    U32 opcode_offset;
    PC_BYTE p_x;
    FORMAT_INDEX format_index = format_index_encoded;
    const XLS_OPCODE format_opcode = (biff_version >= 4) ? X_FORMAT_B4_B8 : X_FORMAT_B2_B3;
    U32 format_len = 0;
    PC_BYTE p_format = NULL;

    *p_is_date_format = FALSE;

    /* search for it in the file */
    opcode_offset = xls_find_record_FORMAT_INDEX(p_xls_load_info, &record_length, format_index, format_opcode);

    if(XLS_NO_RECORD_FOUND != opcode_offset)
    {
        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        if(biff_version >= 8)
        {
            format_len = xls_read_U16_LE(p_x + 2);
            p_format = p_x + 4;
        }
        else if(biff_version >= 4)
        {
            format_len = p_x[2];
            p_format = p_x + 3;
        }
        else /* (biff_version < 4) */
        {
            format_len = p_x[0];
            p_format = p_x + 1;
        }
    }
    else if((biff_version >= 5) && (format_index < 164))
    {   /* only if not found in file consider the hardwired built-in formats */
        PC_SBSTR sbstr_format = empty_string;
        if(14 == format_index)
        {
            sbstr_format = SBSTR_TEXT("dd/mm/yyyy"); /* 'taken from operating system' says Excel doc... British for now says SKS */
            *p_is_date_format = TRUE;
        }
        else if(22 == format_index)
        {
            sbstr_format = SBSTR_TEXT("dd/mm/yyyy hh:mm"); /* 'taken from operating system' says Excel doc... British for now says SKS */
            *p_is_date_format = TRUE;
        }
        else if(format_index < elemof32(ExcelBuiltinFORMAT))
        {
            sbstr_format = ExcelBuiltinFORMAT[format_index].sbstr_format;
            *p_is_date_format = ExcelBuiltinFORMAT[format_index].is_date_format;
        }
        if(CH_NULL == *sbstr_format)
            return(STATUS_OK);
        status_return(quick_ublock_sbstr_add_n(p_quick_ublock_format, sbstr_format, strlen_without_NULLCH));
        return(quick_ublock_nullch_add(p_quick_ublock_format));
    }

    if((NULL == p_format) || (0 == format_len))
        return(STATUS_OK);

    *p_is_date_format = xls_may_be_date_format(p_format, format_len);

    {
    P_UCHARS uchars;
    U32 num_sections = 1;
    BOOL ended = FALSE;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
    quick_ublock_with_buffer_setup(quick_ublock);

    status_assert(xls_quick_ublock_xls_string_add(&quick_ublock, p_format, format_len, p_xls_load_info->sbchar_codepage, FALSE)); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

    status_assert(quick_ublock_nullch_add(&quick_ublock)); /* terminate for ease of copying */

    /* copy to output, substituting Fireworkz syntax for Excel as appropriate */
    uchars = quick_ublock_uchars_wr(&quick_ublock); /* non-const 'cos we do tweak it */

    { /* patch common patterns */
    U32 uchars_n = quick_ublock_bytes(&quick_ublock);
    PC_SBSTR search, replace;
    U32 search_n;
    P_SBSTR found;

    /* AM/PM needs appropriate furtling */
    search = SBSTR_TEXT("AM/PM"); search_n = strlen32(search);
    if(NULL != (found = memstr32(uchars, uchars_n, search, search_n)))
    {
        U32 prefix_len = PtrDiffBytesU32(found, uchars);
        status_return(quick_ublock_uchars_add(p_quick_ublock_format, uchars, prefix_len));
        status_return(quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("\"AM\";")));
        status_return(quick_ublock_uchars_add(p_quick_ublock_format, uchars, prefix_len));
        status_return(quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("\"PM\"")));
        return(quick_ublock_nullch_add(p_quick_ublock_format));
    }

    /* month names need appropriate capitalisation */
    search = SBSTR_TEXT("mmm"); search_n = strlen32(search); replace = SBSTR_TEXT("Mmm");
    if(NULL != (found = memstr32(uchars, uchars_n, search, search_n)))
    {
        memcpy32(found, replace, search_n);
    }
    } /*block*/

    do  {
        switch(PtrGetByte(uchars))
        {
        case CH_NULL:
            ended = TRUE;
            break;

        case CH_SEMICOLON:
            uchars_IncByte_wr(uchars);
            if(num_sections == 3)
            {
                ended = TRUE;
            }
            else
            {
                ++num_sections;
                /* and copy the section delimiter to output */
                status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_SEMICOLON));
            }
            break;

        case CH_BACKWARDS_SLASH: /* Literal next character */
            uchars_IncByte_wr(uchars);
            if(CH_NULL == PtrGetByte(uchars))
            {
                assert0();
                ended = TRUE;
            }
            else
            {
                U32 bytes_of_char = uchars_bytes_of_char(uchars);
                status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_BACKWARDS_SLASH));
                /* and copy the next whole character to the output */
                status_return(status = quick_ublock_uchars_add(p_quick_ublock_format, uchars, bytes_of_char));
                uchars_IncBytes_wr(uchars, bytes_of_char);
            }
            break;

        case CH_QUOTATION_MARK: /* Literal string */
            uchars_IncByte_wr(uchars);
            if(CH_NULL == PtrGetByte(uchars))
            {
                assert0();
                ended = TRUE;
            }
            else
            {
                U32 bytes_of_char = uchars_bytes_of_char(uchars);
                status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_QUOTATION_MARK));
                do  {
                    if(CH_QUOTATION_MARK == PtrGetByte(uchars))
                    {
                        status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_QUOTATION_MARK));
                        uchars_IncBytes_wr(uchars, bytes_of_char); /* skipping past end quote */
                        break;
                    }
                    if(!quick_ublock_uchars_add_fast(p_quick_ublock_format, uchars, bytes_of_char))
                        status_return(status = quick_ublock_uchars_add_slow(p_quick_ublock_format, uchars, bytes_of_char));
                    uchars_IncBytes_wr(uchars, bytes_of_char);
                    if(CH_NULL == PtrGetByte(uchars))
                    {
                        assert0();
                        ended = TRUE;
                    }
                }
                while(!ended);
            }
            break;

        case CH_LEFT_SQUARE_BRACKET: /* Style / Conditional */
            uchars_IncByte_wr(uchars);
            if(CH_NULL == PtrGetByte(uchars))
            {
                assert0();
                ended = TRUE;
            }
            else
            {
                PC_UCHARS start = uchars;
                U32 bytes_of_char;

                do  {
                    if(CH_RIGHT_SQUARE_BRACKET == PtrGetByte(uchars))
                    {   /* CH_NULL-terminate this section so we can do string comparision, also skipping past end square bracket */
                        PtrPutByte(uchars, CH_NULL); uchars_IncByte_wr(uchars);
#if USTR_IS_SBSTR
                        if(0 == C_stricmp((const char *) start, "$"     "\xA3" "-809"))
#else
                        if(0 == C_stricmp((const char *) start, "$" "\xC2\xA3" "-809"))
#endif
                        {
                            status_return(status = quick_ublock_ucs4_add(p_quick_ublock_format, UCH_POUND_SIGN));
                        }
                        else if(0 == C_stricmp((const char *) start, "Red"))
                        {   /* Expect to use SheetXLS which has Red style defined */
                            status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_LEFT_SQUARE_BRACKET));
                            status_return(status = quick_ublock_ustr_add(p_quick_ublock_format, start));
                            status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_RIGHT_SQUARE_BRACKET));
                        }
                        else
                        {   /* all other ones we don't understand! but why not pipe them over so user can see? */
                            status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_LEFT_SQUARE_BRACKET));
                            status_return(status = quick_ublock_ustr_add(p_quick_ublock_format, start));
                            status_return(status = quick_ublock_a7char_add(p_quick_ublock_format, CH_RIGHT_SQUARE_BRACKET));
                        }
                        break;
                    }
                    if(CH_NULL == PtrGetByte(uchars))
                    {
                        assert0();
                        ended = TRUE;
                    }
                    else
                    {
                        bytes_of_char = uchars_bytes_of_char(uchars);
                        uchars_IncBytes_wr(uchars, bytes_of_char);
                    }
                }
                while(!ended);
            }
            break;

        case CH_ASTERISK: /* Multiple next character to fill space */
        case CH_UNDERSCORE: /* Single space of next character */
            uchars_IncByte_wr(uchars);
            if(CH_NULL == PtrGetByte(uchars))
            {
                assert0();
                ended = TRUE;
            }
            else
            {   /* Ignore - Fireworkz has no analagous construct */
                U32 bytes_of_char = uchars_bytes_of_char(uchars);
                /* skip the next whole character */
                uchars_IncBytes_wr(uchars, bytes_of_char);
            }
            break;

        default:
            { /* check for other known things that we don't like */
            U32 bytes_of_char = uchars_bytes_of_char(uchars);
            status_return(status = quick_ublock_uchars_add(p_quick_ublock_format, uchars, bytes_of_char));
            uchars_IncBytes_wr(uchars, bytes_of_char);
            break;
            }
        }
    }
    while(!ended);

    quick_ublock_dispose(&quick_ublock);
    } /*block*/

    return(quick_ublock_nullch_add(p_quick_ublock_format));
}

_Check_return_
static BOOL
xls_check_serial_range(
    _InVal_     F64 f64,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_format)
{
    if((f64 >= 0.0) && (f64 < 73049.0 /*~2199*/))
        return(TRUE);

    quick_ublock_empty(p_quick_ublock_format);
    return(FALSE);
}

_Check_return_
static STATUS
xls_convert_serial_to_date(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_result,
    _InVal_     F64 f64,
    _InVal_     BOOL datemode_1904)
{
    /* days in the month */
    static const S32 xls_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    /* integer part denotes date serial number (since 1.1.1900) */
    S32 dateno = (S32) /*trunc*/ f64;
    /* fractional part denotes hh:mm:ss bit (don't be tempted to round or you could add a day!) */
    S32 seconds = (S32) /*trunc*/ ((f64 - (F64) dateno) * FP_SECS_IN_24);
    S32 month = 0, day = 0;
    S32 year = 1900 -1;
    STATUS status;

    if(0 != dateno)
    {
        S32 lasta = 0, dayno = 0;
        BOOL leap = FALSE;

        if(datemode_1904)
            dateno += 1462;

        do  {
            ++year;
            leap = (year & 3) ? FALSE : TRUE; /* same poor test as old Excel to get dates right! */
            month = 0;
            do  {
                lasta = xls_days[month];
                if(leap && (month == 1))
                    ++lasta;
                dayno += lasta;
                ++month;
            }
            while((dayno < dateno) && (month < 12));
        }
        while(dayno < dateno);

        day = (dateno - (dayno - lasta));
    }
    else
    {
        day = 1;
        month = 1;
        year = datemode_1904 ? 1904 : 1900;
    }

    if(0 == seconds)
    {
        status = quick_ublock_printf(p_quick_ublock_result,
                                     USTR_TEXT("%.2" S32_FMT_POSTFIX "." "%.2" S32_FMT_POSTFIX "." "%.4" S32_FMT_POSTFIX),
                                     day, month, year);
    }
    else
    {
        S32 hours = 0;
        S32 minutes = 0;

        minutes = seconds / 60;
        seconds = seconds % 60;

        hours   = minutes / 60;
        minutes = minutes % 60;

        status = quick_ublock_printf(p_quick_ublock_result,
                                     USTR_TEXT("%.2" S32_FMT_POSTFIX "." "%.2" S32_FMT_POSTFIX "." "%.4" S32_FMT_POSTFIX
                                               " "
                                               "%.2" S32_FMT_POSTFIX ":" "%.2" S32_FMT_POSTFIX ":" "%.2" S32_FMT_POSTFIX),
                                     day, month, year,
                                     hours, minutes, seconds);
    }

    return(status);
}

/******************************************************************************
*
* read/convert an Excel cell
*
******************************************************************************/

_Check_return_
static STATUS
xls_cell_make(
    P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U8 data_type,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_result,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_formula,
    _InRef_     P_STYLE p_style,
    _InVal_     STYLE_HANDLE style_handle)
{
    STATUS status = STATUS_OK;
    const P_DOCU p_docu = p_xls_load_info->p_docu;
    SLR actual_slr;
    LOAD_CELL_FOREIGN load_cell_foreign;
    zero_struct_fn(load_cell_foreign);

    actual_slr.col = p_xls_load_info->current_slr.col + p_xls_load_info->offset_slr.col;
    actual_slr.row = p_xls_load_info->current_slr.row + p_xls_load_info->offset_slr.row;

    status_consume(object_data_from_slr(p_docu, &load_cell_foreign.object_data, &actual_slr));
    load_cell_foreign.original_slr = p_xls_load_info->current_slr;

    load_cell_foreign.style = *p_style;
    load_cell_foreign.style_handle = style_handle;

#if 0
    if( style_bit_test(p_style, STYLE_SW_PS_NUMFORM_NU) &&
        (0 != p_style->para_style.h_numform_nu) &&
        (0 == strcmp("General", array_ustr(&p_style->para_style.h_numform_nu))) )
        DebugBreak();
#endif

    if(quick_ublock_bytes(p_quick_ublock_result) || quick_ublock_bytes(p_quick_ublock_formula))
    {
        OBJECT_ID object_id;

        switch(data_type)
        {
        default: default_unhandled();
#if CHECKING
        case XLS_DATA_TEXT:
#endif
            object_id = OBJECT_ID_TEXT;
            load_cell_foreign.data_type = OWNFORM_DATA_TYPE_TEXT;
            load_cell_foreign.ustr_inline_contents = quick_ublock_ustr_inline(p_quick_ublock_result);
            break;

        case XLS_DATA_SS:
            object_id = OBJECT_ID_SS;
            load_cell_foreign.data_type = OWNFORM_DATA_TYPE_FORMULA;

            /* this determines whether cells are recalced after load - supply some
             * contents and the evaluator won't bother to recalc the cell
             */
            load_cell_foreign.ustr_formula =
                quick_ublock_bytes(p_quick_ublock_formula)
                    ? quick_ublock_ustr(p_quick_ublock_formula)
                    : NULL;

            if(NULL == load_cell_foreign.ustr_formula)
                load_cell_foreign.ustr_inline_contents =
                    quick_ublock_bytes(p_quick_ublock_result)
                        ? quick_ublock_ustr_inline(p_quick_ublock_result)
                        : NULL;
            break;
        }

        status = insert_cell_contents_foreign(p_docu, object_id, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);

        if(status_ok(status) && (p_xls_load_info->max_col < actual_slr.col))
            p_xls_load_info->max_col = actual_slr.col;
    }
    else
    {
        status = insert_cell_style_for_foreign(p_docu, &load_cell_foreign);
    }

    return(status);
}

_Check_return_
static S32
xls_slr_in_docu_area_percent(
    _InRef_     PC_SLR p_slr,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    COL col = p_slr->col - p_docu_area->tl.slr.col;
    ROW row = p_slr->row - p_docu_area->tl.slr.row;
    COL cols_n = p_docu_area->br.slr.col - p_docu_area->tl.slr.col;
    ROW rows_n = p_docu_area->br.slr.row - p_docu_area->tl.slr.row;
    S32 percent;

    /* Excel goes across rows then down */
    percent = (row * 100) / rows_n;

    if(rows_n < 100/2)
    {
        /* column number varying can make some useful (>=1%) contribution */
        S32 column_contribution = (col * 100) / (cols_n * rows_n);
        percent += column_contribution;
    }

    return(percent);
}

static void
xls_promote_general_from_style_to_style_handle(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_STYLE p_style,
    _InoutRef_  P_STYLE_HANDLE p_style_handle)
{
    if(0 != style_selector_bit_test(&p_style->selector, STYLE_SW_PS_NUMFORM_NU))
    {
        PC_USTR ustr_format = array_ustr(&p_style->para_style.h_numform_nu);

        if(P_USTR_NONE != ustr_format)
        {
            if(0 == strcmp("General", (const char *) ustr_format)) /* case normalised by now */
            {
                if(STYLE_HANDLE_NONE == *p_style_handle)
                {
                    *p_style_handle = style_handle_from_name(p_xls_load_info->p_docu, TEXT("General"));

                    if(0 != *p_style_handle)
                    {
                        style_selector_bit_clear(&p_style->selector, STYLE_SW_PS_NUMFORM_NU);
                        al_array_dispose(&p_style->para_style.h_numform_nu);
                    }
                }
            }
        }
    }
}

/* deal with different cell types */

_Check_return_
static STATUS
xls_cell_make_from_excel(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _InVal_     XLS_OPCODE opcode,
    _InVal_     U16 record_length)
{
    STATUS status = STATUS_OK;
    U8 data_type = XLS_DATA_SS;
    PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    XLS_ROW row;
    XLS_COL col;
    XF_INDEX xf_index;
    BYTE xf_data[20]; /* BIFF8 format */
    BOOL is_date_format = FALSE;
    STYLE style;
    STYLE_HANDLE style_handle = STYLE_HANDLE_NONE;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 64);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_formula, 256);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 32);
    quick_ublock_with_buffer_setup(quick_ublock_result);
    quick_ublock_with_buffer_setup(quick_ublock_formula);
    quick_ublock_with_buffer_setup(quick_ublock_format);

    zero_struct(xf_data);
    style_init(&style);

    UNREFERENCED_PARAMETER_InVal_(record_length); /* for now, check CONTINUE later */

    xls_read_cell_address_r2_c2(p_x, &row, &col);

    p_xls_load_info->current_slr.col = (COL) col;
    p_xls_load_info->current_slr.row = (ROW) row;

    p_xls_load_info->process_status.data.percent.current = xls_slr_in_docu_area_percent(&p_xls_load_info->current_slr, &p_xls_load_info->docu_area);
    process_status_reflect(&p_xls_load_info->process_status);

    switch(opcode)
    {
    case X_BLANK_B2:
        {
        xf_index = xls_obtain_xf_index_B2(p_xls_load_info, p_x + 4); /* cell attributes */

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, X_XF_B2);

        break;
        }

    case X_BLANK_B3_B8:
        {
        xf_index = xls_read_U16_LE(p_x + 4); /* XF index */

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);

        break;
        }

    case X_INTEGER_B2:
        {
        S32 s32 = (S32) xls_read_U16_LE(p_x + 7);
        F64 f64 = (F64) s32;

        xf_index = xls_obtain_xf_index_B2(p_xls_load_info, p_x + 4); /* cell attributes */

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, X_XF_B2);

        /* do we have FORMAT defined? */
        if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
        {
            FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

            status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
        }

        if(status_ok(status))
        {
            if(is_date_format && !xls_check_serial_range(f64, &quick_ublock_format))
                is_date_format = FALSE;

            if(is_date_format)
                status = xls_convert_serial_to_date(&quick_ublock_result, f64, p_xls_load_info->datemode_1904);
            else
                status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT(S32_FMT), s32);
        }

        break;
        }

    case X_FORMULA_B3:
    case X_FORMULA_B4:
        {
        if(biff_version >= 5)
        {
            /* What is going on here?! BIFF3/BIFF4 record numbers present in newer BIFF version file (seen in one of R-Comp test files) */
            assert0();
        }
        else if(biff_version >= 3)
        {
            U16 formula_len = (U16) xls_read_U16_LE(p_x + 16);
            PC_BYTE p_formula = p_x + 18;

            xf_index = xls_read_U16_LE(p_x + 4); /* XF index */

            xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, (X_FORMULA_B3 == opcode) ? X_XF_B3 : X_XF_B4);

            status = xls_decode_formula_result(p_xls_load_info, &quick_ublock_result, p_x + 6);

            if(status_ok(status))
                status = xls_decode_formula(p_xls_load_info, &quick_ublock_formula, p_formula, formula_len);

            if(status_fail(status) && (0 == quick_ublock_bytes(&quick_ublock_result)))
            {
                data_type = XLS_DATA_TEXT;
                status = quick_ublock_ustr_add(&quick_ublock_result,
                            (X_FORMULA_B3 == opcode)
                            ? USTR_TEXT("XLS FORMULA B3 FAIL")
                            : USTR_TEXT("XLS FORMULA B4 FAIL"));
            }

            if(status_ok(status))
            {
                /* do we have FORMAT defined? */
                if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
                {
                    FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

                    status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
                }
            }

            break;
        }
        else /* (biff_version == 2) - surely not ... */
        {
            /* What is going on here?! BIFF3/BIFF4 record numbers present in older BIFF version file */
            assert0();
        }
        }

        /*DROPTHRU*/

    case X_FORMULA_B2_B5_B8:
        {
        if(biff_version >= 5) /* SKS 04.11.98 */
        {
            BOOL shared_formula = (0 != (xls_read_U16_LE(p_x + 14) & 0x08));
            U16 formula_len = (U16) xls_read_U16_LE(p_x + 20);
            PC_BYTE p_formula = p_x + 22;

            xf_index = xls_read_U16_LE(p_x + 4); /* XF index */

            xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);

            status = xls_decode_formula_result(p_xls_load_info, &quick_ublock_result, p_x + 6);

            if(status_ok(status))
            {
                if(shared_formula && (formula_len == 5) && (*p_formula == tExp))
                    status = xls_get_shared_formula(p_xls_load_info, &quick_ublock_formula, p_formula, formula_len);
                else
                    status = xls_decode_formula(p_xls_load_info, &quick_ublock_formula, p_formula, formula_len);
            }

            if(status_ok(status))
            {
                /* do we have FORMAT defined? */
                if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
                {
                    FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

                    status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
                }
            }
        }
        else if(biff_version >= 3)
        {
            status = status_check();
            break;
        }
        else /* (biff_version == 2) */
        {
            U16 formula_len = (U16) p_x[16];
            PC_BYTE p_formula = p_x + 17;

            xf_index = xls_obtain_xf_index_B2(p_xls_load_info, p_x + 4); /* cell attributes */

            xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, X_XF_B2);

            status = xls_decode_formula_result(p_xls_load_info, &quick_ublock_result, p_x + 7);

            if(status_ok(status))
                status = xls_decode_formula(p_xls_load_info, &quick_ublock_formula, p_formula, formula_len);

            if(status_ok(status))
            {
                /* do we have FORMAT defined? */
                if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
                {
                    FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

                    status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
                }
            }
        }

        if(status_fail(status) && (0 == quick_ublock_bytes(&quick_ublock_result)))
        {
            data_type = XLS_DATA_TEXT;
            status = quick_ublock_ustr_add(&quick_ublock_result,
                        (X_FORMULA_B3 == opcode)
                        ? USTR_TEXT("XLS FORMULA B3' FAIL")
                        : (X_FORMULA_B4 == opcode)
                        ? USTR_TEXT("XLS FORMULA B4' FAIL")
                        : (2 == biff_version)
                        ? USTR_TEXT("XLS FORMULA B2 FAIL")
                        : USTR_TEXT("XLS FORMULA B5-B8 FAIL"));
        }

        break;
        }

    case X_NUMBER_B2:
        {
        F64 f64 = xls_read_F64(p_x + 7);

        xf_index = xls_obtain_xf_index_B2(p_xls_load_info, p_x + 4); /* cell attributes */

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, X_XF_B2);

        /* do we have FORMAT defined? */
        if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
        {
            FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

            status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
        }

        if(status_ok(status))
        {
            if(is_date_format && !xls_check_serial_range(f64, &quick_ublock_format))
                is_date_format = FALSE;

            if(is_date_format)
                status = xls_convert_serial_to_date(&quick_ublock_result, f64, p_xls_load_info->datemode_1904);
            else
                status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT("%." stringize(DBL_DECIMAL_DIG) "g"), f64);
        }

        break;
        }

    case X_NUMBER_B3_B8:
        {
        F64 f64 = xls_read_F64(p_x + 6);

        xf_index = xls_read_U16_LE(p_x + 4); /* XF index */

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);

        /* do we have FORMAT defined? */
        if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
        {
            FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

            status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
        }

        if(status_ok(status))
        {
            if(is_date_format && !xls_check_serial_range(f64, &quick_ublock_format))
                is_date_format = FALSE;

            if(is_date_format)
                status = xls_convert_serial_to_date(&quick_ublock_result, f64, p_xls_load_info->datemode_1904);
            else
                status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT("%." stringize(DBL_DECIMAL_DIG) "g"), f64);
        }

        break;
        }

    case X_RK_B3_B8:
        {
        union _n
        {
            F64 f64;
            struct _w
            {
#if RISCOS
                U32 hiword;
                U32 loword;
#else
                U32 loword;
                U32 hiword;
#endif
            } w;
        } n;

        PC_BYTE p_rk = p_x + 4;
        U32 rk = xls_read_U32_LE(p_rk + 2);

        xf_index = xls_read_U16_LE(p_rk);

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);

        if(rk & 0x02)
        {   /* (NB signed) integer in top 30 bits */
            S32 ival = rk;
            ival >>= 2;
            n.f64 = (F64) ival;
        }
        else
        {   /* top 30 bits of IEEE 754 FP value */
            n.w.loword = 0;
            n.w.hiword = rk & 0xFFFFFFFC;
        }

        if(rk & 0x01)
            n.f64 /= 100;

        /* do we have FORMAT defined? */
        if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
        {
            FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

            status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
        }

        if(status_ok(status))
        {
            if(is_date_format && !xls_check_serial_range(n.f64, &quick_ublock_format))
                is_date_format = FALSE;

            if(is_date_format)
                status = xls_convert_serial_to_date(&quick_ublock_result, n.f64, p_xls_load_info->datemode_1904);
            else
                status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT("%." stringize(DBL_DECIMAL_DIG) "g"), n.f64);
        }

        break;
        }

    case X_MULRK_B5_B8:
        {
        PC_BYTE p_rk = p_x + 4;
        U32 n_rk = (record_length - (2/*row*/ + 2/*scol*/ + 2/*ecol*/)) / 6;
        U32 rk_idx;

        for(rk_idx = 0; rk_idx < n_rk; rk_idx++, p_rk += 6, p_xls_load_info->current_slr.col += 1)
        {
            union _n
            {
                F64 f64;
                struct _w
                {
#if RISCOS
                    U32 hiword;
                    U32 loword;
#else
                    U32 loword;
                    U32 hiword;
#endif
                } w;
            } n;

            U32 rk = xls_read_U32_LE(p_rk + 2);

            style_handle = STYLE_HANDLE_NONE;
            style_init(&style);
            zero_struct(xf_data);

            xf_index = xls_read_U16_LE(p_rk);

            xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);

            if(rk & 0x02)
            {   /* (NB signed) integer in top 30 bits */
                S32 ival = rk;
                ival >>= 2;
                n.f64 = (F64) ival;
            }
            else
            {   /* top 30 bits of IEEE 754 FP value */
                n.w.loword = 0;
                n.w.hiword = rk & 0xFFFFFFFC;
            }

            if(rk & 0x01)
                n.f64 /= 100;

            /* do we have FORMAT defined? */
            if(0 != (xf_data[9] & XF_USED_ATTRIB_FORMAT))
            {
                FORMAT_INDEX format_index = xls_read_U16_LE(&xf_data[2]);

                status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);
            }

            if(status_ok(status))
            {
                if(is_date_format && !xls_check_serial_range(n.f64, &quick_ublock_format))
                    is_date_format = FALSE;

                if(is_date_format)
                    status = xls_convert_serial_to_date(&quick_ublock_result, n.f64, p_xls_load_info->datemode_1904);
                else
                    status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT("%." stringize(DBL_DECIMAL_DIG) "g"), n.f64);
            }

            if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_result)))
                status = quick_ublock_nullch_add(&quick_ublock_result);

            if(status_ok(status))
            {
                status = xls_style_from_xf_data(p_xls_load_info, &style, xf_data);

                /* Deal with General */
                if(status_ok(status))
                    xls_promote_general_from_style_to_style_handle(p_xls_load_info, &style, &style_handle);
            }

            if(status_ok(status))
                status = xls_cell_make(p_xls_load_info, data_type, &quick_ublock_result, &quick_ublock_formula, &style, style_handle);

            if(status_fail(status))
                style_dispose(&style); /* handles normally donated to style system */

            /* style data reset near loop start */

            quick_ublock_empty(&quick_ublock_result);
            quick_ublock_empty(&quick_ublock_format);
        }

        return(status)/*break*/;
        }

    case X_LABEL_B2:
    case X_LABEL_B3_B8:
        {
        U32 label_len;
        PC_BYTE p_label;

        if(X_LABEL_B2 == opcode)
        {
            label_len = p_x[7];
            p_label = p_x + 8;

            xf_index = xls_obtain_xf_index_B2(p_xls_load_info, p_x + 4); /* cell attributes */
        }
        else /* (X_LABEL_B3_B8 == opcode) */
        {
            label_len = xls_read_U16_LE(p_x + 6);
            p_label = p_x + 8;

            xf_index = xls_read_U16_LE(p_x + 4);
        }

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);

        data_type = XLS_DATA_TEXT;

        status = xls_quick_ublock_xls_string_add(&quick_ublock_result, p_label, label_len, p_xls_load_info->sbchar_codepage, TRUE); /* NB may be unaligned */

        break;
        }

    case X_BOOLERR_B2:
    case X_BOOLERR_B3_B8:
        {
        U8 type, value;
        
        if(X_BOOLERR_B2 == opcode)
        {
            type = p_x[7];
            value = p_x[8];
        }
        else /* (X_BOOLERR_B3_B8 == opcode) */
        {
            type = p_x[6];
            value = p_x[7];

            xf_index = xls_read_U16_LE(p_x + 4);

            xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);
        }

        if(0 == type) /* Boolean value */
        {
            status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT(S32_FMT), (S32) value);
        }
        else if(1 == type) /* Error value */
        {
            PC_USTR p_error;

            switch(value)
            {
            case 0:  p_error = USTR_TEXT("#NULL!"); break;
            case 7:  p_error = USTR_TEXT("#DIV/0!"); break;
            case 15: p_error = USTR_TEXT("#VALUE!"); break;
            case 23: p_error = USTR_TEXT("#REF!"); break;
            case 29: p_error = USTR_TEXT("#NAME!"); break;
            case 36: p_error = USTR_TEXT("#NUM!"); break;
            case 42: p_error = USTR_TEXT("#N/A!"); break;
            default: p_error = USTR_TEXT("#ERROR"); break;
            }

            status = quick_ublock_ustr_add(&quick_ublock_result, p_error);
        }

        break;
        }

    case X_LABELSST_B8: /* SKS 05.09.02 for BIFF8 */
        {
        const U32 sst_entry = xls_read_U32_LE(p_x + 6);
        P_ARRAY_HANDLE_USTR p_array_handle_ustr = NULL;

        xf_index = xls_read_U16_LE(p_x + 4);

        xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, biff_version_opcode_XF);

        data_type = XLS_DATA_TEXT;

        if(array_index_is_valid(&g_h_sst_index, sst_entry))
        {
            p_array_handle_ustr = array_ptr_no_checks(&g_h_sst_index, ARRAY_HANDLE_USTR, sst_entry);

            if(0 == array_elements(p_array_handle_ustr))
                p_array_handle_ustr = NULL; /* some strings may not have converted */
        }

        if(NULL != p_array_handle_ustr)
            status = quick_ublock_uchars_add(&quick_ublock_result, array_ustr(p_array_handle_ustr), array_elements32(p_array_handle_ustr)); /* may contain inlines */
        else
            status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT("LABELSST " U32_FMT " not found (" U32_FMT " entries loaded)"), sst_entry, array_elements32(&g_h_sst_index));

        break;
        }

    default: default_unhandled();
        break;
    }

    if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_result)))
        status = quick_ublock_nullch_add(&quick_ublock_result);

    if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_formula)))
        status = quick_ublock_nullch_add(&quick_ublock_formula);

    if(status_ok(status))
    {
        status = xls_style_from_xf_data(p_xls_load_info, &style, xf_data);

        /* Deal with General */
        if(status_ok(status))
            xls_promote_general_from_style_to_style_handle(p_xls_load_info, &style, &style_handle);
    }

    if(status_ok(status))
        status = xls_cell_make(p_xls_load_info, data_type, &quick_ublock_result, &quick_ublock_formula, &style, style_handle);

    if(status_fail(status))
        style_dispose(&style); /* handles normally donated to style system */

    quick_ublock_dispose(&quick_ublock_result);
    quick_ublock_dispose(&quick_ublock_formula);
    quick_ublock_dispose(&quick_ublock_format);

    return(status);
}

_Check_return_
static STATUS
xls_process_IXFE_B2(
    P_XLS_LOAD_INFO p_xls_load_info,
    U32 opcode_offset,
    /*XLS_OPCODE opcode,*/
    U16 record_length)
{
    PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    XF_INDEX real_xf_index = xls_read_U16_LE(p_x);

    p_xls_load_info->biff2_xf_index = real_xf_index;

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_process_worksheet_record(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     U32 opcode_offset,
    _InVal_     XLS_OPCODE opcode,
    _InVal_     U16 record_length)
{
    switch(opcode)
    {
    case X_BLANK_B2:
    case X_INTEGER_B2:
    case X_NUMBER_B2:
    case X_LABEL_B2:
    case X_BOOLERR_B2:
    case X_FORMULA_B2_B5_B8:
    case X_BLANK_B3_B8:
    case X_NUMBER_B3_B8:
    case X_LABEL_B3_B8:
    case X_BOOLERR_B3_B8:
    case X_FORMULA_B3:
    case X_RK_B3_B8:
    case X_FORMULA_B4:
    case X_MULRK_B5_B8:
    case X_LABELSST_B8:
        return(xls_cell_make_from_excel(p_xls_load_info, opcode_offset, opcode, record_length));

    case X_IXFE_B2:
        return(xls_process_IXFE_B2(p_xls_load_info, opcode_offset, /*opcode,*/ record_length));

    default:
        /* ignore the rest */
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
xls_process_file_data(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    STATUS status = STATUS_OK;

    if(status_ok(status))
        xls_read_CODEPAGE(p_xls_load_info);

    if(status_ok(status))
        xls_read_DATEMODE(p_xls_load_info);

    /* try both sorts of NAME record */

    status_assert(xls_names_make(p_xls_load_info, p_docu, X_DEFINEDNAME_B2_B5_B8));
    /* don't really care about this failing, it'll just stop some formulas compiling */

    status_assert(xls_names_make(p_xls_load_info, p_docu, X_DEFINEDNAME_B3_B4));
    /* don't really care about this failing, it'll just stop some formulas compiling */

    if(status_ok(status))
    {
        U32 opcode_offset = p_xls_load_info->worksheet_substream_offset;
        XLS_OPCODE opcode;
        U16 record_length;

        for(opcode_offset = xls_first_record(p_xls_load_info, opcode_offset, &opcode, &record_length);
            XLS_NO_RECORD_FOUND != opcode_offset;
            opcode_offset = xls_next_record (p_xls_load_info, opcode_offset, &opcode, &record_length))
        {
            status = xls_process_worksheet_record(p_xls_load_info, opcode_offset, opcode, record_length);

            status_assert(status); /* but keep going? */
        }
    }

    return(status);
}

_Check_return_
static S32
xls_get_standard_column_width(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info)
{
    S32 standard_column_width = 0;

    if(biff_version >= 4)
    {
        U16 record_length;
        U32 opcode_offset = xls_find_record_first(p_xls_load_info, p_xls_load_info->worksheet_substream_offset, X_STANDARDWIDTH_B4_B8, &record_length);
        PC_BYTE p_x;

        if(XLS_NO_RECORD_FOUND != opcode_offset)
        {
            p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

            standard_column_width = (S32) xls_read_U16_LE(p_x + 4);

            return(standard_column_width);
        }
    }

    {
    U16 record_length;
    U32 opcode_offset = xls_find_record_first(p_xls_load_info, p_xls_load_info->worksheet_substream_offset, X_DEFCOLWIDTH, &record_length);
    PC_BYTE p_x;

    if(XLS_NO_RECORD_FOUND != opcode_offset)
    {
        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        standard_column_width = (S32) xls_read_U16_LE(p_x);

        standard_column_width <<= 8;

        return(standard_column_width);
    }
    } /*block*/

    return(standard_column_width);
}

_Check_return_
static S32
xls_get_colinfo_column_width(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     XLS_COL col)
{
    U16 record_length;
    U32 opcode_offset;
    PC_BYTE p_x;
    S32 column_width = -1;

    /* Look for a COLINFO record that contains this column */
    opcode_offset = p_xls_load_info->worksheet_substream_offset;

    for(opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, X_COLINFO_B3_B8, &record_length);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next( p_xls_load_info, opcode_offset, X_COLINFO_B3_B8, &record_length))
    {
        XLS_COL col_first, col_last;

        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        col_first = (XLS_COL) xls_read_U16_LE(p_x);
        col_last  = (XLS_COL) xls_read_U16_LE(p_x + 2);

        if((col < col_first) || (col > col_last))
            continue;

        column_width = (S32) xls_read_U16_LE(p_x + 4);
        break;
    }

    return(column_width);
}

_Check_return_
static S32
xls_get_column_width(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     XLS_COL col)
{
    U16 record_length;
    U32 opcode_offset;
    PC_BYTE p_x;
    S32 column_width = -1;

    if(biff_version >= 8)
    {   /* Look for a COLINFO record that contains this column */
    }
    else if(biff_version >= 4)
    {   /* Look in the GCW record */
        const U32 byte_index = ((U32) col) >> 3;
        const U32 bit_mask = ((U32) 1) << (((U32) col) & 0x07); 

        opcode_offset = xls_find_record_first(p_xls_load_info, p_xls_load_info->worksheet_substream_offset, X_GCW_B4_B7, &record_length);

        if(XLS_NO_RECORD_FOUND == opcode_offset)
            return(column_width);

        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        assert(0x20 == xls_read_U16_LE(p_x));
        if(byte_index >= 0x20)
            return(column_width);

        if(0 != (bit_mask & p_x[2 + byte_index]))
        {   /* GCW bit set: apply standard width */
            return(column_width);
        }

        /* GCW bit clear: Look for a COLINFO record that contains this column */
    }
    else if(biff_version == 3)
    {   /* Look for a COLINFO record that contains this column */
    }
    else
    {
        return(column_width);
    }

    /* Look for a COLINFO record that contains this column */
    return(xls_get_colinfo_column_width(p_xls_load_info, col));
}

_Check_return_
static U16
xls_get_colinfo_XF_index(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InVal_     XLS_COL col)
{
    U16 record_length;
    U32 opcode_offset;
    PC_BYTE p_x;
    U16 xf_index = XF_INDEX_INVALID;

    /* Look for a COLINFO record that contains this column */
    opcode_offset = p_xls_load_info->worksheet_substream_offset;

    for(opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, X_COLINFO_B3_B8, &record_length);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next( p_xls_load_info, opcode_offset, X_COLINFO_B3_B8, &record_length))
    {
        XLS_COL col_first, col_last;

        p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        col_first = (XLS_COL) xls_read_U16_LE(p_x);
        col_last  = (XLS_COL) xls_read_U16_LE(p_x + 2);

        if((col < col_first) || (col > col_last))
            continue;

        xf_index = xls_read_U16_LE(p_x + 6);
        break;
    }

    return(xf_index);
}

_Check_return_
static U16
xls_get_default_row_height_twips(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    U16 default_row_height_twips = U16_MAX;
    const XLS_OPCODE defaultrowheight_opcode = (biff_version == 2) ? X_DEFAULTROWHEIGHT_B2 : X_DEFAULTROWHEIGHT_B3_B8;
    U16 record_length;
    U32 opcode_offset = p_xls_load_info->worksheet_substream_offset;

    /* find the default row height */
    opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, defaultrowheight_opcode, &record_length);

    if(XLS_NO_RECORD_FOUND != opcode_offset)
    {
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);

        if(biff_version == 2)
        {
            default_row_height_twips = xls_read_U16_LE(p_x);

            if(0 != (0x8000 & default_row_height_twips))
                default_row_height_twips = U16_MAX;
        }
        else
        {
            default_row_height_twips = xls_read_U16_LE(p_x + 2);
        }
    }

    return(default_row_height_twips);
}

static const struct xls_font_name_to_app_font_name_map
{
    PC_USTR ustr_xls_font_name;
    PCTSTR tstr_app_font_name;
}
xls_font_name_to_app_font_name_map[] =
{
    /* font name in file            neutral font name */
    { USTR_TEXT("Arial"),           TEXT("Helvetica") },    /* default */
    { USTR_TEXT("Courier New"),     TEXT("Courier") },
    { USTR_TEXT("Courier"),         TEXT("Courier") },      /* Excel files converted from Lotus */
    { USTR_TEXT("MS Sans Serif"),   TEXT("Helvetica") },
    { USTR_TEXT("Times New Roman"), TEXT("Times") }
};

_Check_return_
static STATUS
font_spec_name_from_xls_FONT_record_name(
    _InoutRef_  P_FONT_SPEC p_font_spec,
    _In_z_      PC_USTR ustr_xls_font_name)
{
    U32 i;

    for(i = 0; i < elemof32(xls_font_name_to_app_font_name_map); ++i)
    {
        if(ustr_compare_equals(ustr_xls_font_name, xls_font_name_to_app_font_name_map[i].ustr_xls_font_name))
        {
            return(font_spec_name_alloc(p_font_spec, xls_font_name_to_app_font_name_map[i].tstr_app_font_name));
        }
    }

    /* must return a default */
    return(font_spec_name_alloc(p_font_spec, xls_font_name_to_app_font_name_map[0].tstr_app_font_name));
}

_Check_return_
static STATUS
font_spec_from_xls_FONT_record(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _OutRef_    P_FONT_SPEC p_font_spec,
    _InVal_     U32 opcode_offset,
    _InVal_     U16 record_length)
{
    PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
    U16 height_twips = xls_read_U16_LE(p_x);
    const U16 option_flags = xls_read_U16_LE(p_x + 2);
    U32 font_len;
    PC_BYTE p_font;
    STATUS status = STATUS_OK;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
    quick_ublock_with_buffer_setup(quick_ublock);

    zero_struct_ptr_fn(p_font_spec);

    p_font_spec->size_x = 0;
    p_font_spec->size_y = height_twips;

    p_font_spec->bold      = (U8) (0 != (option_flags & 0x01));
    p_font_spec->italic    = (U8) (0 != (option_flags & 0x02));
    p_font_spec->underline = (U8) (0 != (option_flags & 0x04));

    if(biff_version >= 5)
    {
        const U16 weight = xls_read_U16_LE(p_x + 6);
        const U8 underline = p_x[10];
        p_font_spec->bold      = (U8) (weight >= 600);
        p_font_spec->underline = (U8) (0 != underline);
    }

    if(biff_version >= 5)
    {
        font_len = p_x[14];
        p_font = p_x + 15;
    }
    else if(biff_version >= 3)
    {
        font_len = p_x[6];
        p_font = p_x + 7;
    }
    else /* (biff_version < 3) */
    {
        font_len = p_x[4];
        p_font = p_x + 5;
    }

    status = xls_quick_ublock_xls_string_add(&quick_ublock, p_font, font_len, p_xls_load_info->sbchar_codepage, FALSE); /* for USTR_IS_SBSTR this may substitute a replacement character for any non-Latin-1 */

    if(status_ok(status))
        status = quick_ublock_nullch_add(&quick_ublock);

    if(status_ok(status))
        status = font_spec_name_from_xls_FONT_record_name(p_font_spec, quick_ublock_ustr(&quick_ublock));

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

_Check_return_
static STATUS
style_from_xls_FONT_record(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_STYLE p_style,
    _InVal_     U32 opcode_offset,
    _InVal_     U16 record_length)
{
    STATUS status = STATUS_OK;

    status_return(font_spec_from_xls_FONT_record(p_xls_load_info, &p_style->font_spec, opcode_offset, record_length));

    /* avoid setting bit if not needed */
    if(0 != p_style->font_spec.size_x)
        style_bit_set(p_style, STYLE_SW_FS_SIZE_X);

    style_bit_set(p_style, STYLE_SW_FS_SIZE_Y);

    if(0 != p_style->font_spec.h_app_name_tstr)
        style_bit_set(p_style, STYLE_SW_FS_NAME);

    style_bit_set(p_style, STYLE_SW_FS_BOLD);
    style_bit_set(p_style, STYLE_SW_FS_ITALIC);
    style_bit_set(p_style, STYLE_SW_FS_UNDERLINE);

    if(biff_version >= 3)
    {
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
        const U16 colour_index = xls_read_U16_LE(p_x + 4);

        if(rgb_from_colour_index(&p_style->font_spec.colour, colour_index))
        {
            style_bit_set(p_style, STYLE_SW_FS_COLOUR);

            status = STATUS_DONE;
        }
    }

    return(status);
}

_Check_return_
static STATUS
xls_style_from_xf_data(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_STYLE p_style,
    _In_reads_c_(20) P_BYTE p_xf_data /* BIFF8 format */)
{
    STATUS status = STATUS_OK;

    if(0 != (p_xf_data[9]  & XF_USED_ATTRIB_BACKGROUND))
    {
        const BYTE pattern_colour            = (BYTE) ((readval_U16_LE(&p_xf_data[18])      ) & 0x007F); /* [6..0] */
     /* const BYTE pattern_background_colour = (BYTE) ((readval_U16_LE(&p_xf_data[18]) >>  7) & 0x007F); */ /* [13..7] */

        if(rgb_from_colour_index(&p_style->para_style.rgb_back, pattern_colour))
            style_bit_set(p_style, STYLE_SW_PS_RGB_BACK);
    }

    if(0 != (p_xf_data[9]  & XF_USED_ATTRIB_ALIGNMENT))
    {
        /* XF_HOR_ALIGN:  0 General 1 Left 2 Centred 3 Right 4 Filled 5 Justified (BIFF4-BIFF8) 6 Centred across selection (BIFF4-BIFF8) 7 Distributed (BIFF8, available in Excel 10.0 (Excel XP) and later only) */
        /* XF_VERT_ALIGN: 0 Top 1 Centred 2 Bottom 3 Justified (BIFF5-BIFF8) 4 Distributed (BIFF8, available in Excel 10.0 (Excel XP) and later only) */
        const BYTE horizontal_alignment = (p_xf_data[6]     ) & 0x07;
        const BYTE vertical_alignment   = (p_xf_data[6] >> 4) & 0x07;

        switch(horizontal_alignment)
        {
        default: default_unhandled();
        case 0: /* General */
        case 4: /* Filled */
        case 6: /* Centred across selection */
        case 7: /* Distributed */
            break;

        case 1: /* Left */
            style_bit_set(p_style, STYLE_SW_PS_JUSTIFY);
            p_style->para_style.justify = SF_JUSTIFY_LEFT;
            break;

        case 2: /* Centred */
            style_bit_set(p_style, STYLE_SW_PS_JUSTIFY);
            p_style->para_style.justify = SF_JUSTIFY_CENTRE;
            break;

        case 3: /* Right */
            style_bit_set(p_style, STYLE_SW_PS_JUSTIFY);
            p_style->para_style.justify = SF_JUSTIFY_RIGHT;
            break;

        case 5: /* Justified */
            style_bit_set(p_style, STYLE_SW_PS_JUSTIFY);
            p_style->para_style.justify = SF_JUSTIFY_BOTH;
            break;
        }

        switch(vertical_alignment)
        {
        default: default_unhandled();
        case 3: /* Justified */
        case 4: /* Distributed */
            break;

        case 0: /* Top */
            style_bit_set(p_style, STYLE_SW_PS_JUSTIFY_V);
            p_style->para_style.justify_v = SF_JUSTIFY_V_TOP;
            break;

        case 1: /* Centred */
            style_bit_set(p_style, STYLE_SW_PS_JUSTIFY_V);
            p_style->para_style.justify_v = SF_JUSTIFY_V_CENTRE;
            break;

        case 2: /* Bottom */
            style_bit_set(p_style, STYLE_SW_PS_JUSTIFY_V);
            p_style->para_style.justify_v = SF_JUSTIFY_V_BOTTOM;
            break;
        }
    }

    if(0 != (p_xf_data[9] & XF_USED_ATTRIB_FONT))
    {
        const FONT_INDEX font_index = xls_read_U16_LE(p_xf_data + 0);
        const XLS_OPCODE font_opcode = ((biff_version == 3) || (biff_version == 4)) ? X_FONT_B3_B4 : X_FONT_B2_B5_B8;
        U16 record_length;
        U32 opcode_offset = xls_find_record_FONT_INDEX(p_xls_load_info, &record_length, font_index, font_opcode);

        if(XLS_NO_RECORD_FOUND != opcode_offset)
        {
            STYLE style;

            style_init(&style);

            status = style_from_xls_FONT_record(p_xls_load_info, &style, opcode_offset, record_length);

            if(status_ok(status))
            {
                style_copy(p_style, &style, &style.selector);
            }

            style_dispose(&style);
        }
    }

    /* do we have FORMAT defined? */
    if(0 != (p_xf_data[9] & XF_USED_ATTRIB_FORMAT))
    {
        FORMAT_INDEX format_index = xls_read_U16_LE(&p_xf_data[2]);
        BOOL is_date_format;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 32);
        quick_ublock_with_buffer_setup(quick_ublock_format);

        status = xls_check_format(p_xls_load_info, format_index, &quick_ublock_format, &is_date_format);

        if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_format)))
        {
            PC_USTR ustr_format = quick_ublock_ustr(&quick_ublock_format);

            if(CH_NULL != PtrGetByte(ustr_format))
            {
                ARRAY_HANDLE_USTR array_handle_ustr = 0;

                if(0 == C_stricmp("General", (const char *) ustr_format)) /* SKS 20aug2012 - have now seen uppercase GENERAL in a FORMAT record */
                    ustr_format = USTR_TEXT("General"); /* ensure case normalised */

                if(status_ok(status = al_ustr_set(&array_handle_ustr, ustr_format)))
                {
                    if(is_date_format)
                    {
                        assert(0 == style_selector_bit_test(&p_style->selector, STYLE_SW_PS_NUMFORM_DT));
                        style_bit_set(p_style, STYLE_SW_PS_NUMFORM_DT);
                        p_style->para_style.h_numform_dt = array_handle_ustr;
                    }
                    else
                    {
                        assert(0 == style_selector_bit_test(&p_style->selector, STYLE_SW_PS_NUMFORM_NU));
                        style_bit_set(p_style, STYLE_SW_PS_NUMFORM_NU);
                        p_style->para_style.h_numform_nu = array_handle_ustr;
                    }
                }
            }
        }

        quick_ublock_dispose(&quick_ublock_format);
    }

    return(status);
}

_Check_return_
static STATUS
xls_slurp_from_XF_INDEX_as_style(
    _InRef_     P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_STYLE p_style,
    _InVal_     XF_INDEX xf_index,
    _InVal_     XLS_OPCODE xf_opcode)
{
    BYTE xf_data[20]; /* BIFF8 format */

    xls_slurp_xf_data_from_XF_INDEX(p_xls_load_info, xf_data, xf_index, xf_opcode);

    return(xls_style_from_xf_data(p_xls_load_info, p_style, xf_data));
}

#define XLS_COL_DEFAULT_MARGIN_LEFT  ((PIXIT) (FP_PIXITS_PER_MM *  0.8))
#define XLS_COL_DEFAULT_MARGIN_RIGHT ((PIXIT) (FP_PIXITS_PER_MM *  0.8))

#define XLS_DEFAULT_PARA_START       ((PIXIT) (FP_PIXITS_PER_MM *  0.4))
#define XLS_DEFAULT_PARA_END         ((PIXIT) (FP_PIXITS_PER_MM *  0.4))

_Check_return_
static STATUS
xls_apply_structure_style_for_column(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InVal_     XLS_COL loading_col,
    _InRef_     P_STYLE p_style)
{
    DOCU_AREA docu_area;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, p_style);
    style_docu_area_add_parm.add_without_subsume = TRUE;

    docu_area = *p_docu_area;

    docu_area_set_cols(&docu_area, (COL) loading_col + p_xls_load_info->offset_slr.col, 1);

    return(xls_style_docu_area_add(p_xls_load_info, &docu_area, &style_docu_area_add_parm));
}

_Check_return_
static STATUS
xls_apply_structure_style_for_row(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InVal_     XLS_ROW loading_row,
    _InRef_     P_STYLE p_style)
{
    DOCU_AREA docu_area;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, p_style);

    docu_area = *p_docu_area;

    docu_area_set_rows(&docu_area, (ROW) loading_row + p_xls_load_info->offset_slr.row, 1);

    if(!p_xls_load_info->p_msg_insert_foreign->insert || p_xls_load_info->loi_as_table)
    {   /* apply this style to the whole row when loading, or when inserting as table */
        docu_area_set_whole_row(&docu_area);
    }

    return(xls_style_docu_area_add(p_xls_load_info, &docu_area, &style_docu_area_add_parm));
}

_Check_return_
static STATUS
xls_apply_structure_style_for_rows(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InRef_     P_STYLE p_style)
{
    DOCU_AREA docu_area;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, p_style);

    docu_area = *p_docu_area;

    if(!p_xls_load_info->p_msg_insert_foreign->insert || p_xls_load_info->loi_as_table)
    {   /* apply this style to whole rows when loading, or when inserting as table */
        docu_area_set_whole_row(&docu_area);
    }

    return(xls_style_docu_area_add(p_xls_load_info, &docu_area, &style_docu_area_add_parm));
}

_Check_return_
static PIXIT
xls_get_pixit_width_of_zero_in_style(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_STYLE p_style)
{
    PIXIT pixit_width_of_zero = 0;

    if(0 != p_style->font_spec.h_app_name_tstr)
    {
        STATUS status;

        if(status_ok(status = fonty_handle_from_font_spec(&p_style->font_spec, FALSE /*draft_mode*/)))
        {
            const PC_UCHARS uchars = USTR_TEXT("0");
            const U32 uchars_n = 1;
            const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;
            FONTY_CHUNK fonty_chunk;

            fonty_chunk_info_read_uchars(p_docu, &fonty_chunk, fonty_handle, uchars, uchars_n, 0 /* trail_spaces */);

            pixit_width_of_zero = fonty_chunk.width;
        }

        status_assert(status);
        reportf(TEXT("width of zero=") PIXIT_TFMT TEXT(" in %s ") S32_TFMT, pixit_width_of_zero, array_tstr(&p_style->font_spec.h_app_name_tstr), p_style->font_spec.size_y);

        pixit_width_of_zero = (PIXITS_PER_INCH / 96) * muldiv64_round_floor(pixit_width_of_zero, 96, PIXITS_PER_INCH); /* round formatting value to 96 dpi pixels */
        reportf(TEXT("width of zero=") PIXIT_TFMT TEXT(" in %s ") S32_TFMT, pixit_width_of_zero, array_tstr(&p_style->font_spec.h_app_name_tstr), p_style->font_spec.size_y);
    }

    return(pixit_width_of_zero);
}

_Check_return_
static PIXIT
xls_get_pixit_with_of_zero_in_default_font(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    /* get width of character '0' in the default font */
    PIXIT pixit_width_of_zero = PIXITS_PER_INCH / 10;
    const XLS_OPCODE font_opcode = ((biff_version == 3) || (biff_version == 4)) ? X_FONT_B3_B4 : X_FONT_B2_B5_B8;
    U16 record_length;
    U32 opcode_offset = xls_find_record_FONT_INDEX(p_xls_load_info, &record_length, (FONT_INDEX) 0, font_opcode);

    if(XLS_NO_RECORD_FOUND != opcode_offset)
    {
        STATUS status;
        STYLE default_style;

        style_init(&default_style);

        if(status_ok(status = style_from_xls_FONT_record(p_xls_load_info, &default_style, opcode_offset, record_length)))
        {
            pixit_width_of_zero = xls_get_pixit_width_of_zero_in_style(p_xls_load_info->p_docu, &default_style);
        }

        status_assert(status);

        style_dispose(&default_style);
    }

    return(pixit_width_of_zero);
}

/* set the default cell style (XF_INDEX 15) over the interior */

_Check_return_
static STATUS
xls_apply_structure_style_default_cell(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    STYLE_HANDLE style_handle = STYLE_HANDLE_NONE; /* named style (General) covering the docu_area */
    STYLE style;

    style_init(&style);

    /* get the default cell style (XF_INDEX 15) */
    status_return(xls_slurp_from_XF_INDEX_as_style(p_xls_load_info, &style, (XF_INDEX) 15, biff_version_opcode_XF));

    /* Deal with General */
    xls_promote_general_from_style_to_style_handle(p_xls_load_info, &style, &style_handle);

    if(style_selector_any(&style.selector))
    {
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
        status_return(xls_style_docu_area_add(p_xls_load_info, p_docu_area, &style_docu_area_add_parm));
    }

    /* NB this order allows user to set other bits in the named style (General) to override underlying effects more easily */
    if(STYLE_HANDLE_NONE != style_handle)
    {
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle);
        status_return(xls_style_docu_area_add(p_xls_load_info, p_docu_area, &style_docu_area_add_parm));
    }

    style_dispose(&style);

    return(STATUS_OK);
}

/* column width styles span up/down into table margins */

_Check_return_
static STATUS
xls_apply_structure_style_column_widths(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    const S32 standard_column_width = xls_get_standard_column_width(p_xls_load_info);
    const PIXIT pixit_width_of_zero = xls_get_pixit_with_of_zero_in_default_font(p_xls_load_info);
    XLS_COL loading_col;
    STYLE style;

    style_init(&style);

    style.col_style.width = (pixit_width_of_zero * standard_column_width) >> 8;
    style_bit_set(&style, STYLE_SW_CS_WIDTH);
    reportf(TEXT("area column width=") PIXIT_TFMT, style.col_style.width);

    {
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
    status_return(xls_style_docu_area_add(p_xls_load_info, p_docu_area, &style_docu_area_add_parm));
    } /*block*/

    for(loading_col = p_xls_load_info->s_col; loading_col < p_xls_load_info->e_col; ++loading_col)
    {
        S32 column_width = xls_get_column_width(p_xls_load_info, loading_col);

        if(column_width >= 0)
            style.col_style.width = (pixit_width_of_zero * column_width) >> 8;
        else
            style.col_style.width = (pixit_width_of_zero * standard_column_width) >> 8;

        reportf(TEXT("loading_col=") COL_TFMT TEXT(" width=") PIXIT_TFMT, (COL) loading_col, style.col_style.width);

        status_return(xls_apply_structure_style_for_column(p_xls_load_info, p_docu_area, loading_col, &style));
    }

    return(STATUS_OK);
}

/* other column styles are set over the interior and do not span up/down into table margins */

_Check_return_
static STATUS
xls_apply_structure_style_column_others_for_column(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _InVal_     XLS_COL loading_col)
{
    const U16 xf_index = xls_get_colinfo_XF_index(p_xls_load_info, loading_col);

    if(XF_INDEX_INVALID != xf_index)
    {
        STYLE style;

        style_init(&style);

        status_return(xls_slurp_from_XF_INDEX_as_style(p_xls_load_info, &style, xf_index, biff_version_opcode_XF));

        { /* Deal with General (by ignoring it here - has likely been brought up from base style) */
        STYLE_HANDLE style_handle = STYLE_HANDLE_NONE;
        xls_promote_general_from_style_to_style_handle(p_xls_load_info, &style, &style_handle);
        } /*block*/

        if(style_selector_any(&style.selector))
            status_return(xls_apply_structure_style_for_column(p_xls_load_info, p_docu_area, loading_col, &style));

        style_dispose(&style);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_apply_structure_style_column_others(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    XLS_COL loading_col;

    for(loading_col = p_xls_load_info->s_col; loading_col < p_xls_load_info->e_col; ++loading_col)
    {
        status_return(xls_apply_structure_style_column_others_for_column(p_xls_load_info, p_docu_area, loading_col));
    }

    return(STATUS_OK);
}

/* set the default row height over the inserted interior area */

_Check_return_
static STATUS
xls_apply_structure_style_default_row_height(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area,
    _OutRef_    P_U16 p_default_row_height_twips)
{
    *p_default_row_height_twips = xls_get_default_row_height_twips(p_xls_load_info);

    if(U16_MAX != *p_default_row_height_twips)
    {
        STYLE style;

        style_init(&style);

        style.row_style.height = (PIXIT) *p_default_row_height_twips;
        style_bit_set(&style, STYLE_SW_RS_HEIGHT);

        style.row_style.height_fixed = 1;
        style_bit_set(&style, STYLE_SW_RS_HEIGHT_FIXED);

        status_return(xls_apply_structure_style_for_rows(p_xls_load_info, p_docu_area, &style));
    }

    { /* need to tweak the paragraph style for Excel import into normal templates as it usually has much smaller row heights and internal cell margins */
    STYLE style;

    style_init(&style);

    style.para_style.margin_left  = 32; /* 1.6 points */
    style.para_style.margin_right = 32;
    style.para_style.margin_para  = 0;
    style.para_style.para_start   = 16; /* 0.8 points */ /* (2 * PIXITS_PER_RISCOS) */
    style.para_style.para_end     = 16;
    style_bit_set(&style, STYLE_SW_PS_MARGIN_LEFT);
    style_bit_set(&style, STYLE_SW_PS_MARGIN_RIGHT);
    style_bit_set(&style, STYLE_SW_PS_MARGIN_PARA);
    style_bit_set(&style, STYLE_SW_PS_PARA_START);
    style_bit_set(&style, STYLE_SW_PS_PARA_END);

    {
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
    status_return(xls_style_docu_area_add(p_xls_load_info, p_docu_area, &style_docu_area_add_parm));
    } /*block*/
    } /*block*/

    if(!p_xls_load_info->p_msg_insert_foreign->insert)
    {   /* and maybe tweak the grid size downwards too */
        const P_DOCU p_docu = p_xls_load_info->p_docu;
        p_docu->page_def.grid_size = MIN(p_docu->page_def.grid_size, 16); /* 0.8 points */ /* (2 * PIXITS_PER_RISCOS) */
        page_def_validate(&p_docu->page_def);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_apply_structure_style_row_heights(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    U16 default_row_height_twips;
    STYLE style;
    const XLS_OPCODE row_opcode = (biff_version == 2) ? X_ROW_B2 : X_ROW_B3_B8;
    U16 record_length;
    U32 opcode_offset = p_xls_load_info->worksheet_substream_offset;

    status_return(xls_apply_structure_style_default_row_height(p_xls_load_info, p_docu_area, &default_row_height_twips));

    style_init(&style);

    style_bit_set(&style, STYLE_SW_RS_HEIGHT);

    style.row_style.height_fixed = 1;
    style_bit_set(&style, STYLE_SW_RS_HEIGHT_FIXED);

    for(opcode_offset = xls_find_record_first(p_xls_load_info, opcode_offset, row_opcode, &record_length);
        XLS_NO_RECORD_FOUND != opcode_offset;
        opcode_offset = xls_find_record_next( p_xls_load_info, opcode_offset, row_opcode, &record_length))
    {
        PC_BYTE p_x = p_xls_record(p_xls_load_info, opcode_offset, record_length);
        const XLS_ROW loading_row = (XLS_ROW) xls_read_U16_LE(p_x);
        const U16 row_height_twips = xls_read_U16_LE(p_x + 6);

        if(0 != (0x8000 & row_height_twips))
            continue;

        if(row_height_twips == default_row_height_twips)
            continue;

        /* set only explicit row heights which differ from the default */
        style.row_style.height = (PIXIT) row_height_twips;

        reportf(TEXT("loading_row=") ROW_TFMT TEXT(" height=") PIXIT_TFMT, (ROW) loading_row, style.row_style.height);

        status_return(xls_apply_structure_style_for_row(p_xls_load_info, p_docu_area, loading_row, &style));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
xls_apply_structure_style(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    DOCU_AREA docu_area_table = p_xls_load_info->docu_area_table;
    DOCU_AREA docu_area_interior = p_xls_load_info->docu_area_interior;

    if(p_xls_load_info->loi_as_table)
        status_return(foreign_load_file_apply_style_table(p_xls_load_info->p_docu, &p_xls_load_info->docu_area_table, p_xls_load_info->lhs_extra_cols, p_xls_load_info->top_extra_rows, p_xls_load_info->bot_extra_rows));

    if(p_xls_load_info->loi_as_table)
    {
        /* use these docu_area */
    }
    else
    {
        if(p_xls_load_info->p_msg_insert_foreign->insert)
        {
            /* when inserting, apply structure styles to just the given docu_area */
        }
        else
        {   /* when loading, apply structure styles to the whole document where appropriate */
            docu_area_set_whole_col(&docu_area_interior);
            docu_area_set_whole_row(&docu_area_interior);

            docu_area_set_whole_col(&docu_area_table);
            docu_area_set_whole_row(&docu_area_table);
        }
    }

    /* set the default cell style (XF_INDEX 15) over the interior area */
    status_return(xls_apply_structure_style_default_cell(p_xls_load_info, &docu_area_interior));

    /* column width styles span up/down into table margins */
    status_return(xls_apply_structure_style_column_widths(p_xls_load_info, &docu_area_table));

    /* other column styles are set over the interior area and do not span up/down into table margins */
    status_return(xls_apply_structure_style_column_others(p_xls_load_info, &docu_area_interior));

    /* set only explicit row heights over the interior area */
    status_return(xls_apply_structure_style_row_heights(p_xls_load_info, &docu_area_interior));

    return(STATUS_OK);
}

/******************************************************************************
*
* main routine to load an Excel file
*
******************************************************************************/

static void
xls_static_data_dispose(void)
{
    if(g_data_allocated)
    {
        g_data_allocated = FALSE;
        al_array_dispose(&g_h_data);
    }
    else
    {
        g_h_data = 0;
    }

    g_file_end_offset = 0;

    xls_sst_index_dispose();
}

_Check_return_
static STATUS
xls_workbook_once(
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info)
{
    STATUS status;

    /* only decrypt once per file not once every worksheet ... */
    status = xls_decrypt_file(p_xls_load_info);

    /* just dump once per file not once every worksheet ... */
    if(status_ok(status))
        xls_dump_records(p_xls_load_info);

    /* process any workbook globals */

    /* retain SST between worksheets */
    if(status_ok(status) && (biff_version >= 8))
        status_assert(xls_read_SST(p_xls_load_info));

    return(status);
}

_Check_return_
static STATUS
xls_insert_foreign(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_XLS_LOAD_INFO p_xls_load_info,
    _InoutRef_  P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    STATUS status = STATUS_OK;
    P_POSITION p_position = &p_msg_insert_foreign->position;
    SLR interior_size_slr = SLR_INIT;

    p_xls_load_info->p_docu = p_docu;
    p_xls_load_info->p_msg_insert_foreign = p_msg_insert_foreign;

    p_xls_load_info->sbchar_codepage = SBCHAR_CODEPAGE_WINDOWS_1252; /* until proven otherwise */

    docu_area_init(&p_xls_load_info->docu_area_interior);
    docu_area_init(&p_xls_load_info->docu_area_table);

    p_xls_load_info->process_status.flags.foreground = TRUE;
    p_xls_load_info->process_status.reason.type = UI_TEXT_TYPE_RESID;
    p_xls_load_info->process_status.reason.text.resource_id = MSG_STATUS_CONVERTING; /* that's what we're about to do */
    process_status_begin(p_docu, &p_xls_load_info->process_status, PROCESS_STATUS_PERCENT);

    if(0 == p_msg_insert_foreign->retry_with_this_arg)
    {
        g_h_data = 0;

        if(0 != p_msg_insert_foreign->array_handle)
        {
            if(status_done(status = try_grokking_potential_cf_array_for_xls(&p_msg_insert_foreign->array_handle, &g_h_data)))
            {
                reportf(TEXT("*** Excel file loaded from handle ") U32_TFMT TEXT(" (is a compound document) ***"), g_h_data);
                g_data_allocated = TRUE;
                g_file_end_offset = (U32) status;
            }
            else
            {
                g_h_data = p_msg_insert_foreign->array_handle;
                reportf(TEXT("*** Excel file loaded from handle ") U32_TFMT TEXT(" (is not a compound document) ***"), g_h_data);
                g_file_end_offset = array_elements(&g_h_data);
            }
        }
        else
        {
            PCTSTR p_pathname = p_msg_insert_foreign->filename;

            if(status_done(status = try_grokking_potential_cf_file_for_xls(p_pathname, &g_h_data)))
            {
                reportf(TEXT("*** Excel file %s is a compound document file ***"), p_pathname);
                g_data_allocated = TRUE;
                g_file_end_offset = (U32) status;
            }
            else if(STATUS_OK == status)
            {
                reportf(TEXT("*** Excel file %s is not a compound document file ***"), p_pathname);

                if(status_ok(status = file_memory_load(p_docu, &g_h_data, p_pathname, &p_xls_load_info->process_status, 16)))
                {
                    g_data_allocated = TRUE;
                    g_file_end_offset = array_elements(&g_h_data);
                }
            }
        }
    }
    else
    {
        trace_1(TRACE__XLS_LOADB, TEXT("*** Excel file load continuing from handle ") U32_TFMT TEXT(" ***"), g_h_data);
    }

    /* If it's a Letter-derived document, insert (or load) as table */
    if(p_docu->flags.base_single_col)
        p_xls_load_info->loi_as_table = TRUE;

    if(status_ok(status))
    {
        p_xls_load_info->p_file_start = array_basec(&g_h_data, BYTE);
        p_xls_load_info->file_end_offset = g_file_end_offset;

        trace_3(TRACE__XLS_LOADB, TEXT("*** Excel file loaded @ ") PTR_XTFMT TEXT(", end offset=") U32_XTFMT TEXT(", retry_arg=") U32_XTFMT,
                p_xls_load_info->p_file_start, p_xls_load_info->file_end_offset, p_msg_insert_foreign->retry_with_this_arg);

        if(status_ok(status = xls_read_first_BOF(p_xls_load_info)))
        {
            if(0 == p_msg_insert_foreign->retry_with_this_arg)
            {
                status = xls_workbook_once(p_xls_load_info);
            }
            else
            {
                assert(p_xls_load_info->process_multiple_worksheets);
            }
        }
    }

    if(status_ok(status) && p_xls_load_info->process_multiple_worksheets)
    {
        if(biff_version >= 5)
            status = xls_find_BOUNDSHEET(p_xls_load_info, &p_msg_insert_foreign->retry_with_this_arg);

        if(biff_version == 4)
            status = xls_find_SHEETHDR(p_xls_load_info, &p_msg_insert_foreign->retry_with_this_arg);
    }

    if(status_ok(status))
        status = xls_read_DIMENSIONS(p_xls_load_info, p_xls_load_info->worksheet_substream_offset, &p_xls_load_info->s_col, &p_xls_load_info->e_col, &p_xls_load_info->s_row, &p_xls_load_info->e_row);

    if(status_ok(status))
    {
        /* Best to be paranoid - first worksheet used to miss empty test (and could still do for old files) */
        if(p_xls_load_info->s_col == p_xls_load_info->e_col) p_xls_load_info->e_col++;
        if(p_xls_load_info->s_row == p_xls_load_info->e_row) p_xls_load_info->e_row++;

#if 0
        interior_size_slr.col = p_xls_load_info->e_col - p_xls_load_info->s_col;
        interior_size_slr.row = p_xls_load_info->e_row - p_xls_load_info->s_row;
#else
        interior_size_slr.col = p_xls_load_info->e_col; /* don't adjust start here - keep blank leading cols/rows from sheet */
        interior_size_slr.row = p_xls_load_info->e_row;
#endif

        docu_area_init(&p_xls_load_info->docu_area);
        p_xls_load_info->docu_area.br.slr = interior_size_slr;

        if(p_xls_load_info->loi_as_table)
        {
            /* we require an extra LHS column for positioning if inserting as a table at the first column */
            if(0 == p_position->slr.col)
                p_xls_load_info->lhs_extra_cols = 1;

            p_xls_load_info->top_extra_rows = 1;
            p_xls_load_info->bot_extra_rows = 1;

            /* docu_area needs to be this much bigger */
            p_xls_load_info->docu_area.br.slr.col += p_xls_load_info->lhs_extra_cols;
            p_xls_load_info->docu_area.br.slr.row += p_xls_load_info->top_extra_rows + p_xls_load_info->bot_extra_rows;
        }

        /* split the document at insertion point */
        status = cells_docu_area_insert(p_docu, p_position /*updated*/, &p_xls_load_info->docu_area, p_position);
    }

    if(status_ok(status))
    {
        SLR slr = p_position->slr; /* was updated by cells_docu_area_insert() */

        docu_area_set_cols(&p_xls_load_info->docu_area_table, slr.col, interior_size_slr.col);
        docu_area_set_rows(&p_xls_load_info->docu_area_table, slr.row, interior_size_slr.row);

        if(p_xls_load_info->loi_as_table)
        {   /* table docu_area needs to be this much bigger */
            p_xls_load_info->docu_area_table.br.slr.col += p_xls_load_info->lhs_extra_cols;
            p_xls_load_info->docu_area_table.br.slr.row += p_xls_load_info->top_extra_rows + p_xls_load_info->bot_extra_rows;
        }

        p_xls_load_info->docu_area_interior = p_xls_load_info->docu_area_table;

        if(p_xls_load_info->loi_as_table)
        {   /* interior docu_area is derived from table docu_area, without the margins */
            p_xls_load_info->docu_area_interior.tl.slr.col += p_xls_load_info->lhs_extra_cols;
            p_xls_load_info->docu_area_interior.tl.slr.row += p_xls_load_info->top_extra_rows;
            p_xls_load_info->docu_area_interior.br.slr.row -= p_xls_load_info->bot_extra_rows;
        }

        /* now offset slr to interior tl */
        slr.col += p_xls_load_info->lhs_extra_cols;
        slr.row += p_xls_load_info->top_extra_rows;

        /* remember the address of the tl cell in the Fireworkz document of the Excel worksheet area being inserted */
        p_xls_load_info->offset_slr = slr;

#if 0
        /* offset by the tl of the area we are inserting from the Excel worksheet */
        /* NB the offset may be negative */
        /* consider: copy area from middle of Excel worksheet, paste at tl of Fireworkz document */
        p_xls_load_info->offset_slr.col -= p_xls_load_info->s_col;
        p_xls_load_info->offset_slr.row -= p_xls_load_info->s_row;
#else
        /* didn't trim off leading cols/rows so no further offset required */
#endif

        {
        const COL docu_cols = n_cols_logical(p_docu);
        const ROW docu_rows = n_rows(p_docu);
        status = format_col_row_extents_set(p_docu,
                                            MAX(docu_cols, slr.col + interior_size_slr.col),
                                            MAX(docu_rows, slr.row + interior_size_slr.row + p_xls_load_info->bot_extra_rows));
        } /*block*/
    }

    if(status_ok(status))
        xls_read_PALETTE(p_xls_load_info);

    if(status_ok(status))
        status = xls_apply_structure_style(p_xls_load_info);

    /* parse ROW blocks to find rightmost actual cell */

    if(status_ok(status))
        status = xls_process_file_data(p_docu, p_xls_load_info);

    process_status_end(&p_xls_load_info->process_status);

    return(status);
}

T5_MSG_PROTO(extern, xls_msg_insert_foreign, P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    STATUS status;
    XLS_LOAD_INFO xls_load_info;
    zero_struct_fn(xls_load_info);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 == p_msg_insert_foreign->retry_with_this_arg)
    {   /* not a continuation of previous workbook */
        xls_static_data_dispose();
    }

    if(NULL == (arg_stack = al_ptr_calloc_elem(P_USTR, MAXSTACK, &status)))
        return(status);

    status = xls_insert_foreign(p_docu, &xls_load_info, p_msg_insert_foreign);

    al_ptr_dispose((P_P_ANY) &arg_stack);

    if(status_fail(status))
        p_msg_insert_foreign->retry_with_this_arg = 0;

    if(0 == p_msg_insert_foreign->retry_with_this_arg) /* dispose of loaded data, SST strings and index when no longer needed */
    {
        xls_static_data_dispose();
    }
    else
    {   /* next time round, loaded worksheet must be inserted at A1 as inserting at any other position is crazy */
        position_init(&p_msg_insert_foreign->position);
    }

    return(status);
}

/* end of fl_xls_loadb.c */
