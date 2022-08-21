/* ob_ss.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spreadsheet object module for Fireworkz */

/* MRJC April 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "ob_ss/ss_linest.h"

#include "ob_toolb/xp_toolb.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#include "cmodules/mrofmun.h"

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_SS)
extern PC_U8 rb_ss_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_SS &rb_ss_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_SS LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_SS DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_SS LOAD_RESOURCES

/*
internal functions
*/

OBJECT_PROTO(extern, object_ss);

_Check_return_
static STATUS
ss_function_add_argument(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_        STATUS message,
    _InVal_     U32 argument_index,
    _InVal_     BOOL in_line_wrap);

_Check_return_
static STATUS
ss_function_get_help(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     STATUS message);

_Check_return_
static STATUS
ss_function_paste_to_editing_line(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     S32 category,
    _InVal_     S32 item_number);

_Check_return_
static STATUS
ss_object_from_text(
    P_P_DOCU p_p_docu,
    _InRef_     PC_SLR p_slr,
    _In_opt_z_  PC_USTR ustr_result,
    _In_opt_z_  PC_USTR ustr_formula,
    _InRef_opt_ PC_SLR p_slr_offset,
    _InRef_opt_ PC_REGION p_region_saved,
    P_STATUS p_compile_error,
    P_S32 p_pos,
    _In_opt_z_  PCTSTR tstr_autoformat_style,
    _InVal_     BOOL try_autoformat,
    _InVal_     BOOL please_uref_overwrite,
    _InVal_     BOOL force_recalc,
    _InVal_     BOOL clip_data_from_cut_operation);

/* -------------------------------------------------------------------------
 * Define the constructs table.
 * ------------------------------------------------------------------------- */

static const ARG_TYPE
ss_args_s32[] =
{
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
ss_args_bool[] =
{
    ARG_TYPE_BOOL | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
ss_args_ustr[] =
{
    ARG_TYPE_USTR,
    ARG_TYPE_NONE
};

static const ARG_TYPE
ss_args_name[] =
{
    ARG_TYPE_USTR,
    ARG_TYPE_USTR,
    ARG_TYPE_USTR,
    ARG_TYPE_NONE
};

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "ChoicesSheetCalcAuto",   ss_args_bool,               T5_CMD_CHOICES_SS_CALC_AUTO },
    { "ChoicesSheetCalcBG",     ss_args_bool,               T5_CMD_CHOICES_SS_CALC_BACKGROUND },
    { "ChoicesSheetCalcOnLoad", ss_args_bool,               T5_CMD_CHOICES_SS_CALC_ON_LOAD },
    { "ChoicesSheetCalcAdditionalRounding", ss_args_bool,   T5_CMD_CHOICES_SS_CALC_ADDITIONAL_ROUNDING },
    { "ChoicesSheetEditInCell", ss_args_bool,               T5_CMD_CHOICES_SS_EDIT_IN_CELL },
    { "ChoicesSheetAltFormulaStyle", ss_args_bool,          T5_CMD_CHOICES_SS_ALTERNATE_FORMULA_STYLE },
    { "ChoicesChartUpdateAuto", ss_args_bool,               T5_CMD_CHOICES_CHART_UPDATE_AUTO },

    { "ICR",                    NULL,                       (T5_MESSAGE) IL_RETURN },

                                                                                                    /*   fi                                     reject if file insertion */
                                                                                                    /*      ti                                  reject if template insertion */
                                                                                                    /*         mi                               maybe interactive */
                                                                                                    /*            ur                            unrecordable */
                                                                                                    /*               up                         unrepeatable */
                                                                                                    /*                  xi                      exceptional inline */
                                                                                                    /*                     md                   modify document */
                                                                                                    /*                        mf                memory froth */
                                                                                                    /*                           nn             supress newline on save */
                                                                                                    /*                              cp          check for protection */
                                                                                                    /*                                 sm       send via maeve */
                                                                                                    /*                                    ba    wrap with CUR_CHANGE_BEFORE/CUR_CHANGE_AFTER */
                                                                                                    /*                                       fo send to focus owner */

    { "NewExpression",          ss_args_ustr,               T5_CMD_NEW_EXPRESSION,                      { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 } },
    { "ReplicateDown",          NULL,                       T5_CMD_REPLICATE_DOWN,                      { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "ReplicateRight",         NULL,                       T5_CMD_REPLICATE_RIGHT,                     { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "ReplicateUp",            NULL,                       T5_CMD_REPLICATE_UP,                        { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "ReplicateLeft",          NULL,                       T5_CMD_REPLICATE_LEFT,                      { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "MakeText",               NULL,                       T5_CMD_SS_MAKE_TEXT,                        { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1 } },
    { "MakeNumber",             NULL,                       T5_CMD_SS_MAKE_NUMBER,                      { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1 } },
    { "AutoSum",                NULL,                       T5_CMD_AUTO_SUM,                            { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 } },
    { "InsertOperatorPlus",     NULL,                       T5_CMD_INSERT_OPERATOR_PLUS,                { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 } },
    { "InsertOperatorMinus",    NULL,                       T5_CMD_INSERT_OPERATOR_MINUS,               { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 } },
    { "InsertOperatorTimes",    NULL,                       T5_CMD_INSERT_OPERATOR_TIMES,               { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 } },
    { "InsertOperatorDivide",   NULL,                       T5_CMD_INSERT_OPERATOR_DIVIDE,              { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 } },
    { "Functions",              ss_args_s32,                T5_CMD_SS_FUNCTIONS,                        { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "ActivateMenuFunc",       NULL,                       T5_CMD_ACTIVATE_MENU_FUNCTION_SELECTOR,     { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "NameIntro",              ss_args_s32,                T5_CMD_SS_NAME_INTRO,                       { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "Name",                   ss_args_name,               T5_CMD_SS_NAME,                             { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

static STYLE_SELECTOR style_selector_ob_ss;

static STYLE_SELECTOR style_selector_ob_ss_numform_all;

static const SS_FUNC_TABLE
ss_func_table[] =
{
    SS_FUNC_TABLE_ENTRY(SS_FUNC_NULL,               NULL),

    /* ones that were always built-in */
    SS_FUNC_TABLE_ENTRY(SS_FUNC_UOP_NOT,            c_uop_not),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_UOP_MINUS,          c_uop_minus),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_UOP_PLUS,           c_uop_plus),

    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_AND,            c_bop_and),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_CONCATENATE,    c_bop_concatenate),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_DIV,            c_bop_div),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_SUB,            c_bop_sub),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_OR,             c_bop_or),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_ADD,            c_bop_add),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_POWER,          c_bop_power),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_BOP_MUL,            c_bop_mul),

    SS_FUNC_TABLE_ENTRY(SS_FUNC_REL_EQ,             c_rel_eq),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_REL_GT,             c_rel_gt),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_REL_GTEQ,           c_rel_gteq),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_REL_LT,             c_rel_lt),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_REL_LTEQ,           c_rel_lteq),
    SS_FUNC_TABLE_ENTRY(SS_FUNC_REL_NEQ,            c_rel_neq),

    SS_FUNC_TABLE_ENTRY(SS_FUNC_IF,                 c_if),

    /* ones that had been split off */
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ABS,               c_abs),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ACOS,              c_acos),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ACOSEC,            c_acosec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ACOSECH,           c_acosech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ACOSH,             c_acosh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ACOT,              c_acot),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ACOTH,             c_acoth),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ADDRESS,           c_address),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_AGE,               c_age),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ASEC,              c_asec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ASECH,             c_asech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ASIN,              c_asin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ASINH,             c_asinh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ATAN,              c_atan),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ATAN_2,            c_atan_2),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ATANH,             c_atanh),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BASE,              c_base),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BESSELI,           c_besseli),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BESSELJ,           c_besselj),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BESSELK,           c_besselk),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BESSELY,           c_bessely),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BETA,              c_beta),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BETA_DIST,         c_beta_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BETA_INV,          c_beta_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BIN,               c_bin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BIN2DEC,           c_bin2dec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BIN2HEX,           c_bin2hex),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BIN2OCT,           c_bin2oct),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BINOM_DIST,        c_binom_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BINOM_DIST_RANGE,  c_binom_dist_range),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_BINOM_INV,         c_binom_inv),
	SS_FUNC_TABLE_ENTRY(SS_SPLIT_BITAND,            c_bitand),
	SS_FUNC_TABLE_ENTRY(SS_SPLIT_BITLSHIFT,         c_bitlshift),
	SS_FUNC_TABLE_ENTRY(SS_SPLIT_BITOR,             c_bitor),
	SS_FUNC_TABLE_ENTRY(SS_SPLIT_BITRSHIFT,         c_bitrshift),
	SS_FUNC_TABLE_ENTRY(SS_SPLIT_BITXOR,            c_bitxor),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ACOS,            c_c_acos),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ACOSEC,          c_c_acosec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ACOSECH,         c_c_acosech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ACOSH,           c_c_acosh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ACOT,            c_c_acot),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ACOTH,           c_c_acoth),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ADD,             c_c_add),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ASEC,            c_c_asec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ASECH,           c_c_asech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ASIN,            c_c_asin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ASINH,           c_c_asinh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ATAN,            c_c_atan),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ATANH,           c_c_atanh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_COMPLEX,         c_c_complex),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_CONJUGATE,       c_c_conjugate),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_COS,             c_c_cos),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_COSEC,           c_c_cosec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_COSECH,          c_c_cosech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_COSH,            c_c_cosh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_COT,             c_c_cot),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_COTH,            c_c_coth),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_DIV,             c_c_div),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_EXP,             c_c_exp),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_IMAGINARY,       c_c_imaginary),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_LN,              c_c_ln),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_MUL,             c_c_mul),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_POWER,           c_c_power),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_RADIUS,          c_c_radius),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_REAL,            c_c_real),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_ROUND,           c_c_round),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_SEC,             c_c_sec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_SECH,            c_c_sech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_SIN,             c_c_sin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_SINH,            c_c_sinh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_SQRT,            c_c_sqrt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_SUB,             c_c_sub),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_TAN,             c_c_tan),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_TANH,            c_c_tanh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_C_THETA,           c_c_theta),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CEILING,           c_ceiling),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CHAR,              c_char),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CHISQ_DIST,        c_chisq_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CHISQ_DIST_RT,     c_chisq_dist_rt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CHISQ_INV,         c_chisq_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CHISQ_INV_RT,      c_chisq_inv_rt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CHISQ_TEST,        c_chisq_test),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CHOOSE,            c_choose),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CLEAN,             c_clean),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CODE,              c_code),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COL,               c_col),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COLS,              c_cols),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COMBIN,            c_combin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COMBINA,           c_combina),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COMMAND,           c_command),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CONFIDENCE_NORM,   c_confidence_norm),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CONFIDENCE_T,      c_confidence_t),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CORREL,            c_correl),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COS,               c_cos),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COSEC,             c_cosec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COSECH,            c_cosech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COSH,              c_cosh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COT,               c_cot),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COTH,              c_coth),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COVARIANCE_P,      c_covariance_p),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_COVARIANCE_S,      c_covariance_s),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CTERM,             c_cterm),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_CURRENT_CELL,      c_current_cell),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DATE,              c_date),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DATEVALUE,         c_datevalue),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DAY,               c_day),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DAYNAME,           c_dayname),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DAYS,              c_days),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DAYS_360,          c_days_360),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DB,                c_db),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DDB,               c_ddb),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DEC2BIN,           c_dec2bin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DEC2HEX,           c_dec2hex),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DEC2OCT,           c_dec2oct),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DECIMAL,           c_decimal),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DEG,               c_deg),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DELTA,             c_delta),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DEREF,             c_deref),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DOLLAR,            c_dollar),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_DOUBLECLICK,       c_doubleclick),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_EDATE,             c_edate),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_EOMONTH,           c_eomonth),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ERF,               c_erf),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ERFC,              c_erfc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_EVEN,              c_even),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_EXACT,             c_exact),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_EXP,               c_exp),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_EXPON_DIST,        c_expon_dist),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_F_DIST,            c_F_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_F_DIST_RT,         c_F_dist_rt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_F_INV,             c_F_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_F_INV_RT,          c_F_inv_rt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_F_TEST,            c_F_test),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FACT,              c_fact),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FACTDOUBLE,        c_factdouble),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FALSE,             c_false),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FIND,              c_find),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FISHER,            c_fisher),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FISHERINV,         c_fisherinv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FIXED,             c_fixed),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FLIP,              c_flip),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FLOOR,             c_floor),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FORECAST,          c_forecast),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FORMULA_TEXT,      c_formula_text),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FREQUENCY,         c_frequency),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FV,                c_fv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_FVSCHEDULE,        c_fvschedule),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_GAMMA,             c_gamma),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_GAMMA_DIST,        c_gamma_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_GAMMA_INV,         c_gamma_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_GAMMALN,           c_gammaln),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_GESTEP,            c_gestep),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_GRAND,             c_grand),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_GROWTH,            c_growth),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_HEX2BIN,           c_hex2bin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_HEX2DEC,           c_hex2dec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_HEX2OCT,           c_hex2oct),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_HOUR,              c_hour),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_HYPGEOM_DIST,      c_hypgeom_dist),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_INDEX,             c_index),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_INT,               c_int),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_INTERCEPT,         c_intercept),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISBLANK,           c_isblank),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISERR,             c_iserr),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISERROR,           c_iserror),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISEVEN,            c_iseven),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISLOGICAL,         c_islogical),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISNA,              c_isna),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISNONTEXT,         c_isnontext),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISNUMBER,          c_isnumber),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISODD,             c_isodd),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISOWEEKNUM,        c_isoweeknum),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISREF,             c_isref),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ISTEXT,            c_istext),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_JOIN,              c_join),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LARGE,             c_large),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LEFT,              c_left),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LENGTH,            c_length),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LINEST,            c_linest),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LISTCOUNT,         c_listcount),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LN,                c_ln),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LOG,               c_log),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LOGEST,            c_logest),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LOGNORM_DIST,      c_lognorm_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LOGNORM_INV,       c_lognorm_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_LOWER,             c_lower),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_M_DETERM,          c_m_determ),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_M_INVERSE,         c_m_inverse),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_M_MULT,            c_m_mult),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_M_UNIT,            c_m_unit),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MID,               c_mid),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MINUTE,            c_minute),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MOD,               c_mod),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MODE_SNGL,         c_mode_sngl),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MONTH,             c_month),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MONTHDAYS,         c_monthdays),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MONTHNAME,         c_monthname),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_MROUND,            c_mround),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_N,                 c_n),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NA,                c_na),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NEGBINOM_DIST,     c_negbinom_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NORM_DIST,         c_norm_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NORM_S_DIST,       c_norm_s_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NORM_INV,          c_norm_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NORM_S_INV,        c_norm_s_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NOW,               c_now),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_NPER,              c_nper),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_OCT2BIN,           c_oct2bin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_OCT2DEC,           c_oct2dec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_OCT2HEX,           c_oct2hex),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODD,               c_odd),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_BETADIST,      c_odf_betadist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_COMPLEX,       c_odf_complex),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_FV,            c_odf_fv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMABS,         c_c_radius),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMAGINARY,     c_c_imaginary),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMARGUMENT,    c_c_theta),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMCONJUGATE,   c_c_conjugate),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMCOS,         c_c_cos),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMDIV,         c_c_div),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMEXP,         c_c_exp),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMLN,          c_c_ln),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMLOG10,       c_c_log_10),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMLOG2,        c_c_log_2),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMPOWER,       c_c_power),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMPRODUCT,     c_c_mul),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMREAL,        c_c_real),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMSIN,         c_c_sin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMSQRT,        c_c_sqrt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMSUB,         c_c_sub),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IMSUM,         c_c_add),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_INDEX,         c_odf_index),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_INT,           c_odf_int),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_IRR,           c_odf_irr),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_LOG10,         c_odf_log10),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_MOD,           c_odf_mod),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_PMT,           c_odf_pmt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_TDIST,         c_odf_tdist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ODF_TYPE,          c_odf_type),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PAGE,              c_page),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PAGES,             c_pages),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PEARSON,           c_pearson),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PERCENTILE_EXC,    c_percentile_exc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PERCENTILE_INC,    c_percentile_inc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PERCENTRANK_EXC,   c_percentrank_exc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PERCENTRANK_INC,   c_percentrank_inc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PERMUT,            c_permut),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PHI,               c_phi),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PI,                c_pi),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PMT,               c_pmt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_POISSON_DIST,      c_poisson_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_POWER,             c_bop_power),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PROB,              c_prob),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PROPER,            c_proper),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_PV,                c_pv),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_QUARTILE_EXC,      c_quartile_exc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_QUARTILE_INC,      c_quartile_inc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_QUOTIENT,          c_quotient),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RAD,               c_rad),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RAND,              c_rand),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RANDBETWEEN,       c_randbetween),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RATE,              c_rate),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RANK,              c_rank),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RANK_EQ,           c_rank_eq),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_REPLACE,           c_replace),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_REPT,              c_rept),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_REVERSE,           c_reverse),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RIGHT,             c_right),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ROUND,             c_round),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ROUNDDOWN,         c_rounddown),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ROUNDUP,           c_roundup),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ROW,               c_row),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_ROWS,              c_rows),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_RSQ,               c_rsq),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SERIESSUM,         c_seriessum),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SEC,               c_sec),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SECH,              c_sech),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SECOND,            c_second),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SET_NAME,          c_set_name),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SGN,               c_sgn),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SIN,               c_sin),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SINH,              c_sinh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SLN,               c_sln),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SLOPE,             c_slope),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SMALL,             c_small),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SORT,              c_sort),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SPEARMAN,          c_spearman),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SQR,               c_sqr),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_STANDARDIZE,       c_standardize),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_STEYX,             c_steyx),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_STRING,            c_string),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SUBSTITUTE,        c_substitute),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SUMPRODUCT,        c_sumproduct),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SUM_X2MY2,         c_sum_x2my2),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SUM_X2PY2,         c_sum_x2py2),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SUM_XMY2,          c_sum_xmy2),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_SYD,               c_syd),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_T,                 c_t),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_T_DIST,            c_t_dist),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_T_DIST_2T,         c_t_dist_2t),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_T_DIST_RT,         c_t_dist_rt),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_T_INV,             c_t_inv),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_T_INV_2T,          c_t_inv_2t),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_T_TEST,            c_t_test),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TAN,               c_tan),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TANH,              c_tanh),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TERM,              c_term),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TEXT,              c_text),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TIME,              c_time),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TIMEVALUE,         c_timevalue),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TODAY,             c_today),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TRANSPOSE,         c_transpose),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TREND,             c_trend),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TRIM,              c_trim),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TRIMMEAN,          c_trimmean),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TRUE,              c_true),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TRUNC,             c_trunc),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_TYPE,              c_type),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_UPPER,             c_upper),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_VALUE,             c_value),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_VERSION,           c_version),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_WEEKDAY,           c_weekday),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_WEEKNUMBER,        c_weeknumber),
    SS_FUNC_TABLE_ENTRY(SS_SPLIT_WEIBULL_DIST,      c_weibull_dist),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_YEAR,              c_year),

    SS_FUNC_TABLE_ENTRY(SS_SPLIT_Z_TEST,            c_z_test)
};

