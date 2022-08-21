/* ev_tabl.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Tables of data for evaluator */

/* MRJC February 1991 / May 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

//#include "ob_ss/resource/resource.h"

/******************************************************************************
*
* table of permissible argument types
*
******************************************************************************/

/* generic argument lists */

/* dereference to any primitive type */
static const EV_TYPE arg_CON[]  = { 1, EM_CONST };

static const EV_TYPE arg_ARY[]  = { 1, EM_ARY };

static const EV_TYPE arg_DAT[]  = { 1, EM_DAT };

static const EV_TYPE arg_INT[]  = { 1, EM_INT };

static const EV_TYPE arg_REA[]  = { 1, EM_REA };

static const EV_TYPE arg_SLR[]  = { 1, EM_SLR };

static const EV_TYPE arg_STR[]  = { 1, EM_STR };

/* Logical: not primitive, actually integer (which includes logical) or real */
static const EV_TYPE arg_BOO[]  = { 1, EM_LOGICAL };

/* IoD: integer or date */
static const EV_TYPE arg_IoD[]  = { 1, EM_INT | EM_DAT };

/* IoR: integer or real */
static const EV_TYPE arg_IoR[]  = { 1, EM_INT | EM_REA };

/* IoRoD: integer, real or date */
static const EV_TYPE arg_IoRoD[] = { 1, EM_INT | EM_REA | EM_DAT };

/* A_R: array and real  */
static const EV_TYPE arg_A_R[]  = { 2, EM_ARY,
                                       EM_REA };

/* D_I: date and integer */
static const EV_TYPE arg_D_I[]  = { 2, EM_DAT,
                                       EM_INT };

/* R_A: real and array */
static const EV_TYPE arg_R_A[]  = { 2, EM_REA,
                                       EM_ARY };

/* R_I: real and integer */
static const EV_TYPE arg_R_I[]  = { 2, EM_REA,
                                       EM_INT };

/* S_I: string and integer */
static const EV_TYPE arg_S_I[]  = { 2, EM_STR,
                                       EM_INT };

/* ARR: array, array, real  */
static const EV_TYPE arg_AAR[]  = { 3, EM_ARY,
                                       EM_ARY,
                                       EM_REA };

/* real, integer, integer */
static const EV_TYPE arg_RII[]  = { 3, EM_REA,
                                       EM_INT,
                                       EM_INT };

/* real, real, integer */
static const EV_TYPE arg_RRI[]  = { 3, EM_REA,
                                       EM_REA,
                                       EM_INT };

/* real, real, string */
static const EV_TYPE arg_RRS[]  = { 3, EM_REA,
                                       EM_REA,
                                       EM_STR };

/* function-specific argument lists */

static const EV_TYPE arg_adr[]  = { 5, EM_INT,
                                       EM_INT,
                                       EM_INT,
                                       EM_INT,
                                       EM_STR };

static const EV_TYPE arg_bdi[]  = { 4, EM_INT,
                                       EM_INT,
                                       EM_REA,
                                       EM_INT };

static const EV_TYPE arg_bdr[]  = { 4, EM_INT,
                                       EM_REA,
                                       EM_INT,
                                       EM_INT };

static const EV_TYPE arg_bse[]  = { 2, EM_REA | EM_INT,
                                       EM_INT };

static const EV_TYPE arg_bdv[]  = { 3, EM_INT,
                                       EM_REA,
                                       EM_REA };

static const EV_TYPE arg_bti[]  = { 6, EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_INT,
                                       EM_REA,
                                       EM_REA };

static const EV_TYPE arg_bto[]  = { 6, EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_INT };

static const EV_TYPE arg_cho[]  = { 2, EM_INT,
                                       EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY };

#if defined(COMPLEX_STRING)
static const EV_TYPE arg_CPX[]  = { 1, EM_REA | EM_ARY | EM_STR };
static const EV_TYPE arg_C_I[]  = { 2, EM_REA | EM_ARY | EM_STR,
                                       EM_INT };
static const EV_TYPE arg_IMA[]  = { 1, EM_REA          | EM_STR };
#else
static const EV_TYPE arg_CPX[]  = { 1, EM_REA | EM_ARY };
static const EV_TYPE arg_C_I[]  = { 2, EM_REA | EM_ARY,
                                       EM_INT };
static const EV_TYPE arg_IMA[]  = { 1, EM_REA          | EM_STR };
#endif

static const EV_TYPE arg_cvr[]  = { 2, EM_REA | EM_STR,
                                       EM_INT };

static const EV_TYPE arg_dan[]  = { 2, EM_INT | EM_DAT,
                                       EM_INT };

static const EV_TYPE arg_dbs[]  = { 2, EM_ARY,
                                       EM_CDX };

static const EV_TYPE arg_ddi[]  = { 3, EM_DAT,
                                       EM_DAT,
                                       EM_INT };

static const EV_TYPE arg_drf[]  = { 1, EM_REA | EM_SLR | EM_STR | EM_DAT | EM_BLK | EM_ERR | EM_INT };

static const EV_TYPE arg_for[]  = { 4, EM_STR,
                          /*EM_SLR |*/ EM_REA, /* SKS after 1.08b2 fixes Paul Swan custom function for("i",1,a2) bug */
                          /*EM_SLR |*/ EM_REA,
                                       EM_REA };

static const EV_TYPE arg_fnd[]  = { 3, EM_STR,
                                       EM_STR,
                                       EM_INT };

static const EV_TYPE arg_hvl[]  = { 4, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_INT,
                                       EM_ARY,
                                       EM_INT,
                                       EM_INT }; /* SKS 02aug96 adds optional parameter */

static const EV_TYPE arg_idx[]  = { 2, EM_ARY,
                                       EM_INT };

static const EV_TYPE arg_if[]   = { 2, EM_LOGICAL,
                                       EM_ANY };

static const EV_TYPE arg_llg[]  = { 4, EM_ARY,
                                       EM_ARY,
                                       EM_REA,
                                       EM_ARY };

static const EV_TYPE arg_lkp[]  = { 4, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_INT,
                                       EM_ARY,
                                       EM_ARY,
                                       EM_INT };

static const EV_TYPE arg_mat[]  = { 3, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_INT,
                                       EM_ARY,
                                       EM_INT };

#define arg_iro arg_A_R
#define arg_mir arg_A_R

static const EV_TYPE arg_mix[]  = { 1, EM_REA | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_INT };

static const EV_TYPE arg_n[]    = { 1, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_ERR | EM_INT };

static const EV_TYPE arg_ndi[]  = { 4, EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_INT };

static const EV_TYPE arg_ndp[]  = { 2, EM_REA | EM_INT, /* SKS 03may14 added EM_INT for rounding precision where simple enough (decimal_places >= 0) */
                                       EM_INT };

static const EV_TYPE arg_nls[]  = { 1, EM_REA | EM_ARY };

static const EV_TYPE arg_pag[]  = { 2, EM_SLR,
                                       EM_INT };

static const EV_TYPE arg_pct[]  = { 2, EM_ARY,
                                       EM_REA };

static const EV_TYPE arg_poi[]  = { 3, EM_INT,
                                       EM_REA,
                                       EM_INT };

static const EV_TYPE arg_prk[]  = { 3, EM_ARY,
                                       EM_REA,
                                       EM_INT };

static const EV_TYPE arg_rco[]  = { 1, EM_SLR | EM_ARY };

static const EV_TYPE arg_rel[]  = { 1, EM_REA | EM_STR | EM_DAT | EM_BLK | EM_INT };

static const EV_TYPE arg_res[]  = { 1, EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_ERR | EM_INT };

static const EV_TYPE arg_rnk[]  = { 2, EM_ARY,
                                       EM_INT };

static const EV_TYPE arg_req[]  = { 3, EM_INT,
                                       EM_ARY,
                                       EM_INT };

static const EV_TYPE arg_rpl[]  = { 4, EM_STR,
                                       EM_INT,
                                       EM_INT,
                                       EM_STR };

static const EV_TYPE arg_setn[] = { 2, EM_STR,
                                       EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_INT };

static const EV_TYPE arg_setv[] = { 4, EM_SLR | EM_ARY,
                                       EM_REA | EM_STR | EM_DAT | EM_ARY | EM_INT,
                                       EM_INT,
                                       EM_INT };

static const EV_TYPE arg_ssm[]  = { 4, EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_ARY };

static const EV_TYPE arg_sub[]  = { 4, EM_STR,
                                       EM_STR,
                                       EM_STR,
                                       EM_INT };

static const EV_TYPE arg_trd[]  = { 2, EM_ARY,
                                       EM_ARY | EM_REA };

static const EV_TYPE arg_trm[]  = { 2, EM_ARY,
                                       EM_REA };

static const EV_TYPE arg_txt[]  = { 2, EM_REA | EM_STR | EM_DAT | EM_INT,
                                       EM_STR };

static const EV_TYPE arg_typ[]  = { 1, EM_REA | EM_SLR | EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_ERR | EM_INT };

static const EV_TYPE arg_tyx[]  = { 1, EM_REA |          EM_STR | EM_DAT | EM_ARY | EM_BLK | EM_ERR | EM_INT };

static const EV_TYPE arg_wei[]  = { 4, EM_REA,
                                       EM_REA,
                                       EM_REA,
                                       EM_INT };

/******************************************************************************
*
* table of operators and functions
*
******************************************************************************/

#define FP_AGG(ex_type, no_exec, control, var, nodep, self, load_recalc, event_type, rpn_alias) \
    { (ex_type), (no_exec), (control), (var), (nodep), (self), (load_recalc), (event_type), (rpn_alias) }

#define NAI 0              /* na integer */
#define NAP FP_AGG(EXEC_EXEC, 0, 0, 0, 0, 0, 0, 0, 0)   /* na parms */
#define NAS OBJECT_ID_NONE /* na object id */
#define NIX 0              /* na table index */
#define NAA NULL           /* na argument flags address */
#define NAT 0              /* na type */

#define EXCTRL(a, b) \
    FP_AGG(EXEC_CONTROL, (a), (b), 0, 0, 0, 0, 0, 0)

#define EXDBASE(a, rpn_alias) \
    FP_AGG(EXEC_DBASE, (a), 0, 0, 0, 0, 0, 0, (rpn_alias))

/* cater for members no longer being in original order and CHECKING_ONLY */
#define rpn_table_entry(rpn_type, n_args, max_additional_args, category, fun_parms, object_id, table_index, arg_types, ev_idno) \
    { (rpn_type), (n_args), (max_additional_args), fun_parms, (category), (object_id), (table_index), (arg_types) CHECKING_ONLY_ARG(ev_idno) }

#define rpn_table_entry_INV \
    rpn_table_entry( RPN_INV, 0, 0, EV_RESO_NOTME, NAP, NAS, NIX, NAA, 0 )

/*                   [type]   [#args] [category]  [func bits] [object id to call]     [index in object's table][argument types]{[own id]} */

static const RPNDEF
_rpn_table[] =
{
    /* externally visible types (SS_CONSTANT) */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_REAL ), /* real */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_LOGICAL ), /* logical */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_WORD16 ), /* word16 */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_WORD32 ), /* word32 */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_DATE ), /* date */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_STRING ), /* string */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_BLANK ), /* blank */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_ERROR ), /* error */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_ARRAY ), /* array */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_WORD8_UNUSED ), /* word8 */ /* NB no longer used */

    /* internal types start here (SS_DATA) */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_SLR ), /* slr */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_RANGE ), /* range */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_FIELD ), /* field */
    rpn_table_entry( RPN_DAT, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     DATA_ID_NAME ), /* name */

#if defined(EV_IDNO_U16_FORCE)
    /* test 64 */
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,

    /* test 64 */
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,

    /* test 64 */
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,

    /* test 64 */
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
    rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV, rpn_table_entry_INV,
