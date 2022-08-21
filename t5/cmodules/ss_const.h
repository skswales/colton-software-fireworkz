/* ss_const.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Data definitions */

/* MRJC April 1992 */

#ifndef __ss_const_h
#define __ss_const_h

#define LEAP_YEAR_ACTUAL(year) ( \
    (((year    ) % 4  ) ? 0 : 1) && \
    (((year    ) % 100) ? 1 : (((year    ) % 400) ? 0 : 1)) )

/* maximum size of symbol */
#define EV_INTNAMLEN        25
#define BUF_EV_INTNAMLEN    (EV_INTNAMLEN + 1)
#define EV_LONGNAMLEN       (1/*[*/ + MAX_PATHSTRING/*rooted-doc*/ + 1/*]*/ + EV_MAX_RANGE_LEN + 1/*CH_NULL*/) /* was 200 */
#define BUF_EV_LONGNAMLEN   (EV_LONGNAMLEN + 1)

/* maximum size of a string value  -
*  this is also the maximum string length
*  of a calculated result of whatever type
*/
#define EV_MAX_STRING_LEN   200
#define BUF_EV_MAX_STRING_LEN (EV_MAX_STRING_LEN + 1)

/* maximum size of expressions in input and output forms */
#define EV_MAX_IN_LEN       1000        /* (textual form) */
#define BUF_EV_MAX_IN_LEN   (EV_MAX_IN_LEN + 1)
#define EV_MAX_OUT_LEN      1000        /* (compiled RPN form) */
#define BUF_EV_MAX_OUT_LEN  (EV_MAX_OUT_LEN + 1)

#if RISCOS || WINDOWS

#if defined(DOCNO_BITS)
#define EV_DOCNO PACKED_DOCNO
#define EV_DOCNO_BITS       DOCNO_BITS
#else
typedef U8 EV_DOCNO;
#define EV_DOCNO_BITS       8
#endif

typedef S32 EV_COL; /* really restricted to S16 for packing in EV_SLR but ARM Norcroft makes poor code for signed 16-bit accesses */
#define EV_COL_BITS         16
#define EV_COL_SBF          SBF

typedef S32 EV_ROW;
#define EV_ROW_BITS         32

#define EV_MAX_COL          ((EV_COL) 1 << (EV_COL_BITS - 1 - 1))   /* extra -1 for sign bit */
#define EV_MAX_ROW          ((EV_ROW) 1 << (EV_ROW_BITS - 1 - 1))

#define EV_MAX_ARRAY_ELES   0x08000000  /* maximum number of array elements */

#define EV_MAX_SLR_LEN      (1/*%*/ + 1/*$*/ + 4/*cols*/ + 1/*$*/ + 10/*rows*/)
#define BUF_EV_MAX_SLR_LEN  (EV_MAX_SLR_LEN + 1)

#define EV_MAX_RANGE_LEN    (EV_MAX_SLR_LEN + 1/*:*/ + EV_MAX_SLR_LEN)
#define BUF_EV_MAX_RANGE_LEN (EV_MAX_RANGE_LEN + 1)

/*#define EV_IDNO_U32*/ /* Largest RPN token size ***only for debugging*** */
#if defined(EV_IDNO_U32)
typedef U32 EV_IDNO; typedef EV_IDNO * P_EV_IDNO;
#define EV_IDNO_U16 /* Larger RPN token size */
#else
#define EV_IDNO_U16 /* Larger RPN token size to fit all new functions */
#if defined(EV_IDNO_U16)
typedef U16 EV_IDNO; typedef EV_IDNO * P_EV_IDNO;
#else
typedef U8 EV_IDNO; typedef EV_IDNO * P_EV_IDNO;
#endif
#endif /* EV_IDNO */

#else
#error Needs sizeof things defining
#endif

/*
evaluator data types
*/

/*
slr type for evaluator
*/

typedef struct EV_SLR
{
    UBF docno      : EV_DOCNO_BITS;
    UBF abs_col    : 1;
    UBF abs_row    : 1;
    UBF bad_ref    : 1;
    UBF ext_ref    : 1;
    UBF dbase_rel  : 1;
    UBF no_dep     : 1;
    UBF circ       : 1;
    UBF spare      : 1;
    EV_COL_SBF col : EV_COL_BITS;   /* try at top of word for ARM ASR */
    EV_ROW row;
}
EV_SLR, * P_EV_SLR; typedef const EV_SLR * PC_EV_SLR;