enum CHOICES_SS_CONTROL_IDS
{
    CHOICES_SS_ID_GROUP = (OBJECT_ID_SS * 1000) + 75,
    CHOICES_SS_ID_CALC_AUTO,
    CHOICES_SS_ID_CALC_BACKGROUND,
    CHOICES_SS_ID_CALC_ON_LOAD,
    CHOICES_SS_ID_CALC_ADDITIONAL_ROUNDING,
    CHOICES_SS_ID_EDIT_IN_CELL,
    CHOICES_SS_ID_ALTERNATE_FORMULA_STYLE,

    CHOICES_CHART_ID_GROUP,
    CHOICES_CHART_ID_UPDATE_AUTO
};

static /*poked*/ DIALOG_CONTROL
choices_ss_group =
{
    CHOICES_SS_ID_GROUP, DIALOG_MAIN_GROUP,
    { 0, 0, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
choices_ss_group_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_GROUP), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
choices_ss_calc_auto =
{
    CHOICES_SS_ID_CALC_AUTO, CHOICES_SS_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_ss_calc_auto_data = { { 0 }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_CALC_AUTO) };

static const DIALOG_CONTROL
choices_ss_calc_background =
{
    CHOICES_SS_ID_CALC_BACKGROUND, CHOICES_SS_ID_GROUP,
    { CHOICES_SS_ID_CALC_AUTO, CHOICES_SS_ID_CALC_AUTO },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_ss_calc_background_data = { { 0 }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_CALC_BACKGROUND) };

static const DIALOG_CONTROL
choices_ss_calc_on_load =
{
    CHOICES_SS_ID_CALC_ON_LOAD, CHOICES_SS_ID_GROUP,
    { CHOICES_SS_ID_CALC_BACKGROUND, CHOICES_SS_ID_CALC_BACKGROUND },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_ss_calc_on_load_data = { { 0 }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_CALC_ON_LOAD) };

static const DIALOG_CONTROL
choices_ss_calc_additional_rounding =
{
    CHOICES_SS_ID_CALC_ADDITIONAL_ROUNDING, CHOICES_SS_ID_GROUP,
    { CHOICES_SS_ID_CALC_ON_LOAD, CHOICES_SS_ID_CALC_ON_LOAD },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_ss_calc_additional_rounding_data = { { 0 }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_CALC_ADDITIONAL_ROUNDING) };

static const DIALOG_CONTROL
choices_ss_edit_in_cell =
{
    CHOICES_SS_ID_EDIT_IN_CELL, CHOICES_SS_ID_GROUP,
    { CHOICES_SS_ID_CALC_ADDITIONAL_ROUNDING, CHOICES_SS_ID_CALC_ADDITIONAL_ROUNDING },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_ss_edit_in_cell_data = { { 0 }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_EDIT_IN_CELL) };

static const DIALOG_CONTROL
choices_ss_alternate_formula_style =
{
    CHOICES_SS_ID_ALTERNATE_FORMULA_STYLE, CHOICES_SS_ID_GROUP,
    { CHOICES_SS_ID_EDIT_IN_CELL, CHOICES_SS_ID_EDIT_IN_CELL },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_ss_alternate_formula_style_data = { { 0 }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_ALTERNATE_FORMULA_STYLE) };

/* saves having ob_charb */
static const DIALOG_CONTROL
choices_chart_group =
{
    CHOICES_CHART_ID_GROUP, DIALOG_MAIN_GROUP,
    { CHOICES_SS_ID_GROUP, CHOICES_SS_ID_GROUP, CHOICES_SS_ID_GROUP, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
choices_chart_group_data = { UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_CHART_GROUP), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
choices_chart_update_auto =
{
    CHOICES_CHART_ID_UPDATE_AUTO, CHOICES_CHART_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_chart_update_auto_data = { { 0 }, UI_TEXT_INIT_RESID(SS_MSG_DIALOG_CHOICES_CHART_UPDATE_AUTO) };

static const DIALOG_CTL_CREATE
choices_ss_ctl_create[] =
{
    { &choices_ss_group, &choices_ss_group_data },
    { &choices_ss_calc_auto, &choices_ss_calc_auto_data },
    { &choices_ss_calc_background, &choices_ss_calc_background_data },
    { &choices_ss_calc_on_load, &choices_ss_calc_on_load_data },
    { &choices_ss_calc_additional_rounding, &choices_ss_calc_additional_rounding_data },
    { &choices_ss_edit_in_cell, &choices_ss_edit_in_cell_data },
    { &choices_ss_alternate_formula_style, &choices_ss_alternate_formula_style_data },

    { &choices_chart_group, &choices_chart_group_data },
    { &choices_chart_update_auto, &choices_chart_update_auto_data }
};

static void
ss_show_error(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    P_EV_CELL p_ev_cell;

    status_line_clear(p_docu, STATUS_LINE_LEVEL_SS_FORMULA_LINE);

    if(P_DATA_NONE != (p_ev_cell = p_ev_cell_object_from_slr(p_docu, p_slr)))
    {
        if(DATA_ID_ERROR == p_ev_cell->ev_parms.data_id)
        {
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
            quick_ublock_with_buffer_setup(quick_ublock);

            status_assert(resource_lookup_quick_ublock(&quick_ublock, p_ev_cell->ss_constant.ss_error.status));
            status_assert(quick_ublock_nullch_add(&quick_ublock));

            switch(p_ev_cell->ss_constant.ss_error.type)
            {
            case ERROR_NORMAL:
                status_line_setf(p_docu, STATUS_LINE_LEVEL_SS_FORMULA_LINE, SS_MSG_STATUS_ERR, quick_ublock_ustr(&quick_ublock));
                break;

            default: default_unhandled();
#if CHECKING
            case ERROR_CUSTOM:
            case ERROR_PROPAGATED:
#endif
                {
                UCHARZ ustr_buf_slr[BUF_EV_LONGNAMLEN];
                EV_DOCNO ev_docno = ev_docno_from_p_docu(p_docu);
                EV_SLR ev_slr;

                zero_struct(ev_slr);
                ev_slr.docno = p_ev_cell->ss_constant.ss_error.docno; /* equivalent UBF */
                ev_slr.col = p_ev_cell->ss_constant.ss_error.col; /* equivalent SBF */
                ev_slr.row = p_ev_cell->ss_constant.ss_error.row;

                (void) ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf_slr), elemof32(ustr_buf_slr), ev_docno, &ev_slr);

                status_line_setf(p_docu,
                                 STATUS_LINE_LEVEL_SS_FORMULA_LINE,
                                 (p_ev_cell->ss_constant.ss_error.type == ERROR_CUSTOM) ? SS_MSG_STATUS_CUSTOM_ERR : SS_MSG_STATUS_PROPAGATED_ERR,
                                 quick_ublock_ustr(&quick_ublock),
                                 ustr_buf_slr);
                break;
                }
            }

            quick_ublock_dispose(&quick_ublock);
        }
    }
}

/*
main events
*/

_Check_return_
static STATUS
ob_ss_save_names(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, OBJECT_ID_SS, T5_CMD_SS_NAME, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 3);
        const EV_DOCNO ev_docno = ev_docno_from_p_docu(p_docu);
        STATUS ev_item_no = 0;
        EV_RESOURCE ev_resource;
        RESOURCE_SPEC resource_spec;

        ev_enum_resource_init(&ev_resource, &resource_spec, EV_RESO_NAMES, ev_item_no, ev_docno, ev_docno);

        while((ev_item_no = ev_enum_resource_get(&resource_spec, &ev_resource)) >= 0)
        {
            p_args[0].val.ustr = array_ustr(&resource_spec.h_id_ustr);
            p_args[1].val.ustr = array_ustr(&resource_spec.h_definition_ustr);
            p_args[2].val.ustr = resource_spec.ustr_description;
            status_break(status = ownform_save_arglist(arglist_handle, OBJECT_ID_SS, p_construct_table, p_of_op_format));
        }

        if(ev_item_no < 0)
            if(STATUS_FAIL != ev_item_no)
                status = ev_item_no;

        ev_enum_resource_dispose(&resource_spec);

        p_args[0].val.ustr = NULL;
        p_args[1].val.ustr = NULL;
        p_args[2].val.ustr = NULL;
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
ob_ss_msg_data_save_1(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.data_class >= DATA_SAVE_DOC)
    {
        status = ob_ss_save_names(p_docu, p_of_op_format);
    }

    return(status);
}

_Check_return_
static STATUS
ob_ss_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_SAVE p_msg_save)
{
    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__DATA_SAVE_1:
        return(ob_ss_msg_data_save_1(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_ob_ss)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_DOCU_COLROW:
        consume_bool(ev_event_occurred(ev_docno_from_p_docu(p_docu), EV_EVENT_CURRENT_CELL));

        /*FALLTHRU*/

    case T5_MSG_RECALCED:
        status_assert(ss_formula_reflect_contents(p_docu));
        ss_show_error(p_docu, &p_docu->cur.slr);
        return(STATUS_OK);

    case T5_MSG_SUPPORTER_LOADED:
        ev_todo_add_doc_dependents(ev_docno_from_p_docu(p_docu));
        return(STATUS_OK);

    case T5_MSG_NAME_UREF:
        ev_field_names_check((P_NAME_UREF) p_data);
        return(STATUS_OK);

    case T5_MSG_SAVE:
        return(ob_ss_msg_save(p_docu, (PC_MSG_SAVE) p_data));

    case T5_MSG_CUR_CHANGE_AFTER:
        return(ss_formula_reflect_contents(p_docu));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* ob_ss uref events
*
******************************************************************************/

PROC_UREF_EVENT_PROTO(static, ob_ss_uref_event)
{
    STATUS status = STATUS_OK;

    /* free resources that may be owned by cells going away */
    switch(uref_message)
    {
    case Uref_Msg_CLOSE1:
    case Uref_Msg_Delete:
    case Uref_Msg_Overwrite:
        {
        SCAN_BLOCK scan_block;

        if(Uref_Msg_CLOSE1 == uref_message)
            status = cells_scan_init(p_docu, &scan_block, SCAN_MATRIX, SCAN_WHOLE, NULL, OBJECT_ID_SS);
        else
        {
            DOCU_AREA docu_area;
            docu_area_from_region(&docu_area, &p_uref_event_block->uref_parms.source.region);
            status = cells_scan_init(p_docu, &scan_block, SCAN_MATRIX, SCAN_AREA, &docu_area, OBJECT_ID_SS);
        }

        if(status_done(status))
        {
            OBJECT_DATA object_data;

            while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
            {
                if(P_DATA_NONE != object_data.u.p_ev_cell)
                    ev_cell_free_resources(object_data.u.p_ev_cell);
            }
        }

        break;
        }

    default:
        break;
    }

    if(status_ok(status))
        status = ev_uref_uref_event(p_docu, uref_message, p_uref_event_block);

    return(status);
}

/******************************************************************************
*
* popup a dialog which has a list box of available functions from the given class
*
******************************************************************************/

typedef struct FUNCTION_LIST_ENTRY
{
    S32 ev_category;
    S32 ev_item_no;
    S32 ev_status_line_number;

    QUICK_UBLOCK quick_ublock;
}
FUNCTION_LIST_ENTRY, * P_FUNCTION_LIST_ENTRY; typedef const FUNCTION_LIST_ENTRY * PC_FUNCTION_LIST_ENTRY; 

typedef struct FUNCTION_LIST_STATE
{
    ARRAY_HANDLE list_handle;
    BOOL status_line_changed;
    S32 selected_item;
}
FUNCTION_LIST_STATE, * P_FUNCTION_LIST_STATE;

enum SS_FUNCTIONS_CONTROL_IDS
{
    SS_FUNCTIONS_ID_LIST = 944
};

static /*poked*/ DIALOG_CONTROL
ss_functions_list =
{
    SS_FUNCTIONS_ID_LIST, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0 }, /* filled in at run-time */
    { DRT(LTLT, LIST_TEXT), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
ss_functions_insert_data = { { DIALOG_COMPLETION_OK }, UI_TEXT_INIT_RESID(MSG_INSERT) };

static const DIALOG_CTL_CREATE
ss_functions_ctl_create[] =
{
    { &dialog_main_group },

    { &ss_functions_list, &stdlisttext_data },

    { &defbutton_ok, &ss_functions_insert_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static UI_SOURCE
function_list_source;

_Check_return_
static STATUS
dialog_ss_functions_ctl_fill_source(
    _InoutRef_  P_DIALOG_MSG_CTL_FILL_SOURCE p_dialog_msg_ctl_fill_source)
{
    switch(p_dialog_msg_ctl_fill_source->dialog_control_id)
    {
    case SS_FUNCTIONS_ID_LIST:
        p_dialog_msg_ctl_fill_source->p_ui_source = &function_list_source;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_ss_functions_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    const P_FUNCTION_LIST_STATE p_function_list_state = (P_FUNCTION_LIST_STATE) p_dialog_msg_ctl_create_state->client_handle;

    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case SS_FUNCTIONS_ID_LIST:
        p_dialog_msg_ctl_create_state->state_set.state.list_text.itemno = p_function_list_state->selected_item;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_ss_functions_ctl_state_change(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    const P_FUNCTION_LIST_STATE p_function_list_state = (P_FUNCTION_LIST_STATE) p_dialog_msg_ctl_state_change->client_handle;
    STATUS status = STATUS_OK;

    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case SS_FUNCTIONS_ID_LIST:
        {
        status_line_clear(p_docu, STATUS_LINE_LEVEL_FUNCTION_SELECTOR);
        p_function_list_state->selected_item = p_dialog_msg_ctl_state_change->new_state.list_text.itemno;
        if(array_index_is_valid(&p_function_list_state->list_handle, p_function_list_state->selected_item))
        {
            P_FUNCTION_LIST_ENTRY p_function_list_entry = array_ptr_no_checks(&p_function_list_state->list_handle, FUNCTION_LIST_ENTRY, p_function_list_state->selected_item);
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, elemof32("some...space...for...a...status...message"));
            quick_ublock_with_buffer_setup(quick_ublock);

            status = ss_function_get_help(&quick_ublock, p_function_list_entry->ev_status_line_number);

            if(status_ok(status))
                status = quick_ublock_nullch_add(&quick_ublock);

            if(status_ok(status))
            {
                status_line_setf(p_docu, STATUS_LINE_LEVEL_FUNCTION_SELECTOR, SS_MSG_STATUS_STATUS_LINE_BASE, quick_ublock_ustr(&quick_ublock));
                p_function_list_state->status_line_changed = TRUE;
            }

            quick_ublock_dispose(&quick_ublock);
        }

        break;
        }

    default:
        break;
    }

    return(status);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_ss_functions)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_FILL_SOURCE:
        return(dialog_ss_functions_ctl_fill_source((P_DIALOG_MSG_CTL_FILL_SOURCE) p_data));

    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_ss_functions_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_ss_functions_ctl_state_change(p_docu, (PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
ss_ui_quick_ublock_func_name_add(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR ustr)
{
    STATUS status;
    SS_DECOMPILER_OPTIONS ss_decompiler_options = g_ss_decompiler_options;

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

    status = quick_ublock_func_name_add(p_quick_ublock, ustr);

    g_ss_decompiler_options = ss_decompiler_options;

    return(status);
}

_Check_return_
static STATUS
ss_function_textual_representation_from(
    P_FUNCTION_LIST_ENTRY p_function_list_entry,
    _In_z_      PC_USTR ustr,
    _InVal_     S32 n_args)
{
    status_return(ss_ui_quick_ublock_func_name_add(&p_function_list_entry->quick_ublock, ustr));

    if(n_args > 0)
    {
        U32 argument_index;

        status_return(quick_ublock_a7char_add(&p_function_list_entry->quick_ublock, CH_LEFT_PARENTHESIS));

        for(argument_index = 0; argument_index < (U32) n_args; ++argument_index)
            status_return(ss_function_add_argument(&p_function_list_entry->quick_ublock, p_function_list_entry->ev_status_line_number, argument_index, FALSE));

        status_return(quick_ublock_a7char_add(&p_function_list_entry->quick_ublock, CH_RIGHT_PARENTHESIS));
    }

    return(quick_ublock_nullch_add(&p_function_list_entry->quick_ublock));
}

/******************************************************************************
*
* sorting QUICK_UBLOCKs is easier if you abandon using static buffers...
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, proc_qsort_function_list, FUNCTION_LIST_ENTRY)
{
    QSORT_ARG1_VAR_DECL(PC_FUNCTION_LIST_ENTRY, p_function_list_entry_1);
    QSORT_ARG2_VAR_DECL(PC_FUNCTION_LIST_ENTRY, p_function_list_entry_2);

    PC_USTR ustr_1 = quick_ublock_ustr(&p_function_list_entry_1->quick_ublock);
    PC_USTR ustr_2 = quick_ublock_ustr(&p_function_list_entry_2->quick_ublock);

    return(ustr_compare_nocase(ustr_1, ustr_2)); 
}

T5_CMD_PROTO(static, t5_cmd_ss_functions)
{
    STATUS status = STATUS_OK;
    S32 category = EV_RESO_ALL_VISIBLE;
    STATUS caption_resource_id;
    STATUS help_topic_resource_id;
    FUNCTION_LIST_STATE function_list_state;
    P_FUNCTION_LIST_ENTRY p_function_list_entry;
    PIXIT max_width = 0;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    zero_struct(function_list_state);

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        P_ARGLIST_ARG p_arg;
        if(arg_present(&p_t5_cmd->arglist_handle, 0, &p_arg))
            category = p_arg->val.s32;
    }

    if(category == EV_RESO_ALL_VISIBLE)
    {
        caption_resource_id    = SS_MSG_DIALOG_FUNCTIONS_ALL;
        help_topic_resource_id = SS_MSG_DLG_HT_FUNCTIONS_ALL;
    }
    else if((category < 0) || (category > EV_RESO_COMPAT))
    {
        category = -1;
        caption_resource_id    = SS_MSG_DIALOG_FUNCTIONS_SOME;
        help_topic_resource_id = SS_MSG_DLG_HT_FUNCTIONS_SOME;
    }
    else
    {
        caption_resource_id    = (category - EV_RESO_ENGINEER) + SS_MSG_DIALOG_FUNCTIONS_ENGINEER;
        help_topic_resource_id = (category - EV_RESO_ENGINEER) + SS_MSG_DLG_HT_FUNCTIONS_ENGINEER;
    }

    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_DATABASE - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_DATABASE - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_DATE     - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_DATE     - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_FINANCE  - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_FINANCE  - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_MATHS    - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_MATHS    - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_MATRIX   - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_MATRIX   - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_MISC     - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_MISC     - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_STATS    - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_STATS    - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_STRING   - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_STRING   - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_TRIG     - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_TRIG     - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_CONTROL  - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_CONTROL  - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_LOOKUP   - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_LOOKUP   - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_CUSTOM   - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_CUSTOM   - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_LOGICAL  - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_LOGICAL  - EV_RESO_ENGINEER));
    assert_EQ((SS_MSG_DIALOG_FUNCTIONS_COMPAT   - SS_MSG_DIALOG_FUNCTIONS_ENGINEER), (EV_RESO_COMPAT   - EV_RESO_ENGINEER));

    function_list_source.type = UI_SOURCE_TYPE_NONE;

    {
    SS_RECOG_CONTEXT ss_recog_context;
    EV_RESOURCE ev_resource;
    RESOURCE_SPEC resource_spec;
    S32 ui_item_no = 0;

    ss_recog_context_push(&ss_recog_context);

    ev_enum_resource_init(&ev_resource, &resource_spec, (category == -1) ? EV_RESO_ALL_VISIBLE : category, 0, DOCNO_NONE, ev_docno_from_p_docu(p_docu));

    for(;;)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(*p_function_list_entry), 1);
        STATUS ev_item_no;

        /* SKS 09jun93 - 'some functions' list is now read in from config file */
        if(category == -1)
        {
            const PC_DOCU p_docu_config = p_docu_from_config();
            const PC_ARRAY_HANDLE p_ui_numform_handle = &p_docu_config->numforms;
            PC_USTR ustr_function_name = NULL;
            const ARRAY_INDEX n_elements = array_elements(p_ui_numform_handle);

            while(ui_item_no < n_elements)
            {
                const PC_UI_NUMFORM p_ui_numform = array_ptrc(p_ui_numform_handle, UI_NUMFORM, ui_item_no);

                if(p_ui_numform->numform_class != UI_NUMFORM_CLASS_FUNCTIONS_SOME)
                {
                    ++ui_item_no;
                    continue;
                }

                ustr_function_name = p_ui_numform->ustr_numform;
                break;
            }

            if(NULL == ustr_function_name)
                break;

            ++ui_item_no;

            if((ev_item_no = ev_func_lookup(ustr_function_name)) < 0)
                continue;

            ev_resource.item_no = ev_item_no;
        }

        if((ev_item_no = ev_enum_resource_get(&resource_spec, &ev_resource)) < 0)
        {
            if(STATUS_FAIL != ev_item_no)
                status = ev_item_no;
            break;
        }

        if(NULL != (p_function_list_entry = al_array_extend_by(&function_list_state.list_handle, FUNCTION_LIST_ENTRY, 1, &array_init_block, &status)))
        {
            PIXIT width;

            /* each ***entry*** has a quick buf for textual representation in list box! */
            /*quick_block_setup_null(p_function_list_entry->quick_block);*/

            p_function_list_entry->ev_category = ev_resource.category;
            p_function_list_entry->ev_item_no = ev_item_no;

            switch(ev_resource.category)
            {
            case EV_RESO_CUSTOM:
            case EV_RESO_NAMES:
                p_function_list_entry->ev_status_line_number = 0;
                break;

            default:
                p_function_list_entry->ev_status_line_number = SS_MSG_STATUS_GET_NUMBER_FROM_RPN(ev_item_no);
                break;
            }

            status = ss_function_textual_representation_from(p_function_list_entry, array_ustr(&resource_spec.h_id_ustr), resource_spec.n_args);

            /* but this contains inlines!!! */
            width = ui_width_from_ustr(quick_ublock_ustr(&p_function_list_entry->quick_ublock));
            max_width = MAX(max_width, width);
        }

        status_break(status);
    }

    ev_enum_resource_dispose(&resource_spec);

    ss_recog_context_pull(&ss_recog_context);
    } /*block*/

    /* sort the elements (esp. for custom functions/names) (also note RPN order different to ASCII order, especially in German etc.!) */
    switch(category)
    {
    case -1: /* 'quick' list never sorted */
        break;

    default:
        al_array_qsort(&function_list_state.list_handle, proc_qsort_function_list);
        break;
    }

    /* make a source of text pointers to these elements for list box processing */
    if(status_ok(status))
        status = ui_source_create_ub(&function_list_state.list_handle, &function_list_source, UI_TEXT_TYPE_USTR_PERM, offsetof32(FUNCTION_LIST_ENTRY, quick_ublock));

    if(status_ok(status))
    {
        { /* make appropriate size box */
        const PIXIT buttons_width = DIALOG_DEFOK_H + DIALOG_STDSPACING_H + DIALOG_STDCANCEL_H;
        PIXIT_SIZE list_size;
        DIALOG_CMD_CTL_SIZE_ESTIMATE dialog_cmd_ctl_size_estimate;
        dialog_cmd_ctl_size_estimate.p_dialog_control = &ss_functions_list;
        dialog_cmd_ctl_size_estimate.p_dialog_control_data = &stdlisttext_data;
        ui_dlg_ctl_size_estimate(&dialog_cmd_ctl_size_estimate);
        dialog_cmd_ctl_size_estimate.size.x += max_width;
        ui_list_size_estimate(array_elements(&function_list_state.list_handle), &list_size);
        dialog_cmd_ctl_size_estimate.size.x += list_size.cx;
        dialog_cmd_ctl_size_estimate.size.y += list_size.cy;
        dialog_cmd_ctl_size_estimate.size.x = MAX(dialog_cmd_ctl_size_estimate.size.x, buttons_width);
        ss_functions_list.relative_offset[2] = dialog_cmd_ctl_size_estimate.size.x;
        ss_functions_list.relative_offset[3] = dialog_cmd_ctl_size_estimate.size.y;
        } /*block*/

        {
        DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, ss_functions_ctl_create, elemof32(ss_functions_ctl_create), help_topic_resource_id);
        /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
        dialog_cmd_process_dbox.caption.text.resource_id = caption_resource_id;
        dialog_cmd_process_dbox.p_proc_client = dialog_event_ss_functions;
        dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &function_list_state;
        status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
        } /*block*/

        if(function_list_state.status_line_changed)
            status_line_clear(p_docu, STATUS_LINE_LEVEL_FUNCTION_SELECTOR);

        if(status == STATUS_FAIL)
            status = STATUS_OK;
        else if(status_ok(status))
        {
            if(array_index_is_valid(&function_list_state.list_handle, function_list_state.selected_item))
            {
                /* selected function to use NB. UI list unsorted so maps directly onto field list index */
                p_function_list_entry = array_ptr_no_checks(&function_list_state.list_handle, FUNCTION_LIST_ENTRY, function_list_state.selected_item);
                status = ss_function_paste_to_editing_line(p_docu, p_function_list_entry->ev_category, p_function_list_entry->ev_item_no);
            }
        }
    }

    ui_lists_dispose_ub(&function_list_state.list_handle, &function_list_source, offsetof32(FUNCTION_LIST_ENTRY, quick_ublock));

    return(status);
}

/* Given category and item number get the relevant argument text
 * and then paste the resulting data into the editing line.
 */

_Check_return_
static STATUS
ss_function_paste_to_editing_line(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     S32 category,
    _InVal_     S32 item_number)
{
    SS_RECOG_CONTEXT ss_recog_context;
    SS_DECOMPILER_OPTIONS ss_decompiler_options = g_ss_decompiler_options;
    EV_RESOURCE ev_resource;
    RESOURCE_SPEC resource_spec;
    STATUS ev_item_no = item_number;
    STATUS status = STATUS_OK;

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

    ev_enum_resource_init(&ev_resource, &resource_spec, category, ev_item_no, DOCNO_NONE, ev_docno_from_p_docu(p_docu));

    if((ev_item_no = ev_enum_resource_get(&resource_spec, &ev_resource)) >= 0)
    {
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, elemof32("function(....and....,....some....,....arguments....)"));
        quick_ublock_with_buffer_setup(quick_ublock);

        /* allocate some store for the command and bosch it into there. NB. no terminating CH_NULL wanted in quick_block */
        if(status_ok(status))
            if(g_ss_decompiler_options.initial_formula_equals)
                status = quick_ublock_a7char_add(&quick_ublock, CH_EQUALS_SIGN);

        if(status_ok(status))
            status = quick_ublock_func_name_add(&quick_ublock, array_ustr(&resource_spec.h_id_ustr));

        /* strap all the argument descriptions onto the end encased in in-lines */
        if(status_ok(status) && (resource_spec.n_args > 0))
            if(status_ok(status = quick_ublock_a7char_add(&quick_ublock, CH_LEFT_PARENTHESIS)))
            {
                S32 argument_index = 0;

                for(;;)
                {
                    status_break(status = ss_function_add_argument(&quick_ublock, SS_MSG_STATUS_GET_NUMBER_FROM_RPN(item_number), argument_index, TRUE));

                    if(++argument_index >= resource_spec.n_args)
                    {
                        status = quick_ublock_a7char_add(&quick_ublock, CH_RIGHT_PARENTHESIS);
                        break;
                    }
                }
            }

        ev_enum_resource_dispose(&resource_spec);

        if(status_ok(status))
        {
            T5_MESSAGE t5_message = (OBJECT_ID_SLE == p_docu->focus_owner) ? T5_MSG_PASTE_EDITLINE : T5_MSG_ACTIVATE_EDITLINE;
            T5_PASTE_EDITLINE t5_paste_editline;
            t5_paste_editline.p_quick_ublock = &quick_ublock;
            t5_paste_editline.select = FALSE;
            t5_paste_editline.special_select = FALSE;
            status = object_call_id(OBJECT_ID_SLE, p_docu, t5_message, &t5_paste_editline);
        }

        quick_ublock_dispose(&quick_ublock);
    }
    else
    {
        if(STATUS_FAIL != ev_item_no)
            status = ev_item_no;
    }

    ss_recog_context_pull(&ss_recog_context);

    g_ss_decompiler_options = ss_decompiler_options;

    return(status);
}

/* Generate argument text given the argument index.
 * The argument can also be embdeded within an in-line sequence for auto-highlighting.
 * This is called from various functions within the code.
 */

_Check_return_
static STATUS
ss_function_add_arg_inline(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     STATUS message,
    _InVal_     U32 argument_index,
    _InVal_     BOOL in_line_wrap,
    _In_reads_(arg_len) PC_UCHARS message_string,
    _InVal_     U32 arg_len)
{
    if(in_line_wrap)
    {
        BYTE msg_token[4];
        /* data consists of msg token and the text to display for the inline */
        MULTIPLE_DATA multiple_data[3];

        msg_token[0] = (BYTE) ((message - SS_MSG_BASE)     );
        msg_token[1] = (BYTE) ((message - SS_MSG_BASE) >> 8);
        msg_token[2] = (BYTE) (argument_index     );
        msg_token[3] = (BYTE) (argument_index >> 8);

        multiple_data[0].p_data = msg_token;
        multiple_data[0].n_bytes = elemof32(msg_token);

        multiple_data[1].p_data = de_const_cast(P_ANY, message_string); /* not written to */
        multiple_data[1].n_bytes = arg_len;

        multiple_data[2].p_data = empty_string;
        multiple_data[2].n_bytes = 1;

        return(inline_quick_ublock_from_multiple_data(p_quick_ublock, IL_SLE_ARGUMENT, IL_TYPE_PRIVATE, multiple_data, elemof32(multiple_data)));
    }

    return(quick_ublock_uchars_add(p_quick_ublock, message_string, arg_len));
}

_Check_return_
static STATUS
ss_function_add_argument_try(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_        STATUS message,
    _InVal_     U32 argument_index,
    _InVal_     BOOL in_line_wrap,
    _In_z_      PC_USTR message_string_in /*pointing past CH_VERTICAL_LINE*/)
{
    PC_USTR message_string = message_string_in;
    U32 argument_number = 0; /* allow |x:number| without any digits given */

    /* found another | - just skip it */
    if(CH_VERTICAL_LINE == PtrGetByte(message_string))
        return(1);

    /* get argument number from digits */
    while(sbchar_isdigit(PtrGetByte(message_string)))
    {
        const U32 digit = (U32) PtrGetByte(message_string) - CH_DIGIT_ZERO;
        ustr_IncByte(message_string);

        argument_number *= 10;
        argument_number += digit;
    }

    /* message_string pointing at first char after digit sequence */

    if(argument_number == argument_index)
    {
        PC_USTR ustr = message_string;
        U32 arg_len;

        /* place a function arg separator char before the parameter if second or subsequent parameter */
        if(0 != argument_index)
        {
            if(!in_line_wrap && (CH_LEFT_CURLY_BRACKET == PtrGetByte(message_string)))
            {   /* for optional parameters, place the separator AFTER the opening bracket */
                status_return(quick_ublock_a7char_add(p_quick_ublock, CH_LEFT_CURLY_BRACKET));
                ustr_IncByte(message_string);

            }

            status_return(quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context_alt.function_arg_sep));
        }

        ustr = message_string;

        for(;;)
        {
            /* argument name is terminated by : or | (can't contain ||) */
            if(CH_NULL == PtrGetByte(ustr))
                break;

            if(CH_COLON == PtrGetByte(ustr))
                break;

            if(CH_VERTICAL_LINE == PtrGetByte(ustr))
            {
                assert(CH_VERTICAL_LINE != PtrGetByteOff(ustr, 1)); /* || shouldn't be present in argument name */
                break;
            }

            ustr_IncByte(ustr);
        }

        arg_len = PtrDiffBytesU32(ustr, message_string);

        status_return(ss_function_add_arg_inline(p_quick_ublock, message, argument_index, in_line_wrap, message_string, arg_len));

        return(0); /* added the requested arg, so caller can terminate */
    }

    /* skip this whole delimited sequence */
    for(;;)
    {
        if(CH_NULL == PtrGetByte(message_string))
            break;

        if(CH_VERTICAL_LINE == PtrGetByte(message_string))
        {
            ustr_IncByte(message_string);

            /* | followed by more text or CH_NULL terminates delimited sequence */
            if(CH_VERTICAL_LINE != PtrGetByte(message_string))
                break;
        }

        ustr_IncByte(message_string);
    }

    return(PtrDiffBytesS32(message_string, message_string_in));
}

_Check_return_
static STATUS
ss_function_add_argument(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_        STATUS message,
    _InVal_     U32 argument_index,
    _InVal_     BOOL in_line_wrap)
{
    PC_USTR message_string;

    if((0 != message) && (NULL != (message_string = resource_lookup_ustr_no_default(message))))
    {
        for(;;)
        {
            U8 ch = PtrGetByte(message_string);

            if(CH_NULL == ch)
                break;

            ustr_IncByte(message_string);

            if(CH_VERTICAL_LINE == ch)
            {
                STATUS status = ss_function_add_argument_try(p_quick_ublock, message, argument_index, in_line_wrap, message_string);
                U32 uchars_n;

                if(!status_done(status))
                    return(status); /* failure or OK iff argument added */

                uchars_n = (U32) status;
                ustr_IncBytes(message_string, uchars_n);
                continue;
            }

            /* other characters are ignored here */
        }
    }

    /* Unable to locate a suitable entry for this argument so default to the standard (empty) message */
    if(NULL != (message_string = resource_lookup_ustr_no_default(SS_MSG_STATUS_ARG_DEFAULT_STRING)))
    {
        const U32 arg_len = ustrlen32(message_string);

        if(0 == message)
            message = SS_MSG_STATUS_ARG_DEFAULT_STRING;

        if(0 != argument_index)
            status_return(quick_ublock_ucs4_add(p_quick_ublock, g_ss_recog_context_alt.function_arg_sep));

        return(ss_function_add_arg_inline(p_quick_ublock, message, argument_index, in_line_wrap, message_string, arg_len));
    }

    return(STATUS_OK);
}

/* Given an index for an argument set the status line to the help for that command.
 * This is called by the SLE for argument highlighting.
 */

_Check_return_
static STATUS
ss_function_get_argument_help_try(
    _InRef_     P_SS_FUNCTION_ARGUMENT_HELP p_ss_function_argument_help,
    _In_z_      PC_USTR message_string_in)
{
    PC_USTR message_string = message_string_in;
    U32 argument_number = 0;

    /* found another | - just skip it */
    if(CH_VERTICAL_LINE == PtrGetByte(message_string))
        return(1);

    /* get argument number from digits */
    while(sbchar_isdigit(PtrGetByte(message_string)))
    {
        const U32 digit = (U32) PtrGetByte(message_string) - CH_DIGIT_ZERO;
        ustr_IncByte(message_string);

        argument_number *= 10;
        argument_number += digit;
    }

    /* message_string pointing to first character after digit sequence */

    if(argument_number == p_ss_function_argument_help->arg_index)
    {
        for(;;)
        {
            U8 ch = PtrGetByte(message_string);

            if(CH_NULL == ch)
                break;

            ustr_IncByte(message_string);

            if(CH_COLON == ch)
                break; /* message_string pointing to first character of help for this argument */

            if(CH_VERTICAL_LINE == ch)
            {
                assert(CH_VERTICAL_LINE != PtrGetByte(message_string)); /* || shouldn't be present in argument name */
                /* no help present for this arg */
                return(PtrDiffBytesS32(message_string, message_string_in));
            }
        }

        for(;;)
        {
            U8 ch = PtrGetByte(message_string);

            if(CH_NULL == ch)
                break;

            ustr_IncByte(message_string);

            if(CH_VERTICAL_LINE == ch)
            {
                ch = PtrGetByte(message_string);

                if(CH_VERTICAL_LINE != ch)
                    break;

                /* || is allowed in help string */
                ustr_IncByte(message_string);
            }

            status_return(quick_ublock_ucs4_add(p_ss_function_argument_help->p_quick_ublock, ch));
        }

        status_return(quick_ublock_nullch_add(p_ss_function_argument_help->p_quick_ublock));

        return(PtrDiffBytesS32(message_string, message_string_in));
    }

    /* skip this whole delimited sequence */
    for(;;)
    {
        if(CH_NULL == PtrGetByte(message_string))
            break;

        ustr_IncByte(message_string);

        if(CH_VERTICAL_LINE == PtrGetByte(message_string))
        {
            ustr_IncByte(message_string);

            /* | followed by more text or CH_NULL terminates delimited sequence */
            if(CH_VERTICAL_LINE != PtrGetByte(message_string))
                break;
        }
    }

    return(PtrDiffBytesS32(message_string, message_string_in));
}

/* Attempt to look in the string for the argument type */

T5_MSG_PROTO(static, ss_function_get_argument_help, _InRef_ P_SS_FUNCTION_ARGUMENT_HELP p_ss_function_argument_help)
{
    PC_USTR message_string = resource_lookup_ustr_no_default(p_ss_function_argument_help->help_id + SS_MSG_BASE);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == message_string)
        return(STATUS_OK);

    for(;;)
    {
        U8 ch = PtrGetByte(message_string);

        if(CH_NULL == ch)
            break;

        ustr_IncByte(message_string);

        if(CH_VERTICAL_LINE == ch)
        {
            STATUS status = ss_function_get_argument_help_try(p_ss_function_argument_help, message_string);
            U32 uchars_n;

            status_return(status);

            uchars_n = (U32) status;
            ustr_IncBytes(message_string, uchars_n);
            continue;
        }

        /* other characters are ignored here */
    }

    return(STATUS_OK);
}