#endif

    /* general RPN starts here */
    rpn_table_entry( RPN_LCL, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_LCL_ARGUMENT ), /* argument name */

    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_BRACKETS ), /* brackets */
    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_SPACE ), /* space */
    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_RETURN ), /* return */
    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_EQUALS ), /* equals */
    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_END ), /* end of expression */
    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_COND ), /* conditional expression */
    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_SKIPFALSE ), /* skip rpn if false */
    rpn_table_entry( RPN_FRM, NAI, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FRM_SKIPTRUE ), /* skip rpn if true */

    rpn_table_entry( RPN_UOP,   1, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_UOP_NOT,        arg_BOO, RPN_UOP_NOT ),
    rpn_table_entry( RPN_UOP,   1, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_UOP_MINUS,      arg_IoR, RPN_UOP_MINUS ), /* unary - */ /* SKS 15may14 was arg_REA */
    rpn_table_entry( RPN_UOP,   1, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_UOP_PLUS,       arg_IoRoD, RPN_UOP_PLUS ), /* unary + */ /* ditto */

    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_AND,        arg_BOO, RPN_BOP_AND ),
    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_DIV,        arg_IoRoD, RPN_BOP_DIVIDE ),
    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_SUB,        arg_IoRoD, RPN_BOP_MINUS ),
    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_OR,         arg_BOO, RPN_BOP_OR ),
    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_ADD,        arg_IoRoD, RPN_BOP_PLUS ),
    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_POWER,      arg_REA, RPN_BOP_POWER ),
    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_MUL,        arg_IoRoD, RPN_BOP_TIMES ),
    rpn_table_entry( RPN_BOP,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_BOP_CONCATENATE,arg_STR, RPN_BOP_CONCATENATE ),

    rpn_table_entry( RPN_REL,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_REL_EQ,         arg_rel, RPN_REL_EQUALS ),
    rpn_table_entry( RPN_REL,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_REL_GT,         arg_rel, RPN_REL_GT ),
    rpn_table_entry( RPN_REL,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_REL_GTEQ,       arg_rel, RPN_REL_GTEQUAL ),
    rpn_table_entry( RPN_REL,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_REL_LT,         arg_rel, RPN_REL_LT ),
    rpn_table_entry( RPN_REL,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_REL_LTEQ,       arg_rel, RPN_REL_LTEQUAL ),
    rpn_table_entry( RPN_REL,   2, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS,           SS_FUNC_REL_NEQ,        arg_rel, RPN_REL_NOTEQUAL ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ABS,           arg_IoR, RPN_FNF_ABS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ACOS,          arg_REA, RPN_FNF_ACOS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ACOSEC,        arg_REA, RPN_FNF_ACOSEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ACOSECH,       arg_REA, RPN_FNF_ACOSECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ACOSH,         arg_REA, RPN_FNF_ACOSH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ACOT,          arg_REA, RPN_FNF_ACOT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ACOTH,         arg_REA, RPN_FNF_ACOTH ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_AGE,           arg_DAT, RPN_FNF_AGE ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_MISC,      FP_AGG(EXEC_ALERT, 0, 0, 0, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_STR, RPN_FNV_ALERT ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_LOGICAL,   FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_AND ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ASEC,          arg_REA, RPN_FNF_ASEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ASECH,         arg_REA, RPN_FNF_ASECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ASIN,          arg_REA, RPN_FNF_ASIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ASINH,         arg_REA, RPN_FNF_ASINH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ATAN,          arg_REA, RPN_FNF_ATAN ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ATAN_2,        arg_REA, RPN_FNF_ATAN_2 ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ATANH,         arg_REA, RPN_FNF_ATANH ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_AVEDEV ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_AVG ),

    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BETA,          arg_REA, RPN_FNF_BETA ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BIN,           arg_ARY, RPN_FNF_BIN ),
    rpn_table_entry( RPN_FNV,  -1, 1, EV_RESO_CONTROL,   EXCTRL(CONTROL_BREAK, 0),
                                                              NAS,                    NIX,                    arg_INT, RPN_FNV_BREAK ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ACOS,        arg_CPX, RPN_FNF_C_ACOS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ACOSEC,      arg_CPX, RPN_FNF_C_ACOSEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ACOSECH,     arg_CPX, RPN_FNF_C_ACOSECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ACOSH,       arg_CPX, RPN_FNF_C_ACOSH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ACOT,        arg_CPX, RPN_FNF_C_ACOT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ACOTH,       arg_CPX, RPN_FNF_C_ACOTH ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ADD,         arg_CPX, RPN_FNF_C_ADD ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ASEC,        arg_CPX, RPN_FNF_C_ASEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ASECH,       arg_CPX, RPN_FNF_C_ASECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ASIN,        arg_CPX, RPN_FNF_C_ASIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ASINH,       arg_CPX, RPN_FNF_C_ASINH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ATAN,        arg_CPX, RPN_FNF_C_ATAN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ATANH,       arg_CPX, RPN_FNF_C_ATANH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_CONJUGATE,   arg_CPX, RPN_FNF_C_CONJUGATE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_COS,         arg_CPX, RPN_FNF_C_COS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_COSEC,       arg_CPX, RPN_FNF_C_COSEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_COSECH,      arg_CPX, RPN_FNF_C_COSECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_COSH,        arg_CPX, RPN_FNF_C_COSH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_COT,         arg_CPX, RPN_FNF_C_COT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_COTH,        arg_CPX, RPN_FNF_C_COTH ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_DIV,         arg_CPX, RPN_FNF_C_DIV ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_EXP,         arg_CPX, RPN_FNF_C_EXP ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_LN,          arg_CPX, RPN_FNF_C_LN ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_MUL,         arg_CPX, RPN_FNF_C_MUL ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_POWER,       arg_CPX, RPN_FNF_C_POWER ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_RADIUS,      arg_CPX, RPN_FNF_C_RADIUS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_SEC,         arg_CPX, RPN_FNF_C_SEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_SECH,        arg_CPX, RPN_FNF_C_SECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_SIN,         arg_CPX, RPN_FNF_C_SIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_SINH,        arg_CPX, RPN_FNF_C_SINH ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_SUB,         arg_CPX, RPN_FNF_C_SUB ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_TAN,         arg_CPX, RPN_FNF_C_TAN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_TANH,        arg_CPX, RPN_FNF_C_TANH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_THETA,       arg_CPX, RPN_FNF_C_THETA ),

    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CEILING,       arg_REA, RPN_FNV_CEILING ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CHAR,          arg_INT, RPN_FNF_CHAR ),
    rpn_table_entry( RPN_FNV,  -3, 0, EV_RESO_LOOKUP,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CHOOSE,        arg_cho, RPN_FNV_CHOOSE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CLEAN,         arg_STR, RPN_FNF_CLEAN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CODE,          arg_STR, RPN_FNF_CODE ),
    rpn_table_entry( RPN_FNV,  -1, 1, EV_RESO_LOOKUP,    FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 1/*nodep*/, 1/*self*/, 0, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_COL,           arg_rco, RPN_FNV_COL ),
    rpn_table_entry( RPN_FNV,  -1, 1, EV_RESO_LOOKUP,    FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 1/*nodep*/, 1/*self*/, 0, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_COLS,          arg_ARY, RPN_FNV_COLS ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COMBIN,        arg_INT, RPN_FNF_COMBIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_NOTME,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COMMAND,       arg_STR, RPN_FNF_COMMAND ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_CONTINUE, 0),
                                                              NAS,                    NIX,                    NAA,     RPN_FN0_CONTINUE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COS,           arg_REA, RPN_FNF_COS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COSEC,         arg_REA, RPN_FNF_COSEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COSECH,        arg_REA, RPN_FNF_COSECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COSH,          arg_REA, RPN_FNF_COSH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COT,           arg_REA, RPN_FNF_COT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COTH,          arg_REA, RPN_FNF_COTH ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_COUNT ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_COUNTA ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CTERM,         arg_REA, RPN_FNF_CTERM ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_MISC,      FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 0, 0, 0, EV_EVENT_CURRENT_CELL/*event_type*/, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_CURRENT_CELL,  NAA,     RPN_FN0_CURRENT_CELL ),

    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DATE,          arg_INT, RPN_FNF_DATE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DATEVALUE,     arg_STR, RPN_FNF_DATEVALUE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DAVG, RPN_FNV_AVG),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DAVG ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DAY,           arg_DAT, RPN_FNF_DAY ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DAYNAME,       arg_dan, RPN_FNV_DAYNAME ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DAYS_360,      arg_ddi, RPN_FNV_DAYS_360 ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DCOUNT, RPN_FNV_COUNT),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DCOUNT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DCOUNTA, RPN_FNV_COUNTA),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DCOUNTA ),
    rpn_table_entry( RPN_FNV,  -5, 1, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DDB,           arg_REA, RPN_FNV_DDB ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DEG,           arg_REA, RPN_FNF_DEG ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DEREF,         arg_drf, RPN_FNF_DEREF ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_DEVSQ ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DMAX, RPN_FNV_MAX),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DMAX ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DMIN, RPN_FNV_MIN),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DMIN ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DOLLAR,        arg_ndp, RPN_FNV_DOLLAR ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_MISC,      FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 0, 0, 0, EV_EVENT_DOUBLECLICK/*event_type*/, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_DOUBLECLICK,   NAA,     RPN_FN0_DOUBLECLICK ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DPRODUCT, RPN_FNV_PRODUCT),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DPRODUCT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DSTD, RPN_FNV_STD),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DSTD ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DSTDP, RPN_FNV_STDP),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DSTDP ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DSUM, RPN_FNV_SUM),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DSUM ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DVAR, RPN_FNV_VAR),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DVAR ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATABASE,  EXDBASE(DBASE_DVARP, RPN_FNV_VARP),
                                                              NAS,                    NIX,                    arg_dbs, RPN_FNF_DVARP ),

    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_ELSE, EVS_CNT_ELSE),
                                                              NAS,                    NIX,                    NAA,     RPN_FN0_ELSE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_ELSEIF, EVS_CNT_ELSEIF),
                                                              NAS,                    NIX,                    arg_BOO, RPN_FNF_ELSEIF ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_ENDIF, EVS_CNT_ENDIF),
                                                              NAS,                    NIX,                    NAA,     RPN_FN0_ENDIF ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_ENDWHILE, EVS_CNT_ENDWHILE),
                                                              NAS,                    NIX,                    NAA,     RPN_FN0_ENDWHILE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_EVEN,          arg_REA, RPN_FNF_EVEN ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_EXACT,         arg_STR, RPN_FNF_EXACT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_EXP,           arg_REA, RPN_FNF_EXP ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FACT,          arg_INT, RPN_FNF_FACT ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_LOGICAL,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FALSE,         NAA,     RPN_FN0_FALSE ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FIND,          arg_fnd, RPN_FNV_FIND ),
    rpn_table_entry( RPN_FNV,  -2, 2, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FIXED,         arg_RII, RPN_FNV_FIXED ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FLIP,          arg_ARY, RPN_FNF_FLIP ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FLOOR,         arg_REA, RPN_FNV_FLOOR ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_CONTROL,   EXCTRL(CONTROL_FOR, EVS_CNT_FOR),
                                                              NAS,                    NIX,                    arg_for, RPN_FNV_FOR ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FORMULA_TEXT,  arg_SLR, RPN_FNF_FORMULA_TEXT ),
    rpn_table_entry( RPN_FNM,  -1, 0, EV_RESO_CONTROL,   NAP, NAS,                    0,                      NAA,     RPN_FNM_FUNCTION ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FV,            arg_REA, RPN_FNF_FV ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_FV,        arg_REA, RPN_FNV_ODF_FV ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_GAMMALN,       arg_REA, RPN_FNF_GAMMALN ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_GEOMEAN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_GOTO, 0),
                                                              NAS,                    NIX,                    arg_SLR, RPN_FNF_GOTO ),
    rpn_table_entry( RPN_FNV,  -1, 2, EV_RESO_STATS,     FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 0, 0, 1/*load_recalc*/, 0, 0), /* SKS 29apr14 added load_recalc bit */
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_GRAND,         arg_REA, RPN_FNV_GRAND ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_GROWTH,        arg_trd, RPN_FNF_GROWTH ),

    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_HARMEAN ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_LOOKUP,    FP_AGG(EXEC_LOOKUP, LOOKUP_HLOOKUP, 0, 0, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_hvl, RPN_FNV_HLOOKUP ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_HOUR,          arg_DAT, RPN_FNF_HOUR ),

    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS,           SS_FUNC_IF,             arg_if,  RPN_FNF_IF ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_IF, EVS_CNT_IFC),
                                                              NAS,                    NIX,                    arg_BOO, RPN_FNF_IFC ),
    rpn_table_entry( RPN_FNV,  -4, 2, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_INDEX,         arg_idx, RPN_FNV_INDEX ), /* SKS 11apr93 was 0, EV_RESO_LOOKUP */
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_MISC,      FP_AGG(EXEC_ALERT, 0, 0, 0, 0, 0, 1/*load_recalc*/, 0, 0),
                                                              NAS,                    NIX,                    arg_STR, RPN_FNV_INPUT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_INT,           arg_IoRoD, RPN_FNF_INT ), /* SKS 08apr14 was arg_IoR */
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_FINANCE,   FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_R_A, RPN_FNF_IRR ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISBLANK,       arg_CON, RPN_FNF_ISBLANK ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISERR,         arg_CON, RPN_FNF_ISERR ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISERROR,       arg_CON, RPN_FNF_ISERROR ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISEVEN,        arg_IoR, RPN_FNF_ISEVEN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISLOGICAL,     arg_CON, RPN_FNF_ISLOGICAL ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISNA,          arg_CON, RPN_FNF_ISNA ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISTEXT,        arg_CON, RPN_FNF_ISNONTEXT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISNUMBER,      arg_CON, RPN_FNF_ISNUMBER ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISODD,         arg_IoR, RPN_FNF_ISODD ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISREF,         arg_typ, RPN_FNF_ISREF ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISTEXT,        arg_CON, RPN_FNF_ISTEXT ),

    rpn_table_entry( RPN_FNV,  -1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_JOIN,          arg_STR, RPN_FNV_JOIN ),

    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_KURT ),

    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LARGE,         arg_idx, RPN_FNF_LARGE ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LEFT,          arg_S_I, RPN_FNV_LEFT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LENGTH,        arg_STR, RPN_FNF_LENGTH ),
    rpn_table_entry( RPN_FNV,  -2, 4, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LINEST,        arg_llg, RPN_FNV_LINEST ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LISTCOUNT,     arg_ARY, RPN_FNF_LISTCOUNT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LN,            arg_REA, RPN_FNF_LN ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LOG,           arg_REA, RPN_FNV_LOG ),
    rpn_table_entry( RPN_FNV,  -2, 3, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LOGEST,        arg_llg, RPN_FNV_LOGEST ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_LOOKUP,    FP_AGG(EXEC_LOOKUP, LOOKUP_LOOKUP, 0, 0, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_lkp, RPN_FNV_LOOKUP ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LOWER,         arg_STR, RPN_FNF_LOWER ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATRIX,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_M_DETERM,      arg_ARY, RPN_FNF_M_DETERM ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATRIX,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_M_INVERSE,     arg_ARY, RPN_FNF_M_INVERSE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATRIX,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_M_MULT,        arg_ARY, RPN_FNF_M_MULT ),

    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_LOOKUP,    FP_AGG(EXEC_LOOKUP, LOOKUP_MATCH, 0, 0, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mat, RPN_FNF_MATCH ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_MAX ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_MEDIAN ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MID,           arg_S_I, RPN_FNF_MID ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_MIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MINUTE,        arg_DAT, RPN_FNF_MINUTE ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mir, RPN_FNF_MIRR ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MOD,           arg_IoR, RPN_FNF_MOD ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MODE_SNGL,     arg_ARY, RPN_FNF_MODE_SNGL ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MONTH,         arg_DAT, RPN_FNF_MONTH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MONTHDAYS,     arg_DAT, RPN_FNF_MONTHDAYS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MONTHNAME,     arg_IoD, RPN_FNF_MONTHNAME ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_MROUND,        arg_REA, RPN_FNF_MROUND ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_N,             arg_n,   RPN_FNF_N ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_NEXT, EVS_CNT_NEXT),
                                                              NAS,                    NIX,                    NAA,     RPN_FN0_NEXT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_LOGICAL,   NAP, OBJECT_ID_SS,           SS_SPLIT_NOT,           arg_BOO, RPN_FNF_NOT ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_DATE,      FP_AGG(EXEC_EXEC, 0, 0, 0, 0, 0, 1/*load_recalc*/, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_NOW,           NAA,     RPN_FN0_NOW ),
    rpn_table_entry( RPN_FNV,  -4, 2, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_NPER,          arg_REA, RPN_FNV_NPER ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_FINANCE,   FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_R_A, RPN_FNF_NPV ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODD,           arg_REA, RPN_FNF_ODD ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_LOGICAL,   FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_OR ),

    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_MISC,      FP_AGG(EXEC_EXEC, 0, 0, 0, 1/*nodep*/, 1/*self*/, 1/*load_recalc*/, 0, 0), /* SKS 14jun95 added load_recalc bit as reformat on load might stuff it */
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_PAGE,          arg_pag, RPN_FNV_PAGE ),
    rpn_table_entry( RPN_FNV,  -1, 1, EV_RESO_MISC,      FP_AGG(EXEC_EXEC, 0, 0, 0, 0, 0, 1/*load_recalc*/, 0, 0), /* SKS 14jun95 added load_recalc bit as reformat on load might stuff it */
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_PAGES,         arg_INT, RPN_FNV_PAGES ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PERCENTILE_INC, arg_pct, RPN_FNF_PERCENTILE_INC ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PERMUT,        arg_INT, RPN_FNF_PERMUT ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PI,            NAA,     RPN_FN0_PI ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PMT,           arg_REA, RPN_FNF_PMT ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_PMT,       arg_REA, RPN_FNF_ODF_PMT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_POWER,         arg_REA, RPN_FNF_POWER ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_MATHS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_PRODUCT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PROPER,        arg_STR, RPN_FNF_PROPER ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PV,            arg_REA, RPN_FNF_PV ),

    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_QUARTILE_INC,  arg_idx, RPN_FNF_QUARTILE_INC ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_RAD,           arg_REA, RPN_FNF_RAD ),
    rpn_table_entry( RPN_FNV,  -1, 1, EV_RESO_STATS,     FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 0, 0, 1/*load_recalc*/, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_RAND,          arg_REA, RPN_FNV_RAND ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 0, 0, 1/*load_recalc*/, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_RANDBETWEEN,   arg_REA, RPN_FNF_RANDBETWEEN ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_RANK,          arg_rnk, RPN_FNV_RANK ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_RANK_EQ,       arg_req, RPN_FNV_RANK_EQ ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_RATE,          arg_REA, RPN_FNF_RATE ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_REPEAT, EVS_CNT_REPEAT),
                                                              NAS,                    NIX,                    NAA,     RPN_FN0_REPEAT ),
    rpn_table_entry( RPN_FNF,   4, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_REPLACE,       arg_rpl, RPN_FNF_REPLACE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_REPT,          arg_S_I, RPN_FNF_REPT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_RESULT, 0),
                                                              NAS,                    NIX,                    arg_res, RPN_FNF_RESULT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_REVERSE,       arg_STR, RPN_FNF_REVERSE ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_RIGHT,         arg_S_I, RPN_FNV_RIGHT ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ROUND,         arg_ndp, RPN_FNV_ROUND ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ROUNDDOWN,     arg_ndp, RPN_FNF_ROUNDDOWN ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ROUNDUP,       arg_ndp, RPN_FNF_ROUNDUP ),
    rpn_table_entry( RPN_FNV,  -1, 1, EV_RESO_LOOKUP,    FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 1/*nodep*/, 1/*self*/, 0, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_ROW,           arg_rco, RPN_FNV_ROW ),
    rpn_table_entry( RPN_FNV,  -1, 1, EV_RESO_LOOKUP,    FP_AGG(EXEC_EXEC, 0, 0, 1/*var*/, 1/*nodep*/, 1/*self*/, 0, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_ROWS,          arg_ARY, RPN_FNV_ROWS ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SEC,           arg_REA, RPN_FNF_SEC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SECH,          arg_REA, RPN_FNF_SECH ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SECOND,        arg_DAT, RPN_FNF_SECOND ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MISC,      FP_AGG(EXEC_EXEC, 0, 0, 0, 2/*nodep*/, 0, 0, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_SET_NAME,      arg_setn, RPN_FNF_SET_NAME ),
    rpn_table_entry( RPN_FNV,  -3, 2, EV_RESO_MISC,      FP_AGG(EXEC_SETVALUE, 0, 0, 0, 1/*nodep*/, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_setv, RPN_FNV_SET_VALUE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SGN,           arg_REA, RPN_FNF_SGN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SIN,           arg_REA, RPN_FNF_SIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SINH,          arg_REA, RPN_FNF_SINH ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_SKEW ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SLN,           arg_REA, RPN_FNF_SLN ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SMALL,         arg_idx, RPN_FNF_SMALL ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SORT,          arg_idx, RPN_FNV_SORT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SPEARMAN,      arg_ARY, RPN_FNF_SPEARMAN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SQR,           arg_REA, RPN_FNF_SQR ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_STANDARDIZE,   arg_REA, RPN_FNF_STANDARDIZE ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_STD ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_STDP ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_STRING,        arg_ndp, RPN_FNV_STRING ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SUBSTITUTE,    arg_sub, RPN_FNV_SUBSTITUTE ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_MATHS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_SUM ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_MATHS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_SUMSQ ),
    rpn_table_entry( RPN_FNF,   4, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SYD,           arg_REA, RPN_FNF_SYD ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_T,             arg_n,   RPN_FNF_T ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TAN,           arg_REA, RPN_FNF_TAN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_TRIG,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TANH,          arg_REA, RPN_FNF_TANH ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TERM,          arg_REA, RPN_FNF_TERM ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TEXT,          arg_txt, RPN_FNF_TEXT ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TIME,          arg_INT, RPN_FNF_TIME ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TIMEVALUE,     arg_STR, RPN_FNF_TIMEVALUE ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_DATE,      FP_AGG(EXEC_EXEC, 0, 0, 0, 0, 0, 1/*load_recalc*/, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_TODAY,         NAA,     RPN_FN0_TODAY ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATRIX,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TRANSPOSE,     arg_ARY, RPN_FNF_TRANSPOSE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TREND,         arg_trd, RPN_FNF_TREND ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TRIM,          arg_STR, RPN_FNF_TRIM ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TRIMMEAN,      arg_trm, RPN_FNF_TRIMMEAN ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_LOGICAL,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TRUE,          NAA,     RPN_FN0_TRUE ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TRUNC,         arg_ndp, RPN_FNV_TRUNC ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_TYPE,          arg_typ, RPN_FNF_TYPE ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_UNTIL, EVS_CNT_UNTIL),
                                                              NAS,                    NIX,                    arg_BOO, RPN_FNF_UNTIL ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_UPPER,         arg_STR, RPN_FNF_UPPER ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STRING,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_VALUE,         arg_STR, RPN_FNF_VALUE ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_VAR ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_VARP ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_MISC,      FP_AGG(EXEC_EXEC, 0, 0, 0, 0, 0, 1/*load_recalc*/, 0, 0),
                                                              OBJECT_ID_SS_SPLIT,     SS_SPLIT_VERSION,       NAA,     RPN_FN0_VERSION ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_LOOKUP,    FP_AGG(EXEC_LOOKUP, LOOKUP_VLOOKUP, 0, 0, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_hvl, RPN_FNV_VLOOKUP ),

    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_WEEKDAY,       arg_D_I, RPN_FNV_WEEKDAY ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_WEEKNUMBER,    arg_DAT, RPN_FNF_WEEKNUMBER ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_CONTROL,   EXCTRL(CONTROL_WHILE, EVS_CNT_WHILE),
                                                              NAS,                    NIX,                    arg_BOO, RPN_FNF_WHILE ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_YEAR,          arg_DAT, RPN_FNF_YEAR ),

#if 1 /* New functions should go here (in the order they appear in ev_eval.h and in messages) until we have another big sort-out */

    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SUMPRODUCT,    arg_ARY, RPN_FNV_SUMPRODUCT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SUM_X2MY2,     arg_ARY, RPN_FNF_SUM_X2MY2 ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SUM_X2PY2,     arg_ARY, RPN_FNF_SUM_X2PY2 ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SUM_XMY2,      arg_ARY, RPN_FNF_SUM_XMY2 ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_TYPE,      arg_tyx, RPN_FNF_ODF_TYPE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_ARY, RPN_FNF_COUNTBLANK ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COVARIANCE_P,  arg_ARY, RPN_FNF_COVARIANCE_P ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PEARSON,       arg_ARY, RPN_FNF_PEARSON ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_RSQ,           arg_ARY, RPN_FNF_RSQ ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SLOPE,         arg_ARY, RPN_FNF_SLOPE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_INTERCEPT,     arg_ARY, RPN_FNF_INTERCEPT ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FORECAST,      arg_R_A, RPN_FNF_FORECAST ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_STEYX,         arg_ARY, RPN_FNF_STEYX ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_SKEW_P ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BESSELJ,       arg_REA, RPN_FNF_BESSELJ ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BESSELY,       arg_REA, RPN_FNF_BESSELY ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_AVERAGEA ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_MAXA ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_MINA ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_STDEVA ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_STDEVPA ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_VARA ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_VARPA ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PERCENTRANK_INC, arg_prk, RPN_FNV_PERCENTRANK_INC ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ERF,           arg_REA, RPN_FNV_ERF ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ERFC,          arg_REA, RPN_FNF_ERFC ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COVARIANCE_S,  arg_ARY, RPN_FNF_COVARIANCE_S ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_LOGICAL,   FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_mix, RPN_FNV_XOR ),
    rpn_table_entry( RPN_FNV,  -2, 2, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_COMPLEX,     arg_RRS, RPN_FNV_C_COMPLEX ),
    rpn_table_entry( RPN_FNV,  -3, 3, EV_RESO_LOOKUP,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ADDRESS,       arg_adr, RPN_FNV_ADDRESS ),
    rpn_table_entry( RPN_FNV,  -5, 1, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DB,            arg_REA, RPN_FNV_DB ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_FINANCE,   NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FVSCHEDULE,    arg_R_A, RPN_FNF_FVSCHEDULE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_QUOTIENT,      arg_REA, RPN_FNF_QUOTIENT ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_EXPON_DIST,    arg_RRI, RPN_FNV_EXPON_DIST ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_COMBINA,       arg_INT, RPN_FNF_COMBINA ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FACTDOUBLE,    arg_INT, RPN_FNF_FACTDOUBLE ),
    rpn_table_entry( RPN_FNF,   4, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_SERIESSUM,     arg_ssm, RPN_FNF_SERIESSUM ),
    rpn_table_entry( RPN_FNV,  -5, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_HYPGEOM_DIST,  arg_INT, RPN_FNV_HYPGEOM_DIST ),
    rpn_table_entry( RPN_FNF,   4, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_WEIBULL_DIST,  arg_wei, RPN_FNF_WEIBULL_DIST ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_POISSON_DIST,  arg_poi, RPN_FNV_POISSON_DIST ),
    rpn_table_entry( RPN_FNF,   4, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BINOM_DIST,    arg_bdi, RPN_FNF_BINOM_DIST ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BINOM_DIST_RANGE, arg_bdr, RPN_FNV_BINOM_DIST_RANGE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FISHER,        arg_REA, RPN_FNF_FISHER ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FISHERINV,     arg_REA, RPN_FNF_FISHERINV ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_STATS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_MULTINOMIAL ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BESSELI,       arg_REA, RPN_FNF_BESSELI ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BESSELK,       arg_REA, RPN_FNF_BESSELK ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_NORM_DIST,     arg_ndi, RPN_FNV_NORM_DIST ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_NORM_S_DIST,   arg_RII, RPN_FNV_NORM_S_DIST ), /* doesn't matter that arg is overlong */
    rpn_table_entry( RPN_FNV,  -2, 3, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LOGNORM_DIST,  arg_ndi, RPN_FNV_LOGNORM_DIST ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PHI,           arg_REA, RPN_FNF_PHI ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DELTA,         arg_REA, RPN_FNV_DELTA ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_GESTEP,        arg_REA, RPN_FNV_GESTEP ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_INT,       arg_IoRoD, RPN_FNF_ODF_INT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_FREQUENCY,     arg_ARY, RPN_FNF_FREQUENCY ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_MATRIX,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_M_UNIT,        arg_INT, RPN_FNF_M_UNIT ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IRR,       arg_iro, RPN_FNV_ODF_IRR ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_NORM_INV,      arg_REA, RPN_FNF_NORM_INV ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_NORM_S_INV,    arg_REA, RPN_FNF_NORM_S_INV ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_LOG10,     arg_REA, RPN_FNF_ODF_LOG10 ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_GAMMA,         arg_REA, RPN_FNF_GAMMA ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_NEGBINOM_DIST, arg_bdi, RPN_FNV_NEGBINOM_DIST ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_INDEX,     arg_idx, RPN_FNV_ODF_INDEX ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_GAMMA_DIST,    arg_wei, RPN_FNV_GAMMA_DIST ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_GAMMA_INV,     arg_REA, RPN_FNF_GAMMA_INV ),
    rpn_table_entry( RPN_FNV,  -4, 3, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BETA_DIST,     arg_bti, RPN_FNV_BETA_DIST ),
    rpn_table_entry( RPN_FNV,  -4, 3, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_BETADIST,  arg_bto, RPN_FNV_ODF_BETADIST ),
    rpn_table_entry( RPN_FNV,  -4, 2, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BETA_INV,      arg_REA, RPN_FNV_BETA_INV ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CORREL,        arg_ARY, RPN_FNF_CORREL ),
    rpn_table_entry( RPN_FN0,   0, 0, EV_RESO_MISC,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_NA,            NAA,     RPN_FN0_NA ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CHISQ_DIST,    arg_RII, RPN_FNV_CHISQ_DIST ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CHISQ_INV,     arg_RII, RPN_FNF_CHISQ_INV ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CHISQ_DIST_RT, arg_RII, RPN_FNF_CHISQ_DIST_RT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CHISQ_INV_RT,  arg_RII, RPN_FNF_CHISQ_INV_RT ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_F_DIST,        arg_ndi, RPN_FNV_F_DIST ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_F_INV,         arg_REA, RPN_FNF_F_INV ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_F_DIST_RT,     arg_REA, RPN_FNF_F_DIST_RT ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_F_INV_RT,      arg_REA, RPN_FNF_F_INV_RT ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_T_DIST,        arg_RRI, RPN_FNV_T_DIST ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_T_INV,         arg_REA, RPN_FNF_T_INV ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_T_DIST_2T,     arg_REA, RPN_FNF_T_DIST_2T ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_T_INV_2T,      arg_REA, RPN_FNF_T_INV_2T ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_T_DIST_RT,     arg_REA, RPN_FNF_T_DIST_RT ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_TDIST,     arg_REA, RPN_FNF_ODF_TDIST ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CONFIDENCE_NORM, arg_REA, RPN_FNF_CONFIDENCE_NORM ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CONFIDENCE_T,  arg_REA, RPN_FNF_CONFIDENCE_T ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PERCENTILE_EXC, arg_pct, RPN_FNF_PERCENTILE_EXC ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_QUARTILE_EXC,  arg_idx, RPN_FNF_QUARTILE_EXC ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BINOM_INV,     arg_bdv, RPN_FNF_BINOM_INV ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_Z_TEST,        arg_A_R, RPN_FNV_Z_TEST ),
    rpn_table_entry( RPN_FNV,  -4, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PROB,          arg_AAR, RPN_FNV_PROB ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_CHISQ_TEST,    arg_ARY, RPN_FNF_CHISQ_TEST ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_F_TEST,        arg_ARY, RPN_FNF_F_TEST ),
    rpn_table_entry( RPN_FNF,   4, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_T_TEST,        arg_AAR, RPN_FNF_T_TEST ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMABS,     arg_IMA, RPN_FNF_ODF_IMABS ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMAGINARY, arg_IMA, RPN_FNF_ODF_IMAGINARY ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMARGUMENT, arg_IMA, RPN_FNF_ODF_IMARGUMENT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMCONJUGATE, arg_IMA, RPN_FNF_ODF_IMCONJUGATE ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMCOS,     arg_IMA, RPN_FNF_ODF_IMCOS ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMDIV,     arg_IMA, RPN_FNF_ODF_IMDIV ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMEXP,     arg_IMA, RPN_FNF_ODF_IMEXP ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMLN,      arg_IMA, RPN_FNF_ODF_IMLN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMLOG10,   arg_IMA, RPN_FNF_ODF_IMLOG10 ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMLOG2,    arg_IMA, RPN_FNF_ODF_IMLOG2 ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMPOWER,   arg_IMA, RPN_FNF_ODF_IMPOWER ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMPRODUCT, arg_IMA, RPN_FNF_ODF_IMPRODUCT ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMREAL,    arg_IMA, RPN_FNF_ODF_IMREAL ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMSIN,     arg_IMA, RPN_FNF_ODF_IMSIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMSQRT,    arg_IMA, RPN_FNF_ODF_IMSQRT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMSUB,     arg_IMA, RPN_FNF_ODF_IMSUB ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_IMSUM,     arg_IMA, RPN_FNF_ODF_IMSUM ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_IMAGINARY,   arg_CPX, RPN_FNF_C_IMAGINARY ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_REAL,        arg_CPX, RPN_FNF_C_REAL ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_ROUND,       arg_C_I, RPN_FNV_C_ROUND ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_C_SQRT,        arg_CPX, RPN_FNF_C_SQRT ),

    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_COMPAT,    NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_COMPLEX,   arg_RRS, RPN_FNV_ODF_COMPLEX ),
    rpn_table_entry( RPN_FNF,   3, 0, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_LOGNORM_INV,   arg_REA, RPN_FNF_LOGNORM_INV ),
    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_STATS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_PERCENTRANK_EXC, arg_prk, RPN_FNV_PERCENTRANK_EXC ),

    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_EDATE,         arg_D_I, RPN_FNF_EDATE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_EOMONTH,       arg_D_I, RPN_FNF_EOMONTH ),

    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ODF_MOD,       arg_IoR, RPN_FNF_ODF_MOD ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BIN2DEC,       arg_cvr, RPN_FNF_BIN2DEC ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BIN2HEX,       arg_cvr, RPN_FNV_BIN2HEX ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BIN2OCT,       arg_cvr, RPN_FNV_BIN2OCT ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DEC2BIN,       arg_cvr, RPN_FNV_DEC2BIN ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DEC2HEX,       arg_cvr, RPN_FNV_DEC2HEX ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DEC2OCT,       arg_cvr, RPN_FNV_DEC2OCT ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_HEX2BIN,       arg_cvr, RPN_FNV_HEX2BIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_HEX2DEC,       arg_cvr, RPN_FNF_HEX2DEC ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_HEX2OCT,       arg_cvr, RPN_FNV_HEX2OCT ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_OCT2BIN,       arg_cvr, RPN_FNV_OCT2BIN ),
    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_OCT2DEC,       arg_cvr, RPN_FNF_OCT2DEC ),
    rpn_table_entry( RPN_FNV,  -2, 1, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_OCT2HEX,       arg_cvr, RPN_FNV_OCT2HEX ),

    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BITAND,        arg_REA, RPN_FNF_BITAND ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BITLSHIFT,     arg_R_I, RPN_FNF_BITLSHIFT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BITOR,         arg_REA, RPN_FNF_BITOR ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BITRSHIFT,     arg_R_I, RPN_FNF_BITRSHIFT ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_ENGINEER,  NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BITXOR,        arg_REA, RPN_FNF_BITXOR ),

    rpn_table_entry( RPN_FNV,  -3, 1, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_BASE,          arg_bse, RPN_FNV_BASE ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_MATHS,     NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DECIMAL,       arg_cvr, RPN_FNF_DECIMAL ),

    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_MATHS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_GCD ),
    rpn_table_entry( RPN_FNV,  -2, 0, EV_RESO_MATHS,     FP_AGG(EXEC_ARRAY_RANGE, 0, 0, 1/*var*/, 0, 0, 0, 0, 0),
                                                              NAS,                    NIX,                    arg_nls, RPN_FNV_LCM ),

    rpn_table_entry( RPN_FNF,   1, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_ISOWEEKNUM,    arg_DAT, RPN_FNF_ISOWEEKNUM ),
    rpn_table_entry( RPN_FNF,   2, 0, EV_RESO_DATE,      NAP, OBJECT_ID_SS_SPLIT,     SS_SPLIT_DAYS,          arg_DAT, RPN_FNF_DAYS ),