#define EV_SLR_INIT { 0 /*...*/ } /* this aggregate initialiser gives poor code on ARM Norcroft */

#define ev_slr_init(p_ev_slr) \
    zero_struct_ptr(p_ev_slr)

#define ev_slr_docno(p_ev_slr) \
    UBF_UNPACK(EV_DOCNO, (p_ev_slr)->docno)

#define EV_DOCNO_PACK(ev_docno) \
    UBF_PACK(ev_docno)

#define ev_slr_col(p_ev_slr) \
    SBF_UNPACK(EV_COL, (p_ev_slr)->col)

#define EV_COL_PACK(ev_col) \
    SBF_PACK(ev_col)

#define ev_slr_row(p_ev_slr) ( \
    (p_ev_slr)->row )

#if RISCOS && defined(__CC_NORCROFT)
#define ev_slr_docno_set(p_ev_slr, ev_docno) \
    PtrPutByteOff(p_ev_slr, 0 /*yuk*/, (BYTE) (ev_docno))
#else
#define ev_slr_docno_set(p_ev_slr, ev_docno) \
    (p_ev_slr)->docno = EV_DOCNO_PACK(ev_docno)
#endif

#define ev_slr_row_set(p_ev_slr, ev_row) \
    (p_ev_slr)->row = (ev_row)

#define ev_slr_flags_init(p_ev_slr) \
    PtrPutByteOff(p_ev_slr, 1 /*yuk*/, 0)

#if RISCOS && defined(__CC_NORCROFT)
#define ev_slr_col_set(p_ev_slr, ev_col) \
    writeval_S16(PtrAddBytes(P_BYTE, p_ev_slr, 2)/*&->col*/, (ev_col))
/* NB callers will need typepack.h */
#else
#define ev_slr_col_set(p_ev_slr, ev_col) \
    (p_ev_slr)->col = EV_COL_PACK(ev_col)
#endif

/*
range type
*/

typedef struct EV_RANGE
{
    EV_SLR s;
    EV_SLR e;
}
EV_RANGE, * P_EV_RANGE; typedef const EV_RANGE * PC_EV_RANGE;

#define ev_range_init(p_ev_range) \
    zero_struct_ptr(p_ev_range)

/*
date/time type
*/

typedef S32 EV_DATE_DATE; typedef EV_DATE_DATE * P_EV_DATE_DATE; typedef const EV_DATE_DATE * PC_EV_DATE_DATE;
#define EV_DATE_NULL S32_MIN

typedef S32 EV_DATE_TIME; typedef EV_DATE_TIME * P_EV_DATE_TIME; typedef const EV_DATE_TIME * PC_EV_DATE_TIME;
#define EV_TIME_NULL S32_MIN

typedef struct EV_DATE
{
    EV_DATE_DATE date;
    EV_DATE_TIME time;
}
EV_DATE, * P_EV_DATE; typedef const EV_DATE * PC_EV_DATE;

static inline void
ev_date_init(
    _OutRef_    P_EV_DATE p_ev_date)
{
    p_ev_date->date = EV_DATE_NULL;
    p_ev_date->time = EV_TIME_NULL;
}

/*
evaluator error type
*/

typedef struct EV_ERROR
{
    STATUS      status; /* negative */
    EV_ROW      row;
    UBF         docno : 8;
    UBF         type  : 2;
    UBF         spare : 8-2;
    EV_COL_SBF  col   : EV_COL_BITS;
}
EV_ERROR, * P_EV_ERROR;

#define ERROR_NORMAL 0
#define ERROR_CUSTOM 1
#define ERROR_PROPAGATED 2

/*
array structure
*/

typedef struct EV_ARRAY
{
    S32 x_size;
    S32 y_size;
    ARRAY_HANDLE elements;
}
EV_ARRAY;

/*
string data (not CH_NULL-terminated)
*/

typedef struct EV_STRING
{
    U32 size;
    P_UCHARS uchars;
}
EV_STRING, * P_EV_STRING;

typedef struct EV_STRINGC
{
    U32 size;
    PC_UCHARS uchars;
}
EV_STRINGC, * P_EV_STRINGC; typedef const EV_STRINGC * PC_EV_STRINGC;