/* Generate a suitable help string for the given status line help string.
 * To do this we must remove all the embeded sequences from the line
 * (i.e. all argument help) and then pass the string to the status line.
 */

_Check_return_
static STATUS
ss_function_get_help_try(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_z_      PC_USTR message_string_in)
{
    PC_USTR message_string = message_string_in;

    /* found another | - just add one | to output */
    if(CH_VERTICAL_LINE == PtrGetByte(message_string))
    {
        status_return(quick_ublock_a7char_add(p_quick_ublock, CH_VERTICAL_LINE));
        return(1);
    }

    /* skip all digits */
    while(sbchar_isdigit(PtrGetByte(message_string)))
        ustr_IncByte(message_string);

    /* message_string pointing at first char after digit sequence */

    { /* add argument name (terminated by : or |) to output, then skip help string (terminated by |) */
    BOOL skip = FALSE;

    for(;;)
    {
        U8 ch = PtrGetByte(message_string);

        if(CH_NULL == ch)
            break;

        ustr_IncByte(message_string);

        if(CH_COLON == ch)
        {
            skip = TRUE; /* into skip state */
        }
        else if(CH_VERTICAL_LINE == ch)
        {
            ch = PtrGetByte(message_string);

            if(CH_VERTICAL_LINE != ch)
                break;

            ustr_IncByte(message_string);

            assert(skip); /* || is allowed in help string, not in argument name */
        }

        if(skip)
            continue;

        status_return(quick_ublock_ucs4_add(p_quick_ublock, ch));
    }
    } /*block*/

    return(PtrDiffBytesS32(message_string, message_string_in));
}