#endif /* end of any new functions */

    rpn_table_entry( RPN_FNM,  -1, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FNM_CUSTOMCALL ),

    rpn_table_entry( RPN_FNA,  -1, 0, EV_RESO_NOTME,     NAP, NAS,                    NIX,                    NAA,     RPN_FNA_MAKEARRAY )
};

const RPNDEF * const rpn_table = &_rpn_table[0];

/*
lowercase ASCII sorted compiler function lookup
*/

typedef struct LOOKDEF
{
    PC_A7STR sz_id;
    EV_IDNO ev_idno;
    EV_IDNO ev_idno_function_id;
#if !defined(EV_IDNO_U16)
    U8 _padding[4-2*sizeof(EV_IDNO)]; /* to 8 */
#endif
}
LOOKDEF; typedef const LOOKDEF * PC_LOOKDEF;

#define look_table_entr0(id, ev_idno) \
    { (id), (ev_idno), 0 }

#define look_table_entry(id, ev_idno_both) \
    { (id), (ev_idno_both), (ev_idno_both) }

static const LOOKDEF
look_table[] = /* ordered by id for bsearch()*/
{
    look_table_entr0("!",               RPN_UOP_NOT),

    look_table_entr0("&",               RPN_BOP_AND),
    look_table_entr0("&<",              RPN_BOP_CONCATENATE),
    look_table_entr0("*",               RPN_BOP_TIMES),
    look_table_entr0("+",               RPN_BOP_PLUS),
    look_table_entr0("-",               RPN_BOP_MINUS),
    look_table_entr0("/",               RPN_BOP_DIVIDE),

    look_table_entr0("<",               RPN_REL_LT),
    look_table_entr0("<=",              RPN_REL_LTEQUAL),
    look_table_entr0("<>",              RPN_REL_NOTEQUAL),
    look_table_entr0("=",               RPN_REL_EQUALS),
    look_table_entr0(">",               RPN_REL_GT),
    look_table_entr0(">=",              RPN_REL_GTEQUAL),

    look_table_entr0("^",               RPN_BOP_POWER),

    look_table_entry("abs",             RPN_FNF_ABS),
    look_table_entry("acos",            RPN_FNF_ACOS),
    look_table_entry("acosec",          RPN_FNF_ACOSEC),
    look_table_entry("acosech",         RPN_FNF_ACOSECH),
    look_table_entry("acosh",           RPN_FNF_ACOSH),
    look_table_entry("acot",            RPN_FNF_ACOT),
    look_table_entry("acoth",           RPN_FNF_ACOTH),
    look_table_entry("address",         RPN_FNV_ADDRESS),
    look_table_entry("age",             RPN_FNF_AGE),
    look_table_entry("alert",           RPN_FNV_ALERT),
    look_table_entry("and",             RPN_FNV_AND),
    look_table_entry("asec",            RPN_FNF_ASEC),
    look_table_entry("asech",           RPN_FNF_ASECH),
    look_table_entry("asin",            RPN_FNF_ASIN),
    look_table_entry("asinh",           RPN_FNF_ASINH),
    look_table_entry("atan",            RPN_FNF_ATAN),
    look_table_entry("atan_2",          RPN_FNF_ATAN_2),
    look_table_entry("atanh",           RPN_FNF_ATANH),
    look_table_entry("avedev",          RPN_FNV_AVEDEV),
    look_table_entry("averagea",        RPN_FNV_AVERAGEA),
    look_table_entry("avg",             RPN_FNV_AVG),

    look_table_entry("base",            RPN_FNV_BASE),
    look_table_entry("besseli",         RPN_FNF_BESSELI),
    look_table_entry("besselj",         RPN_FNF_BESSELJ),
    look_table_entry("besselk",         RPN_FNF_BESSELK),
    look_table_entry("bessely",         RPN_FNF_BESSELY),
    look_table_entry("beta",            RPN_FNF_BETA),
    look_table_entry("beta.dist",       RPN_FNV_BETA_DIST),
    look_table_entry("beta.inv",        RPN_FNV_BETA_INV),
    look_table_entry("bin",             RPN_FNF_BIN),
    look_table_entry("bin2dec",         RPN_FNF_BIN2DEC),
    look_table_entry("bin2hex",         RPN_FNV_BIN2HEX),
    look_table_entry("bin2oct",         RPN_FNV_BIN2OCT),
    look_table_entry("binom.dist",      RPN_FNF_BINOM_DIST),
    look_table_entry("binom.dist.range",RPN_FNV_BINOM_DIST_RANGE),
    look_table_entry("binom.inv",       RPN_FNF_BINOM_INV),
    look_table_entry("bitand",          RPN_FNF_BITAND),
    look_table_entry("bitlshift",       RPN_FNF_BITLSHIFT),
    look_table_entry("bitor",           RPN_FNF_BITOR),
    look_table_entry("bitrshift",       RPN_FNF_BITRSHIFT),
    look_table_entry("bitxor",          RPN_FNF_BITXOR),
    look_table_entry("break",           RPN_FNV_BREAK),

    look_table_entry("c_acos",          RPN_FNF_C_ACOS),
    look_table_entry("c_acosec",        RPN_FNF_C_ACOSEC),
    look_table_entry("c_acosech",       RPN_FNF_C_ACOSECH),
    look_table_entry("c_acosh",         RPN_FNF_C_ACOSH),
    look_table_entry("c_acot",          RPN_FNF_C_ACOT),
    look_table_entry("c_acoth",         RPN_FNF_C_ACOTH),
    look_table_entry("c_add",           RPN_FNF_C_ADD),
    look_table_entry("c_asec",          RPN_FNF_C_ASEC),
    look_table_entry("c_asech",         RPN_FNF_C_ASECH),
    look_table_entry("c_asin",          RPN_FNF_C_ASIN),
    look_table_entry("c_asinh",         RPN_FNF_C_ASINH),
    look_table_entry("c_atan",          RPN_FNF_C_ATAN),
    look_table_entry("c_atanh",         RPN_FNF_C_ATANH),
    look_table_entry("c_complex",       RPN_FNV_C_COMPLEX),
    look_table_entry("c_conjugate",     RPN_FNF_C_CONJUGATE),
    look_table_entry("c_cos",           RPN_FNF_C_COS),
    look_table_entry("c_cosec",         RPN_FNF_C_COSEC),
    look_table_entry("c_cosech",        RPN_FNF_C_COSECH),
    look_table_entry("c_cosh",          RPN_FNF_C_COSH),
    look_table_entry("c_cot",           RPN_FNF_C_COT),
    look_table_entry("c_coth",          RPN_FNF_C_COTH),
    look_table_entry("c_div",           RPN_FNF_C_DIV),
    look_table_entry("c_exp",           RPN_FNF_C_EXP),
    look_table_entry("c_imaginary",     RPN_FNF_C_IMAGINARY),
    look_table_entry("c_ln",            RPN_FNF_C_LN),
    look_table_entry("c_mul",           RPN_FNF_C_MUL),
    look_table_entry("c_power",         RPN_FNF_C_POWER),
    look_table_entry("c_radius",        RPN_FNF_C_RADIUS),
    look_table_entry("c_real",          RPN_FNF_C_REAL),
    look_table_entry("c_round",         RPN_FNV_C_ROUND),
    look_table_entry("c_sec",           RPN_FNF_C_SEC),
    look_table_entry("c_sech",          RPN_FNF_C_SECH),
    look_table_entry("c_sin",           RPN_FNF_C_SIN),
    look_table_entry("c_sinh",          RPN_FNF_C_SINH),
    look_table_entry("c_sqrt",          RPN_FNF_C_SQRT),
    look_table_entry("c_sub",           RPN_FNF_C_SUB),
    look_table_entry("c_tan",           RPN_FNF_C_TAN),
    look_table_entry("c_tanh",          RPN_FNF_C_TANH),
    look_table_entry("c_theta",         RPN_FNF_C_THETA),

    look_table_entry("ceiling",         RPN_FNV_CEILING),
    look_table_entry("char",            RPN_FNF_CHAR),
    look_table_entry("chisq.dist",      RPN_FNV_CHISQ_DIST),
    look_table_entry("chisq.dist.rt",   RPN_FNF_CHISQ_DIST_RT),
    look_table_entry("chisq.inv",       RPN_FNF_CHISQ_INV),
    look_table_entry("chisq.inv.rt",    RPN_FNF_CHISQ_INV_RT),
    look_table_entry("chisq.test",      RPN_FNF_CHISQ_TEST),
    look_table_entry("choose",          RPN_FNV_CHOOSE),
    look_table_entry("clean",           RPN_FNF_CLEAN),
    look_table_entry("code",            RPN_FNF_CODE),
    look_table_entry("col",             RPN_FNV_COL),
    look_table_entry("cols",            RPN_FNV_COLS),
    look_table_entry("combin",          RPN_FNF_COMBIN),
    look_table_entry("combina",         RPN_FNF_COMBINA),
    look_table_entry("command",         RPN_FNF_COMMAND),
    look_table_entry("confidence.norm", RPN_FNF_CONFIDENCE_NORM),
    look_table_entry("confidence.t",    RPN_FNF_CONFIDENCE_T),
    look_table_entry("continue",        RPN_FN0_CONTINUE),
    look_table_entry("correl",          RPN_FNF_CORREL),
    look_table_entry("cos",             RPN_FNF_COS),
    look_table_entry("cosec",           RPN_FNF_COSEC),
    look_table_entry("cosech",          RPN_FNF_COSECH),
    look_table_entry("cosh",            RPN_FNF_COSH),
    look_table_entry("cot",             RPN_FNF_COT),
    look_table_entry("coth",            RPN_FNF_COTH),
    look_table_entry("count",           RPN_FNV_COUNT),
    look_table_entry("counta",          RPN_FNV_COUNTA),
    look_table_entry("countblank",      RPN_FNF_COUNTBLANK),
    look_table_entry("covariance.p",    RPN_FNF_COVARIANCE_P),
    look_table_entry("covariance.s",    RPN_FNF_COVARIANCE_S),
    look_table_entry("cterm",           RPN_FNF_CTERM),
    look_table_entry("current_cell",    RPN_FN0_CURRENT_CELL),

    look_table_entry("date",            RPN_FNF_DATE),
    look_table_entry("datevalue",       RPN_FNF_DATEVALUE),
    look_table_entry("davg",            RPN_FNF_DAVG),
    look_table_entry("day",             RPN_FNF_DAY),
    look_table_entry("dayname",         RPN_FNV_DAYNAME),
    look_table_entry("days",            RPN_FNF_DAYS),
    look_table_entry("days_360",        RPN_FNV_DAYS_360),
    look_table_entry("db",              RPN_FNV_DB),
    look_table_entry("dcount",          RPN_FNF_DCOUNT),
    look_table_entry("dcounta",         RPN_FNF_DCOUNTA),
    look_table_entry("ddb",             RPN_FNV_DDB),
    look_table_entry("dec2bin",         RPN_FNV_DEC2BIN),
    look_table_entry("dec2hex",         RPN_FNV_DEC2HEX),
    look_table_entry("dec2oct",         RPN_FNV_DEC2OCT),
    look_table_entry("decimal",         RPN_FNF_DECIMAL),
    look_table_entry("deg",             RPN_FNF_DEG),
    look_table_entry("delta",           RPN_FNV_DELTA),
    look_table_entry("deref",           RPN_FNF_DEREF),
    look_table_entry("devsq",           RPN_FNV_DEVSQ),
    look_table_entry("dmax",            RPN_FNF_DMAX),
    look_table_entry("dmin",            RPN_FNF_DMIN),
    look_table_entry("dollar",          RPN_FNV_DOLLAR),
    look_table_entry("doubleclick",     RPN_FN0_DOUBLECLICK),
    look_table_entry("dproduct",        RPN_FNF_DPRODUCT),
    look_table_entry("dstd",            RPN_FNF_DSTD),
    look_table_entry("dstdp",           RPN_FNF_DSTDP),
    look_table_entry("dsum",            RPN_FNF_DSUM),
    look_table_entry("dvar",            RPN_FNF_DVAR),
    look_table_entry("dvarp",           RPN_FNF_DVARP),

    look_table_entry("edate",           RPN_FNF_EDATE),
    look_table_entry("else",            RPN_FN0_ELSE),
    look_table_entry("elseif",          RPN_FNF_ELSEIF),
    look_table_entry("endif",           RPN_FN0_ENDIF),
    look_table_entry("endwhile",        RPN_FN0_ENDWHILE),
    look_table_entry("eomonth",         RPN_FNF_EOMONTH),
    look_table_entry("erf",             RPN_FNV_ERF),
    look_table_entry("erfc",            RPN_FNF_ERFC),
    look_table_entry("even",            RPN_FNF_EVEN),
    look_table_entry("exact",           RPN_FNF_EXACT),
    look_table_entry("exp",             RPN_FNF_EXP),
    look_table_entry("expon.dist",      RPN_FNV_EXPON_DIST),

    look_table_entry("f.dist",          RPN_FNV_F_DIST),
    look_table_entry("f.dist.rt",       RPN_FNF_F_DIST_RT),
    look_table_entry("f.inv",           RPN_FNF_F_INV),
    look_table_entry("f.inv.rt",        RPN_FNF_F_INV_RT),
    look_table_entry("f.test",          RPN_FNF_F_TEST),
    look_table_entry("fact",            RPN_FNF_FACT),
    look_table_entry("factdouble",      RPN_FNF_FACTDOUBLE),
    look_table_entry("false",           RPN_FN0_FALSE),
    look_table_entry("find",            RPN_FNV_FIND),
    look_table_entry("fisher",          RPN_FNF_FISHER),
    look_table_entry("fisherinv",       RPN_FNF_FISHERINV),
    look_table_entry("fixed",           RPN_FNV_FIXED),
    look_table_entry("flip",            RPN_FNF_FLIP),
    look_table_entry("floor",           RPN_FNV_FLOOR),
    look_table_entry("for",             RPN_FNV_FOR),
    look_table_entry("forecast",        RPN_FNF_FORECAST),
    look_table_entry("formula_text",    RPN_FNF_FORMULA_TEXT),
    look_table_entry("frequency",       RPN_FNF_FREQUENCY),
    look_table_entry("function",        RPN_FNM_FUNCTION),
    look_table_entry("fv",              RPN_FNF_FV),
    look_table_entry("fvschedule",      RPN_FNF_FVSCHEDULE),

    look_table_entry("gamma",           RPN_FNF_GAMMA),
    look_table_entry("gamma.dist",      RPN_FNV_GAMMA_DIST),
    look_table_entry("gamma.inv",       RPN_FNF_GAMMA_INV),
    look_table_entry("gammaln",         RPN_FNF_GAMMALN),
    look_table_entry("gcd",             RPN_FNV_GCD),
    look_table_entry("geomean",         RPN_FNV_GEOMEAN),
    look_table_entry("gestep",          RPN_FNV_GESTEP),
    look_table_entry("goto",            RPN_FNF_GOTO),
    look_table_entry("grand",           RPN_FNV_GRAND),
    look_table_entry("growth",          RPN_FNF_GROWTH),

    look_table_entry("harmean",         RPN_FNV_HARMEAN),
    look_table_entry("hex2bin",         RPN_FNV_HEX2BIN),
    look_table_entry("hex2dec",         RPN_FNF_HEX2DEC),
    look_table_entry("hex2oct",         RPN_FNV_HEX2OCT),
    look_table_entry("hlookup",         RPN_FNV_HLOOKUP),
    look_table_entry("hour",            RPN_FNF_HOUR),
    look_table_entry("hypgeom.dist",    RPN_FNV_HYPGEOM_DIST),

    look_table_entry("if",              RPN_FNF_IF),
    look_table_entry("index",           RPN_FNV_INDEX),
    look_table_entry("input",           RPN_FNV_INPUT),
    look_table_entry("int",             RPN_FNF_INT),
    look_table_entry("intercept",       RPN_FNF_INTERCEPT),
    look_table_entry("irr",             RPN_FNF_IRR),
    look_table_entry("isblank",         RPN_FNF_ISBLANK),
    look_table_entry("iserr",           RPN_FNF_ISERR),
    look_table_entry("iserror",         RPN_FNF_ISERROR),
    look_table_entry("iseven",          RPN_FNF_ISEVEN),
    look_table_entry("islogical",       RPN_FNF_ISLOGICAL),
    look_table_entry("isna",            RPN_FNF_ISNA),
    look_table_entry("isnontext",       RPN_FNF_ISNONTEXT),
    look_table_entry("isnumber",        RPN_FNF_ISNUMBER),
    look_table_entry("isodd",           RPN_FNF_ISODD),
    look_table_entry("isoweeknum",      RPN_FNF_ISOWEEKNUM),
    look_table_entry("isref",           RPN_FNF_ISREF),
    look_table_entry("istext",          RPN_FNF_ISTEXT),

    look_table_entry("join",            RPN_FNV_JOIN),

    look_table_entry("kurt",            RPN_FNV_KURT),

    look_table_entry("large",           RPN_FNF_LARGE),
    look_table_entry("lcm",             RPN_FNV_LCM),
    look_table_entry("left",            RPN_FNV_LEFT),
    look_table_entry("length",          RPN_FNF_LENGTH),
    look_table_entry("linest",          RPN_FNV_LINEST),
    look_table_entry("listcount",       RPN_FNF_LISTCOUNT),
    look_table_entry("ln",              RPN_FNF_LN),
    look_table_entry("log",             RPN_FNV_LOG),
    look_table_entry("logest",          RPN_FNV_LOGEST),
    look_table_entry("lognorm.dist",    RPN_FNV_LOGNORM_DIST),
    look_table_entry("lognorm.inv",     RPN_FNF_LOGNORM_INV),
    look_table_entry("lookup",          RPN_FNV_LOOKUP),
    look_table_entry("lower",           RPN_FNF_LOWER),

    look_table_entry("m_determ",        RPN_FNF_M_DETERM),
    look_table_entry("m_inverse",       RPN_FNF_M_INVERSE),
    look_table_entry("m_mult",          RPN_FNF_M_MULT),
    look_table_entry("m_unit",          RPN_FNF_M_UNIT),

    look_table_entry("match",           RPN_FNF_MATCH),
    look_table_entry("max",             RPN_FNV_MAX),
    look_table_entry("maxa",            RPN_FNV_MAXA),
    look_table_entry("median",          RPN_FNV_MEDIAN),
    look_table_entry("mid",             RPN_FNF_MID),
    look_table_entry("min",             RPN_FNV_MIN),
    look_table_entry("mina",            RPN_FNV_MINA),
    look_table_entry("minute",          RPN_FNF_MINUTE),
    look_table_entry("mirr",            RPN_FNF_MIRR),
    look_table_entry("mod",             RPN_FNF_MOD),
    look_table_entry("mode.sngl",       RPN_FNF_MODE_SNGL),
    look_table_entry("month",           RPN_FNF_MONTH),
    look_table_entry("monthdays",       RPN_FNF_MONTHDAYS),
    look_table_entry("monthname",       RPN_FNF_MONTHNAME),
    look_table_entry("mround",          RPN_FNF_MROUND),
    look_table_entry("multinomial",     RPN_FNV_MULTINOMIAL),

    look_table_entry("n",               RPN_FNF_N),
    look_table_entry("na",              RPN_FN0_NA),
    look_table_entry("negbinom.dist",   RPN_FNV_NEGBINOM_DIST),
    look_table_entry("next",            RPN_FN0_NEXT),
    look_table_entry("norm.dist",       RPN_FNV_NORM_DIST),
    look_table_entry("norm.inv",        RPN_FNF_NORM_INV),
    look_table_entry("norm.s.dist",     RPN_FNV_NORM_S_DIST),
    look_table_entry("norm.s.inv",      RPN_FNF_NORM_S_INV),
    look_table_entry("not",             RPN_FNF_NOT),
    look_table_entry("now",             RPN_FN0_NOW),
    look_table_entry("nper",            RPN_FNV_NPER),
    look_table_entry("npv",             RPN_FNF_NPV),

    look_table_entry("oct2bin",         RPN_FNV_OCT2BIN),
    look_table_entry("oct2dec",         RPN_FNF_OCT2DEC),
    look_table_entry("oct2hex",         RPN_FNV_OCT2HEX),
    look_table_entry("odd",             RPN_FNF_ODD),

    look_table_entry("odf.betadist",    RPN_FNV_ODF_BETADIST),
    look_table_entry("odf.complex",     RPN_FNV_ODF_COMPLEX),
    look_table_entry("odf.fv",          RPN_FNV_ODF_FV),
    look_table_entry("odf.imabs",       RPN_FNF_ODF_IMABS),
    look_table_entry("odf.imaginary",   RPN_FNF_ODF_IMAGINARY),
    look_table_entry("odf.imargument",  RPN_FNF_ODF_IMARGUMENT),
    look_table_entry("odf.imconjugate", RPN_FNF_ODF_IMCONJUGATE),
    look_table_entry("odf.imcos",       RPN_FNF_ODF_IMCOS),
    look_table_entry("odf.imdiv",       RPN_FNF_ODF_IMDIV),
    look_table_entry("odf.imexp",       RPN_FNF_ODF_IMEXP),
    look_table_entry("odf.imln",        RPN_FNF_ODF_IMLN),
    look_table_entry("odf.imlog10",     RPN_FNF_ODF_IMLOG10),
    look_table_entry("odf.imlog2",      RPN_FNF_ODF_IMLOG2),
    look_table_entry("odf.impower",     RPN_FNF_ODF_IMPOWER),
    look_table_entry("odf.improduct",   RPN_FNF_ODF_IMPRODUCT),
    look_table_entry("odf.imreal",      RPN_FNF_ODF_IMREAL),
    look_table_entry("odf.imsin",       RPN_FNF_ODF_IMSIN),
    look_table_entry("odf.imsqrt",      RPN_FNF_ODF_IMSQRT),
    look_table_entry("odf.imsub",       RPN_FNF_ODF_IMSUB),
    look_table_entry("odf.imsum",       RPN_FNF_ODF_IMSUM),
    look_table_entry("odf.index",       RPN_FNV_ODF_INDEX),
    look_table_entry("odf.int",         RPN_FNF_ODF_INT),
    look_table_entry("odf.irr",         RPN_FNV_ODF_IRR),
    look_table_entry("odf.log10",       RPN_FNF_ODF_LOG10),
    look_table_entry("odf.mod",         RPN_FNF_ODF_MOD),
    look_table_entry("odf.pmt",         RPN_FNF_ODF_PMT),
    look_table_entry("odf.tdist",       RPN_FNF_ODF_TDIST),
    look_table_entry("odf.type",        RPN_FNF_ODF_TYPE),

    look_table_entry("or",              RPN_FNV_OR),

    look_table_entry("page",            RPN_FNV_PAGE),
    look_table_entry("pages",           RPN_FNV_PAGES),
    look_table_entry("pearson",         RPN_FNF_PEARSON),
    look_table_entry("percentile.exc",  RPN_FNF_PERCENTILE_EXC),
    look_table_entry("percentile.inc",  RPN_FNF_PERCENTILE_INC),
    look_table_entry("percentrank.exc", RPN_FNV_PERCENTRANK_EXC),
    look_table_entry("percentrank.inc", RPN_FNV_PERCENTRANK_INC),
    look_table_entry("permut",          RPN_FNF_PERMUT),
    look_table_entry("phi",             RPN_FNF_PHI),
    look_table_entry("pi",              RPN_FN0_PI),
    look_table_entry("pmt",             RPN_FNF_PMT),
    look_table_entry("poisson.dist",    RPN_FNV_POISSON_DIST),
    look_table_entry("power",           RPN_FNF_POWER),
    look_table_entry("prob",            RPN_FNV_PROB),
    look_table_entry("product",         RPN_FNV_PRODUCT),
    look_table_entry("proper",          RPN_FNF_PROPER),
    look_table_entry("pv",              RPN_FNF_PV),

    look_table_entry("quartile.exc",    RPN_FNF_QUARTILE_EXC),
    look_table_entry("quartile.inc",    RPN_FNF_QUARTILE_INC),
    look_table_entry("quotient",        RPN_FNF_QUOTIENT),

    look_table_entry("rad",             RPN_FNF_RAD),
    look_table_entry("rand",            RPN_FNV_RAND),
    look_table_entry("randbetween",     RPN_FNF_RANDBETWEEN),
    look_table_entry("rank",            RPN_FNV_RANK),
    look_table_entry("rank.eq",         RPN_FNV_RANK_EQ),
    look_table_entry("rate",            RPN_FNF_RATE),
    look_table_entry("repeat",          RPN_FN0_REPEAT),
    look_table_entry("replace",         RPN_FNF_REPLACE),
    look_table_entry("rept",            RPN_FNF_REPT),
    look_table_entry("result",          RPN_FNF_RESULT),
    look_table_entry("reverse",         RPN_FNF_REVERSE),
    look_table_entry("right",           RPN_FNV_RIGHT),
    look_table_entry("round",           RPN_FNV_ROUND),
    look_table_entry("rounddown",       RPN_FNF_ROUNDDOWN),
    look_table_entry("roundup",         RPN_FNF_ROUNDUP),
    look_table_entry("row",             RPN_FNV_ROW),
    look_table_entry("rows",            RPN_FNV_ROWS),
    look_table_entry("rsq",             RPN_FNF_RSQ),

    look_table_entry("sec",             RPN_FNF_SEC),
    look_table_entry("sech",            RPN_FNF_SECH),
    look_table_entry("second",          RPN_FNF_SECOND),
    look_table_entry("seriessum",       RPN_FNF_SERIESSUM),
    look_table_entry("set_name",        RPN_FNF_SET_NAME),
    look_table_entry("set_value",       RPN_FNV_SET_VALUE),
    look_table_entry("sgn",             RPN_FNF_SGN),
    look_table_entry("sin",             RPN_FNF_SIN),
    look_table_entry("sinh",            RPN_FNF_SINH),
    look_table_entry("skew",            RPN_FNV_SKEW),
    look_table_entry("skew.p",          RPN_FNV_SKEW_P),
    look_table_entry("sln",             RPN_FNF_SLN),
    look_table_entry("slope",           RPN_FNF_SLOPE),
    look_table_entry("small",           RPN_FNF_SMALL),
    look_table_entry("sort",            RPN_FNV_SORT),
    look_table_entry("spearman",        RPN_FNF_SPEARMAN),
    look_table_entry("sqr",             RPN_FNF_SQR),
    look_table_entry("standardize",     RPN_FNF_STANDARDIZE),
    look_table_entry("std",             RPN_FNV_STD),
    look_table_entry("stdeva",          RPN_FNV_STDEVA),
    look_table_entry("stdevpa",         RPN_FNV_STDEVPA),
    look_table_entry("stdp",            RPN_FNV_STDP),
    look_table_entry("steyx",           RPN_FNF_STEYX),
    look_table_entry("string",          RPN_FNV_STRING),
    look_table_entry("substitute",      RPN_FNV_SUBSTITUTE),
    look_table_entry("sum",             RPN_FNV_SUM),
    look_table_entry("sum_x2my2",       RPN_FNF_SUM_X2MY2), /* NB C_stricmp does lower-case comparision, so underscore sorts lower than any letter */
    look_table_entry("sum_x2py2",       RPN_FNF_SUM_X2PY2),
    look_table_entry("sum_xmy2",        RPN_FNF_SUM_XMY2),
    look_table_entry("sumproduct",      RPN_FNV_SUMPRODUCT),
    look_table_entry("sumsq",           RPN_FNV_SUMSQ),
    look_table_entry("syd",             RPN_FNF_SYD),

    look_table_entry("t",               RPN_FNF_T),
    look_table_entry("t.dist",          RPN_FNV_T_DIST),
    look_table_entry("t.dist.2t",       RPN_FNF_T_DIST_2T),
    look_table_entry("t.dist.rt",       RPN_FNF_T_DIST_RT),
    look_table_entry("t.inv",           RPN_FNF_T_INV),
    look_table_entry("t.inv.2t",        RPN_FNF_T_INV_2T),
    look_table_entry("t.test",          RPN_FNF_T_TEST),
    look_table_entry("tan",             RPN_FNF_TAN),
    look_table_entry("tanh",            RPN_FNF_TANH),
    look_table_entry("term",            RPN_FNF_TERM),
    look_table_entry("text",            RPN_FNF_TEXT),
    look_table_entry("time",            RPN_FNF_TIME),
    look_table_entry("timevalue",       RPN_FNF_TIMEVALUE),
    look_table_entry("today",           RPN_FN0_TODAY),
    look_table_entry("transpose",       RPN_FNF_TRANSPOSE),
    look_table_entry("trend",           RPN_FNF_TREND),
    look_table_entry("trim",            RPN_FNF_TRIM),
    look_table_entry("trimmean",        RPN_FNF_TRIMMEAN),
    look_table_entry("true",            RPN_FN0_TRUE),
    look_table_entry("trunc",           RPN_FNV_TRUNC),
    look_table_entry("type",            RPN_FNF_TYPE),

    look_table_entry("until",           RPN_FNF_UNTIL),
    look_table_entry("upper",           RPN_FNF_UPPER),

    look_table_entry("value",           RPN_FNF_VALUE),
    look_table_entry("var",             RPN_FNV_VAR),
    look_table_entry("vara",            RPN_FNV_VARA),
    look_table_entry("varp",            RPN_FNV_VARP),
    look_table_entry("varpa",           RPN_FNV_VARPA),
    look_table_entry("version",         RPN_FN0_VERSION),
    look_table_entry("vlookup",         RPN_FNV_VLOOKUP),

    look_table_entry("weekday",         RPN_FNV_WEEKDAY),
    look_table_entry("weeknumber",      RPN_FNF_WEEKNUMBER),
    look_table_entry("weibull.dist",    RPN_FNF_WEIBULL_DIST),
    look_table_entry("while",           RPN_FNF_WHILE),

    look_table_entry("xor",             RPN_FNV_XOR),

    look_table_entry("year",            RPN_FNF_YEAR),

    look_table_entry("z.test",          RPN_FNV_Z_TEST),

    look_table_entr0("|",               RPN_BOP_OR),

    /* number of functions out of sequence, not to be searched for by bsearch ... */

#define LOOK_TABLE_EXTRA 3

    /* extra out-of-sequence functions start here */

    look_table_entr0("+",               RPN_UOP_PLUS),
    look_table_entr0("-",               RPN_UOP_MINUS),

    look_table_entry("if",              RPN_FNF_IFC)
};