/*
database binary resource
*/

typedef struct DB_BLOB
{
    U32 size;
    P_BYTE data;
}
DB_BLOB, * P_DB_BLOB;

/*
data identifier numbers - externally visible numbers only (need to pack into small bitfield)
*/

enum DATA_ID_NUMBERS
{
    RPN_DAT_REAL        = 0,
    RPN_DAT_BOOL8       = 1,
    RPN_DAT_WORD8       = 2,
    RPN_DAT_WORD16      = 3,
    RPN_DAT_WORD32      = 4,
    RPN_DAT_STRING      = 5,
    RPN_DAT_ARRAY       = 6,
    RPN_DAT_DATE        = 7,
    RPN_DAT_BLANK       = 8,
    RPN_DAT_ERROR       = 9,
    RPN_DAT_NEXT_NUMBER = 10        /* (RPN_NUMBERS follow here) */
};

/*
* evaluator constant type (external mixed data)
* see the dependent EV_DATA type below
*/

typedef S16 RPN_OFFSET;

typedef S32 EV_HANDLE; typedef EV_HANDLE * P_EV_HANDLE;

typedef union EV_CONSTANT
{
    F64             fp;             /* floating point constant */
    BOOL            boolean;        /* Boolean value */
    S32             integer;        /* integer constant */
    EV_STRING       string_wr;      /* string constant (writeable) */
    EV_STRINGC      string;         /* string constant (const) */
    EV_ARRAY        ev_array;       /* array */
    EV_DATE         ev_date;        /* date */
    EV_ERROR        ev_error;       /* error */
}
EV_CONSTANT, * P_EV_CONSTANT;

/*
evaluator mixed data type
*/

typedef union EV_DATA_ARG
{
    F64             fp;             /* floating point */
    BOOL            boolean;        /* Boolean value */
    S32             integer;        /* integer */
    EV_STRING       string_wr;      /* string (writeable) */
    EV_STRINGC      string;         /* string (const) */
    EV_ARRAY        ev_array;       /* array */
    EV_DATE         ev_date;        /* date */
    EV_ERROR        ev_error;       /* error */

    EV_CONSTANT     ev_constant;    /* all the above, as copied by ev_cell_constant_from_data() */

#if CHECKING
    U32             debug_words[4];
    U8              debug_bytes[16];
#endif

    EV_SLR          slr;
    EV_RANGE        range;
    EV_HANDLE       h_name;

    RPN_OFFSET      cond_pos;

    DB_BLOB         db_blob;        /* for database binary resource */
}
EV_DATA_ARG;

typedef struct EV_DATA
{
    EV_IDNO did_num;                /* has small enum DATA_ID_NUMBERS subset of EV_IDNO, but also some larger ones like RPN_DAT_SLR */
    U8 local_data;                  /* if set, resources are owned by this structure */
#if defined(EV_IDNO_U32)
    U8 _spare[4-1];
#else
    U8 _spare[4-1-sizeof(EV_IDNO)];
#endif

    EV_DATA_ARG arg;
}
EV_DATA, * P_EV_DATA, ** P_P_EV_DATA; typedef const EV_DATA * PC_EV_DATA;

typedef struct SS_DECOMPILER_OPTIONS
{
    UBF lf : 1;
    UBF cr : 1;
    UBF initial_formula_equals : 1;
    UBF range_colon_separator : 1;
    UBF upper_case_function : 1;
    UBF upper_case_slr : 1;
    UBF zero_args_function_parentheses : 1;
    UBF _spare8 : 1;
}
SS_DECOMPILER_OPTIONS, * P_SS_DECOMPILER_OPTIONS; typedef const SS_DECOMPILER_OPTIONS * PC_SS_DECOMPILER_OPTIONS;

typedef struct SS_RECOG_CONTEXT
{
    U8 ui_flag; /* 0 -> canonical file representations only, 1 -> extra parsing for UI */
    U8 alternate_function_flag; /* 1 -> bother trying alternate function name encode/decode */
    U8 function_arg_sep;
    U8 array_col_sep;
    U8 array_row_sep;
    U8 thousands_char;
    U8 decimal_point_char;
    U8 list_sep_char;
    U8 date_sep_char;
    U8 alternate_date_sep_char;
    U8 time_sep_char;
    U8 _spare4;
    U8 _spare5;
    U8 _spare6;
    U8 _spare7;
    U8 _spare8;
}
SS_RECOG_CONTEXT, * P_SS_RECOG_CONTEXT;