_Check_return_
static STATUS
ss_function_get_help(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InVal_     STATUS message)
{
    PC_USTR message_string = resource_lookup_ustr_no_default(message);
    U32 offset = 0;

    if(NULL == message_string)
        return(STATUS_OK);

    for(;;)
    {
        U32 bytes_of_char;
        UCS4 ucs4 = ustr_char_decode_off(message_string, offset, /*ref*/bytes_of_char);

        if(UCH_NULL == ucs4)
            break;

        offset += bytes_of_char;

        if(CH_VERTICAL_LINE == ucs4)
        {
            STATUS status = ss_function_get_help_try(p_quick_ublock, ustr_AddBytes(message_string, offset));
            U32 uchars_n;

            status_return(status);

            uchars_n = (U32) status;

            offset += uchars_n; /* skip over this delimited sequence - it may have output something */
            continue;
        }

        /* add unescaped characters to output */
        status_return(quick_ublock_ucs4_add(p_quick_ublock, ucs4));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* alert and input dialogs
*
******************************************************************************/

enum ALERT_QUERY_CONTROL_IDS
{
    ALERT_QUERY_ID_BUT_1 = IDOK,

    ALERT_QUERY_ID_TEXT_1 = 30,
    ALERT_QUERY_ID_BUT_2,
    INPUT_QUERY_INPUT
};

static /*poked*/ DIALOG_CONTROL
alert_query_text_1 =
{
    ALERT_QUERY_ID_TEXT_1, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDTEXT_V },
    { DRT(LTLT, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
alert_query_text_1_data = { { UI_TEXT_TYPE_NONE } };

static const DIALOG_CONTROL
alert_query_but_1 =
{
    ALERT_QUERY_ID_BUT_1, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, ALERT_QUERY_ID_TEXT_1, ALERT_QUERY_ID_TEXT_1 },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_PUSHBUTTON
alert_query_but_1_data = { { 0 }, { UI_TEXT_TYPE_NONE } };

static const DIALOG_CONTROL
alert_query_but_2 =
{
    ALERT_QUERY_ID_BUT_2, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, ALERT_QUERY_ID_BUT_1, ALERT_QUERY_ID_BUT_1, ALERT_QUERY_ID_BUT_1 },
    { DIALOG_CONTENTS_CALC, -DIALOG_DEFPUSHEXTRA_V, DIALOG_STDSPACING_H, -DIALOG_DEFPUSHEXTRA_V },
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_PUSHBUTTON
alert_query_but_2_data = { { 0 }, { UI_TEXT_TYPE_NONE } };

static const DIALOG_CTL_CREATE
alert_query_ctl_create[] =
{
    { &alert_query_text_1, &alert_query_text_1_data },
    { &alert_query_but_1,  &alert_query_but_1_data },
    { &alert_query_but_2,  &alert_query_but_2_data },
};

/*
routine to receive dialog events
*/

_Check_return_
static STATUS
dialog_alert_ctl_pushbutton(P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case ALERT_QUERY_ID_BUT_1:
        alert_result = ALERT_RESULT_BUT_1;
        break;

    case ALERT_QUERY_ID_BUT_2:
        alert_result = ALERT_RESULT_BUT_2;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_alert)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_COMPLETE_DBOX:
        alert_result = ALERT_RESULT_CLOSE;
        return(STATUS_OK);

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_alert_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, ss_msg_ss_alert_exec, _InoutRef_ P_SS_INPUT_EXEC p_ss_input_exec)
{
    STATUS status;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    alert_result = 0;

    status_assert(ui_text_alloc_from_ss_string(&alert_query_text_1_data.caption, p_ss_input_exec->p_ss_string_message));
    status_assert(ui_text_alloc_from_ss_string(&alert_query_but_1_data.caption,  p_ss_input_exec->p_ss_string_but_1));
    status_assert(ui_text_alloc_from_ss_string(&alert_query_but_2_data.caption,  p_ss_input_exec->p_ss_string_but_2));

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, alert_query_ctl_create, elemof32(alert_query_ctl_create), SS_MSG_DIALOG_ALERT_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = SS_MSG_DIALOG_ALERT_CAPTION;
    dialog_cmd_process_dbox.bits.modeless = 1;
    dialog_cmd_process_dbox.bits.dialog_position_type = ENUM_PACK(UBF, DIALOG_POSITION_CENTRE_WINDOW);
    dialog_cmd_process_dbox.p_proc_client = dialog_event_alert;
    if(NULL == p_ss_input_exec->p_ss_string_but_2)
        dialog_cmd_process_dbox.n_ctls -= 1;
    status = object_call_DIALOG_with_docu(p_docu_from_ev_docno(p_ss_input_exec->ev_docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    h_alert_dialog = dialog_cmd_process_dbox.modeless_h_dialog;
    } /*block*/

    ui_text_dispose(&alert_query_text_1_data.caption);
    ui_text_dispose(&alert_query_but_1_data.caption);
    ui_text_dispose(&alert_query_but_2_data.caption);

    return(status);
}

/*
re-uses ALERT controls where possible
*/

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
input_query_text_1_data = { { UI_TEXT_TYPE_NONE }, { 1 /*left_text*/, 0 /*(not) centre_text*/ } };

static const DIALOG_CONTROL
input_query_input =
{
    INPUT_QUERY_INPUT, DIALOG_CONTROL_WINDOW,

    { ALERT_QUERY_ID_TEXT_1, ALERT_QUERY_ID_TEXT_1, ALERT_QUERY_ID_TEXT_1, DIALOG_CONTROL_SELF },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDEDIT_V },

    { DRT(LBRT, EDIT), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_EDIT
input_query_input_data = { { { FRAMED_BOX_EDIT } } };

static /*poked*/ DIALOG_CONTROL
input_query_but_1 =
{
    ALERT_QUERY_ID_BUT_1, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, INPUT_QUERY_INPUT, INPUT_QUERY_INPUT },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
input_query_ctl_create[] =
{
    { &alert_query_text_1, &input_query_text_1_data },
    { &input_query_input,  &input_query_input_data },
    { &input_query_but_1,  &alert_query_but_1_data },
    { &alert_query_but_2,  &alert_query_but_2_data },
};

/******************************************************************************
*
* input dialog box
*
******************************************************************************/

_Check_return_
static STATUS
dialog_event_input_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    ui_dlg_get_edit(p_dialog_msg_ctl_pushbutton->h_dialog, INPUT_QUERY_INPUT, &ui_text_input); /*donate*/

    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case ALERT_QUERY_ID_BUT_1:
        input_result = ALERT_RESULT_BUT_1;
        break;

    case ALERT_QUERY_ID_BUT_2:
        input_result = ALERT_RESULT_BUT_2;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_input)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_COMPLETE_DBOX:
        input_result = ALERT_RESULT_CLOSE;
        return(STATUS_OK);

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_event_input_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, ss_msg_ss_input_exec, _InoutRef_ P_SS_INPUT_EXEC p_ss_input_exec)
{
    STATUS status;
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    input_result = 0;

    ui_text_input.type = UI_TEXT_TYPE_NONE;

    status_assert(ui_text_alloc_from_ss_string(&input_query_text_1_data.caption, p_ss_input_exec->p_ss_string_message));
    status_assert(ui_text_alloc_from_ss_string(&alert_query_but_1_data.caption,  p_ss_input_exec->p_ss_string_but_1));
    status_assert(ui_text_alloc_from_ss_string(&alert_query_but_2_data.caption,  p_ss_input_exec->p_ss_string_but_2));

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, input_query_ctl_create, elemof32(input_query_ctl_create), SS_MSG_DIALOG_INPUT_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = SS_MSG_DIALOG_INPUT_CAPTION;
    dialog_cmd_process_dbox.bits.modeless = 1;
    dialog_cmd_process_dbox.bits.dialog_position_type = ENUM_PACK(UBF, DIALOG_POSITION_CENTRE_WINDOW);
    dialog_cmd_process_dbox.p_proc_client = dialog_event_input;
    if(NULL == p_ss_input_exec->p_ss_string_but_2)
        dialog_cmd_process_dbox.n_ctls -= 1;
    status = object_call_DIALOG_with_docu(p_docu_from_ev_docno(p_ss_input_exec->ev_docno), DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    h_input_dialog = dialog_cmd_process_dbox.modeless_h_dialog;
    } /*block*/

    ui_text_dispose(&input_query_text_1_data.caption);
    ui_text_dispose(&alert_query_but_1_data.caption);
    ui_text_dispose(&alert_query_but_2_data.caption);

    return(status);
}

T5_MSG_PROTO(static, ss_msg_docu_supporters, _InoutRef_ P_DOCU_DEP_SUP p_docu_dep_sup)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    tree_get_supporting_docs(ev_docno_from_p_docu(p_docu), p_docu_dep_sup);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_docu_dependents, _InoutRef_ P_DOCU_DEP_SUP p_docu_dep_sup)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    tree_get_dependent_docs(ev_docno_from_p_docu(p_docu), p_docu_dep_sup);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_object_in_cell_allowed, P_OBJECT_IN_CELL_ALLOWED p_object_in_cell_allowed)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_object_in_cell_allowed->in_cell_allowed = global_preferences.ss_edit_in_cell;

    return(STATUS_OK);
}

static void
ss_choice_process(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     T5_MESSAGE t5_message)
{
    const OBJECT_ID object_id = OBJECT_ID_SS;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    if(status_ok(arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.fBool = ui_dlg_get_check(h_dialog, dialog_control_id);
        status_consume(execute_command_reperr(p_docu, t5_message, &arglist_handle, object_id));
        arglist_dispose(&arglist_handle);
    }
}

_Check_return_
static STATUS
ss_choice_save(
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     BOOL val,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    const OBJECT_ID object_id = OBJECT_ID_SS;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.fBool = val;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
ss_text_paste_to_editing_line(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR ustr)
{
    T5_MESSAGE t5_message = (OBJECT_ID_SLE == p_docu->focus_owner) ? T5_MSG_PASTE_EDITLINE : T5_MSG_ACTIVATE_EDITLINE;
    T5_PASTE_EDITLINE t5_paste_editline;
    QUICK_UBLOCK quick_ublock;
    quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr, ustrlen32(ustr));

    status_assert(object_call_id(OBJECT_ID_SLE, p_docu, T5_CMD_SELECTION_CLEAR, P_DATA_NONE));

    t5_paste_editline.p_quick_ublock = &quick_ublock;
    t5_paste_editline.select = FALSE;
    t5_paste_editline.special_select = FALSE;
    return(object_call_id(OBJECT_ID_SLE, p_docu, t5_message, &t5_paste_editline));
}

T5_CMD_PROTO(static, ss_cmd_insert_operator)
{
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    switch(t5_message)
    {
    default: default_unhandled();
    case T5_CMD_INSERT_OPERATOR_PLUS:
        return(ss_text_paste_to_editing_line(p_docu, USTR_TEXT("+")));

    case T5_CMD_INSERT_OPERATOR_MINUS:
        return(ss_text_paste_to_editing_line(p_docu, USTR_TEXT("-")));

    case T5_CMD_INSERT_OPERATOR_TIMES:
        return(ss_text_paste_to_editing_line(p_docu, USTR_TEXT("*")));

    case T5_CMD_INSERT_OPERATOR_DIVIDE:
        return(ss_text_paste_to_editing_line(p_docu, USTR_TEXT("/")));
    }
}

static void
ss_cmd_force_recalc_for(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     enum SCAN_INIT_WHAT scan_what)
{
    SCAN_BLOCK scan_block;
    BOOL modified = FALSE;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_MATRIX, scan_what, NULL, OBJECT_ID_SS)))
    {
        OBJECT_DATA object_data;

        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            P_EV_CELL p_ev_cell = object_data.u.p_ev_cell;

            if((P_DATA_NONE != p_ev_cell) && !p_ev_cell->ev_parms.data_only)
            {
                EV_SLR ev_slr;
                ev_slr_from_slr(p_docu, &ev_slr, &object_data.data_ref.arg.slr);
                ev_todo_add_slr(&ev_slr);
                modified = TRUE;
            }
        }
    }

    if(modified)
        docu_modify(p_docu);
}

T5_CMD_PROTO(static, ss_cmd_force_recalc)
{
    BOOL all = FALSE;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        P_ARGLIST_ARG p_arg;
        if(arg_present(&p_t5_cmd->arglist_handle, 0, &p_arg))
            all = TRUE;
    }

    if(all)
    {
        DOCNO docno = DOCNO_NONE;

        while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
            ss_cmd_force_recalc_for(p_docu_from_docno_valid(docno), SCAN_WHOLE);
    }
    else
        ss_cmd_force_recalc_for(p_docu, p_docu->mark_info_cells.h_markers ? SCAN_MARKERS : SCAN_WHOLE);

    ev_recalc_start(TRUE);

    return(STATUS_OK);
}

/******************************************************************************
*
* splat in a sum() expression
*
******************************************************************************/

_Check_return_
static STATUS
auto_sum_exp_add(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_at,
    _InRef_     PC_SLR p_slr_s,
    _InRef_     PC_SLR p_slr_e)
{
    STATUS status = STATUS_OK;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    for(;;)
    {
        U32 len;
        EV_SLR ev_slr;
        UCHARZ another_ustr_buf[50];

        status_break(status = quick_ublock_ustr_add(&quick_ublock, global_preferences.ss_alternate_formula_style ? USTR_TEXT("=SUM(") : USTR_TEXT("sum(")));

        ev_slr_from_slr(p_docu, &ev_slr, p_slr_s);
        len = ev_dec_slr_ustr_buf(ustr_bptr(another_ustr_buf), elemof32(another_ustr_buf), ev_slr_docno(&ev_slr), &ev_slr);
        status_break(status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(another_ustr_buf), len));

        if(global_preferences.ss_alternate_formula_style)
            status_break(status = quick_ublock_ucs4_add(&quick_ublock, UCH_COLON));

        ev_slr_from_slr(p_docu, &ev_slr, p_slr_e);
        len = ev_dec_slr_ustr_buf(ustr_bptr(another_ustr_buf), elemof32(another_ustr_buf), ev_slr_docno(&ev_slr), &ev_slr);
        status_break(status = quick_ublock_uchars_add(&quick_ublock, uchars_bptr(another_ustr_buf), len));

        status_break(status = quick_ublock_ustr_add_n(&quick_ublock, USTR_TEXT(")"), strlen_with_NULLCH));

        status = ss_object_from_text(&p_docu,
                                     p_slr_at,
                                     NULL,
                                     quick_ublock_ustr(&quick_ublock),
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     FALSE,
                                     FALSE,
                                     FALSE,
                                     FALSE);

        break;
        /*NOTREACHED*/
    }

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

/******************************************************************************
*
* convert unknown style 'names' to style
*
******************************************************************************/

static inline bool
ss_convert_unknown_style_name_fs_colour(
    _InoutRef_  P_STYLE p_style_in /* style in (updated) */,
    _In_z_      PCTSTR tstr_style_name,
    _In_z_      PCTSTR tstr_colour_name,
    _InVal_     U8 r,
    _InVal_     U8 g,
    _InVal_     U8 b)
{
    if(0 != tstrcmp(tstr_style_name, tstr_colour_name))
        return(false);

    rgb_set(&p_style_in->font_spec.colour, r, g, b);
    style_bit_set(p_style_in, STYLE_SW_FS_COLOUR);
    return(true);
}

static void
ss_convert_unknown_style_name(
    _InoutRef_  P_STYLE p_style_in /* style in (updated) */,
    _In_z_      PCTSTR tstr_style_name)
{
    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("Red"), 0xFF, 0x00, 0x00))
        return;

    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("Green"), 0x00, 0xFF, 0x00))
        return;

    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("Blue"), 0x00, 0x00, 0xFF))
        return;

    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("Cyan"), 0x00, 0xFF, 0xFF))
        return;

    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("Magenta"), 0xFF, 0x00, 0xFF))
        return;

    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("Yellow"), 0xFF, 0xFF, 0x00))
        return;

    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("Black"), 0x00, 0x00, 0x00))
        return;

    if(ss_convert_unknown_style_name_fs_colour(p_style_in, tstr_style_name, TEXT("White"), 0xFF, 0xFF, 0xFF))
        return;
}

/******************************************************************************
*
* convert a cell to its output text
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
ss_object_convert_to_output_text(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock   /* text out */ /*appended*/,
    _OutRef_    P_PIXIT p_pixit_text_width      /* width of text out */,
    _OutRef_    P_PIXIT p_pixit_text_height     /* height of text out */,
    _OutRef_    P_PIXIT p_pixit_base_line       /* base line position out */,
    _OutRef_    P_FONTY_HANDLE p_fonty_handle   /* fonty handle out */,
    _InoutRef_  P_STYLE p_style_in              /* style in (updated) */,
    P_EV_CELL p_ev_cell                     /* result in */)
{
    STATUS status = STATUS_OK;
    const EV_DOCNO ev_docno = ev_docno_from_p_docu(p_docu);
    U32 strip_nullch = 0;

    if(ev_doc_check_is_custom(ev_docno))
    {
        /* decode cell contents as it would appear in the formula line i.e. with alternate/foreign UI if wanted */
        status_return(ev_cell_decode_ui(p_quick_ublock, p_ev_cell, ev_docno));

#if RISCOS
        status_return(quick_ublock_nullch_add(p_quick_ublock));

        strip_nullch = 1;
#endif
    }
    else
    {
        NUMFORM_PARMS numform_parms;
        SS_DATA ss_data;
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock_style, 50);
        quick_tblock_with_buffer_setup(quick_tblock_style);

        ss_data_from_ev_cell(&ss_data, p_ev_cell);

        /*zero_struct(numform_parms);*/
        numform_parms.ustr_numform_numeric = array_ustr(&p_style_in->para_style.h_numform_nu);
        numform_parms.ustr_numform_datetime = array_ustr(&p_style_in->para_style.h_numform_dt);
        numform_parms.ustr_numform_texterror = array_ustr(&p_style_in->para_style.h_numform_se);
        numform_parms.p_numform_context = get_p_numform_context(p_docu);

        status = numform(p_quick_ublock, &quick_tblock_style, &ss_data, &numform_parms);

        if(status_ok(status) && (quick_tblock_chars(&quick_tblock_style) > 1) /* always CH_NULL-terminated*/)
        {   /* override pieces of supplied style with style from numform */
            PCTSTR tstr_style_name = quick_tblock_tstr(&quick_tblock_style);
            STYLE_HANDLE style_handle = style_handle_from_name(p_docu, tstr_style_name);

            if(0 != style_handle)
            {
                P_STYLE p_style = p_style_from_handle(p_docu, style_handle);

                if(NULL != p_style)
                    style_copy(p_style_in, p_style, &style_selector_all);
            }
            else
            {
                ss_convert_unknown_style_name(p_style_in, tstr_style_name);
            }
        }

        quick_tblock_dispose(&quick_tblock_style);

        status_return(status);

        /*quick_ublock_nullch_strip(p_quick_ublock);*/ /* leave terminated temporarily - helps RISC OS Font_ScanString */
        strip_nullch = 1;
    }

    if(status_ok(status = fonty_handle_from_font_spec(&p_style_in->font_spec, p_docu->flags.draft_mode)))
    {
        const PC_UCHARS uchars = quick_ublock_uchars(p_quick_ublock);
        const U32 uchars_n = quick_ublock_bytes(p_quick_ublock) - strip_nullch; /* omit CH_NULL terminator if present */
        const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;
        FONTY_CHUNK fonty_chunk;
        PIXIT leading;

        *p_fonty_handle = fonty_handle;

        fonty_chunk_info_read_uchars(
            p_docu,
            &fonty_chunk,
            fonty_handle,
            uchars,
            uchars_n,
            0 /* trail_spaces */);

        leading = style_leading_from_style(p_style_in, &p_style_in->font_spec, p_docu->flags.draft_mode);

        *p_pixit_text_width = fonty_chunk.width;
        *p_pixit_text_height = leading;
        *p_pixit_base_line = fonty_base_line(leading, p_style_in->font_spec.size_y, fonty_chunk.ascent);
    }

    if(strip_nullch)
        quick_ublock_nullch_strip(p_quick_ublock); /* now it becomes unterminated */

    return(status);
}