/*
definition of types available
*/

typedef struct TYPES
{
    A7CHARZ a7str_id[10];
    EV_TYPE type_flags;
}
TYPES; typedef const TYPES * PC_TYPES;

static const TYPES
type_table[] =
{
    /* these ones can be FUNCTION() argument types */ /* these must be UI language-invariant */
    { "array",      EM_ARY },
    { "date",       EM_DAT },
    { "error",      EM_ERR },
    { "number",     EM_REA },
    { "reference",  EM_SLR },
    { "text",       EM_STR },

#define TYPE_TABLE_EXTRA 1

    /* these ones can NOT be FUNCTION() argument types but are valid types */
    { "blank",      EM_BLK }
};

/******************************************************************************
*
* dispose of resources in resource spec block
*
******************************************************************************/

extern void
ev_enum_resource_dispose(
    _InoutRef_  P_RESOURCE_SPEC p_resource_spec)
{
    al_array_dispose(&p_resource_spec->h_id_ustr);
    al_array_dispose(&p_resource_spec->h_definition_ustr);
    ustr_clr(&p_resource_spec->ustr_description);
}

/******************************************************************************
*
* initialise resource enumerator
*
* --in--
* item_no is rpn or item number, 0 to start
*
* doc is only relevant for names
* and custom functions
*
******************************************************************************/