#if RISCOS
typedef struct RISCOS_TIME_ORDINALS
{
    S32 centiseconds;
    S32 seconds;
    S32 minutes;
    S32 hours;
    S32 day;
    S32 month;
    S32 year;
    S32 day_of_week;
    S32 day_of_year;
}
RISCOS_TIME_ORDINALS;
#endif

#define RESOURCE_NUM_EVAL 1

/*
error definition
*/

#define EVAL_ERR_BASE   (STATUS_ERR_INCREMENT * OBJECT_ID_SS)

#define EVAL_ERR(n)     (EVAL_ERR_BASE - (n))

#define EVAL_ERR_IRR                 EVAL_ERR(0)
#define EVAL_ERR_ERROR_RQ            EVAL_ERR(1)
#define EVAL_ERR_BAD_DATE            EVAL_ERR(2)
#define EVAL_ERR_LOOKUP              EVAL_ERR(3)
#define EVAL_ERR_NEG_ROOT            EVAL_ERR(4)
#define EVAL_ERR_PROPAGATED          EVAL_ERR(5)
#define EVAL_ERR_DIVIDEBY0           EVAL_ERR(6)
#define EVAL_ERR_BAD_INDEX           EVAL_ERR(7)
#define EVAL_ERR_FPERROR             EVAL_ERR(8)
#define EVAL_ERR_CIRC                EVAL_ERR(9)
#define EVAL_ERR_CANTEXTREF          EVAL_ERR(10)
#define EVAL_ERR_ARRAYEXPECTED       EVAL_ERR(11)
#define EVAL_ERR_BADRPN              EVAL_ERR(12)
#define EVAL_ERR_EXPTOOLONG          EVAL_ERR(13)
#define EVAL_ERR_CALC_FAILURE        EVAL_ERR(14)
#define EVAL_ERR_BADEXPR             EVAL_ERR(15)
#define EVAL_ERR_UNEXNUMBER          EVAL_ERR(16)
#define EVAL_ERR_UNEXSTRING          EVAL_ERR(17)
#define EVAL_ERR_UNEXDATE            EVAL_ERR(18)
#define EVAL_ERR_NAMEUNDEF           EVAL_ERR(19)
#define EVAL_ERR_UNEXRANGE           EVAL_ERR(20)
#define EVAL_ERR_INTERNAL            EVAL_ERR(21)
#define EVAL_ERR_CUSTOMUNDEF         EVAL_ERR(22)
#define EVAL_ERR_NESTEDARRAY         EVAL_ERR(23)
#define EVAL_ERR_UNEXARRAY           EVAL_ERR(24)
#define EVAL_ERR_LOCALUNDEF          EVAL_ERR(25)
#define EVAL_ERR_NORETURN            EVAL_ERR(26)
#define EVAL_ERR_ODF_NA              EVAL_ERR(27)
#define EVAL_ERR_NOTIMPLEMENTED      EVAL_ERR(28)
#define EVAL_ERR_BADSLR              EVAL_ERR(29)
#define EVAL_ERR_NOTIME              EVAL_ERR(30)
#define EVAL_ERR_NODATE              EVAL_ERR(31)
#define EVAL_ERR_BADIDENT            EVAL_ERR(32)
#define EVAL_ERR_BADCOMPLEX          EVAL_ERR(33)
#define EVAL_ERR_OUTOFRANGE          EVAL_ERR(34)
#define EVAL_ERR_SUBSCRIPT           EVAL_ERR(35)
#define EVAL_ERR_BADCONTROL          EVAL_ERR(36)
#define EVAL_ERR_BADGOTO             EVAL_ERR(37)
#define EVAL_ERR_BADLOOPNEST         EVAL_ERR(38)
#define EVAL_ERR_ARGRANGE            EVAL_ERR(39)
#define EVAL_ERR_BADTIME             EVAL_ERR(40)
#define EVAL_ERR_BADIFNEST           EVAL_ERR(41)
#define EVAL_ERR_MISMATCHED_MATRICES EVAL_ERR(42)
#define EVAL_ERR_MATRIX_NOT_NUMERIC  EVAL_ERR(43)
#define EVAL_ERR_MATRIX_WRONG_SIZE   EVAL_ERR(44)
#define EVAL_ERR_MIXED_SIGNS         EVAL_ERR(45)
#define EVAL_ERR_ACCURACY_LOST       EVAL_ERR(46)
#define EVAL_ERR_UNEXFORMULA         EVAL_ERR(47)
#define EVAL_ERR_FUNARGS             EVAL_ERR(48)
#define EVAL_ERR_CBRACKETS           EVAL_ERR(49)
#define EVAL_ERR_DBASENEST           EVAL_ERR(50)
#define EVAL_ERR_ARGTYPE             EVAL_ERR(51)
#define EVAL_ERR_MACROEXISTS         EVAL_ERR(52)
#define EVAL_ERR_ARGCUSTTYPE         EVAL_ERR(53)
#define EVAL_ERR_ARRAYEXPAND         EVAL_ERR(54)
#define EVAL_ERR_CARRAY              EVAL_ERR(55)
#define EVAL_ERR_BEYONDEXTENTS       EVAL_ERR(56)
#define EVAL_ERR_BAD_LOG             EVAL_ERR(57)
#define EVAL_ERR_MATRIX_NOT_SQUARE   EVAL_ERR(58)
#define EVAL_ERR_MATRIX_SINGULAR     EVAL_ERR(59)
#define EVAL_ERR_NO_VALID_DATA       EVAL_ERR(60)
#define EVAL_ERR_DATABASE            EVAL_ERR(61)
#define EVAL_ERR_ODF_VALUE           EVAL_ERR(62)
#define EVAL_ERR_ODF_NUM             EVAL_ERR(63)