/******************************************************************************
*
* make an ss object given some text
*
******************************************************************************/

_Check_return_
static STATUS
ss_object_from_text(
    P_P_DOCU p_p_docu,
    _InRef_     PC_SLR p_slr,
    _In_opt_z_  PC_USTR ustr_result,
    _In_opt_z_  PC_USTR ustr_formula,
    _InRef_opt_ PC_SLR p_slr_offset,
    _InRef_opt_ PC_REGION p_region_saved,
    P_STATUS p_compile_error,
    P_S32 p_pos,
    _In_opt_z_  PCTSTR tstr_autoformat_style,
    _InVal_     BOOL try_autoformat,
    _InVal_     BOOL please_uref_overwrite,
    _InVal_     BOOL force_recalc,
    _InVal_     BOOL clip_data_from_cut_operation)
{
    COMPILER_OUTPUT compiler_output;
    STATUS status;
    EV_SLR ev_slr;
    EV_SLR ev_slr_offset;
    BOOL use_ev_slr_offset = FALSE;
    BOOL make_ss_object = 1;
    P_DOCU p_docu = *p_p_docu;
    SS_DATA ss_data_autoformat;
    STYLE_HANDLE style_handle_autoformat = STYLE_HANDLE_NONE;

    /* compiler may move document table, so calculate docnos here ... */
    ev_slr_from_slr(p_docu, &ev_slr, p_slr);

    if((NULL != p_slr_offset) && ((p_slr_offset->col || p_slr_offset->row)))
    {
        ev_slr_from_slr(p_docu, &ev_slr_offset, p_slr_offset);
        use_ev_slr_offset = TRUE;
    }

    if(NULL != p_compile_error)
        *p_compile_error = STATUS_OK;

    ss_data_set_blank(&ss_data_autoformat);

    if(try_autoformat)
    {
        if((NULL != ustr_formula) || (NULL != ustr_result))
        {
            ARRAY_HANDLE h_mrofmun;
            if(status_ok(mrofmun_get_list(p_docu, &h_mrofmun)))
                status_assert(autoformat(&ss_data_autoformat, &style_handle_autoformat, (NULL != ustr_formula) ? ustr_formula : ustr_result, &h_mrofmun));
        }
    }

    if(STYLE_HANDLE_NONE != style_handle_autoformat)
    {
        /* squirt in de-formatted constant */
        zero_struct(compiler_output);
        compiler_output.ss_data = ss_data_autoformat;
        compiler_output.ev_parms.data_only = 1;
        compiler_output.ev_parms.data_id = ss_data_get_data_id(&compiler_output.ss_data);
        compiler_output.ev_parms.style_handle_autoformat = UBF_PACK(style_handle_autoformat);
        status = STATUS_DONE;
        trace_1(TRACE_APP_SKEL,
                TEXT("autoformat found style: %s"),
                array_tstr(&(p_style_from_handle(p_docu, (STYLE_HANDLE) compiler_output.ev_parms.style_handle_autoformat)->h_style_name_tstr)));
    }
    else
    {
        /* document table may move... */
        const DOCNO docno = docno_from_p_docu(p_docu);

        if(status_fail(status = ev_compile(&compiler_output, ustr_result, ustr_formula, &ev_slr)))
        {
            /* if compilation errors are soft, send the error message to the status line,
             * and make a text cell containing the formula
             */
            if(NULL != p_compile_error)
            {
                make_ss_object = 0;
                *p_compile_error = status;
                if(NULL != p_pos)
                    *p_pos = compiler_output.chars_processed;
                status = STATUS_OK;
            }
        }

        if(tstr_autoformat_style)
        {
            STYLE_HANDLE style_handle = style_handle_from_name(p_docu, tstr_autoformat_style);

            if(0 != style_handle)
            {
                assert((U32) style_handle < 256);
                compiler_output.ev_parms.style_handle_autoformat = UBF_PACK(style_handle);
            }
        }

        p_docu = *p_p_docu = p_docu_from_docno(docno);
    }

    /* is there anything compiled ? */
    if(make_ss_object && status_ok(status))
    {
        if(please_uref_overwrite)
        {
            /* tell dependents about it */
            UREF_PARMS uref_parms;
            region_from_two_slrs(&uref_parms.source.region, p_slr, p_slr, TRUE);
            uref_event(p_docu, Uref_Msg_Overwrite, &uref_parms);
        }

        if(!status)
            status = cells_blank_make(p_docu, p_slr);
        else
        {
            P_EV_CELL p_ev_cell;

            /* make the object non-null for now */
            if(status_ok(status = object_realloc(p_docu,
                                                 (P_P_ANY) &p_ev_cell,
                                                 p_slr,
                                                 OBJECT_ID_SS,
                                                 ev_compiler_output_size(&compiler_output))))
            {
                docu_modify(p_docu);

                {
                EV_RANGE ev_range_scope;
                BOOL restrict_range;

#if 0
                restrict_range = FALSE; /* SKS 18aug95 try to allow pasted formulas how punters would like them */
#else
                /* SKS 07aug06 cut-paste leaves cell references to outside the block unmodified, copy-paste adjusts cell references */
                restrict_range = clip_data_from_cut_operation && (p_region_saved && use_ev_slr_offset);
#endif

                if(restrict_range)
                    ev_range_from_two_slrs(p_docu, &ev_range_scope, &p_region_saved->tl, &p_region_saved->br);

                if(status_ok(status = ev_cell_from_compiler_output(p_ev_cell,
                                                                   &compiler_output,
                                                                   use_ev_slr_offset ? &ev_slr_offset : NULL,
                                                                   restrict_range ? &ev_range_scope : NULL)))
                {
                    BOOL add_todo = force_recalc; /* SKS 07jul96 add ability to control forcing on recalc for wingey punters */

                    if(!compiler_output.ev_parms.data_only)
                    {
                        if(compiler_output.load_recalc)
                            add_todo = 1;

                        if(use_ev_slr_offset)
                            add_todo = 1;

                        switch(compiler_output.ev_parms.data_id)
                        {
                        case DATA_ID_BLANK:
                        case DATA_ID_ERROR:
                            add_todo = 1;
                            break;

                        default:
                            break;
                        }
                    }

                    status = ev_add_compiler_output_to_tree(&compiler_output, &ev_slr, add_todo);
                }
                } /*block*/

#if TRACE_ALLOWED
                trace_2(TRACE_MODULE_EVAL,
                        TEXT("ss_object_from_text ") S32_TFMT TEXT(" bytes, constant: ") S32_TFMT,
                        ev_compiler_output_size(&compiler_output),
                        compiler_output.ev_parms.data_only);
#endif
            }
        }
    }

    ev_compiler_output_dispose(&compiler_output);

    return(status);
}

/******************************************************************************
*
* calculate pixit size of object
*
******************************************************************************/

static void
ss_object_size(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PIXIT_POINT p_pixit_point /* object size out */,
    _InRef_     PC_SLR p_slr)
{
    STYLE style;
    STYLE_SELECTOR selector;

    style_selector_copy(&selector, &style_selector_para_leading);
    style_selector_bit_set(&selector, STYLE_SW_CS_WIDTH);
    style_selector_bit_set(&selector, STYLE_SW_PS_PARA_START);
    style_selector_bit_set(&selector, STYLE_SW_PS_PARA_END);

    /* find out size of cell */
    style_init(&style);
    style_from_slr(p_docu, &style, &selector, p_slr);

    p_pixit_point->x = style.col_style.width;
    p_pixit_point->y = style_leading_from_style(&style, &style.font_spec, p_docu->flags.draft_mode)
                       + style.para_style.para_start
                       + style.para_style.para_end;
}

static void
ss_offset_from_style(
    _OutRef_    P_PIXIT_POINT p_offset,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_PIXIT p_text_width,
    _InRef_     PC_PIXIT p_text_height,
    _InRef_     PC_SKEL_RECT p_skel_rect_object)
{
    p_offset->x = p_style->para_style.margin_left;
    p_offset->y = p_style->para_style.para_start;

    switch(p_style->para_style.justify)
    {
    default:
        break;

    case SF_JUSTIFY_CENTRE:
    case SF_JUSTIFY_RIGHT:
        {
        PIXIT format_width = p_style->col_style.width
                           - p_style->para_style.margin_right
                           - p_style->para_style.margin_left
                           - *p_text_width;

        if(SF_JUSTIFY_CENTRE == p_style->para_style.justify)
            format_width /= 2;

        if(format_width > 0)
            p_offset->x += format_width;

        break;
        }
    }

    switch(p_style->para_style.justify_v)
    {
    default:
        break;

    case SF_JUSTIFY_V_CENTRE:
    case SF_JUSTIFY_V_BOTTOM:
        {
        PIXIT vertical_space = p_skel_rect_object->br.pixit_point.y
                             - p_skel_rect_object->tl.pixit_point.y
                             - p_style->para_style.para_start
                             - p_style->para_style.para_end
                             - *p_text_height;

        if(SF_JUSTIFY_V_CENTRE == p_style->para_style.justify_v)
            vertical_space /= 2;

        if(vertical_space > 0)
            p_offset->y += vertical_space;

        break;
        }
    }
}

T5_MSG_PROTO(static, split_ev_call, _InRef_ P_EV_SPLIT_EXEC_DATA p_ev_split_exec_data)
{
    P_PROC_EXEC p_proc_exec = ss_func_table[p_ev_split_exec_data->object_table_index].p_proc_exec;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    (*p_proc_exec) (p_ev_split_exec_data->args, p_ev_split_exec_data->n_args, p_ev_split_exec_data->p_ss_data_res, p_ev_split_exec_data->p_cur_slr);

    return(STATUS_OK);
}

/******************************************************************************
*
* spreadsheet object event handler
*
******************************************************************************/

_Check_return_
static STATUS
ss_msg_startup(void)
{
#if CHECKING
    {
    const EV_DOCNO ev_docno = DOCNO_CONFIG;
    const EV_COL ev_col = 0x1234;
    const EV_ROW ev_row = 0x56789ABC;
    EV_SLR ev_slr;
    ev_slr_flags_init(&ev_slr);
    ev_slr_docno_set(&ev_slr, ev_docno);
    ev_slr_col_set(&ev_slr, ev_col);
    ev_slr_row_set(&ev_slr, ev_row);
    assert(ev_docno == ev_slr_docno(&ev_slr));
    assert(ev_col == ev_slr_col(&ev_slr));
    assert(ev_row == ev_slr_row(&ev_slr));
    } /*block*/

    assert_EQ(SS_FUNC__MAX, elemof32(ss_func_table));
    {
    U32 i;
    for(i = 1; i < elemof32(ss_func_table); ++i)
        assert(i == ss_func_table[i].index);
    } /*block*/
#endif

    status_return(resource_init(OBJECT_ID_SS, P_BOUND_MESSAGES_OBJECT_ID_SS, P_BOUND_RESOURCES_OBJECT_ID_SS));

#if WINDOWS
    {
    static const RESOURCE_BITMAP_ID ss_toolbar_common_btn_16x16_4bpp  = { OBJECT_ID_SS, SS_ID_BM_TOOLBAR_COM_BTN_ID + 0 }; /* 96 dpi buttons, 4 bpp */
    static const RESOURCE_BITMAP_ID ss_toolbar_common_btn_24x24_4bpp  = { OBJECT_ID_SS, SS_ID_BM_TOOLBAR_COM_BTN_ID + 1 }; /* 120 dpi buttons, 4 bpp */
    static const RESOURCE_BITMAP_ID ss_toolbar_common_btn_16x16_32bpp = { OBJECT_ID_SS, SS_ID_BM_TOOLBAR_COM_BTN_ID + 2 }; /* 96 dpi buttons, 32 bpp */
    static const RESOURCE_BITMAP_ID ss_toolbar_common_btn_24x24_32bpp = { OBJECT_ID_SS, SS_ID_BM_TOOLBAR_COM_BTN_ID + 3 }; /* 120 dpi buttons, 32 bpp */
    status_assert(resource_bitmap_tool_size_register(&ss_toolbar_common_btn_16x16_4bpp,  16, 16));
    status_assert(resource_bitmap_tool_size_register(&ss_toolbar_common_btn_24x24_4bpp,  24, 24));
    status_assert(resource_bitmap_tool_size_register(&ss_toolbar_common_btn_16x16_32bpp, 16, 16));
    status_assert(resource_bitmap_tool_size_register(&ss_toolbar_common_btn_24x24_32bpp, 24, 24));
    } /*block*/
#endif

    /* initialise bitmap constants */
    style_selector_copy(&style_selector_ob_ss, &style_selector_font_spec);
    void_style_selector_or(&style_selector_ob_ss, &style_selector_ob_ss, &style_selector_para_leading);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_CS_WIDTH);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_MARGIN_LEFT);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_MARGIN_RIGHT);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_NUMFORM_NU);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_NUMFORM_DT);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_NUMFORM_SE);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_JUSTIFY);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_JUSTIFY_V);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_PARA_START);
    style_selector_bit_set(&style_selector_ob_ss, STYLE_SW_PS_PARA_END);

    style_selector_clear(&style_selector_ob_ss_numform_all);
    style_selector_bit_set(&style_selector_ob_ss_numform_all, STYLE_SW_PS_NUMFORM_NU);
    style_selector_bit_set(&style_selector_ob_ss_numform_all, STYLE_SW_PS_NUMFORM_DT);
    style_selector_bit_set(&style_selector_ob_ss_numform_all, STYLE_SW_PS_NUMFORM_SE);

    trace_6(TRACE_OUT | TRACE_ANY, TEXT("ss_constant: ") U32_TFMT TEXT(", ss_data: ") U32_TFMT TEXT(", ev_slr: ") U32_TFMT TEXT(", LIST_ITEMOVH: ") S32_TFMT TEXT(", CELL_OVH: ") S32_TFMT TEXT(", OVH_EV_CELL: ") S32_TFMT,
            sizeof32(SS_CONSTANT), sizeof32(SS_DATA), sizeof32(EV_SLR), (S32) LIST_ITEMOVH, (S32) CELL_OVH, (S32) OVH_EV_CELL);

    return(register_object_construct_table(OBJECT_ID_SS, object_construct_table, FALSE /* no inlines */));
}

_Check_return_
static STATUS
ss_msg_exit1(void)
{
    ev_exit();
    ev_recalc_stop();
    return(resource_close(OBJECT_ID_SS));
}

_Check_return_
static STATUS
ss_msg_init_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    const P_SS_INSTANCE_DATA p_ss_instance = p_object_instance_data_SS(p_docu);

    zero_struct_ptr(p_ss_instance);

    p_ss_instance->ev_slr_double_click.docno = EV_DOCNO_PACK(DOCNO_NONE);

    { /* add evaluator uref dependency on whole doc */
    REGION region = REGION_INIT;
    region.whole_col = TRUE;
    region.whole_row = TRUE;
    status_return(uref_add_dependency(p_docu, &region, ob_ss_uref_event, 0, &p_ss_instance->ss_doc.uref_handle, FALSE));
    } /*block*/

    /* main event handler catches current col/row changes */
    return(maeve_event_handler_add(p_docu, maeve_event_ob_ss, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
ss_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    if(DOCNO_CONFIG != docno_from_p_docu(p_docu)) /* SKS 26apr95 ignore config document */
    {
        /* add implied style region to handle mrofmuns */
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        DOCU_AREA docu_area;
        STYLE style;

        /* set up 'scope' of implied style */
        style_init(&style);
        style.para_style.h_numform_nu = 0;
        style.para_style.h_numform_dt = 0;
        style.para_style.h_numform_se = 0;
        style_selector_copy(&style.selector, &style_selector_numform);

        STYLE_DOCU_AREA_ADD_IMPLIED(&style_docu_area_add_parm, NULL, OBJECT_ID_SS, T5_EXT_STYLE_MROFMUN, 0, REGION_LOWER);
        style_docu_area_add_parm.type = STYLE_DOCU_AREA_ADD_TYPE_IMPLIED;
        style_docu_area_add_parm.data.p_style = &style;
        style_docu_area_add_parm.internal = TRUE;
        style_docu_area_add_parm.region_class = REGION_LOWER;

        docu_area_init(&docu_area);
        docu_area.whole_col = 1;
        docu_area.whole_row = 1;

        status_return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
ss_msg_close_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_ob_ss, (CLIENT_HANDLE) 0);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    STATUS status = STATUS_OK;

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(ss_msg_startup());
        break;

    case T5_MSG_IC__STARTUP_CONFIG:
        status_return(load_object_config_file(OBJECT_ID_SS));
        break;

    case T5_MSG_IC__EXIT1:
        status = ss_msg_exit1();
        break;

    /* initialise object in new document thunk */
    case T5_MSG_IC__INIT_THUNK:
        status_return(ss_msg_init_thunk(p_docu));
        break;

    /* initialise object in new document */
    case T5_MSG_IC__INIT1:
        status_return(ss_msg_init1(p_docu));
        break;

    case T5_MSG_IC__CLOSE_THUNK:
        status = ss_msg_close_thunk(p_docu);
        break;

    default:
        break;
    }

    /* pass this sideways */
    status_accumulate(status, proc_event_ui_ss_direct(p_docu, t5_message, de_const_cast(MSG_INITCLOSE *, p_msg_initclose)));

    return(status);
}

T5_MSG_PROTO(static, ss_msg_ss_linest, _InoutRef_ P_SS_LINEST p_ss_linest)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(linest(p_ss_linest->p_proc_get, p_ss_linest->p_proc_put, p_ss_linest->client_handle, p_ss_linest->m, p_ss_linest->n));
}