extern void
ev_enum_resource_init(
    _OutRef_    P_EV_RESOURCE p_ev_resource,
    _OutRef_    P_RESOURCE_SPEC p_resource_spec,
    _InVal_     S32 category,
    _InVal_     S32 item_no,
    _InVal_     EV_DOCNO ev_docno_to,
    _InVal_     EV_DOCNO ev_docno_from)
{
    zero_struct_ptr(p_resource_spec);

    p_ev_resource->category = category;
    p_ev_resource->ev_docno_to = ev_docno_to;
    p_ev_resource->ev_docno_from = ev_docno_from;
    p_ev_resource->item_no = item_no;
}

/******************************************************************************
*
* get text about resource
*
* -out--
* < 0 error (end)
* >=0 resource found
*
******************************************************************************/

_Check_return_
static STATUS /* rpn number out */
ev_enum_resource_get_custom(
    _InoutRef_  P_RESOURCE_SPEC p_resource_spec,
    P_EV_RESOURCE p_ev_resource)
{
    STATUS status = STATUS_FAIL;
    const ARRAY_INDEX custom_table_entries = array_elements(&custom_def_deptable.h_table);
    ARRAY_INDEX custom_num;
    P_EV_CUSTOM p_ev_custom;

    for(custom_num = p_ev_resource->item_no, p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, custom_num);
        custom_num < custom_table_entries;
        ++custom_num, ++p_ev_custom)
    {
        if(p_ev_custom->flags.to_be_deleted)
            continue;

        if(p_ev_custom->flags.undefined)
            continue;

        if( (p_ev_resource->ev_docno_to != DOCNO_NONE) &&
            (p_ev_resource->ev_docno_to != ev_slr_docno(&p_ev_custom->owner)) )
            continue;

        if(ev_slr_docno(&p_ev_custom->owner) != p_ev_resource->ev_docno_from)
        {
            UCHARZ ustr_buf[BUF_MAX_PATHSTRING];
            (void) ev_write_docname_ustr_buf(ustr_bptr(ustr_buf), MAX_PATHSTRING, ev_slr_docno(&p_ev_custom->owner), p_ev_resource->ev_docno_from);
            status_break(status = al_ustr_append(&p_resource_spec->h_id_ustr, ustr_bptr(ustr_buf)));
        }

        status_break(status = al_ustr_append(&p_resource_spec->h_id_ustr, ustr_bptr(p_ev_custom->ustr_custom_id)));

        if((p_resource_spec->n_args = p_ev_custom->args->n) == 0)
            p_resource_spec->n_args = 1;

        p_resource_spec->max_additional_args = 0;

        status = custom_num;
        p_ev_resource->item_no = custom_num + 1;
        break;
    }

    return(status);
}