#define EVAL_ERR_END                 EVAL_ERR(64)

#define EVAL_ERR_ODF_DIV0            EVAL_ERR_DIVIDEBY0

#define ev_slr_equal(pc_ev_slr1, pc_ev_slr2) ( \
    (pc_ev_slr1)->docno == (pc_ev_slr2)->docno && \
    (pc_ev_slr1)->col == (pc_ev_slr2)->col && \
    (pc_ev_slr1)->row == (pc_ev_slr2)->row )

#define ev_slr_compare(pc_ev_slr1, pc_ev_slr2) ( \
    (pc_ev_slr1)->docno < (pc_ev_slr2)->docno \
    ? -1 \
    : (pc_ev_slr1)->docno > (pc_ev_slr2)->docno \
    ? 1 \
    : (pc_ev_slr1)->row < (pc_ev_slr2)->row \
    ? -1 \
    : (pc_ev_slr1)->row > (pc_ev_slr2)->row \
    ? 1 \
    : (pc_ev_slr1)->col < (pc_ev_slr2)->col \
    ? -1 \
    : (pc_ev_slr1)->col > (pc_ev_slr2)->col \
    ? 1 \
    : 0 )

_Check_return_
extern F64
real_floor( /* as OpenDocument / Microsoft Excel INT() */
    _InVal_     F64 f64_in);

_Check_return_
extern F64
real_trunc( /* as Colton Software Fireworkz & PipeDream / Lotus 1-2-3 INT() */
    _InVal_     F64 f64_in);

_Check_return_
extern STATUS /* DONE iff converted in range */
real_to_integer_force(
    _InoutRef_  P_EV_DATA p_ev_data);

/*ncr*/
extern BOOL
real_to_integer_try(
    _InoutRef_  P_EV_DATA p_ev_data);

_Check_return_
extern STATUS
ss_array_dup(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in);