T5_MSG_PROTO(static, ss_msg_click_left_double, _InoutRef_ P_OBJECT_DOUBLE_CLICK p_object_double_click)
{
    const P_EV_SLR p_ev_slr = &p_object_instance_data_SS(p_docu)->ev_slr_double_click;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(p_object_double_click->data_ref.data_space == DATA_SLOT);

    ev_slr_from_slr(p_docu, p_ev_slr, &p_object_double_click->data_ref.arg.slr);

    if(ev_event_occurred(ev_slr_docno(p_ev_slr), EV_EVENT_DOUBLECLICK))
        p_object_double_click->processed = 1;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_object_how_big, _InoutRef_ P_OBJECT_HOW_BIG p_object_how_big)
{
    PIXIT_POINT pixit_point;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(p_object_how_big->object_data.data_ref.data_space == DATA_SLOT);

    ss_object_size(p_docu, &pixit_point, &p_object_how_big->object_data.data_ref.arg.slr);

    p_object_how_big->skel_rect.br = p_object_how_big->skel_rect.tl;
    p_object_how_big->skel_rect.br.pixit_point.x += pixit_point.x;
    p_object_how_big->skel_rect.br.pixit_point.y += pixit_point.y;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_object_how_wide, _InoutRef_ P_OBJECT_HOW_WIDE p_object_how_wide)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_object_how_wide->object_data.u.p_object)
    {
        STYLE style;
        FONTY_HANDLE fonty_handle;
        PIXIT text_height, text_width, base_line;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);

        /* set up effects of which we want details */
        style_init(&style);
        style_from_slr(p_docu, &style, &style_selector_ob_ss, &p_object_how_wide->object_data.data_ref.arg.slr);

        p_object_how_wide->width = 0;

        status = ss_object_convert_to_output_text(p_docu,
                                                  &quick_ublock,
                                                  &text_width,
                                                  &text_height,
                                                  &base_line,
                                                  &fonty_handle,
                                                  &style,
                                                  p_object_how_wide->object_data.u.p_ev_cell);
        if(status_ok(status) && text_width)
            p_object_how_wide->width =
                text_width
                + style.para_style.margin_left
                + style.para_style.margin_right;

        quick_ublock_dispose(&quick_ublock);
    }

    return(STATUS_OK);
}

static void
ss_event_redraw_show_content(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw)
{
    STATUS status;
    STYLE style;
    FONTY_HANDLE fonty_handle;
    PIXIT text_height, text_width, base_line;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    assert(p_object_redraw->object_data.data_ref.data_space == DATA_SLOT);

    /* set up effects of which we want details */
    style_init(&style);
    style_from_slr(p_docu, &style, &style_selector_ob_ss, &p_object_redraw->object_data.data_ref.arg.slr);

    if(status_ok(status = ss_object_convert_to_output_text(p_docu,
                                                           &quick_ublock,
                                                           &text_width,
                                                           &text_height,
                                                           &base_line,
                                                           &fonty_handle,
                                                           &style,
                                                           p_object_redraw->object_data.u.p_ev_cell)))
    {
        const PC_REDRAW_CONTEXT p_redraw_context = &p_object_redraw->redraw_context;
        PIXIT_POINT pixit_point = p_object_redraw->skel_rect_object.tl.pixit_point;
        PIXIT_POINT offset;

        ss_offset_from_style(&offset, &style, &text_width, &text_height, &p_object_redraw->skel_rect_object);
        pixit_point.x += offset.x;
        pixit_point.y += offset.y;

        fonty_text_paint_simple_uchars(
            p_docu, p_redraw_context, &pixit_point,
            quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock),
            base_line, &p_object_redraw->rgb_back, fonty_handle);
    }

    status_assert(status);

    quick_ublock_dispose(&quick_ublock);
}

static void
ss_event_redraw_show_selection(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OBJECT_REDRAW p_object_redraw)
{
    BOOL do_invert;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(p_object_redraw->flags.show_content)
        do_invert = p_object_redraw->flags.marked_now;
    else
        do_invert = (p_object_redraw->flags.marked_now != p_object_redraw->flags.marked_screen);

    if(do_invert)
        host_invert_rectangle_filled(&p_object_redraw->redraw_context,
                                     &p_object_redraw->pixit_rect_object,
                                     &p_object_redraw->rgb_fore,
                                     &p_object_redraw->rgb_back);
}

T5_MSG_PROTO(static, ss_event_redraw, _InoutRef_ P_OBJECT_REDRAW p_object_redraw)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_object_redraw->flags.show_content && (P_DATA_NONE != p_object_redraw->object_data.u.p_object))
        ss_event_redraw_show_content(p_docu, p_object_redraw);

    if(p_object_redraw->flags.show_selection)
        ss_event_redraw_show_selection(p_docu, p_object_redraw);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_object_compare, _InoutRef_ P_OBJECT_COMPARE p_object_compare)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_object_compare->p_object_1 && p_object_compare->p_object_2)
    {
        SS_DATA ss_data_1, ss_data_2;

        ss_data_from_ev_cell(&ss_data_1, (PC_EV_CELL) p_object_compare->p_object_1);
        ss_data_from_ev_cell(&ss_data_2, (PC_EV_CELL) p_object_compare->p_object_2);

        p_object_compare->res = ss_data_compare(&ss_data_1, &ss_data_2, FALSE, FALSE);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_object_copy, _InoutRef_ P_OBJECT_COPY p_object_copy)
{
    STATUS status = STATUS_OK;
    P_EV_CELL p_ev_cell_from = p_ev_cell_object_from_slr(p_docu, &p_object_copy->slr_from);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_ev_cell_from)
    {
        S32 exp_len = ev_len(p_ev_cell_from);
        P_EV_CELL p_ev_cell_to;

        if(status_ok(status = object_realloc(p_docu, (P_P_ANY) &p_ev_cell_to, &p_object_copy->slr_to, OBJECT_ID_SS, exp_len)))
        {
            p_ev_cell_from = p_ev_cell_object_from_slr(p_docu, &p_object_copy->slr_from); /* reload */

            PTR_ASSERT(p_ev_cell_from);

            if(P_DATA_NONE != p_ev_cell_from)
            {
                EV_SLR ev_slr_offset, ev_slr_to;
                SLR slr_offset;

                ev_exp_copy(p_ev_cell_to, p_ev_cell_from);

                /* adjust refs for copy offset */
                slr_offset.col = p_object_copy->slr_to.col - p_object_copy->slr_from.col;
                slr_offset.row = p_object_copy->slr_to.row - p_object_copy->slr_from.row;
                ev_slr_from_slr(p_docu, &ev_slr_offset, &slr_offset);
                ev_exp_refs_adjust(p_ev_cell_to, &ev_slr_offset, NULL);

                /* add cell to tree */
                ev_slr_from_slr(p_docu, &ev_slr_to, &p_object_copy->slr_to);
                status = ev_add_ev_cell_to_tree(p_ev_cell_to, &ev_slr_to);
            }
        }
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_object_data_read, _InoutRef_ P_OBJECT_DATA_READ p_object_data_read)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_object_data_read->object_data.u.p_object)
    {
        const PC_EV_CELL p_ev_cell = p_object_data_read->object_data.u.p_ev_cell;

        ss_data_from_ev_cell(&p_object_data_read->ss_data, p_ev_cell);

        p_object_data_read->constant = p_ev_cell->ev_parms.data_only;
    }
    else
    {
        ss_data_set_blank(&p_object_data_read->ss_data);

        p_object_data_read->constant = 1;
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_object_read_text, _InoutRef_ P_OBJECT_READ_TEXT p_object_read_text)
{
    P_EV_CELL p_ev_cell = p_object_read_text->object_data.u.p_ev_cell;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_ev_cell)
    {
        switch(p_object_read_text->type)
        {
        default:
            return(ev_cell_decode(p_object_read_text->p_quick_ublock, p_ev_cell, ev_docno_from_p_docu(p_docu)));

        case OBJECT_READ_TEXT_EDIT:
            if(!p_ev_cell->ev_parms.data_only)
            { /* can only possibly in-cell edit constants */
                return(STATUS_FAIL);
            }

            /*FALLTHRU*/

        case OBJECT_READ_TEXT_RESULT:
            {
            FONTY_HANDLE fonty_handle;
            PIXIT text_height, text_width, base_line;
            STYLE style;

            assert(p_object_read_text->object_data.data_ref.data_space == DATA_SLOT);

            style_init(&style);
            style_from_slr(p_docu, &style, &style_selector_ob_ss, &p_object_read_text->object_data.data_ref.arg.slr);

            return(ss_object_convert_to_output_text(p_docu,
                                                    p_object_read_text->p_quick_ublock,
                                                    &text_width,
                                                    &text_height,
                                                    &base_line,
                                                    &fonty_handle,
                                                    &style,
                                                    p_object_read_text->object_data.u.p_ev_cell));
            }
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
formula_load_furtle_ext_ref(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InoutRef_  P_PC_USTR p_ustr)
{
    STATUS status = STATUS_OK;
    PC_USTR ustr = *p_ustr; /* contents, past the opening [ */
    PC_USTR ustr_right = ustrchr(ustr, CH_RIGHT_SQUARE_BRACKET);
    PC_USTR ustr_match;
    U32 n_chars;

    if(NULL == ustr_right) /* no closing ] - malformed ext. ref. - not our concern at this level */
        return(status);

    if(NULL != (ustr_match = ustrchr(ustr, CH_FULL_STOP)))
    {
        /* try stripping RISC OS user library directory prefixes */
        for(;;) /* loop for structure */
        {
            ustr_match = USTR_TEXT("Boot:Choices.Fireworkz.Library."); /* 2.00 location */
            n_chars = ustrlen32(ustr_match);

            if(0 == memcmp(ustr, ustr_match, n_chars))
            {   /* skip this prefix */
                ustr_IncBytes(ustr, n_chars);
                break;
            }

            ustr_match = USTR_TEXT("Choices:Fireworkz.Library."); /* pre-2.00 location */
            n_chars = ustrlen32(ustr_match);

            if(0 == memcmp(ustr, ustr_match, n_chars))
            {   /* skip this prefix */
                ustr_IncBytes(ustr, n_chars);
                break;
            }

            ustr_match = USTR_TEXT("!Fireworkz.User.Library."); /* pre-1.34/10 location */
            n_chars = ustrlen32(ustr_match);

            if(0 == memcmp(ustr, ustr_match, n_chars))
            {   /* skip this prefix */
                ustr_IncBytes(ustr, n_chars);
                break;
            }

            break; /* end of loop for structure */
            /*NOTREACHED*/
        }

        if(NULL != (ustr_match = ustrchr(ustr, CH_FULL_STOP)))
        {
        }
    }

    /* emit the ext. ref. contents from here to the end */
    status = quick_ublock_uchars_add(p_quick_ublock, ustr, PtrDiffBytesU32(ustr_right, ustr));

    *p_ustr = ustr_right;
    return(status);
}

_Check_return_
static STATUS
formula_load_sort_out_ext_refs(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _In_z_      PC_USTR ustr_formula)
{
    STATUS status = STATUS_OK;
    PC_USTR ustr_span_start = ustr_formula;
    PC_USTR ustr = ustr_span_start;

    for(;;)
    {
        U8 u8 = PtrGetByte(ustr);

        if(CH_NULL == u8)
        {   /* emit the remaining portion and terminate */
            U32 uchars_n = PtrDiffBytesU32(ustr, ustr_span_start) + 1 /*CH_NULL*/;
            status = quick_ublock_uchars_add(p_quick_ublock, ustr_span_start, uchars_n);
            break;
        }

        ustr_IncByte(ustr);

        if(CH_QUOTATION_MARK == u8)
        {   /* consume the string contents so we don't find ext. refs. in there */
            for(;;)
            {
                u8 = PtrGetByte(ustr);

                if(CH_NULL == u8)
                    break; /* string ended - badly terminated */

                ustr_IncByte(ustr);

                if(CH_QUOTATION_MARK == u8)
                {
                    if(CH_QUOTATION_MARK != PtrGetByteOff(ustr, 1))
                        break; /* string ended normally */

                    ustr_IncByte(ustr); /* two quotation marks */
                }
            }

            continue;
        }

        if(CH_LEFT_SQUARE_BRACKET != u8)
            continue;

        { /* emit the current portion, including the opening [ of the ext. ref. */
        U32 uchars_n = PtrDiffBytesU32(ustr, ustr_span_start);
        status_break(status = quick_ublock_uchars_add(p_quick_ublock, ustr_span_start, uchars_n));
        } /*block*/

        /* pointing at contents of an ext. ref. so furtle as necessary */
        status_break(status = formula_load_furtle_ext_ref(p_quick_ublock, &ustr));

        ustr_span_start = ustr; /* starting at the closing ] of the ext. ref. */
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_load_cell_ownform, _InoutRef_ P_LOAD_CELL_OWNFORM p_load_cell_ownform)
{
    STATUS status = STATUS_OK;
    SLR slr_offset;
    PC_USTR ustr_result = NULL;
    PC_USTR ustr_formula = NULL;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_formula, 256);
    quick_ublock_with_buffer_setup(quick_ublock_formula);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* check the data type */
    switch(p_load_cell_ownform->data_type)
    {
    default: default_unhandled();
#if CHECKING
    case OWNFORM_DATA_TYPE_TEXT:
    case OWNFORM_DATA_TYPE_DATE:
    case OWNFORM_DATA_TYPE_CONSTANT:
    case OWNFORM_DATA_TYPE_ARRAY:
    case OWNFORM_DATA_TYPE_OWNER:
#endif
        ustr_formula = (PC_USTR) p_load_cell_ownform->ustr_inline_contents;
        break;

    case OWNFORM_DATA_TYPE_FORMULA:
        ustr_result = (PC_USTR) p_load_cell_ownform->ustr_inline_contents;
        ustr_formula = p_load_cell_ownform->ustr_formula;

        /* --- code to cope with early generation Resultz files */
        if(!ustr_formula && ustr_result)
        {
            ustr_formula = ustr_result;
            ustr_result = NULL;
        }
        /* --- */

        if((NULL != ustr_formula) && (NULL != ustrchr(ustr_formula, CH_LEFT_SQUARE_BRACKET))) /* most don't have ext. refs. */
        {
            if(status_fail(status = formula_load_sort_out_ext_refs(&quick_ublock_formula, ustr_formula)))
            {
                quick_ublock_dispose(&quick_ublock_formula);
                return(status);
            }

            ustr_formula = quick_ublock_ustr(&quick_ublock_formula);
        }
        break;

    case OWNFORM_DATA_TYPE_DRAWFILE:
        return(STATUS_FAIL);
    }

    /* adjust refs for load offset */
    slr_offset.col = p_load_cell_ownform->object_data.data_ref.arg.slr.col - p_load_cell_ownform->original_slr.col;
    slr_offset.row = p_load_cell_ownform->object_data.data_ref.arg.slr.row - p_load_cell_ownform->original_slr.row;

    {
    STATUS compile_error;
    S32 pos;
    status = ss_object_from_text(&p_docu,
                                 &p_load_cell_ownform->object_data.data_ref.arg.slr,
                                 ustr_result,
                                 ustr_formula,
                                 &slr_offset,
                                 &p_load_cell_ownform->region_saved,
                                 &compile_error,
                                 &pos,
                                 p_load_cell_ownform->tstr_autoformat_style,
                                 FALSE,
                                 FALSE,
                                 global_preferences.ss_calc_on_load,
                                 p_load_cell_ownform->clip_data_from_cut_operation);

    if(status_fail(compile_error))
    {
        /* load formula as text */
        LOAD_CELL_OWNFORM load_cell_ownform = *p_load_cell_ownform;
        if(NULL != load_cell_ownform.ustr_formula)
        {
            load_cell_ownform.ustr_inline_contents = (PC_USTR_INLINE) load_cell_ownform.ustr_formula;
            load_cell_ownform.ustr_formula = NULL;
        }
        status = object_call_id(OBJECT_ID_TEXT, p_docu, T5_MSG_LOAD_CELL_OWNFORM, &load_cell_ownform);
        p_load_cell_ownform->processed = load_cell_ownform.processed;
    }
    else
        p_load_cell_ownform->processed = 1;
    } /*block*/

    quick_ublock_dispose(&quick_ublock_formula);

    return(status);
}

T5_MSG_PROTO(static, ss_msg_save_cell_ownform, _InoutRef_ P_SAVE_CELL_OWNFORM p_save_cell_ownform)
{
    STATUS status = STATUS_OK;
    const PC_EV_CELL p_ev_cell = p_save_cell_ownform->object_data.u.p_ev_cell;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_ev_cell)
    {
        const EV_DOCNO ev_docno = ev_docno_from_p_docu(p_docu);
        BOOL save_data = TRUE;

        if(p_ev_cell->ev_parms.style_handle_autoformat)
        {
            const PC_STYLE p_style = p_style_from_handle(p_docu, UBF_UNPACK(STYLE_HANDLE, p_ev_cell->ev_parms.style_handle_autoformat));

            if(p_style && style_bit_test(p_style, STYLE_SW_NAME))
                p_save_cell_ownform->tstr_autoformat_style = array_tstr(&p_style->h_style_name_tstr);
        }

        if(p_ev_cell->ev_parms.data_only)
        {
        switch(p_ev_cell->ev_parms.data_id)
        {
        case DATA_ID_REAL:
        case DATA_ID_LOGICAL:
        case DATA_ID_WORD8:
        case DATA_ID_WORD16:
        case DATA_ID_WORD32:
            p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_CONSTANT;
            break;

        case DATA_ID_STRING:
            p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_TEXT;
            break;

        case DATA_ID_ARRAY:
            p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_ARRAY;
            break;

        case DATA_ID_DATE:
            p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_DATE;
            break;

        default:
            p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_OWNER;
            break;
            }
        }
        else
        {
            if(ev_doc_check_is_custom(ev_docno)) /* SKS 09apr96 don't save data behind the formulae in custom sheets */
                save_data = FALSE;

            p_save_cell_ownform->data_type = OWNFORM_DATA_TYPE_FORMULA;
            status = ev_decompile(&p_save_cell_ownform->formula_data_quick_ublock, p_ev_cell, 0, ev_docno);
        }

        if(status_ok(status) && save_data)
        {
            SS_DATA ss_data;
            ss_data_from_ev_cell(&ss_data, p_ev_cell);
            status = ss_data_decode(&p_save_cell_ownform->contents_data_quick_ublock, &ss_data, ev_docno);
        }
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_load_cell_foreign, _InoutRef_ P_LOAD_CELL_FOREIGN p_load_cell_foreign)
{
    STATUS status = STATUS_OK;
    SLR slr_offset;
    PC_USTR ustr_result = NULL;
    PC_USTR ustr_formula = NULL;
    BOOL try_autoformat = TRUE;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* check the data type */
    switch(p_load_cell_foreign->data_type)
    {
    default: default_unhandled();
#if CHECKING
    case OWNFORM_DATA_TYPE_TEXT:
    case OWNFORM_DATA_TYPE_DATE:
    case OWNFORM_DATA_TYPE_CONSTANT:
    case OWNFORM_DATA_TYPE_ARRAY:
    case OWNFORM_DATA_TYPE_OWNER:
#endif
        ustr_formula = (PC_USTR) p_load_cell_foreign->ustr_inline_contents; /* surely we should mean result! */
        break;

    case OWNFORM_DATA_TYPE_FORMULA:
        ustr_result = (PC_USTR) p_load_cell_foreign->ustr_inline_contents;
        ustr_formula = p_load_cell_foreign->ustr_formula;
        break;

    case OWNFORM_DATA_TYPE_DRAWFILE:
        return(STATUS_FAIL);
    }

    /* if we will be setting any explicit number formatting style, don't autoformat here */
    if(style_selector_test(&p_load_cell_foreign->style.selector, &style_selector_ob_ss_numform_all))
        try_autoformat = FALSE;

    /* adjust refs for load offset */
    slr_offset.col = p_load_cell_foreign->object_data.data_ref.arg.slr.col - p_load_cell_foreign->original_slr.col;
    slr_offset.row = p_load_cell_foreign->object_data.data_ref.arg.slr.row - p_load_cell_foreign->original_slr.row;

    {
    STATUS compile_error;
    S32 pos;
    status = ss_object_from_text(&p_docu,
                                 &p_load_cell_foreign->object_data.data_ref.arg.slr,
                                 ustr_result,
                                 ustr_formula,
                                 &slr_offset,
                                 NULL,
                                 &compile_error,
                                 &pos,
                                 NULL,
                                 try_autoformat,
                                 FALSE,
                                 global_preferences.ss_calc_on_load,
                                 FALSE /*clip_data_from_cut_operation*/);

    if(status_fail(compile_error) && ((NULL != ustr_formula) || (NULL != ustr_result)))
    {
        /* try to load failed foreign formula / result as text object */
        LOAD_CELL_OWNFORM load_cell_ownform;
        zero_struct(load_cell_ownform);
        load_cell_ownform.object_data = p_load_cell_foreign->object_data;
        load_cell_ownform.original_slr = p_load_cell_foreign->original_slr;
        load_cell_ownform.data_type = OWNFORM_DATA_TYPE_TEXT;
        load_cell_ownform.ustr_inline_contents = (PC_USTR_INLINE) ((NULL != ustr_formula) ? ustr_formula : ustr_result);
        status = object_call_id(OBJECT_ID_TEXT, p_docu, T5_MSG_LOAD_CELL_OWNFORM, &load_cell_ownform);
        p_load_cell_foreign->processed = load_cell_ownform.processed;
    }
    else
    {
        p_load_cell_foreign->processed = 1;
    }
    } /*block*/

    return(status);
}

T5_MSG_PROTO(static, ss_msg_object_read_text_draft, _InoutRef_ P_OBJECT_READ_TEXT_DRAFT p_object_read_text_draft)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_object_read_text_draft->object_data.u.p_object)
    {
        FONTY_HANDLE fonty_handle;
        PIXIT text_height, text_width, base_line;
        STYLE style;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);

        assert(p_object_read_text_draft->object_data.data_ref.data_space == DATA_SLOT);

        /* set up effects of which we want details */
        style_init(&style);
        style_from_slr(p_docu, &style, &style_selector_ob_ss, &p_object_read_text_draft->object_data.data_ref.arg.slr);

        if(status_ok(status = ss_object_convert_to_output_text(p_docu,
                                                               &quick_ublock,
                                                               &text_width,
                                                               &text_height,
                                                               &base_line,
                                                               &fonty_handle,
                                                               &style,
                                                               p_object_read_text_draft->object_data.u.p_ev_cell)))
        {
            if(0 != quick_ublock_bytes(&quick_ublock))
            {
                U8 effects[PLAIN_EFFECT_COUNT];
                PIXIT_POINT offset;
                const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
                QUICK_UBLOCK_WITH_BUFFER(quick_ublock_plain, 100);
                quick_ublock_with_buffer_setup(quick_ublock_plain);

                ss_offset_from_style(&offset, &style, &text_width, &text_height, &p_object_read_text_draft->skel_rect_object);

                if(status_ok(status))
                    status = plain_text_spaces_out(&quick_ublock_plain, chars_from_pixit(offset.x, p_font_context->space_width));

                zero_array(effects);

                if(status_ok(status))
                    status = plain_text_effects_update(&quick_ublock_plain, effects, &style.font_spec);

                if(status_ok(status))
                    status = quick_ublock_uchars_add(&quick_ublock_plain, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock));

                if(status_ok(status))
                    status = plain_text_effects_off(&quick_ublock_plain, effects);

                if(status_ok(status))
                    status = plain_text_segment_out(&p_object_read_text_draft->h_plain_text, &quick_ublock_plain);

                quick_ublock_dispose(&quick_ublock_plain);
            }
        }
        quick_ublock_dispose(&quick_ublock);

        /* chuck it all away if we failed */
        if(status_fail(status))
            plain_text_dispose(&p_object_read_text_draft->h_plain_text);
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_new_object_from_text, _InoutRef_ P_NEW_OBJECT_FROM_TEXT p_new_object_from_text)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(p_new_object_from_text->data_ref.data_space == DATA_SLOT);

    {
    SKEL_POINT skel_point;
    SS_RECOG_CONTEXT ss_recog_context;

    skel_point_from_slr_tl(p_docu, &skel_point, &p_new_object_from_text->data_ref.arg.slr);

    ss_recog_context_push(&ss_recog_context);

    status = ss_object_from_text(&p_docu,
                                 &p_new_object_from_text->data_ref.arg.slr,
                                 NULL,
                                 quick_ublock_ustr(p_new_object_from_text->p_quick_ublock),
                                 NULL,
                                 NULL,
                                 &p_new_object_from_text->status,
                                 &p_new_object_from_text->pos,
                                 NULL,
                                 p_new_object_from_text->try_autoformat,
                                 p_new_object_from_text->please_uref_overwrite,
                                 FALSE,
                                 FALSE);

    ss_recog_context_pull(&ss_recog_context);

    if(status_ok(p_new_object_from_text->status))
    {
        PIXIT_POINT pixit_point;
        SKEL_RECT skel_rect_new;

        ss_object_size(p_docu, &pixit_point, &p_new_object_from_text->data_ref.arg.slr);

        skel_rect_new.tl = skel_point;
        skel_rect_new.br = skel_point;
        skel_rect_new.br.pixit_point.x += pixit_point.x;
        skel_rect_new.br.pixit_point.y += pixit_point.y;

        status = format_object_size_set(p_docu,
                                        &skel_rect_new,
                                        NULL,
                                        &p_new_object_from_text->data_ref.arg.slr,
                                        !p_new_object_from_text->please_redraw);
        docu_modify(p_docu);
    }
    else
        status = object_call_id(OBJECT_ID_TEXT, p_docu, T5_MSG_NEW_OBJECT_FROM_TEXT, p_new_object_from_text);
    } /*block*/

    return(status);
}