_Check_return_
static STATUS /* rpn number out */
ev_enum_resource_get_names(
    _InoutRef_  P_RESOURCE_SPEC p_resource_spec,
    P_EV_RESOURCE p_ev_resource)
{
    STATUS status = STATUS_FAIL;
    const ARRAY_INDEX name_table_entries = array_elements(&name_def_deptable.h_table);
    ARRAY_INDEX name_num;
    P_EV_NAME p_ev_name;

    for(name_num = p_ev_resource->item_no, p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);
        name_num < name_table_entries;
        ++name_num, ++p_ev_name)
    {
        if(p_ev_name->flags.to_be_deleted)
            continue;

        if(p_ev_name->flags.undefined)
            continue;

        if( (p_ev_resource->ev_docno_to != DOCNO_NONE) &&
            (p_ev_resource->ev_docno_to != ev_slr_docno(&p_ev_name->owner)) )
            continue;

        if(ev_slr_docno(&p_ev_name->owner) != p_ev_resource->ev_docno_from)
        {
            UCHARZ ustr_buf[BUF_MAX_PATHSTRING];
            (void) ev_write_docname_ustr_buf(ustr_bptr(ustr_buf), MAX_PATHSTRING, ev_slr_docno(&p_ev_name->owner), p_ev_resource->ev_docno_from);
            status_break(status = al_ustr_append(&p_resource_spec->h_id_ustr, ustr_bptr(ustr_buf)));
        }

        status_break(status = al_ustr_append(&p_resource_spec->h_id_ustr, ustr_bptr(p_ev_name->ustr_name_id)));

        {
        UCHARZ * ustr;
        U32 len;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);
        status_break(status = ss_data_decode(&quick_ublock, &p_ev_name->def_data, p_ev_resource->ev_docno_from));
        len = quick_ublock_bytes(&quick_ublock);
        if(NULL != (ustr = al_array_alloc(&p_resource_spec->h_definition_ustr, UCHARZ, len + 1 /*CH_NULL*/, &array_init_block_uchars, &status)))
        {
            memcpy32(ustr, quick_ublock_uchars(&quick_ublock), len);
            PtrPutByteOff(ustr, len, CH_NULL);
        }
        quick_ublock_dispose(&quick_ublock);
        status_break(status);
        } /*block*/

        status_break(status = ustr_set(&p_resource_spec->ustr_description, p_ev_name->ustr_description));

        p_resource_spec->n_args = 0;
        p_resource_spec->max_additional_args = 0;

        status = name_num;
        p_ev_resource->item_no = name_num + 1;
        break;
    }

    return(status);
}