_Check_return_
_Ret_notnull_
extern P_EV_DATA
ss_array_element_index_wr(
    _InRef_     P_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

_Check_return_
_Ret_valid_
extern PC_EV_DATA
ss_array_element_index_borrow(
    _InRef_     PC_EV_DATA p_ev_data_in,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

_Check_return_
extern STATUS
ss_array_element_make(
    _InoutRef_  P_EV_DATA p_ev_data,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

extern void
ss_array_element_read(
    _OutRef_    P_EV_DATA p_ev_data,
    _InRef_     PC_EV_DATA p_ev_data_src,
    _InVal_     S32 ix,
    _InVal_     S32 iy);

_Check_return_
extern STATUS
ss_array_make(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 x_size,
    _InVal_     S32 y_size);

_Check_return_
extern S32
ss_data_compare(
    _InRef_     PC_EV_DATA p_ev_data1,
    _InRef_     PC_EV_DATA p_ev_data2,
    _InVal_     BOOL blanks_equal_zero,
    _InVal_     BOOL allow_wild_match);

#define ss_data_copy(pd_o, pd_i) ( \
    (*pd_o) = (*pd_i), \
    (pd_o)->local_data = 0 )

extern void
ss_data_free_resources(
    _InoutRef_  P_EV_DATA p_ev_data);

_Check_return_
extern STATUS
ss_data_resource_copy(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_in);

_Check_return_
extern STATUS
ss_decode_constant(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_EV_DATA p_ev_data);

_Check_return_
extern STATUS
ss_recog_constant(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str);

extern void
ss_recog_context_pull(
    P_SS_RECOG_CONTEXT p_ss_recog_context /* const save area */);

extern void
ss_recog_context_push(
    P_SS_RECOG_CONTEXT p_ss_recog_context /* save area, filled*/);

_Check_return_ _Success_(return > 0)
extern STATUS
ss_recog_boolean(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str_in);

_Check_return_ _Success_(return > 0)
extern STATUS
ss_recog_number(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str_in);

_Check_return_ _Success_(return > 0)
extern STATUS
ss_recog_string(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str);

_Check_return_
extern BOOL
ss_string_is_blank(
    _InRef_     PC_EV_DATA p_ev_data);

_Check_return_
extern STATUS
ss_string_dup(
    _OutRef_    P_EV_DATA p_ev_data_out,
    _InRef_     PC_EV_DATA p_ev_data_src);

_Check_return_
extern STATUS
ss_string_make_uchars(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n);

_Check_return_
extern STATUS
ss_string_make_ustr(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR ustr);

_Check_return_
extern U32
ss_string_skip_leading_whitespace(
    _InRef_     PC_EV_DATA p_ev_data);

_Check_return_
extern U32
ss_string_skip_leading_whitespace_uchars(
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InRef_     U32 uchars_n);

/*
two_nums_type_match result values
*/

#define TWO_INTS  0
#define TWO_REALS 1
#define TWO_MIXED 2

/*ncr*/
extern S32
two_nums_type_match(
    _InoutRef_  P_EV_DATA p_ev_data1,
    _InoutRef_  P_EV_DATA p_ev_data2,
    _InVal_     BOOL size_worry);

_Check_return_
extern F64
ui_strtod(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_PC_USTR p_ustr);

_Check_return_
extern S32
ui_strtol(
    _In_z_      PC_USTR ustr,
    _Out_opt_   P_P_USTR p_ustr,
    _In_        int radix);

extern const EV_DATA
ev_data_real_zero;

extern SS_DECOMPILER_OPTIONS
g_ss_decompiler_options;

extern SS_RECOG_CONTEXT
g_ss_recog_context;

extern SS_RECOG_CONTEXT
g_ss_recog_context_alt;

/* inline functions may call our exported functions */

_Check_return_
static inline EV_IDNO
ev_integer_size(
    _InVal_     S32 integer)
{
    if( (integer <=   (S32) S8_MAX) &&
        (integer >=   (S32) S8_MIN) )
        return(RPN_DAT_WORD8);

    /* S16_MAX + 1 for ARM immediate constant */
    if( (integer <   ((S32) S16_MAX + 1)) &&
        (integer > - ((S32) S16_MAX + 1)) /* NB NOT S16_MIN */ )
        return(RPN_DAT_WORD16);

    return(RPN_DAT_WORD32);
}

_Check_return_
static inline BOOL
ev_data_is_blank(
    _InRef_     PC_EV_DATA p_ev_data)
{
    return(RPN_DAT_BLANK == p_ev_data->did_num);
}

_Check_return_
static inline BOOL
ev_data_is_error(
    _InRef_     PC_EV_DATA p_ev_data)
{
    return(RPN_DAT_ERROR == p_ev_data->did_num);
}

static inline void
ev_data_set_blank(
    _OutRef_    P_EV_DATA p_ev_data)
{
    CODE_ANALYSIS_ONLY(zero_struct_ptr(p_ev_data)); /* keep dataflower happy */
    p_ev_data->did_num = RPN_DAT_BLANK;
}

static inline void
ev_data_set_boolean(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     BOOL boolean)
{
    p_ev_data->did_num = RPN_DAT_BOOL8;
    p_ev_data->arg.boolean = boolean;
}

/*ncr*/
extern STATUS
ev_data_set_error(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     STATUS error);

static inline void
ev_data_set_integer(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 s32)
{
    p_ev_data->arg.integer = s32;
    p_ev_data->did_num = ev_integer_size(s32);
}

static inline void
ev_data_set_WORD32( /* set the widest integer type */
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     S32 s32)
{
    p_ev_data->did_num = RPN_DAT_WORD32;
    p_ev_data->arg.integer = s32;
}

static inline void
ev_data_set_real(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 f64)
{
    p_ev_data->did_num = RPN_DAT_REAL;
    p_ev_data->arg.fp = f64;
}

static inline void
ev_data_set_real_ti(
    _OutRef_    P_EV_DATA p_ev_data,
    _InVal_     F64 f64)
{
    p_ev_data->did_num = RPN_DAT_REAL;
    p_ev_data->arg.fp = f64;

    consume_bool(real_to_integer_try(p_ev_data));
}

/*
ss_date.c
*/

extern const S32
ev_days_in_month[];

extern const S32
ev_days_in_month_leap[];

/* conversion to / from dateval */

_Check_return_
extern S32
ss_dateval_to_serial_number(
    _InRef_     PC_EV_DATE_DATE p_ev_date_date);

_Check_return_ _Success_(return >= 0)
extern STATUS
ss_serial_number_to_dateval(
    _OutRef_    P_EV_DATE_DATE p_ev_date_date,
    _InRef_     F64 serial_number);

_Check_return_
extern STATUS
ss_dateval_to_ymd(
    _InRef_     PC_EV_DATE_DATE p_ev_date_date,
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day);

/*ncr*/
extern S32
ss_ymd_to_dateval(
    _OutRef_    P_EV_DATE_DATE p_ev_date_date,
    _In_        S32 year,
    _In_        S32 month,
    _In_        S32 day);

/* conversion to / from timeval */

_Check_return_
extern F64
ss_timeval_to_serial_fraction(
    _InRef_     PC_EV_DATE_TIME p_ev_date_time);

extern void
ss_serial_fraction_to_timeval(
    _OutRef_    P_EV_DATE_TIME p_ev_date_time,
    _InVal_     F64 serial_fraction);

_Check_return_
extern STATUS
ss_timeval_to_hms(
    _InRef_     PC_EV_DATE_TIME p_ev_date_time,
    _OutRef_    P_S32 hours,
    _OutRef_    P_S32 minutes,
    _OutRef_    P_S32 seconds);

/*ncr*/
extern S32
ss_hms_to_timeval(
    _OutRef_    P_EV_DATE_TIME p_ev_date_time,
    _InVal_     S32 hours,
    _InVal_     S32 minutes,
    _InVal_     S32 seconds);

/* date processing */

extern void
ss_date_normalise(
    _InoutRef_  P_EV_DATE p_ev_date);

_Check_return_
extern F64
ss_date_to_serial_number(
    _InRef_     PC_EV_DATE p_ev_date);

_Check_return_
extern STATUS
ss_serial_number_to_date(
    _OutRef_    P_EV_DATE p_ev_date,
    _InVal_     F64 serial_number);

extern void
ss_local_time_as_ymd_hms(
    _OutRef_    P_S32 p_year,
    _OutRef_    P_S32 p_month,
    _OutRef_    P_S32 p_day,
    _OutRef_    P_S32 p_hours,
    _OutRef_    P_S32 p_minutes,
    _OutRef_    P_S32 p_seconds);

extern void
ss_local_time_as_ev_date(
    _OutRef_    P_EV_DATE p_ev_date);

_Check_return_
extern S32
sliding_window_year(
    _InVal_     S32 year);

_Check_return_ _Success_(return >= 0)
extern STATUS
ss_recog_date_time(
    _OutRef_    P_EV_DATA p_ev_data,
    _In_z_      PC_USTR in_str);

_Check_return_
extern STATUS
ss_date_decode(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_EV_DATE p_ev_date);

#endif /* __ss_const_h */

/* end of ss_const.h */