T5_MSG_PROTO(static, t5_ext_style_mrofmun, _InoutRef_ P_IMPLIED_STYLE_QUERY p_implied_style_query)
{
    const P_EV_CELL p_ev_cell = p_ev_cell_object_from_slr(p_docu, &p_implied_style_query->position.slr);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_ev_cell)
    {
        STYLE_HANDLE style_handle = UBF_UNPACK(STYLE_HANDLE, p_ev_cell->ev_parms.style_handle_autoformat);

        if(style_handle > 0)
        {
            P_STYLE p_style = p_style_from_handle(p_docu, style_handle);

            if(NULL != p_style)
                style_copy(p_implied_style_query->p_style, p_style, &style_selector_numform);
        }
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_ss_make_tn)
{
    STATUS status;
    const OBJECT_ID object_id = (t5_message == T5_CMD_SS_MAKE_NUMBER) ? OBJECT_ID_SS : OBJECT_ID_TEXT;
    ARGLIST_HANDLE arglist_handle;

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(status_ok(status = arglist_prepare(&arglist_handle, ss_args_s32)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.object_id = object_id;
        status = execute_command(p_docu, T5_CMD_OBJECT_CONVERT, &arglist_handle, OBJECT_ID_SKEL);
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_replicate)
{
    STATUS status = STATUS_OK;
    const BOOL down  = (T5_CMD_REPLICATE_DOWN  == t5_message);
    const BOOL right = (T5_CMD_REPLICATE_RIGHT == t5_message);
    const BOOL up    = (T5_CMD_REPLICATE_UP    == t5_message);
  /*const BOOL left  = (T5_CMD_REPLICATE_LEFT  == t5_message);*/

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        SLR slr_to;
        SLR slr_from_s;
        SLR slr_from_e;
        COL n_cols;
        ROW n_rows;
        PROCESS_STATUS process_status;

        process_status_init(&process_status);
        process_status.flags.foreground = TRUE;
        process_status.reason.type = UI_TEXT_TYPE_RESID;
        process_status.reason.text.resource_id = MSG_STATUS_PASTING; /* kind of */
        process_status_begin(p_docu, &process_status, PROCESS_STATUS_PERCENT);

        docu_area_normalise(p_docu, &docu_area, p_docu_area_from_markers_first(p_docu));
        slr_from_s = docu_area.tl.slr;
        slr_from_e = docu_area.br.slr;
        if(down)
            slr_from_e.row = slr_from_s.row + 1;
        else if(right)
            slr_from_e.col = slr_from_s.col + 1;
        else if(up)
            slr_from_s.row = slr_from_e.row - 1;
        else /*if(left)*/
            slr_from_s.col = slr_from_e.col - 1;

        n_cols = docu_area.br.slr.col - docu_area.tl.slr.col;
        n_rows = docu_area.br.slr.row - docu_area.tl.slr.row;

        { /* tell dependents about overwrite */
        UREF_PARMS uref_parms;
        region_from_docu_area_max(&uref_parms.source.region, &docu_area);
        if(down)
            uref_parms.source.region.tl.row += 1;
        else if(right)
            uref_parms.source.region.tl.col += 1;
        else if(up)
            uref_parms.source.region.br.row -= 1;
        else /*if(left)*/
            uref_parms.source.region.br.col -= 1;
        uref_event(p_docu, Uref_Msg_Overwrite, &uref_parms);
        } /*block*/

        slr_to = slr_from_s;

        if(down)
            slr_to.row += 1;
        else if(right)
            slr_to.col += 1;
        else if(up)
            slr_to.row -= 1;
        else /*if(left)*/
            slr_to.col -= 1;

        for(;;)
        {
            if(down)
            {
                if(slr_to.row >= docu_area.br.slr.row)
                    break;
            }
            else if(right)
            {
                if(slr_to.col >= docu_area.br.slr.col)
                    break;
            }
            else if(up)
            {
                if(slr_to.row < docu_area.tl.slr.row)
                    break;
            }
            else /*if(left)*/
            {
                if(slr_to.col < docu_area.tl.slr.col)
                    break;
            }

            status_break(status = cells_block_copy_no_uref(p_docu, &slr_to, &slr_from_s, &slr_from_e));

            if(down)
            {
                slr_to.row += 1;
                process_status.data.percent.current = (slr_to.row - slr_from_s.row) * 100 / n_rows;
            }
            else if(right)
            {
                slr_to.col += 1;
                process_status.data.percent.current = (slr_to.col - slr_from_s.col) * 100 / n_cols;
            }
            else if(up)
            {
                slr_to.row -= 1;
                process_status.data.percent.current = (slr_from_e.row - slr_to.row) * 100 / n_rows;
            }
            else /*if(left)*/
            {
                slr_to.col -= 1;
                process_status.data.percent.current = (slr_from_e.col - slr_to.col) * 100 / n_cols;
            }

            process_status_reflect(&process_status);
        }

        process_status_end(&process_status);

        reformat_from_row(p_docu, docu_area.tl.slr.row, REFORMAT_Y);
    }

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_auto_sum)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        ROW row_format = MAX_ROW;
        U8 rhs_ok = 0, bot_ok = 0;

        cur_change_before(p_docu);

        docu_area_normalise(p_docu, &docu_area, p_docu_area_from_markers_first(p_docu));

        /* the column of sums */
        if( (docu_area.br.slr.col > docu_area.tl.slr.col + 1)
            &&
            cells_block_is_blank(p_docu,
                                docu_area.br.slr.col - 1,
                                docu_area.br.slr.col,
                                docu_area.tl.slr.row,
                                docu_area.br.slr.row - docu_area.tl.slr.row))
            rhs_ok = 1;

        /* the row of sums */
        if( (docu_area.br.slr.row > docu_area.tl.slr.row + 1)
            &&
            cells_block_is_blank(p_docu,
                                docu_area.tl.slr.col,
                                docu_area.br.slr.col,
                                docu_area.br.slr.row - 1,
                                1))
            bot_ok = 1;

        if(status_ok(status) && rhs_ok)
        {
            ROW last_row = bot_ok ? (docu_area.br.slr.row - 1) : docu_area.br.slr.row;

            {
            CHECK_PROTECTION check_protection;
            check_protection.docu_area = docu_area;
            check_protection.docu_area.tl.slr.col = docu_area.br.slr.col - 1;
            check_protection.docu_area.br.slr.row = last_row;
            check_protection.status_line_message = FALSE;
            check_protection.use_docu_area = TRUE;
            status = object_skel(p_docu, T5_MSG_CHECK_PROTECTION, &check_protection);
            } /*block*/

            if(status_ok(status))
            {
                SLR slr;

                row_format = MIN(row_format, docu_area.tl.slr.row);

                slr.col = docu_area.br.slr.col - 1;

                for(slr.row = docu_area.tl.slr.row; (slr.row < last_row); slr.row += 1)
                {
                    SLR slr_s, slr_e;

                    slr_s.col = docu_area.tl.slr.col;
                    slr_s.row = slr.row;

                    slr_e.col = docu_area.br.slr.col - 2;
                    slr_e.row = slr.row;

                    status_break(status = auto_sum_exp_add(p_docu, &slr, &slr_s, &slr_e));
                }
            }
        }

        if(status_ok(status) && bot_ok)
        {
            COL last_col = rhs_ok ? (docu_area.br.slr.col - 1) : docu_area.br.slr.col;

            {
            CHECK_PROTECTION check_protection;
            check_protection.docu_area = docu_area;
            check_protection.docu_area.br.slr.col = last_col;
            check_protection.docu_area.tl.slr.row = docu_area.br.slr.row - 1;
            check_protection.status_line_message = FALSE;
            check_protection.use_docu_area = TRUE;
            status = object_skel(p_docu, T5_MSG_CHECK_PROTECTION, &check_protection);
            } /*block*/

            if(status_ok(status))
            {
                SLR slr;

                row_format = MIN(row_format, docu_area.br.slr.row - 1);

                slr.row = docu_area.br.slr.row - 1;

                for(slr.col = docu_area.tl.slr.col; slr.col < last_col; slr.col += 1)
                {
                    SLR slr_s, slr_e;

                    slr_s.col = slr.col;
                    slr_s.row = docu_area.tl.slr.row;

                    slr_e.col = slr.col;
                    slr_e.row = docu_area.br.slr.row - 2;

                    status_break(status = auto_sum_exp_add(p_docu, &slr, &slr_s, &slr_e));
                }
            }
        }

        if(row_format < MAX_ROW)
            reformat_from_row(p_docu, row_format, REFORMAT_Y);

        if(!bot_ok && !rhs_ok)
            status = create_error(SS_ERR_AREA_NOT_BLANK);
    }
    else
        status = ss_function_paste_to_editing_line(p_docu, EV_RESO_MATHS, RPN_FNV_SUM);

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_new_expression)
{
    STATUS status = STATUS_OK;
    PC_USTR ustr = ustr_empty_string;
    U32 wss;
    NEW_OBJECT_FROM_TEXT new_object_from_text;
    QUICK_UBLOCK quick_ublock;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 != n_arglist_args(&p_t5_cmd->arglist_handle))
    {
        P_ARGLIST_ARG p_arg;
        if(arg_present(&p_t5_cmd->arglist_handle, 0, &p_arg))
            ustr = p_arg->val.ustr;
    }

    wss = ss_string_skip_leading_whitespace_uchars(ustr, ustrlen32p1(ustr));

    quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr, ustrlen32p1(ustr));

    data_ref_from_slr(&new_object_from_text.data_ref, &p_docu->cur.slr);
    new_object_from_text.p_quick_ublock = &quick_ublock;
    new_object_from_text.status = STATUS_OK;
    new_object_from_text.please_redraw = TRUE;
    new_object_from_text.please_uref_overwrite = TRUE;
#if 1
    new_object_from_text.try_autoformat = ('=' != PtrGetByteOff(ustr, wss)); /* don't bother if definitely a formula; NB preserve leading whitespace for formula esp. custom */
#else
    new_object_from_text.try_autoformat = FALSE; /* SKS 17nov95 confirms */
#endif

    style_docu_area_uref_hold(p_docu, &p_docu->h_style_docu_area, &p_docu->cur.slr);

    {
    const DOCNO docno = docno_from_p_docu(p_docu);
    status = object_ss(p_docu, T5_MSG_NEW_OBJECT_FROM_TEXT, &new_object_from_text);
    p_docu = p_docu_from_docno(docno);
    } /*block*/

    style_docu_area_uref_release(p_docu, &p_docu->h_style_docu_area, &p_docu->cur.slr, OBJECT_ID_NONE);

    if(status_ok(status) && status_fail(new_object_from_text.status))
    {
        UI_TEXT ui_text;
        ui_text.type = UI_TEXT_TYPE_RESID;
        ui_text.text.resource_id = new_object_from_text.status;
        status_line_set(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, &ui_text);
        p_docu->cur.object_position.object_id = OBJECT_ID_TEXT;
        p_docu->cur.object_position.data = new_object_from_text.pos;
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_object_snapshot, _InRef_ P_OBJECT_SNAPSHOT p_object_snapshot)
{
    STATUS status = STATUS_OK;
    const P_EV_CELL p_ev_cell = p_object_snapshot->object_data.u.p_ev_cell;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(P_DATA_NONE != p_ev_cell)
    {
        if(!p_ev_cell->ev_parms.data_only && (DATA_ID_ERROR != p_ev_cell->ev_parms.data_id))
        {
            SS_DATA ss_data, ss_data_copy;
            ss_data_from_ev_cell(&ss_data, p_ev_cell);
            status_assert(ss_data_resource_copy(&ss_data_copy, &ss_data));

            /* tell dependents about it */
            if(p_object_snapshot->do_uref_overwrite)
            {
                UREF_PARMS uref_parms;
                region_from_two_slrs(&uref_parms.source.region,
                                     &p_object_snapshot->object_data.data_ref.arg.slr,
                                     &p_object_snapshot->object_data.data_ref.arg.slr,
                                     TRUE);
                uref_event(p_docu, Uref_Msg_Overwrite, &uref_parms);
            }

            if(status_ok(status = object_realloc(p_docu, (P_P_ANY) &p_ev_cell, &p_object_snapshot->object_data.data_ref.arg.slr, OBJECT_ID_SS, OVH_EV_CELL)))
            {
                zero_struct(p_ev_cell->ev_parms);
                p_ev_cell->ev_parms.data_only = 1;

                ev_cell_constant_from_data(p_ev_cell, &ss_data_copy);
            }
        }
    }

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_ss_name)
{
    STATUS status = STATUS_OK;
    const P_OF_IP_FORMAT p_of_ip_format = p_t5_cmd->p_of_ip_format;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    PC_USTR ustr_name_id = p_args[0].val.ustr;
    PC_USTR ustr_name_def = p_args[1].val.ustr;
    PC_USTR ustr_description = p_args[2].val.ustr;
    const EV_DOCNO ev_docno = ev_docno_from_p_docu(p_docu);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    PTR_ASSERT(p_of_ip_format);

    if( !p_of_ip_format->flags.insert ||
        (p_of_ip_format->flags.is_template ||
        (find_name_in_list(ev_docno, ustr_name_id) < 0) ) )
    {
        docu_modify(p_docu);

        status = ev_name_make(ustr_name_id, ev_docno, ustr_name_def, 0, ustr_description);
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_ss_name_make, _InRef_ P_SS_NAME_MAKE p_ss_name_make)
{
    STATUS status = STATUS_OK;
    PC_USTR ustr_name_id = p_ss_name_make->ustr_name_id;
    PC_USTR ustr_name_def = p_ss_name_make->ustr_name_def;
    PC_USTR ustr_description = p_ss_name_make->ustr_description;
    DOCNO docno;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_ok(status = docno_from_id(p_docu, &docno, ustr_name_id, FALSE /* ensure */)))
    {
        const U32 doc_prefix_len = (U32) status;

        ustr_name_id = ustr_AddBytes(ustr_name_id, doc_prefix_len);

        docu_modify(p_docu);

        status = ev_name_make(ustr_name_id, (EV_DOCNO) docno, ustr_name_def, p_ss_name_make->undefine, ustr_description);
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_ss_name_ensure, _InoutRef_ P_SS_NAME_ENSURE p_ss_name_ensure)
{
    STATUS status = STATUS_OK;
    PC_USTR ustr_name_id = p_ss_name_ensure->ustr_name_id;
    DOCNO docno;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_ok(status = docno_from_id(p_docu, &docno, ustr_name_id, TRUE /* ensure */)))
    {
        if(docno != DOCNO_NONE)
        {
            const U32 doc_prefix_len = (U32) status;
            ARRAY_INDEX name_num;

            ustr_name_id = ustr_AddBytes(ustr_name_id, doc_prefix_len);

            if((name_num = ensure_name_in_list((EV_DOCNO) docno, ustr_name_id)) >= 0)
            {
                P_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);
                p_ss_name_ensure->ev_handle = p_ev_name->handle;
            }
            else
                status = name_num;
        }
    }

    return(status);
}