_Check_return_
static STATUS /* rpn number out */
ev_enum_resource_get_builtin_functions(
    _InoutRef_  P_RESOURCE_SPEC p_resource_spec,
    P_EV_RESOURCE p_ev_resource)
{
    STATUS status = STATUS_FAIL;
    EV_IDNO rpn_num;
    PC_RPNDEF p_rpndef;

    assert((EV_IDNO) p_ev_resource->item_no < ELEMOF_RPN_TABLE);

    for(rpn_num = (EV_IDNO) p_ev_resource->item_no, p_rpndef = &rpn_table[rpn_num]; rpn_num < RPN_END_BUILT_IN; ++rpn_num, ++p_rpndef)
    {
        if( (p_rpndef->category == (unsigned) p_ev_resource->category) ||
            ((p_rpndef->category != EV_RESO_NOTME) && ((unsigned) p_ev_resource->category == EV_RESO_ALL_VISIBLE)) )
        {
            PC_USTR ustr_name = func_name(rpn_num);
            PTR_ASSERT(ustr_name);
            status_break(status = al_ustr_append(&p_resource_spec->h_id_ustr, ustr_name));

            p_resource_spec->n_args = p_rpndef->n_args;
            p_resource_spec->max_additional_args = p_rpndef->max_additional_args;

            if(p_rpndef->n_args < 0)
            {
                p_resource_spec->n_args = (- (S32) p_rpndef->n_args) - 1;

                if(0 == p_resource_spec->max_additional_args)
                    p_resource_spec->max_additional_args = 1; /* show just the first optional parameter in the function list */
            }

            status = rpn_num;
            p_ev_resource->item_no = (S32) rpn_num + 1;
            break;
        }
    }

    return(status);
}

_Check_return_
extern STATUS /* rpn number out */
ev_enum_resource_get(
    _InoutRef_  P_RESOURCE_SPEC p_resource_spec,
    P_EV_RESOURCE p_ev_resource)
{
    STATUS status = STATUS_FAIL;

    ev_enum_resource_dispose(p_resource_spec);

    switch(p_ev_resource->category)
    {
    case EV_RESO_CUSTOM:
        status = ev_enum_resource_get_custom(p_resource_spec, p_ev_resource);
        break;

    case EV_RESO_NAMES:
        status = ev_enum_resource_get_names(p_resource_spec, p_ev_resource);
        break;

    /* deal with built-in functions */
    default:
        status = ev_enum_resource_get_builtin_functions(p_resource_spec, p_ev_resource);
        break;
    }

    if(status_fail(status))
        ev_enum_resource_dispose(p_resource_spec);

    return(status);
}

/******************************************************************************
*
* compare operators
*
******************************************************************************/

PROC_BSEARCH_PROTO_Z(static, func_lookcomp, U8 /*UTF8Z*/, LOOKDEF)
{
    BSEARCH_KEY_VAR_DECL(PC_U8Z, key_id);
    BSEARCH_DATUM_VAR_DECL(PC_LOOKDEF, datum);
    PC_U8Z datum_id = datum->sz_id;

    return(C_stricmp(key_id, datum_id)); /* very simple comparison for operators */
}

#if CHECKING

PROC_QSORT_PROTO(static, func_lookcomp_qsort, LOOKDEF)
{
    QSORT_ARG1_VAR_DECL(PC_LOOKDEF, lookdef_1);
    QSORT_ARG2_VAR_DECL(PC_LOOKDEF, lookdef_2);

    PC_U8Z s1 = lookdef_1->sz_id;
    PC_U8Z s2 = lookdef_2->sz_id;

    return(C_stricmp(s1, s2)); /* ditto */
}

#endif /* CHECKING */

/******************************************************************************
*
* lookup id in master function table
*
* --out--
* < 0 (SYM_BAD) id not found in function table
* >=0 rpn number of found function
*
******************************************************************************/

#if CHECKING

static bool
ev_func_lookup_check_fnv_zero_ok(
    _InVal_     EV_IDNO i)
{
    switch(i)
    {
    case RPN_FNV_AND:
    case RPN_FNV_AVEDEV:
    case RPN_FNV_AVG:
    case RPN_FNV_COUNT:
    case RPN_FNV_COUNTA:
    case RPN_FNV_DEVSQ:
    case RPN_FNV_GEOMEAN:
    case RPN_FNV_HARMEAN:
    case RPN_FNV_KURT:
    case RPN_FNV_MAX:
    case RPN_FNV_MEDIAN:
    case RPN_FNV_MIN:
    case RPN_FNV_OR:
    case RPN_FNV_PRODUCT:
    case RPN_FNV_SKEW:
    case RPN_FNV_STD:
    case RPN_FNV_STDP:
    case RPN_FNV_SUM:
    case RPN_FNV_SUMSQ:
    case RPN_FNV_VAR:
    case RPN_FNV_VARP:
        /* new ones */
    case RPN_FNV_SUMPRODUCT:
    case RPN_FNV_SKEW_P:
    case RPN_FNV_AVERAGEA:
    case RPN_FNV_MAXA:
    case RPN_FNV_MINA:
    case RPN_FNV_STDEVA:
    case RPN_FNV_STDEVPA:
    case RPN_FNV_VARA:
    case RPN_FNV_VARPA:
    case RPN_FNV_XOR:
    case RPN_FNV_MULTINOMIAL:
    case RPN_FNV_GCD:
    case RPN_FNV_LCM:
        /* unlimited ARRAY_RANGE_XXX */
        return(true);

    case RPN_FNV_CHOOSE:
    case RPN_FNV_JOIN:
        /* unlimited other */
        return(true);

    default:
        return(false);
    }
}

static void
ev_func_lookup_check(void)
{
    assert(elemof32(_rpn_table) == ELEMOF_RPN_TABLE);
#if defined(EV_IDNO_U16)
    assert(RPN_END <= U16_MAX);
#else
    assert(RPN_END <= U8_MAX);
#endif
    assert(ELEMOF_RPN_TABLE <= RPN_END); /* in fact it should be several symbols smaller */
    {
    EV_IDNO i;
    for(i = 0; i < elemof32(_rpn_table); ++i)
    {
        assert((i == _rpn_table[i].own_did_num) || (0 == _rpn_table[i].own_did_num));
        if(_rpn_table[i].rpn_type == RPN_FN0)
        {
            assert(_rpn_table[i].n_args == 0);
            assert(_rpn_table[i].max_additional_args == 0);
        }
        else if(_rpn_table[i].rpn_type == RPN_FNF)
        {
            assert(_rpn_table[i].n_args >= 0);
            assert(_rpn_table[i].max_additional_args == 0);
        }
        else if(_rpn_table[i].rpn_type == RPN_FNV)
        {
            assert(_rpn_table[i].n_args < 0);
            assert(_rpn_table[i].max_additional_args >= 0);
            assert((_rpn_table[i].max_additional_args != 0) || ev_func_lookup_check_fnv_zero_ok(i)); /* some are unlimited */
        }
        else if(_rpn_table[i].rpn_type == RPN_FNM)
        {
            assert(_rpn_table[i].n_args < 0);
            assert(_rpn_table[i].max_additional_args == 0); /* all are unlimited */
        }
    }
    }/*block*/

    check_sorted(look_table, elemof(look_table) - LOOK_TABLE_EXTRA, sizeof(LOOKDEF), func_lookcomp_qsort);
}

#endif /*CHECKING*/

_Check_return_
extern S32
ev_func_lookup(
    _In_z_      PC_USTR ustr_id)
{
    PC_LOOKDEF opr;

#if CHECKING
    { static bool inited = false; if(!inited) { inited = true; ev_func_lookup_check(); } } /*block*/
#endif

    if(g_ss_recog_context.alternate_function_flag)
    {
        U32 i;

        opr = NULL;

        for(i = 0; i < elemof32(look_table) - LOOK_TABLE_EXTRA; ++i)
        {
            PC_LOOKDEF look_2 = &look_table[i];
            PC_USTR ustr_id_2 = look_2->ev_idno_function_id ? resource_lookup_ustr_no_default(SS_MSG_FUNCTION_FROM_RPN(look_2->ev_idno_function_id)) : NULL;

            if(NULL == ustr_id_2)
                ustr_id_2 = (PC_USTR) look_2->sz_id; /* U is superset of A7 */

            if(ustr_compare_equals_nocase(ustr_id, ustr_id_2))
            {
                opr = look_2;
                break;
            }
        }
    }
    else
    {
        opr = (PC_LOOKDEF)
            bsearch(ustr_id, look_table, elemof(look_table) - LOOK_TABLE_EXTRA, sizeof(look_table[0]), func_lookcomp);
    }

    if(NULL == opr)
        return(-1);

    return(opr->ev_idno);
}

/******************************************************************************
*
* return pointer to function/operator name given its rpn number
*
******************************************************************************/

_Check_return_
_Ret_maybenull_z_
extern PC_USTR
func_name(
    _InVal_     EV_IDNO ev_idno)
{
    U32 i;
    PC_LOOKDEF p_lookdef;

    for(i = 0, p_lookdef = look_table; i < elemof32(look_table); ++i, ++p_lookdef)
    {
        if(p_lookdef->ev_idno != ev_idno)
            continue;

        if(g_ss_recog_context.alternate_function_flag)
        {
            if(p_lookdef->ev_idno_function_id)
            {
                PC_USTR ustr = resource_lookup_ustr_no_default(SS_MSG_FUNCTION_FROM_RPN(p_lookdef->ev_idno_function_id));

                if(NULL != ustr)
                    return(ustr);
            }
        }

        return((PC_USTR) p_lookdef->sz_id); /* U is superset of A7 */
    }

    return(NULL);
}

/******************************************************************************
*
* return a type string given the type flags
*
******************************************************************************/

_Check_return_
_Ret_maybenull_z_
extern PC_A7STR
type_name_from_type_flags(
    _InVal_     EV_TYPE type_flags)
{
    U32 i;
    PC_TYPES p_types;

    for(i = 0, p_types = type_table; i < elemof32(type_table); ++i, ++p_types)
    {
        if(p_types->type_flags == type_flags)
            return(p_types->a7str_id);
    }

    return(NULL);
}

/******************************************************************************
*
* compare types
*
******************************************************************************/

PROC_BSEARCH_PROTO_Z(static, type_name_compare, A7CHARZ, TYPES)
{
    BSEARCH_KEY_VAR_DECL(PC_A7STR, a7str_key_id);
    BSEARCH_DATUM_VAR_DECL(PC_TYPES, datum);
    PC_A7STR a7str_datum_id = datum->a7str_id;

    return(/*"C"*/strcmp(a7str_key_id, a7str_datum_id));
}

/******************************************************************************
*
* look up type name in table
*
******************************************************************************/

_Check_return_
extern EV_TYPE
type_name_lookup(
    _In_z_      PC_USTR ustr_id)
{
    /* although we are searching using a USTR for caller convenience, type ids are all A7STR */
    PC_TYPES p_types = (PC_TYPES)
        bsearch(ustr_id, type_table, elemof(type_table) - TYPE_TABLE_EXTRA, sizeof(type_table[0]), type_name_compare);

    if(NULL == p_types)
        return(0);

    return(p_types->type_flags);
}

/* end of ev_tabl.c */