T5_MSG_PROTO(static, ss_msg_ss_name_id_from_handle, _InRef_ P_SS_NAME_ID_FROM_HANDLE p_ss_name_id_from_handle) /* p_quick_ublock appended */
{
    STATUS status = STATUS_OK;
    const ARRAY_INDEX name_num = name_def_from_handle(p_ss_name_id_from_handle->ev_handle);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(name_num >= 0)
    {
        const PC_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);

        if(ev_slr_docno(&p_ev_name->owner) != p_ss_name_id_from_handle->docno)
        {
            UCHARZ ustr_buf[BUF_MAX_PATHSTRING];
            (void) ev_write_docname_ustr_buf(ustr_bptr(ustr_buf), MAX_PATHSTRING, ev_slr_docno(&p_ev_name->owner), p_ss_name_id_from_handle->docno);
            status = quick_ublock_ustr_add(p_ss_name_id_from_handle->p_quick_ublock, ustr_bptr(ustr_buf));
        }

        if(status_ok(status))
            status = quick_ublock_ustr_add_n(p_ss_name_id_from_handle->p_quick_ublock, ustr_bptrc(p_ev_name->ustr_name_id), strlen_with_NULLCH);
    }
    else
        status = create_error(EVAL_ERR_NAMEUNDEF);

    return(status);
}

T5_MSG_PROTO(static, ss_msg_ss_name_read, _InoutRef_ P_SS_NAME_READ p_ss_name_read)
{
    STATUS status = STATUS_OK;
    const ARRAY_INDEX name_num = name_def_from_handle(p_ss_name_read->ev_handle);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    ss_data_set_blank(&p_ss_name_read->ss_data);

    if(name_num >= 0)
    {
        const PC_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);

        if(p_ev_name->flags.undefined)
            status = create_error(EVAL_ERR_NAMEUNDEF);
        else
        {
            PC_SS_DATA p_ss_data = &p_ev_name->def_data;
            SS_DATA ss_data;

            ss_data_set_blank(&ss_data);

            if(p_ss_name_read->follow_indirection)
            {
                switch(ss_data_get_data_id(&p_ev_name->def_data))
                {
                default:
                    break;

                case DATA_ID_SLR:
                    ev_slr_deref(&ss_data, &p_ev_name->def_data.arg.slr);
                    p_ss_data = &ss_data;
                    break;

                case DATA_ID_RANGE:
                    ev_slr_deref(&ss_data, &p_ev_name->def_data.arg.range.s);
                    p_ss_data = &ss_data;
                    break;
                }
            }

            status = ss_data_resource_copy(&p_ss_name_read->ss_data, p_ss_data);

            ss_data_free_resources(&ss_data);

            p_ss_name_read->ustr_description = p_ev_name->ustr_description; /* NB loan to caller */
        }
    }
    else
        status = create_error(EVAL_ERR_NAMEUNDEF);

    if(status_fail(status))
        return(ss_data_set_error(&p_ss_name_read->ss_data, status));

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_choices_query, _InoutRef_ P_CHOICES_QUERY_BLOCK p_choices_query_block)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    choices_ss_calc_auto_data.init_state                = (U8) !global_preferences.ss_calc_manual;
    choices_ss_calc_background_data.init_state          = (U8) !global_preferences.ss_calc_foreground;
    choices_ss_calc_on_load_data.init_state             = (U8)  global_preferences.ss_calc_on_load;
    choices_ss_calc_additional_rounding_data.init_state = (U8)  global_preferences.ss_calc_additional_rounding;
    choices_ss_edit_in_cell_data.init_state             = (U8)  global_preferences.ss_edit_in_cell;
    choices_ss_alternate_formula_style_data.init_state  = (U8)  global_preferences.ss_alternate_formula_style;

    /* saves having ob_charb */
    choices_chart_update_auto_data.init_state           = (U8) !global_preferences.chart_update_manual;

    choices_ss_group.relative_dialog_control_id[0] = p_choices_query_block->tr_dialog_control_id;
    choices_ss_group.relative_dialog_control_id[1] = p_choices_query_block->tr_dialog_control_id;

    p_choices_query_block->tr_dialog_control_id = CHOICES_SS_ID_GROUP;
    p_choices_query_block->br_dialog_control_id = CHOICES_CHART_ID_GROUP;

    return(al_array_add(&p_choices_query_block->ctl_create, DIALOG_CTL_CREATE, elemof32(choices_ss_ctl_create), PC_ARRAY_INIT_BLOCK_NONE, choices_ss_ctl_create));
}

T5_MSG_PROTO(static, ss_msg_choices_set, _InRef_ P_CHOICES_SET_BLOCK p_choices_set_block)
{
    const H_DIALOG h_dialog = p_choices_set_block->h_dialog;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    ss_choice_process(p_docu, h_dialog, CHOICES_SS_ID_CALC_AUTO,                T5_CMD_CHOICES_SS_CALC_AUTO);
    ss_choice_process(p_docu, h_dialog, CHOICES_SS_ID_CALC_BACKGROUND,          T5_CMD_CHOICES_SS_CALC_BACKGROUND);
    ss_choice_process(p_docu, h_dialog, CHOICES_SS_ID_CALC_ON_LOAD,             T5_CMD_CHOICES_SS_CALC_ON_LOAD);
    ss_choice_process(p_docu, h_dialog, CHOICES_SS_ID_CALC_ADDITIONAL_ROUNDING, T5_CMD_CHOICES_SS_CALC_ADDITIONAL_ROUNDING);
    ss_choice_process(p_docu, h_dialog, CHOICES_SS_ID_EDIT_IN_CELL,             T5_CMD_CHOICES_SS_EDIT_IN_CELL);
    ss_choice_process(p_docu, h_dialog, CHOICES_SS_ID_ALTERNATE_FORMULA_STYLE,  T5_CMD_CHOICES_SS_ALTERNATE_FORMULA_STYLE);

    /* saves having ob_charb */
    ss_choice_process(p_docu, h_dialog, CHOICES_CHART_ID_UPDATE_AUTO,           T5_CMD_CHOICES_CHART_UPDATE_AUTO);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, ss_msg_choices_save, _InoutRef_ P_OF_OP_FORMAT p_of_op_format)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    status_return(ss_choice_save(T5_CMD_CHOICES_SS_CALC_AUTO,              !global_preferences.ss_calc_manual,              p_of_op_format));
    status_return(ss_choice_save(T5_CMD_CHOICES_SS_CALC_BACKGROUND,        !global_preferences.ss_calc_foreground,          p_of_op_format));
    status_return(ss_choice_save(T5_CMD_CHOICES_SS_CALC_ON_LOAD,            global_preferences.ss_calc_on_load,             p_of_op_format));
    status_return(ss_choice_save(T5_CMD_CHOICES_SS_CALC_ADDITIONAL_ROUNDING,global_preferences.ss_calc_additional_rounding, p_of_op_format));
    status_return(ss_choice_save(T5_CMD_CHOICES_SS_EDIT_IN_CELL,            global_preferences.ss_edit_in_cell,             p_of_op_format));
    status_return(ss_choice_save(T5_CMD_CHOICES_SS_ALTERNATE_FORMULA_STYLE, global_preferences.ss_alternate_formula_style,  p_of_op_format));

    /* saves having ob_charb */
           return(ss_choice_save(T5_CMD_CHOICES_CHART_UPDATE_AUTO,          !global_preferences.chart_update_manual,        p_of_op_format));
}

T5_CMD_PROTO(static, ss_choices_ss_calc_auto)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(global_preferences.ss_calc_manual != !(p_args[0].val.fBool))
    {
        global_preferences.ss_calc_manual = !(p_args[0].val.fBool);
        if(!global_preferences.ss_calc_manual)
            ev_recalc_start(TRUE);
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, ss_choices_ss_calc_background)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(global_preferences.ss_calc_foreground != !(p_args[0].val.fBool))
    {
        global_preferences.ss_calc_foreground = !(p_args[0].val.fBool);
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, ss_choices_ss_calc_on_load)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(global_preferences.ss_calc_on_load != p_args[0].val.fBool)
    {
        global_preferences.ss_calc_on_load = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, ss_choices_ss_calc_additional_rounding)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(global_preferences.ss_calc_additional_rounding != p_args[0].val.fBool)
    {
        global_preferences.ss_calc_additional_rounding = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, ss_choices_ss_edit_in_cell)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(global_preferences.ss_edit_in_cell != p_args[0].val.fBool)
    {
        global_preferences.ss_edit_in_cell = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, ss_choices_ss_alternate_formula_style)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(global_preferences.ss_alternate_formula_style != p_args[0].val.fBool)
    {
        global_preferences.ss_alternate_formula_style = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, ss_choices_chart_update_auto)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(global_preferences.chart_update_manual != !(p_args[0].val.fBool))
    {
        global_preferences.chart_update_manual = !(p_args[0].val.fBool);
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, object_ss_cmd)
{
    assert(T5_CMD__ACTUAL_END <= T5_CMD__END);

    switch(T5_MESSAGE_CMD_OFFSET(t5_message))
    {
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_UPPER):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_LOWER):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_INICAP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_SWAP):

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_DATE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_FILE_DATE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_PAGE_X):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_PAGE_Y):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_SS_NAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_MS_FIELD):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_WHOLENAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_LEAFNAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_RETURN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_SOFT_HYPHEN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_TAB):
        return(STATUS_FAIL);

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SS_MAKE_TEXT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SS_MAKE_NUMBER):
        return(t5_cmd_ss_make_tn(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_REPLICATE_DOWN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_REPLICATE_RIGHT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_REPLICATE_UP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_REPLICATE_LEFT):
        return(t5_cmd_replicate(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_AUTO_SUM):
        return(t5_cmd_auto_sum(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_OPERATOR_PLUS):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_OPERATOR_MINUS):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_OPERATOR_TIMES):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_OPERATOR_DIVIDE):
        return(ss_cmd_insert_operator(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SS_FUNCTIONS):
        return(t5_cmd_ss_functions(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SS_NAME_INTRO):
        return(t5_cmd_ss_name_intro(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_FORCE_RECALC):
        return(ss_cmd_force_recalc(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_NEW_EXPRESSION):
        return(t5_cmd_new_expression(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SS_NAME):
        return(t5_cmd_ss_name(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CHOICES_SS_CALC_AUTO):
        return(ss_choices_ss_calc_auto(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CHOICES_SS_CALC_BACKGROUND):
        return(ss_choices_ss_calc_background(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CHOICES_SS_CALC_ON_LOAD):
        return(ss_choices_ss_calc_on_load(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CHOICES_SS_CALC_ADDITIONAL_ROUNDING):
        return(ss_choices_ss_calc_additional_rounding(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CHOICES_SS_EDIT_IN_CELL):
        return(ss_choices_ss_edit_in_cell(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CHOICES_SS_ALTERNATE_FORMULA_STYLE):
        return(ss_choices_ss_alternate_formula_style(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CHOICES_CHART_UPDATE_AUTO):
        return(ss_choices_chart_update_auto(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ACTIVATE_MENU_FUNCTION_SELECTOR):
        return(t5_cmd_activate_menu(p_docu, t5_message));

#if CHECKING
    case T5_CMD_ESCAPE:
        return(STATUS_OK);

    case T5_CMD_RETURN:
        assert0();
        return(STATUS_OK);
#endif

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_ss);
OBJECT_PROTO(extern, object_ss)
{
    if(T5_MESSAGE_IS_CMD(t5_message))
        return(object_ss_cmd(p_docu, t5_message, (PC_T5_CMD) p_data));

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(ss_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

#if CHECKING
    case T5_EVENT_KEYS:
        assert0();
        return(STATUS_OK);
#endif

    case T5_EVENT_REDRAW:
        return(ss_event_redraw(p_docu, t5_message, (P_OBJECT_REDRAW) p_data));

    case T5_MSG_SS_LINEST:
        return(ss_msg_ss_linest(p_docu, t5_message, (P_SS_LINEST) p_data));

    case T5_MSG_SS_FUNCTION_ARGUMENT_HELP:
        return(ss_function_get_argument_help(p_docu, t5_message, (P_SS_FUNCTION_ARGUMENT_HELP) p_data));

    case T5_MSG_SS_ALERT_EXEC:
        return(ss_msg_ss_alert_exec(p_docu, t5_message, (P_SS_INPUT_EXEC) p_data));

    case T5_MSG_SS_INPUT_EXEC:
        return(ss_msg_ss_input_exec(p_docu, t5_message, (P_SS_INPUT_EXEC) p_data));

    case T5_MSG_DOCU_SUPPORTERS:
        return(ss_msg_docu_supporters(p_docu, t5_message, (P_DOCU_DEP_SUP) p_data));

    case T5_MSG_DOCU_DEPENDENTS:
        return(ss_msg_docu_dependents(p_docu, t5_message, (P_DOCU_DEP_SUP) p_data));

    case T5_MSG_CLICK_LEFT_DOUBLE:
        return(ss_msg_click_left_double(p_docu, t5_message, (P_OBJECT_DOUBLE_CLICK) p_data));

    case T5_MSG_OBJECT_IN_CELL_ALLOWED:
        return(ss_msg_object_in_cell_allowed(p_docu, t5_message, (P_OBJECT_IN_CELL_ALLOWED) p_data));

    case T5_MSG_OBJECT_HOW_BIG:
        return(ss_msg_object_how_big(p_docu, t5_message, (P_OBJECT_HOW_BIG) p_data)); 

    case T5_MSG_OBJECT_HOW_WIDE:
        return(ss_msg_object_how_wide(p_docu, t5_message, (P_OBJECT_HOW_WIDE) p_data));

    case T5_EXT_STYLE_MROFMUN:
        return(t5_ext_style_mrofmun(p_docu, t5_message, (P_IMPLIED_STYLE_QUERY) p_data));

    /* turn lots of keys pressed message into insert_sub message; */
    case T5_MSG_OBJECT_KEYS:
        return(object_call_id(OBJECT_ID_SLE, p_docu, t5_message, p_data));

    case T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT:
    case T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION:
    case T5_MSG_OBJECT_POSITION_SET:
    case T5_MSG_OBJECT_LOGICAL_MOVE:
    case T5_MSG_OBJECT_DELETE_SUB:
    case T5_MSG_OBJECT_STRING_SEARCH:
    case T5_MSG_OBJECT_STRING_REPLACE:
        return(STATUS_FAIL);

    case T5_MSG_OBJECT_COMPARE:
        return(ss_msg_object_compare(p_docu, t5_message, (P_OBJECT_COMPARE) p_data));

    case T5_MSG_OBJECT_COPY:
        return(ss_msg_object_copy(p_docu, t5_message, (P_OBJECT_COPY) p_data));

    case T5_MSG_OBJECT_DATA_READ:
        return(ss_msg_object_data_read(p_docu, t5_message, (P_OBJECT_DATA_READ) p_data));

    case T5_MSG_OBJECT_READ_TEXT:
        return(ss_msg_object_read_text(p_docu, t5_message, (P_OBJECT_READ_TEXT) p_data));

    case T5_MSG_LOAD_FRAG_OWNFORM:
        return(STATUS_FAIL); /* reject here, let in-cell editing work it out */

    case T5_MSG_LOAD_CELL_OWNFORM:
        return(ss_msg_load_cell_ownform(p_docu, t5_message, (P_LOAD_CELL_OWNFORM) p_data));

    case T5_MSG_SAVE_CELL_OWNFORM:
        return(ss_msg_save_cell_ownform(p_docu, t5_message, (P_SAVE_CELL_OWNFORM) p_data));

    case T5_MSG_LOAD_CELL_FOREIGN:
        return(ss_msg_load_cell_foreign(p_docu, t5_message, (P_LOAD_CELL_FOREIGN) p_data));

    case T5_MSG_OBJECT_READ_TEXT_DRAFT:
        return(ss_msg_object_read_text_draft(p_docu, t5_message, (P_OBJECT_READ_TEXT_DRAFT) p_data));

    case T5_MSG_NEW_OBJECT_FROM_TEXT:
        return(ss_msg_new_object_from_text(p_docu, t5_message, (P_NEW_OBJECT_FROM_TEXT) p_data));

    case T5_MSG_OBJECT_SNAPSHOT:
        return(ss_msg_object_snapshot(p_docu, t5_message, (P_OBJECT_SNAPSHOT) p_data));

    case T5_MSG_SS_NAME_MAKE:
        return(ss_msg_ss_name_make(p_docu, t5_message, (P_SS_NAME_MAKE) p_data));

    case T5_MSG_SS_NAME_ENSURE:
        return(ss_msg_ss_name_ensure(p_docu, t5_message, (P_SS_NAME_ENSURE) p_data));

    case T5_MSG_SS_NAME_ID_FROM_HANDLE:
        return(ss_msg_ss_name_id_from_handle(p_docu, t5_message, (P_SS_NAME_ID_FROM_HANDLE) p_data));

    case T5_MSG_SS_NAME_READ:
        return(ss_msg_ss_name_read(p_docu, t5_message, (P_SS_NAME_READ) p_data));

    case T5_MSG_CHOICES_QUERY:
        return(ss_msg_choices_query(p_docu, t5_message, (P_CHOICES_QUERY_BLOCK) p_data));

    case T5_MSG_CHOICES_SET:
        return(ss_msg_choices_set(p_docu, t5_message, (P_CHOICES_SET_BLOCK) p_data));

    case T5_MSG_CHOICES_SAVE:
        return(ss_msg_choices_save(p_docu, t5_message, (P_OF_OP_FORMAT) p_data));

    case T5_MSG_SS_RPN_EXEC:
        return(split_ev_call(p_docu, t5_message, (P_EV_SPLIT_EXEC_DATA) p_data));

    case T5_MSG_TOOLBAR_TOOL_USER_VIEW_NEW:
    case T5_MSG_TOOLBAR_TOOL_USER_VIEW_DELETE:
    case T5_MSG_TOOLBAR_TOOL_USER_SIZE_QUERY:
    case T5_MSG_TOOLBAR_TOOL_USER_POSN_SET:
    case T5_MSG_TOOLBAR_TOOL_USER_REDRAW:
    case T5_MSG_TOOLBAR_TOOL_USER_MOUSE:
        /* pass these sideways */
        return(proc_event_ui_ss_direct(p_docu, t5_message, p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_ss.c */
